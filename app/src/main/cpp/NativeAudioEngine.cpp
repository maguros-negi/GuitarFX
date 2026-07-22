#include "NativeAudioEngine.h"

#include <algorithm>
#include <cmath>

bool NativeAudioEngine::openInput() {
    oboe::AudioStreamBuilder builder;

    builder.setDirection(oboe::Direction::Input)
            ->setPerformanceMode(
                    oboe::PerformanceMode::LowLatency
            )
            ->setSharingMode(
                    oboe::SharingMode::Exclusive
            )
            ->setFormat(oboe::AudioFormat::Float)
            ->setChannelCount(1)
            ->setInputPreset(
                    oboe::InputPreset::Generic
            )
            ->setErrorCallback(this);

    auto result = builder.openStream(inputStream_);

    if (result != oboe::Result::OK) {
        builder.setSharingMode(
                oboe::SharingMode::Shared
        );

        result = builder.openStream(inputStream_);
    }

    if (
            result != oboe::Result::OK ||
            !inputStream_
            ) {
        setError(
                std::string("Input open failed: ") +
                oboe::convertToText(result)
        );

        state_.store(
                EngineState::Error,
                std::memory_order_release
        );

        return false;
    }

    sampleRate_.store(
            inputStream_->getSampleRate(),
            std::memory_order_relaxed
    );

    inputFramesPerBurst_.store(
            inputStream_->getFramesPerBurst(),
            std::memory_order_relaxed
    );

    inputChannels_.store(
            inputStream_->getChannelCount(),
            std::memory_order_relaxed
    );

    inputBufferCapacity_.store(
            inputStream_->getBufferCapacityInFrames(),
            std::memory_order_relaxed
    );

    inputBufferSize_.store(
            inputStream_->getBufferSizeInFrames(),
            std::memory_order_relaxed
    );

    return true;
}

bool NativeAudioEngine::openOutput() {
    oboe::AudioStreamBuilder builder;

    builder.setDirection(oboe::Direction::Output)
            ->setPerformanceMode(
                    oboe::PerformanceMode::LowLatency
            )
            ->setSharingMode(
                    oboe::SharingMode::Exclusive
            )
            ->setFormat(oboe::AudioFormat::Float)
            ->setSampleRate(
                    sampleRate_.load(
                            std::memory_order_relaxed
                    )
            )
            ->setChannelCount(2)
            ->setUsage(oboe::Usage::Game)
            ->setDataCallback(this)
            ->setErrorCallback(this);

    auto result = builder.openStream(outputStream_);

    if (result != oboe::Result::OK) {
        builder.setSharingMode(
                oboe::SharingMode::Shared
        );

        result = builder.openStream(outputStream_);
    }

    if (
            result != oboe::Result::OK ||
            !outputStream_
            ) {
        setError(
                std::string("Output open failed: ") +
                oboe::convertToText(result)
        );

        state_.store(
                EngineState::Error,
                std::memory_order_release
        );

        return false;
    }

    outputFramesPerBurst_.store(
            outputStream_->getFramesPerBurst(),
            std::memory_order_relaxed
    );

    outputChannels_.store(
            outputStream_->getChannelCount(),
            std::memory_order_relaxed
    );

    outputBufferCapacity_.store(
            outputStream_->getBufferCapacityInFrames(),
            std::memory_order_relaxed
    );

    const int32_t inputBurst =
            inputFramesPerBurst_.load(
                    std::memory_order_relaxed
            );

    const int32_t outputBurst =
            outputFramesPerBurst_.load(
                    std::memory_order_relaxed
            );

    const int32_t largestBurst =
            std::max(inputBurst, outputBurst);

    const int32_t capacity =
            std::max(4096, largestBurst * 8);

    const int32_t channelCount =
            std::max(
                    1,
                    inputChannels_.load(
                            std::memory_order_relaxed
                    )
            );

    inputBuffer_.assign(
            capacity * channelCount,
            0.0f
    );

    effectChain_.prepare(
            static_cast<double>(
                    sampleRate_.load(std::memory_order_relaxed)
            ),
            largestBurst
    );
    outputLimiter_.prepare(
            static_cast<double>(sampleRate_.load(std::memory_order_relaxed))
    );
    clipHoldSamples_ = 0;
    clipDetected_.store(false, std::memory_order_relaxed);

    const int32_t initialBufferRequest =
            std::max(
                    1,
                    outputStream_->getFramesPerBurst() * 2
            );

    const auto bufferResult =
            outputStream_->setBufferSizeInFrames(
                    initialBufferRequest
            );

    if (bufferResult) {
        outputBufferSize_.store(
                bufferResult.value(),
                std::memory_order_relaxed
        );
    } else {
        outputBufferSize_.store(
                outputStream_->getBufferSizeInFrames(),
                std::memory_order_relaxed
        );
    }

    return true;
}

