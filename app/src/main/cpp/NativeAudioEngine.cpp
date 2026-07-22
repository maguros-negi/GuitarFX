#include "NativeAudioEngine.h"

#include <algorithm>
#include <cmath>
#include <cstring>

bool NativeAudioEngine::openInput() {
    oboe::AudioStreamBuilder builder;

    builder.setDirection(oboe::Direction::Input)
            ->setPerformanceMode(oboe::PerformanceMode::LowLatency)
            ->setSharingMode(oboe::SharingMode::Exclusive)
            ->setFormat(oboe::AudioFormat::Float)
            ->setChannelCount(1)
            ->setInputPreset(oboe::InputPreset::Generic);

    auto result = builder.openStream(inputStream_);

    if (result != oboe::Result::OK) {
        builder.setSharingMode(oboe::SharingMode::Shared);
        result = builder.openStream(inputStream_);
    }

    if (result != oboe::Result::OK || !inputStream_) {
        setError(
                std::string("Input open failed: ") +
                oboe::convertToText(result)
        );
        return false;
    }

    sampleRate_ = inputStream_->getSampleRate();
    framesPerBurst_ = inputStream_->getFramesPerBurst();
    inputChannels_ = inputStream_->getChannelCount();

    return true;
}

bool NativeAudioEngine::openOutput() {
    oboe::AudioStreamBuilder builder;

    builder.setDirection(oboe::Direction::Output)
            ->setPerformanceMode(oboe::PerformanceMode::LowLatency)
            ->setSharingMode(oboe::SharingMode::Exclusive)
            ->setFormat(oboe::AudioFormat::Float)
            ->setSampleRate(sampleRate_)
            ->setChannelCount(2)
            ->setUsage(oboe::Usage::Game)
            ->setDataCallback(this)
            ->setErrorCallback(this);

    auto result = builder.openStream(outputStream_);

    if (result != oboe::Result::OK) {
        builder.setSharingMode(oboe::SharingMode::Shared);
        result = builder.openStream(outputStream_);
    }

    if (result != oboe::Result::OK || !outputStream_) {
        setError(
                std::string("Output open failed: ") +
                oboe::convertToText(result)
        );
        return false;
    }

    outputChannels_ = outputStream_->getChannelCount();

    framesPerBurst_ = std::max(
            framesPerBurst_,
            outputStream_->getFramesPerBurst()
    );

    const int32_t capacity = std::max(
            4096,
            framesPerBurst_ * 8
    );

    inputBuffer_.assign(
            capacity * std::max(1, inputChannels_),
            0.0f
    );

    outputStream_->setBufferSizeInFrames(
            outputStream_->getFramesPerBurst() * 2
    );

    return true;
}

bool NativeAudioEngine::start() {
    std::lock_guard<std::mutex> lock(lifecycleMutex_);

    if (running_) {
        return true;
    }

    setError("");

    if (!openInput()) {
        return false;
    }

    if (!openOutput()) {
        inputStream_->close();
        inputStream_.reset();
        return false;
    }

    inputCurrent_ = inputTarget_.load(std::memory_order_relaxed);
    outputCurrent_ = outputTarget_.load(std::memory_order_relaxed);

    bypassMix_ = bypassTarget_.load(std::memory_order_relaxed)
                 ? 1.0f
                 : 0.0f;

    inputPeakDb_.store(-80.0f, std::memory_order_relaxed);
    outputPeakDb_.store(-80.0f, std::memory_order_relaxed);

    auto inResult = inputStream_->requestStart();

    if (inResult != oboe::Result::OK) {
        setError(
                std::string("Input start failed: ") +
                oboe::convertToText(inResult)
        );

        outputStream_->close();
        inputStream_->close();

        outputStream_.reset();
        inputStream_.reset();

        return false;
    }

    auto outResult = outputStream_->requestStart();

    if (outResult != oboe::Result::OK) {
        setError(
                std::string("Output start failed: ") +
                oboe::convertToText(outResult)
        );

        inputStream_->stop();
        outputStream_->close();
        inputStream_->close();

        outputStream_.reset();
        inputStream_.reset();

        return false;
    }

    running_ = true;
    return true;
}

void NativeAudioEngine::stop() {
    std::lock_guard<std::mutex> lock(lifecycleMutex_);

    running_ = false;

    if (outputStream_) {
        outputStream_->stop();
        outputStream_->close();
        outputStream_.reset();
    }

    if (inputStream_) {
        inputStream_->stop();
        inputStream_->close();
        inputStream_.reset();
    }

    inputPeakDb_.store(-80.0f, std::memory_order_relaxed);
    outputPeakDb_.store(-80.0f, std::memory_order_relaxed);
}

