package jp.souta.guitarfx

enum class AudioEngineState {
    STOPPED, RUNNING, DISCONNECTED, ERROR;
    companion object {
        fun fromNativeValue(value: Int) = when (value) {
            1 -> RUNNING
            2 -> DISCONNECTED
            3 -> ERROR
            else -> STOPPED
        }
    }
}

data class AudioStats(
    val running: Boolean = false,
    val state: AudioEngineState = AudioEngineState.STOPPED,
    val sampleRate: Int = 0,
    val inputFramesPerBurst: Int = 0,
    val outputFramesPerBurst: Int = 0,
    val inputBufferCapacity: Int = 0,
    val outputBufferCapacity: Int = 0,
    val inputBufferSize: Int = 0,
    val outputBufferSize: Int = 0,
    val inputChannels: Int = 0,
    val outputChannels: Int = 0,
    val inputPeakDb: Float = -80f,
    val outputPeakDb: Float = -80f,
    val inputXruns: Int = 0,
    val outputXruns: Int = 0,
    val bypassed: Boolean = false,
    val muted: Boolean = false,
    val bufferAdjustments: Int = 0,
    val gateEnabled: Boolean = false,
    val driveEnabled: Boolean = false,
    val eqEnabled: Boolean = false,
    val delayEnabled: Boolean = false,
    val limiterEnabled: Boolean = true,
    val gainReductionDb: Float = 0f,
    val clipDetected: Boolean = false,
    val error: String = ""
)

class AudioEngine {
    companion object { init { System.loadLibrary("guitarfx") } }
    external fun create(): Boolean
    external fun start(): Boolean
    external fun stop()
    external fun destroy()
    external fun setInputGainDb(value: Float)
    external fun setOutputGainDb(value: Float)
    external fun setMuted(value: Boolean)
    external fun setBypassed(value: Boolean)
    external fun setEffectEnabled(effectId: Int, enabled: Boolean)
    external fun setNoiseGateParameters(thresholdDb: Float, attackMs: Float, releaseMs: Float)
    external fun setThreeBandEqGains(lowDb: Float, midDb: Float, highDb: Float)
    external fun setOverdriveParameters(drive: Float, tone: Float, level: Float)
    external fun setDelayParameters(timeMs: Float, feedback: Float, mix: Float)
    external fun setLimiterEnabled(enabled: Boolean)
    external fun beginDynamicChainUpdate(): Boolean
    external fun addDynamicEffect(
        instanceId: String,
        modelId: String,
        enabled: Boolean,
        parameters: FloatArray
    ): Boolean
    external fun setDynamicEffectEnabled(instanceId: String, enabled: Boolean): Boolean
    external fun setDynamicEffectParameters(instanceId: String, parameters: FloatArray): Boolean
    external fun getStats(): FloatArray
    external fun getLastError(): String

    fun readStats(): AudioStats {
        val v = getStats()
        if (v.size < 25) return AudioStats(state = AudioEngineState.ERROR, error = "Native stats format error")
        return AudioStats(
            running = v[0] > 0.5f,
            state = AudioEngineState.fromNativeValue(v[1].toInt()),
            sampleRate = v[2].toInt(),
            inputFramesPerBurst = v[3].toInt(), outputFramesPerBurst = v[4].toInt(),
            inputBufferCapacity = v[5].toInt(), outputBufferCapacity = v[6].toInt(),
            inputBufferSize = v[7].toInt(), outputBufferSize = v[8].toInt(),
            inputChannels = v[9].toInt(), outputChannels = v[10].toInt(),
            inputPeakDb = v[11], outputPeakDb = v[12],
            inputXruns = v[13].toInt(), outputXruns = v[14].toInt(),
            bypassed = v[15] > 0.5f, muted = v[16] > 0.5f,
            bufferAdjustments = v[17].toInt(),
            gateEnabled = v[18] > 0.5f, driveEnabled = v[19] > 0.5f,
            eqEnabled = v[20] > 0.5f, delayEnabled = v[21] > 0.5f,
            limiterEnabled = v[22] > 0.5f,
            gainReductionDb = v[23], clipDetected = v[24] > 0.5f,
            error = getLastError()
        )
    }
}


fun EffectInstance.nativeParameters(registry: EffectRegistry): FloatArray {
    val descriptor = registry.descriptor(modelId) ?: return floatArrayOf()
    return descriptor.parameters.map { parameter ->
        parameterValues[parameter.parameterId] ?: parameter.defaultValue
    }.toFloatArray()
}

fun AudioEngine.installDynamicPedalboard(
    state: PedalboardState,
    registry: EffectRegistry
): Boolean {
    if (!beginDynamicChainUpdate()) return false
    return state.effects.all { effect ->
        addDynamicEffect(
            instanceId = effect.instanceId.value,
            modelId = effect.modelId.persistentId,
            enabled = effect.enabled,
            parameters = effect.nativeParameters(registry)
        )
    }
}