bool NativeAudioEngine::start() {
    std::lock_guard<std::mutex> lock(
            lifecycleMutex_
    );

    if (
            running_.load(
                    std::memory_order_acquire
            )
            ) {
        return true;
    }

    closeStreamsLocked();
    resetMonitoringLocked();

    pendingAudioError_.store(
            0,
            std::memory_order_release
    );

    setError("");

    state_.store(
            EngineState::Stopped,
            std::memory_order_release
    );

    if (!openInput()) {
        closeStreamsLocked();
        return false;
    }

    if (!openOutput()) {
        closeStreamsLocked();
        return false;
    }

    inputCurrent_ =
            inputTarget_.load(
                    std::memory_order_relaxed
            );

    outputCurrent_ =
            outputTarget_.load(
                    std::memory_order_relaxed
            );

    bypassMix_ =
            bypassTarget_.load(
                    std::memory_order_relaxed
            )
            ? 1.0f
            : 0.0f;

    inputPeakDb_.store(
            -80.0f,
            std::memory_order_relaxed
    );

    outputPeakDb_.store(
            -80.0f,
            std::memory_order_relaxed
    );

    const auto inputStartResult =
            inputStream_->requestStart();

    if (
            inputStartResult != oboe::Result::OK
            ) {
        setError(
                std::string("Input start failed: ") +
                oboe::convertToText(
                        inputStartResult
                )
        );

        state_.store(
                EngineState::Error,
                std::memory_order_release
        );

        closeStreamsLocked();
        return false;
    }

    const auto outputStartResult =
            outputStream_->requestStart();

    if (
            outputStartResult != oboe::Result::OK
            ) {
        setError(
                std::string("Output start failed: ") +
                oboe::convertToText(
                        outputStartResult
                )
        );

        state_.store(
                EngineState::Error,
                std::memory_order_release
        );

        closeStreamsLocked();
        return false;
    }

    updateStreamStatisticsLocked();

    running_.store(
            true,
            std::memory_order_release
    );

    state_.store(
            EngineState::Running,
            std::memory_order_release
    );

    return true;
}

void NativeAudioEngine::stop() {
    std::lock_guard<std::mutex> lock(
            lifecycleMutex_
    );

    running_.store(
            false,
            std::memory_order_release
    );

    pendingAudioError_.store(
            0,
            std::memory_order_release
    );

    closeStreamsLocked();
    resetMonitoringLocked();

    inputPeakDb_.store(
            -80.0f,
            std::memory_order_relaxed
    );

    outputPeakDb_.store(
            -80.0f,
            std::memory_order_relaxed
    );

    state_.store(
            EngineState::Stopped,
            std::memory_order_release
    );
}

void NativeAudioEngine::closeStreamsLocked() {
    if (outputStream_) {
        outputStream_->requestStop();
        outputStream_->close();
        outputStream_.reset();
    }

    if (inputStream_) {
        inputStream_->requestStop();
        inputStream_->close();
        inputStream_.reset();
    }

    inputBufferSize_.store(
            0,
            std::memory_order_relaxed
    );

    outputBufferSize_.store(
            0,
            std::memory_order_relaxed
    );
}

