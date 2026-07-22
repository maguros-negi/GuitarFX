package jp.souta.guitarfx

enum class AudioEngineState {
    STOPPED,
    RUNNING,
    DISCONNECTED,
    ERROR;

    companion object {
        fun fromNativeValue(value: Int): AudioEngineState {
            return when (value) {
                1 -> RUNNING
                2 -> DISCONNECTED
                3 -> ERROR
                else -> STOPPED
            }
        }
    }
}

data class AudioStats(
    val running: Boolean = false,
    val state: AudioEngineState =
        AudioEngineState.STOPPED,

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

    val error: String = ""
)

class AudioEngine {
    companion object {
        init {
            System.loadLibrary("guitarfx")
        }
    }

    external fun create(): Boolean
    external fun start(): Boolean
    external fun stop()
    external fun destroy()

    external fun setInputGainDb(value: Float)
    external fun setOutputGainDb(value: Float)
    external fun setMuted(value: Boolean)
    external fun setBypassed(value: Boolean)

    external fun getStats(): FloatArray
    external fun getLastError(): String

    fun readStats(): AudioStats {
        val values = getStats()

        if (values.size < 18) {
            return AudioStats(
                state = AudioEngineState.ERROR,
                error = "Native stats format error"
            )
        }

        return AudioStats(
            running = values[0] > 0.5f,
            state = AudioEngineState.fromNativeValue(
                values[1].toInt()
            ),
            sampleRate = values[2].toInt(),
            inputFramesPerBurst = values[3].toInt(),
            outputFramesPerBurst = values[4].toInt(),
            inputBufferCapacity = values[5].toInt(),
            outputBufferCapacity = values[6].toInt(),
            inputBufferSize = values[7].toInt(),
            outputBufferSize = values[8].toInt(),
            inputChannels = values[9].toInt(),
            outputChannels = values[10].toInt(),
            inputPeakDb = values[11],
            outputPeakDb = values[12],
            inputXruns = values[13].toInt(),
            outputXruns = values[14].toInt(),
            bypassed = values[15] > 0.5f,
            muted = values[16] > 0.5f,
            bufferAdjustments = values[17].toInt(),
            error = getLastError()
        )
    }
}