oboe::DataCallbackResult NativeAudioEngine::onAudioReady(
        oboe::AudioStream*,
        void* audioData,
        int32_t numFrames
) {
    auto* output = static_cast<float*>(audioData);

    if (!inputStream_ || numFrames <= 0 || outputChannels_ <= 0) {
        std::fill(
                output,
                output + numFrames * std::max(1, outputChannels_),
                0.0f
        );

        return oboe::DataCallbackResult::Continue;
    }

    const int samplesNeeded = numFrames * inputChannels_;

    if (samplesNeeded > static_cast<int>(inputBuffer_.size())) {
        std::fill(
                output,
                output + numFrames * outputChannels_,
                0.0f
        );

        return oboe::DataCallbackResult::Continue;
    }

    auto readResult = inputStream_->read(
            inputBuffer_.data(),
            numFrames,
            0
    );

    int32_t framesRead = readResult ? readResult.value() : 0;

    if (framesRead < 0) {
        framesRead = 0;
    }

    const bool isMuted = muted_.load(std::memory_order_relaxed);

    const float inTarget =
            inputTarget_.load(std::memory_order_relaxed);

    const float outTarget =
            outputTarget_.load(std::memory_order_relaxed);

    const float bypassTarget =
            bypassTarget_.load(std::memory_order_relaxed)
            ? 1.0f
            : 0.0f;

    const float smoothing =
            1.0f -
            std::exp(
                    -1.0f /
                    (0.010f * static_cast<float>(sampleRate_))
            );

    float inPeak = 0.0f;
    float outPeak = 0.0f;

    for (int32_t frame = 0; frame < numFrames; ++frame) {
        inputCurrent_ +=
                (inTarget - inputCurrent_) * smoothing;

        outputCurrent_ +=
                (outTarget - outputCurrent_) * smoothing;

        bypassMix_ +=
                (bypassTarget - bypassMix_) * smoothing;

        float input = 0.0f;

        if (frame < framesRead) {
            input = inputBuffer_[frame * inputChannels_];
        }

        inPeak = std::max(inPeak, std::abs(input));

        const float normalSignal =
                input * inputCurrent_ * outputCurrent_;

        const float bypassSignal = input;

        float processed =
                normalSignal * (1.0f - bypassMix_) +
                bypassSignal * bypassMix_;

        if (isMuted) {
            processed = 0.0f;
        }

        outPeak = std::max(
                outPeak,
                std::abs(processed)
        );

        for (
                int channel = 0;
                channel < outputChannels_;
                ++channel
                ) {
            output[
                    frame * outputChannels_ + channel
            ] = processed;
        }
    }

    inputPeakDb_.store(
            linearToDb(inPeak),
            std::memory_order_relaxed
    );

    outputPeakDb_.store(
            linearToDb(outPeak),
            std::memory_order_relaxed
    );

    return oboe::DataCallbackResult::Continue;
}

void NativeAudioEngine::onErrorAfterClose(
        oboe::AudioStream*,
        oboe::Result error
) {
    running_ = false;

    setError(
            std::string("Audio stream disconnected: ") +
            oboe::convertToText(error)
    );
}

void NativeAudioEngine::setInputGainDb(float db) {
    inputTarget_.store(
            dbToLinear(std::clamp(db, -60.0f, 12.0f)),
            std::memory_order_relaxed
    );
}

void NativeAudioEngine::setOutputGainDb(float db) {
    outputTarget_.store(
            dbToLinear(std::clamp(db, -60.0f, 12.0f)),
            std::memory_order_relaxed
    );
}

void NativeAudioEngine::setMuted(bool value) {
    muted_.store(value, std::memory_order_relaxed);
}

void NativeAudioEngine::setBypassed(bool value) {
    bypassTarget_.store(value, std::memory_order_relaxed);
}

std::vector<float> NativeAudioEngine::stats() const {
    float inputXrun = 0.0f;
    float outputXrun = 0.0f;
    float buffer = 0.0f;

    if (inputStream_) {
        auto value = inputStream_->getXRunCount();

        if (value) {
            inputXrun = static_cast<float>(value.value());
        }
    }

    if (outputStream_) {
        auto value = outputStream_->getXRunCount();

        if (value) {
            outputXrun = static_cast<float>(value.value());
        }

        buffer = static_cast<float>(
                outputStream_->getBufferSizeInFrames()
        );
    }

    return {
            running_ ? 1.0f : 0.0f,
            static_cast<float>(sampleRate_),
            static_cast<float>(framesPerBurst_),
            buffer,
            static_cast<float>(inputChannels_),
            static_cast<float>(outputChannels_),
            inputPeakDb_.load(std::memory_order_relaxed),
            outputPeakDb_.load(std::memory_order_relaxed),
            inputXrun,
            outputXrun,
            bypassTarget_.load(std::memory_order_relaxed)
            ? 1.0f
            : 0.0f
    };
}

void NativeAudioEngine::setError(
        const std::string& value
) {
    std::lock_guard<std::mutex> lock(errorMutex_);
    error_ = value;
}

std::string NativeAudioEngine::lastError() const {
    std::lock_guard<std::mutex> lock(errorMutex_);
    return error_;
}

float NativeAudioEngine::dbToLinear(float db) {
    return std::pow(10.0f, db / 20.0f);
}

float NativeAudioEngine::linearToDb(float value) {
    return value <= 0.0001f
           ? -80.0f
           : std::max(
                    -80.0f,
                    20.0f * std::log10(value)
            );
}