void NativeAudioEngine::resetMonitoringLocked() {
    previousInputXRunCount_ = 0;
    previousOutputXRunCount_ = 0;

    inputXRunCount_.store(
            0,
            std::memory_order_relaxed
    );

    outputXRunCount_.store(
            0,
            std::memory_order_relaxed
    );

    bufferAdjustmentCount_.store(
            0,
            std::memory_order_relaxed
    );

    xRunBaselineReady_ = false;
}

void NativeAudioEngine::requestDisconnectHandling(
        oboe::Result error
) {
    int32_t errorCode =
            static_cast<int32_t>(error);

    if (errorCode == 0) {
        errorCode = static_cast<int32_t>(
                oboe::Result::ErrorDisconnected
        );
    }

    int32_t expected = 0;

    pendingAudioError_.compare_exchange_strong(
            expected,
            errorCode,
            std::memory_order_release,
            std::memory_order_relaxed
    );

    running_.store(
            false,
            std::memory_order_release
    );

    state_.store(
            EngineState::Disconnected,
            std::memory_order_release
    );
}

void NativeAudioEngine::processMaintenance() {
    const int32_t pendingError =
            pendingAudioError_.exchange(
                    0,
                    std::memory_order_acq_rel
            );

    if (pendingError != 0) {
        const auto error =
                static_cast<oboe::Result>(
                        pendingError
                );

        {
            std::lock_guard<std::mutex> lock(
                    lifecycleMutex_
            );

            running_.store(
                    false,
                    std::memory_order_release
            );

            closeStreamsLocked();

            inputPeakDb_.store(
                    -80.0f,
                    std::memory_order_relaxed
            );

            outputPeakDb_.store(
                    -80.0f,
                    std::memory_order_relaxed
            );

            state_.store(
                    EngineState::Disconnected,
                    std::memory_order_release
            );
        }

        setError(
                std::string(
                        "Audio device disconnected. "
                        "Reconnect the device and press START: "
                ) +
                oboe::convertToText(error)
        );

        return;
    }

    if (
            !running_.load(
                    std::memory_order_acquire
            )
            ) {
        return;
    }

    std::lock_guard<std::mutex> lock(
            lifecycleMutex_
    );

    if (
            !running_.load(
                    std::memory_order_acquire
            ) ||
            !inputStream_ ||
            !outputStream_
            ) {
        return;
    }

    updateStreamStatisticsLocked();
    monitorXRunsAndTuneBufferLocked();
    updateStreamStatisticsLocked();
}

void NativeAudioEngine::updateStreamStatisticsLocked() {
    if (inputStream_) {
        inputFramesPerBurst_.store(
                inputStream_->getFramesPerBurst(),
                std::memory_order_relaxed
        );

        inputBufferCapacity_.store(
                inputStream_
                        ->getBufferCapacityInFrames(),
                std::memory_order_relaxed
        );

        inputBufferSize_.store(
                inputStream_->getBufferSizeInFrames(),
                std::memory_order_relaxed
        );

        const auto inputXRunResult =
                inputStream_->getXRunCount();

        if (inputXRunResult) {
            inputXRunCount_.store(
                    inputXRunResult.value(),
                    std::memory_order_relaxed
            );
        }
    }

    if (outputStream_) {
        outputFramesPerBurst_.store(
                outputStream_->getFramesPerBurst(),
                std::memory_order_relaxed
        );

        outputBufferCapacity_.store(
                outputStream_
                        ->getBufferCapacityInFrames(),
                std::memory_order_relaxed
        );

        outputBufferSize_.store(
                outputStream_
                        ->getBufferSizeInFrames(),
                std::memory_order_relaxed
        );

        const auto outputXRunResult =
                outputStream_->getXRunCount();

        if (outputXRunResult) {
            outputXRunCount_.store(
                    outputXRunResult.value(),
                    std::memory_order_relaxed
            );
        }
    }
}

