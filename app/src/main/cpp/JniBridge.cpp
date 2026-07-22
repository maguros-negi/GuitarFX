#include "NativeAudioEngine.h"

#include <jni.h>

#include <memory>
#include <mutex>
#include <vector>

static std::unique_ptr<NativeAudioEngine> engine;
static std::mutex engineMutex;

extern "C"
JNIEXPORT jboolean JNICALL
Java_jp_souta_guitarfx_AudioEngine_create(
        JNIEnv*,
        jobject
) {
    std::lock_guard<std::mutex> lock(
            engineMutex
    );

    if (!engine) {
        engine =
                std::make_unique<
                        NativeAudioEngine
                >();
    }

    return JNI_TRUE;
}

extern "C"
JNIEXPORT jboolean JNICALL
Java_jp_souta_guitarfx_AudioEngine_start(
        JNIEnv*,
        jobject
) {
    std::lock_guard<std::mutex> lock(
            engineMutex
    );

    if (!engine) {
        return JNI_FALSE;
    }

    return engine->start()
           ? JNI_TRUE
           : JNI_FALSE;
}

extern "C"
JNIEXPORT void JNICALL
Java_jp_souta_guitarfx_AudioEngine_stop(
        JNIEnv*,
        jobject
) {
    std::lock_guard<std::mutex> lock(
            engineMutex
    );

    if (engine) {
        engine->stop();
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_jp_souta_guitarfx_AudioEngine_destroy(
        JNIEnv*,
        jobject
) {
    std::lock_guard<std::mutex> lock(
            engineMutex
    );

    if (engine) {
        engine->stop();
    }

    engine.reset();
}

extern "C"
JNIEXPORT void JNICALL
Java_jp_souta_guitarfx_AudioEngine_setInputGainDb(
        JNIEnv*,
        jobject,
        jfloat value
) {
    std::lock_guard<std::mutex> lock(
            engineMutex
    );

    if (engine) {
        engine->setInputGainDb(value);
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_jp_souta_guitarfx_AudioEngine_setOutputGainDb(
        JNIEnv*,
        jobject,
        jfloat value
) {
    std::lock_guard<std::mutex> lock(
            engineMutex
    );

    if (engine) {
        engine->setOutputGainDb(value);
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_jp_souta_guitarfx_AudioEngine_setMuted(
        JNIEnv*,
        jobject,
        jboolean value
) {
    std::lock_guard<std::mutex> lock(
            engineMutex
    );

    if (engine) {
        engine->setMuted(
                value == JNI_TRUE
        );
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_jp_souta_guitarfx_AudioEngine_setBypassed(
        JNIEnv*,
        jobject,
        jboolean value
) {
    std::lock_guard<std::mutex> lock(
            engineMutex
    );

    if (engine) {
        engine->setBypassed(
                value == JNI_TRUE
        );
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_jp_souta_guitarfx_AudioEngine_setEffectEnabled(
        JNIEnv*,
        jobject,
        jint effectId,
        jboolean enabled
) {
    std::lock_guard<std::mutex> lock(
            engineMutex
    );

    if (engine) {
        engine->setEffectEnabled(
                static_cast<int32_t>(effectId),
                enabled == JNI_TRUE
        );
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_jp_souta_guitarfx_AudioEngine_setNoiseGateParameters(
        JNIEnv*,
        jobject,
        jfloat thresholdDb,
        jfloat attackMs,
        jfloat releaseMs
) {
    std::lock_guard<std::mutex> lock(engineMutex);
    if (engine) {
        engine->setNoiseGateParameters(thresholdDb, attackMs, releaseMs);
    }
}

extern "C"
JNIEXPORT jfloatArray JNICALL
Java_jp_souta_guitarfx_AudioEngine_getStats(
        JNIEnv* env,
        jobject
) {
    std::lock_guard<std::mutex> lock(
            engineMutex
    );

    if (engine) {
        engine->processMaintenance();
    }

    const auto values =
            engine
            ? engine->stats()
            : std::vector<float>(
                    22,
                    0.0f
            );

    jfloatArray result =
            env->NewFloatArray(
                    static_cast<jsize>(
                            values.size()
                    )
            );

    if (result == nullptr) {
        return nullptr;
    }

    env->SetFloatArrayRegion(
            result,
            0,
            static_cast<jsize>(
                    values.size()
            ),
            values.data()
    );

    return result;
}

extern "C"
JNIEXPORT jstring JNICALL
Java_jp_souta_guitarfx_AudioEngine_getLastError(
        JNIEnv* env,
        jobject
) {
    std::lock_guard<std::mutex> lock(
            engineMutex
    );

    const std::string message =
            engine
            ? engine->lastError()
            : "Engine is not created";

    return env->NewStringUTF(
            message.c_str()
    );
}