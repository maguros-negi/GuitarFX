#pragma once

#include <oboe/Oboe.h>

#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

class NativeAudioEngine : public oboe::AudioStreamDataCallback,
                          public oboe::AudioStreamErrorCallback {
public:
    bool start();
    void stop();

    void processPendingEvents();

    void setInputGainDb(float db);
    void setOutputGainDb(float db);
    void setMuted(bool muted);
    void setBypassed(bool bypassed);

    std::vector<float> stats() const;
    std::string lastError() const;

    oboe::DataCallbackResult onAudioReady(
            oboe::AudioStream* stream,
            void* audioData,
            int32_t numFrames
    ) override;

    void onErrorAfterClose(
            oboe::AudioStream* stream,
            oboe::Result error
    ) override;

private:
    bool openInput();
    bool openOutput();

    void closeStreamsLocked();
    void requestDisconnectHandling(oboe::Result error);
    void setError(const std::string& message);

    static float dbToLinear(float db);
    static float linearToDb(float value);

    std::shared_ptr<oboe::AudioStream> inputStream_;
    std::shared_ptr<oboe::AudioStream> outputStream_;

    mutable std::mutex lifecycleMutex_;
    mutable std::mutex errorMutex_;

    std::string error_;

    std::atomic<bool> running_{false};
    std::atomic<bool> muted_{false};
    std::atomic<bool> bypassTarget_{false};

    std::atomic<int32_t> pendingAudioError_{0};

    std::atomic<float> inputTarget_{1.0f};
    std::atomic<float> outputTarget_{0.501187f};

    float inputCurrent_ = 1.0f;
    float outputCurrent_ = 0.501187f;
    float bypassMix_ = 0.0f;

    std::atomic<float> inputPeakDb_{-80.0f};
    std::atomic<float> outputPeakDb_{-80.0f};

    int32_t sampleRate_ = 0;
    int32_t framesPerBurst_ = 0;
    int32_t inputChannels_ = 0;
    int32_t outputChannels_ = 0;

    std::vector<float> inputBuffer_;
};