void NativeAudioEngine::monitorXRunsAndTuneBufferLocked() {
    if (!outputStream_) {
        return;
    }

    const int32_t currentInputXRuns =
            inputXRunCount_.load(
                    std::memory_order_relaxed
            );

    const int32_t currentOutputXRuns =
            outputXRunCount_.load(
                    std::memory_order_relaxed
            );

    if (!xRunBaselineReady_) {
        previousInputXRunCount_ =
                currentInputXRuns;

        previousOutputXRunCount_ =
                currentOutputXRuns;

        xRunBaselineReady_ = true;
        return;
    }

    const bool inputXRunIncreased =
            currentInputXRuns >
            previousInputXRunCount_;

    const bool outputXRunIncreased =
            currentOutputXRuns >
            previousOutputXRunCount_;

    previousInputXRunCount_ =
            currentInputXRuns;

    previousOutputXRunCount_ =
            currentOutputXRuns;

    if (
            !inputXRunIncreased &&
            !outputXRunIncreased
            ) {
        return;
    }

    const int32_t currentBufferSize =
            outputStream_->getBufferSizeInFrames();

    const int32_t outputBurst =
            std::max(
                    1,
                    outputStream_->getFramesPerBurst()
            );

    const int32_t bufferCapacity =
            outputStream_
                    ->getBufferCapacityInFrames();

    if (
            bufferCapacity <= 0 ||
            currentBufferSize >= bufferCapacity
            ) {
        return;
    }

    const int32_t requestedBufferSize =
            std::min(
                    bufferCapacity,
                    currentBufferSize + outputBurst
            );

    const auto result =
            outputStream_->setBufferSizeInFrames(
                    requestedBufferSize
            );

    if (result) {
        const int32_t appliedBufferSize =
                result.value();

        outputBufferSize_.store(
                appliedBufferSize,
                std::memory_order_relaxed
        );

        if (
                appliedBufferSize >
                currentBufferSize
                ) {
            bufferAdjustmentCount_.fetch_add(
                    1,
                    std::memory_order_relaxed
            );
        }
    }
}

