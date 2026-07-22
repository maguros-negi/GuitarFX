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
    val error: String = ""
)

class AudioEngine {
    companion object {
        init { System.loadLibrary("guitarfx") }
    }

    external fun create(): Boolean
    external fun start(): Boolean
    external fun stop()
    external fun destroy()
    external fun setInputGainDb(value: Float)
    external fun setOutputGainDb(value: Float)
    external fun setMuted(value: Boolean)
    external fun getStats(): FloatArray
    external fun getLastError(): String

    fun readStats(): AudioStats {
        val s = getStats()
        if (s.size < 11) return AudioStats(error = "Native stats format error")
        return AudioStats(
            running = s[0] > 0.5f,
            sampleRate = s[1].toInt(),
            framesPerBurst = s[2].toInt(),
            bufferSize = s[3].toInt(),
            inputChannels = s[4].toInt(),
            outputChannels = s[5].toInt(),
            inputPeakDb = s[6],
            outputPeakDb = s[7],
            inputXruns = s[8].toInt(),
            outputXruns = s[9].toInt(),
            error = getLastError()
        )
    }
}
