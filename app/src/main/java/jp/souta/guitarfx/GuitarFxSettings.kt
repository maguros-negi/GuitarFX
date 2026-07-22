package jp.souta.guitarfx

data class GuitarFxSettings(
    val version: Int = 1,
    val inputGainDb: Float = 0f, val outputGainDb: Float = -6f,
    val muted: Boolean = false, val bypassed: Boolean = false,
    val limiterEnabled: Boolean = true,
    val gateEnabled: Boolean = false, val gateThresholdDb: Float = -50f,
    val gateAttackMs: Float = 5f, val gateReleaseMs: Float = 120f,
    val driveEnabled: Boolean = false, val driveAmount: Float = 35f,
    val driveTone: Float = 50f, val driveLevel: Float = 50f,
    val eqEnabled: Boolean = false, val eqLowDb: Float = 0f,
    val eqMidDb: Float = 0f, val eqHighDb: Float = 0f,
    val delayEnabled: Boolean = false, val delayTimeMs: Float = 350f,
    val delayFeedback: Float = 35f, val delayMix: Float = 25f,
    val selectedEffectId: Int = 1
)

data class GuitarFxPreset(val name: String, val settings: GuitarFxSettings)