oboe::DataCallbackResult
NativeAudioEngine::onAudioReady(
        oboe::AudioStream*,
        void* audioData,
        int32_t numFrames
) {
    auto* output =
            static_cast<float*>(audioData);

    const int32_t outputChannelCount =
            outputChannels_.load(
                    std::memory_order_relaxed
            );

    const int32_t inputChannelCount =
            inputChannels_.load(
                    std::memory_order_relaxed
            );

    if (
            pendingAudioError_.load(
                    std::memory_order_acquire
            ) != 0
            ) {
        if (
                output != nullptr &&
                numFrames > 0 &&
                outputChannelCount > 0
                ) {
            std::fill(
                    output,
                    output +
                    numFrames *
                    outputChannelCount,
                    0.0f
            );
        }

        return oboe::DataCallbackResult::Stop;
    }

    if (
            !inputStream_ ||
            output == nullptr ||
            numFrames <= 0 ||
            outputChannelCount <= 0 ||
            inputChannelCount <= 0
            ) {
        if (
                output != nullptr &&
                numFrames > 0
                ) {
            std::fill(
                    output,
                    output +
                    numFrames *
                    std::max(
                            1,
                            outputChannelCount
                    ),
                    0.0f
            );
        }

        return oboe::DataCallbackResult::Continue;
    }

    const int32_t samplesNeeded =
            numFrames * inputChannelCount;

    if (
            samplesNeeded >
            static_cast<int32_t>(
                    inputBuffer_.size()
            )
            ) {
        std::fill(
                output,
                output +
                numFrames *
                outputChannelCount,
                0.0f
        );

        return oboe::DataCallbackResult::Continue;
    }

    const auto readResult =
            inputStream_->read(
                    inputBuffer_.data(),
                    numFrames,
                    0
            );

    if (!readResult) {
        requestDisconnectHandling(
                readResult.error()
        );

        std::fill(
                output,
                output +
                numFrames *
                outputChannelCount,
                0.0f
        );

        return oboe::DataCallbackResult::Stop;
    }

    int32_t framesRead = readResult.value();

    if (framesRead < 0) {
        framesRead = 0;
    }

    const bool isMuted =
            muted_.load(
                    std::memory_order_relaxed
            );

    const float inputTarget =
            inputTarget_.load(
                    std::memory_order_relaxed
            );

    const float outputTarget =
            outputTarget_.load(
                    std::memory_order_relaxed
            );

    const float bypassTarget =
            bypassTarget_.load(
                    std::memory_order_relaxed
            )
            ? 1.0f
            : 0.0f;

    const float validSampleRate =
            static_cast<float>(
                    std::max(
                            1,
                            sampleRate_.load(
                                    std::memory_order_relaxed
                            )
                    )
            );

    const float smoothing =
            1.0f -
            std::exp(
                    -1.0f /
                    (0.010f * validSampleRate)
            );

    float inputPeak = 0.0f;
    float outputPeak = 0.0f;

    for (
            int32_t frame = 0;
            frame < numFrames;
            ++frame
            ) {
        inputCurrent_ +=
                (inputTarget - inputCurrent_) *
                smoothing;

        outputCurrent_ +=
                (outputTarget - outputCurrent_) *
                smoothing;

        bypassMix_ +=
                (bypassTarget - bypassMix_) *
                smoothing;

        float input = 0.0f;

        if (frame < framesRead) {
            input =
                    inputBuffer_[
                            frame * inputChannelCount
                    ];
        }

        inputPeak = std::max(
                inputPeak,
                std::abs(input)
        );

        const float preEffectSignal =
                input * inputCurrent_;
        const float effectSignal =
                effectChain_.processSample(preEffectSignal);
        const float normalSignal =
                effectSignal * outputCurrent_;

        if (std::abs(normalSignal) >= 1.0f) {
            clipHoldSamples_ = static_cast<int32_t>(validSampleRate);
        } else if (clipHoldSamples_ > 0) {
            --clipHoldSamples_;
        }
        clipDetected_.store(clipHoldSamples_ > 0, std::memory_order_relaxed);

        const float limitedSignal =
                outputLimiter_.processSample(normalSignal);
        const float bypassSignal = input;

        float processed =
                limitedSignal *
                (1.0f - bypassMix_) +
                bypassSignal *
                bypassMix_;

        if (isMuted) {
            processed = 0.0f;
        }

        outputPeak = std::max(
                outputPeak,
                std::abs(processed)
        );

        for (
                int32_t channel = 0;
                channel < outputChannelCount;
                ++channel
                ) {
            output[
                    frame *
                    outputChannelCount +
                    channel
            ] = processed;
        }
    }

    inputPeakDb_.store(
            linearToDb(inputPeak),
            std::memory_order_relaxed
    );

    outputPeakDb_.store(
            linearToDb(outputPeak),
            std::memory_order_relaxed
    );

    return oboe::DataCallbackResult::Continue;
}

void NativeAudioEngine::onErrorAfterClose(
        oboe::AudioStream*,
        oboe::Result error
) {
    requestDisconnectHandling(error);
}

void NativeAudioEngine::setInputGainDb(
        float db
) {
    inputTarget_.store(
            dbToLinear(
                    std::clamp(
                            db,
                            -60.0f,
                            12.0f
                    )
            ),
            std::memory_order_relaxed
    );
}

void NativeAudioEngine::setOutputGainDb(
        float db
) {
    outputTarget_.store(
            dbToLinear(
                    std::clamp(
                            db,
                            -60.0f,
                            12.0f
                    )
            ),
            std::memory_order_relaxed
    );
}

void NativeAudioEngine::setMuted(
        bool value
) {
    muted_.store(
            value,
            std::memory_order_relaxed
    );
}

