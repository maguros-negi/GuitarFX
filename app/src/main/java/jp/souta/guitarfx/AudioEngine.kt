package jp.souta.guitarfx

data class AudioStats(
    val running: Boolean = false,
    val sampleRate: Int = 0,
    val framesPerBurst: Int = 0,
    val bufferSize: Int = 0,
    val inputChannels: Int = 0,
    val outputChannels: Int = 0,
    val inputPeakDb: Float = -80f,
    val outputPeakDb: Float = -80f,
    val inputXruns: Int = 0,
    val outputXruns: Int = 0,
    val bypassed: Boolean = false,
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
        val stats = getStats()

        if (stats.size < 11) {
            return AudioStats(
                error = "Native stats format error"
            )
        }

        return AudioStats(
            running = stats[0] > 0.5f,
            sampleRate = stats[1].toInt(),
            framesPerBurst = stats[2].toInt(),
            bufferSize = stats[3].toInt(),
            inputChannels = stats[4].toInt(),
            outputChannels = stats[5].toInt(),
            inputPeakDb = stats[6],
            outputPeakDb = stats[7],
            inputXruns = stats[8].toInt(),
            outputXruns = stats[9].toInt(),
            bypassed = stats[10] > 0.5f,
            error = getLastError()
        )
    }
}