void NativeAudioEngine::setBypassed(
        bool value
) {
    bypassTarget_.store(
            value,
            std::memory_order_relaxed
    );
}
void NativeAudioEngine::setEffectEnabled(
        int32_t effectId,
        bool enabled
) {
    if (
            effectId < 0 ||
            effectId >= static_cast<int32_t>(EffectId::Count)
            ) {
        return;
    }

    effectChain_.setEffectEnabled(
            static_cast<EffectId>(effectId),
            enabled
    );
}
void NativeAudioEngine::setNoiseGateParameters(
        float thresholdDb,
        float attackMs,
        float releaseMs
) {
    effectChain_.setNoiseGateParameters(thresholdDb, attackMs, releaseMs);
}
void NativeAudioEngine::setThreeBandEqGains(
        float lowDb,
        float midDb,
        float highDb
) {
    effectChain_.setThreeBandEqGains(lowDb, midDb, highDb);
}
void NativeAudioEngine::setOverdriveParameters(
        float drive,
        float tone,
        float level
) {
    effectChain_.setOverdriveParameters(drive, tone, level);
}
void NativeAudioEngine::setDelayParameters(
        float timeMs,
        float feedback,
        float mix
) {
    effectChain_.setDelayParameters(timeMs, feedback, mix);
}
void NativeAudioEngine::setLimiterEnabled(bool enabled) {
    outputLimiter_.setEnabled(enabled);
}

std::vector<float>
NativeAudioEngine::stats() const {
    return {
            running_.load(
                    std::memory_order_acquire
            )
            ? 1.0f
            : 0.0f,

            static_cast<float>(
                    state_.load(
                            std::memory_order_acquire
                    )
            ),

            static_cast<float>(
                    sampleRate_.load(
                            std::memory_order_relaxed
                    )
            ),

            static_cast<float>(
                    inputFramesPerBurst_.load(
                            std::memory_order_relaxed
                    )
            ),

            static_cast<float>(
                    outputFramesPerBurst_.load(
                            std::memory_order_relaxed
                    )
            ),

            static_cast<float>(
                    inputBufferCapacity_.load(
                            std::memory_order_relaxed
                    )
            ),

            static_cast<float>(
                    outputBufferCapacity_.load(
                            std::memory_order_relaxed
                    )
            ),

            static_cast<float>(
                    inputBufferSize_.load(
                            std::memory_order_relaxed
                    )
            ),

            static_cast<float>(
                    outputBufferSize_.load(
                            std::memory_order_relaxed
                    )
            ),

            static_cast<float>(
                    inputChannels_.load(
                            std::memory_order_relaxed
                    )
            ),

            static_cast<float>(
                    outputChannels_.load(
                            std::memory_order_relaxed
                    )
            ),

            inputPeakDb_.load(
                    std::memory_order_relaxed
            ),

            outputPeakDb_.load(
                    std::memory_order_relaxed
            ),

            static_cast<float>(
                    inputXRunCount_.load(
                            std::memory_order_relaxed
                    )
            ),

            static_cast<float>(
                    outputXRunCount_.load(
                            std::memory_order_relaxed
                    )
            ),

            bypassTarget_.load(
                    std::memory_order_relaxed
            )
            ? 1.0f
            : 0.0f,

            muted_.load(
                    std::memory_order_relaxed
            )
            ? 1.0f
            : 0.0f,

            static_cast<float>(
                    bufferAdjustmentCount_.load(
                            std::memory_order_relaxed
                    )
            ),
            effectChain_.isEffectEnabled(EffectId::Gate) ? 1.0f : 0.0f,
            effectChain_.isEffectEnabled(EffectId::Drive) ? 1.0f : 0.0f,
            effectChain_.isEffectEnabled(EffectId::Eq) ? 1.0f : 0.0f,
            effectChain_.isEffectEnabled(EffectId::Delay) ? 1.0f : 0.0f,
            outputLimiter_.isEnabled() ? 1.0f : 0.0f,
            outputLimiter_.gainReductionDb(),
            clipDetected_.load(std::memory_order_relaxed) ? 1.0f : 0.0f
    };
}

void NativeAudioEngine::setError(
        const std::string& value
) {
    std::lock_guard<std::mutex> lock(
            errorMutex_
    );

    error_ = value;
}

std::string
NativeAudioEngine::lastError() const {
    std::lock_guard<std::mutex> lock(
            errorMutex_
    );

    return error_;
}

float NativeAudioEngine::dbToLinear(
        float db
) {
    return std::pow(
            10.0f,
            db / 20.0f
    );
}

float NativeAudioEngine::linearToDb(
        float value
) {
    if (value <= 0.0001f) {
        return -80.0f;
    }

    return std::max(
            -80.0f,
            20.0f * std::log10(value)
    );
}