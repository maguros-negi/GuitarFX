package jp.souta.guitarfx

import java.util.UUID

enum class EffectModelId(val persistentId: String) {
    NOISE_GATE("noise_gate"),
    TIGHT_NOISE_GATE("tight_noise_gate"),
    CLASSIC_OVERDRIVE("classic_overdrive"),
    MID_BOOST_OVERDRIVE("mid_boost_overdrive"),
    TRANSPARENT_OVERDRIVE("transparent_overdrive"),
    DISTORTION("distortion"),
    MODERN_DISTORTION("modern_distortion"),
    VINTAGE_FUZZ("vintage_fuzz"),
    GATED_FUZZ("gated_fuzz"),
    THREE_BAND_EQ("three_band_eq"),
    SCOOP_EQ("scoop_eq"),
    DIGITAL_DELAY("digital_delay"),
    SLAPBACK_DELAY("slapback_delay"),
    AMBIENT_DELAY("ambient_delay"),
    PREAMP("preamp"),
    CLEAN_PREAMP("clean_preamp"),
    HIGH_GAIN_PREAMP("high_gain_preamp"),
    CABINET("cabinet"),
    CABINET_1X12("cabinet_1x12"),
    CABINET_2X12("cabinet_2x12"),
    CABINET_4X12("cabinet_4x12");

    companion object {
        fun fromPersistentId(value: String): EffectModelId? =
            entries.firstOrNull { it.persistentId == value }
    }
}

enum class EffectCategory(val displayName: String) {
    NOISE_GATE("NOISE GATE"),
    OVERDRIVE("OVERDRIVE"),
    DISTORTION("DISTORTION"),
    FUZZ("FUZZ"),
    EQUALIZER("EQUALIZER"),
    DELAY("DELAY"),
    PREAMP("PREAMP"),
    CABINET("CABINET")
}

enum class ParameterValueType { DECIMAL, INTEGER, PERCENT, DECIBEL, DECIBEL_FS, MILLISECONDS, ENUM }

data class ParameterDescriptor(
    val parameterId: String,
    val displayName: String,
    val minimumValue: Float,
    val maximumValue: Float,
    val defaultValue: Float,
    val valueType: ParameterValueType,
    val unit: String = "",
    val stepSize: Float? = null,
    val enumOptions: List<String> = emptyList()
) {
    init {
        require(parameterId.isNotBlank())
        require(displayName.isNotBlank())
        require(minimumValue.isFinite() && maximumValue.isFinite() && defaultValue.isFinite())
        require(minimumValue <= maximumValue)
        require(defaultValue in minimumValue..maximumValue)
        require(stepSize == null || stepSize > 0f)
        require(valueType != ParameterValueType.ENUM || enumOptions.isNotEmpty())
    }

    fun normalize(value: Float): Float =
        (if (value.isFinite()) value else defaultValue).coerceIn(minimumValue, maximumValue)
}

data class EffectDescriptor(
    val modelId: EffectModelId,
    val displayName: String,
    val shortName: String,
    val category: EffectCategory,
    val description: String,
    val parameters: List<ParameterDescriptor>,
    val available: Boolean = true
) {
    init { require(parameters.map { it.parameterId }.distinct().size == parameters.size) }
    fun defaultParameters(): Map<String, Float> = parameters.associate { it.parameterId to it.defaultValue }
    fun parameter(id: String): ParameterDescriptor? = parameters.firstOrNull { it.parameterId == id }
    fun normalizeParameters(values: Map<String, Float>): Map<String, Float> =
        parameters.associate { it.parameterId to it.normalize(values[it.parameterId] ?: it.defaultValue) }
}

@JvmInline
value class EffectInstanceId(val value: String) {
    init { require(value.isNotBlank()) }
    companion object { fun create() = EffectInstanceId(UUID.randomUUID().toString()) }
}

data class EffectInstance(
    val instanceId: EffectInstanceId,
    val modelId: EffectModelId,
    val enabled: Boolean = false,
    val parameterValues: Map<String, Float>,
    val customName: String? = null
)

data class PedalboardState(
    val formatVersion: Int = CURRENT_FORMAT_VERSION,
    val effects: List<EffectInstance> = emptyList(),
    val selectedInstanceId: EffectInstanceId? = null
) {
    val selectedEffect: EffectInstance? get() = effects.firstOrNull { it.instanceId == selectedInstanceId }

    fun addEffect(modelId: EffectModelId, registry: EffectRegistry, insertionIndex: Int = effects.size): PedalboardState {
        val descriptor = registry.descriptor(modelId) ?: return this
        if (!descriptor.available) return this
        val instance = EffectInstance(
            instanceId = EffectInstanceId.create(),
            modelId = modelId,
            parameterValues = descriptor.defaultParameters()
        )
        val list = effects.toMutableList().apply { add(insertionIndex.coerceIn(0, effects.size), instance) }
        return copy(effects = list, selectedInstanceId = instance.instanceId)
    }

    fun removeEffect(id: EffectInstanceId): PedalboardState {
        val index = effects.indexOfFirst { it.instanceId == id }
        if (index < 0) return this
        val list = effects.filterNot { it.instanceId == id }
        val selection = if (selectedInstanceId != id) selectedInstanceId else
            list.getOrNull(index)?.instanceId ?: list.lastOrNull()?.instanceId
        return copy(effects = list, selectedInstanceId = selection)
    }

    fun moveEffect(id: EffectInstanceId, targetIndex: Int): PedalboardState {
        val source = effects.indexOfFirst { it.instanceId == id }
        if (source < 0 || effects.size < 2) return this
        val target = targetIndex.coerceIn(0, effects.lastIndex)
        if (source == target) return this
        val list = effects.toMutableList()
        val item = list.removeAt(source)
        list.add(target, item)
        return copy(effects = list)
    }

    fun moveEffectLeft(id: EffectInstanceId): PedalboardState {
        val index = effects.indexOfFirst { it.instanceId == id }
        return if (index > 0) moveEffect(id, index - 1) else this
    }

    fun moveEffectRight(id: EffectInstanceId): PedalboardState {
        val index = effects.indexOfFirst { it.instanceId == id }
        return if (index in 0 until effects.lastIndex) moveEffect(id, index + 1) else this
    }

    fun selectEffect(id: EffectInstanceId?): PedalboardState =
        if (id == null || effects.any { it.instanceId == id }) copy(selectedInstanceId = id) else this

    fun setEffectEnabled(id: EffectInstanceId, enabled: Boolean): PedalboardState =
        update(id) { it.copy(enabled = enabled) }

    fun setParameter(id: EffectInstanceId, parameterId: String, value: Float, registry: EffectRegistry): PedalboardState {
        val effect = effects.firstOrNull { it.instanceId == id } ?: return this
        val parameter = registry.descriptor(effect.modelId)?.parameter(parameterId) ?: return this
        return update(id) { it.copy(parameterValues = it.parameterValues + (parameterId to parameter.normalize(value))) }
    }

    private fun update(id: EffectInstanceId, transform: (EffectInstance) -> EffectInstance): PedalboardState {
        if (effects.none { it.instanceId == id }) return this
        return copy(effects = effects.map { if (it.instanceId == id) transform(it) else it })
    }

    companion object { const val CURRENT_FORMAT_VERSION = 2 }
}

class EffectRegistry private constructor(descriptors: List<EffectDescriptor>) {
    private val byId = descriptors.associateBy { it.modelId }
    init { require(byId.size == descriptors.size) }
    fun descriptor(id: EffectModelId): EffectDescriptor? = byId[id]
    fun availableEffects(): List<EffectDescriptor> = byId.values.filter { it.available }
        .sortedWith(compareBy({ it.category.ordinal }, { it.displayName }))

    companion object { val Default by lazy { EffectRegistry(defaultDescriptors()) } }
}

private fun p(id: String, name: String, min: Float, max: Float, default: Float, type: ParameterValueType, unit: String) =
    ParameterDescriptor(id, name, min, max, default, type, unit)

private fun gate(model: EffectModelId, name: String, threshold: Float, attack: Float, release: Float) =
    EffectDescriptor(model, name, "GATE", EffectCategory.NOISE_GATE, "", listOf(
        p("threshold_db", "THRESHOLD", -80f, -10f, threshold, ParameterValueType.DECIBEL_FS, "dBFS"),
        p("attack_ms", "ATTACK", 1f, 100f, attack, ParameterValueType.MILLISECONDS, "ms"),
        p("release_ms", "RELEASE", 10f, 1000f, release, ParameterValueType.MILLISECONDS, "ms")))

private fun overdrive(model: EffectModelId, name: String, drive: Float, tone: Float, level: Float) =
    EffectDescriptor(model, name, "OD", EffectCategory.OVERDRIVE, "", listOf(
        p("drive", "DRIVE", 0f, 100f, drive, ParameterValueType.PERCENT, "%"),
        p("tone", "TONE", 0f, 100f, tone, ParameterValueType.PERCENT, "%"),
        p("level", "LEVEL", 0f, 100f, level, ParameterValueType.PERCENT, "%")))

private fun distortion(model: EffectModelId, name: String, amount: Float, bass: Float, middle: Float, treble: Float) =
    EffectDescriptor(model, name, "DIST", EffectCategory.DISTORTION, "", listOf(
        p("distortion", "DISTORTION", 0f, 100f, amount, ParameterValueType.PERCENT, "%"),
        p("bass", "BASS", 0f, 100f, bass, ParameterValueType.PERCENT, "%"),
        p("middle", "MIDDLE", 0f, 100f, middle, ParameterValueType.PERCENT, "%"),
        p("treble", "TREBLE", 0f, 100f, treble, ParameterValueType.PERCENT, "%"),
        p("level", "LEVEL", 0f, 100f, 50f, ParameterValueType.PERCENT, "%")))

private fun fuzz(model: EffectModelId, name: String, amount: Float, tone: Float, bias: Float) =
    EffectDescriptor(model, name, "FUZZ", EffectCategory.FUZZ, "", listOf(
        p("fuzz", "FUZZ", 0f, 100f, amount, ParameterValueType.PERCENT, "%"),
        p("tone", "TONE", 0f, 100f, tone, ParameterValueType.PERCENT, "%"),
        p("bias", "BIAS", 0f, 100f, bias, ParameterValueType.PERCENT, "%"),
        p("level", "LEVEL", 0f, 100f, 50f, ParameterValueType.PERCENT, "%")))

private fun eq(model: EffectModelId, name: String, low: Float, mid: Float, high: Float) =
    EffectDescriptor(model, name, "EQ", EffectCategory.EQUALIZER, "", listOf(
        p("low_db", "LOW", -12f, 12f, low, ParameterValueType.DECIBEL, "dB"),
        p("mid_db", "MID", -12f, 12f, mid, ParameterValueType.DECIBEL, "dB"),
        p("high_db", "HIGH", -12f, 12f, high, ParameterValueType.DECIBEL, "dB")))

private fun delay(model: EffectModelId, name: String, time: Float, feedback: Float, mix: Float) =
    EffectDescriptor(model, name, "DELAY", EffectCategory.DELAY, "", listOf(
        p("time_ms", "TIME", 20f, 1000f, time, ParameterValueType.MILLISECONDS, "ms"),
        p("feedback", "FEEDBACK", 0f, 90f, feedback, ParameterValueType.PERCENT, "%"),
        p("mix", "MIX", 0f, 100f, mix, ParameterValueType.PERCENT, "%")))

private fun preamp(model: EffectModelId, name: String, gain: Float, bass: Float, middle: Float, treble: Float, presence: Float) =
    EffectDescriptor(model, name, "PRE", EffectCategory.PREAMP, "", listOf(
        p("gain", "GAIN", 0f, 100f, gain, ParameterValueType.PERCENT, "%"),
        p("bass", "BASS", 0f, 100f, bass, ParameterValueType.PERCENT, "%"),
        p("middle", "MIDDLE", 0f, 100f, middle, ParameterValueType.PERCENT, "%"),
        p("treble", "TREBLE", 0f, 100f, treble, ParameterValueType.PERCENT, "%"),
        p("presence", "PRESENCE", 0f, 100f, presence, ParameterValueType.PERCENT, "%"),
        p("master", "MASTER", 0f, 100f, 50f, ParameterValueType.PERCENT, "%")))

private fun cabinet(model: EffectModelId, name: String, cabinetModel: Float, mic: Float) =
    EffectDescriptor(model, name, "CAB", EffectCategory.CABINET, "", listOf(
        ParameterDescriptor("cabinet_model", "CABINET", 0f, 2f, cabinetModel, ParameterValueType.ENUM, "", 1f, listOf("1x12", "2x12", "4x12")),
        p("mic_position", "MIC POSITION", 0f, 100f, mic, ParameterValueType.PERCENT, "%")))

private fun defaultDescriptors() = listOf(
    gate(EffectModelId.NOISE_GATE, "Studio Gate", -50f, 5f, 120f),
    gate(EffectModelId.TIGHT_NOISE_GATE, "Tight Gate", -42f, 2f, 65f),

    overdrive(EffectModelId.CLASSIC_OVERDRIVE, "Classic Overdrive", 35f, 50f, 50f),
    overdrive(EffectModelId.MID_BOOST_OVERDRIVE, "Mid Boost OD", 45f, 58f, 62f),
    overdrive(EffectModelId.TRANSPARENT_OVERDRIVE, "Transparent OD", 25f, 55f, 55f),

    distortion(EffectModelId.DISTORTION, "Classic Distortion", 55f, 50f, 50f, 50f),
    distortion(EffectModelId.MODERN_DISTORTION, "Modern Distortion", 72f, 58f, 38f, 62f),

    fuzz(EffectModelId.VINTAGE_FUZZ, "Vintage Fuzz", 65f, 50f, 50f),
    fuzz(EffectModelId.GATED_FUZZ, "Gated Fuzz", 78f, 55f, 72f),

    eq(EffectModelId.THREE_BAND_EQ, "3 Band EQ", 0f, 0f, 0f),
    eq(EffectModelId.SCOOP_EQ, "Scoop EQ", 3f, -6f, 4f),

    delay(EffectModelId.DIGITAL_DELAY, "Digital Delay", 350f, 35f, 25f),
    delay(EffectModelId.SLAPBACK_DELAY, "Slapback Delay", 95f, 12f, 22f),
    delay(EffectModelId.AMBIENT_DELAY, "Ambient Delay", 620f, 58f, 38f),

    preamp(EffectModelId.PREAMP, "Classic Preamp", 40f, 50f, 50f, 50f, 50f),
    preamp(EffectModelId.CLEAN_PREAMP, "Clean Preamp", 18f, 52f, 48f, 55f, 58f),
    preamp(EffectModelId.HIGH_GAIN_PREAMP, "High Gain Preamp", 72f, 58f, 42f, 60f, 62f),

    cabinet(EffectModelId.CABINET, "Cabinet Select", 0f, 50f),
    cabinet(EffectModelId.CABINET_1X12, "Open 1x12", 0f, 58f),
    cabinet(EffectModelId.CABINET_2X12, "Balanced 2x12", 1f, 50f),
    cabinet(EffectModelId.CABINET_4X12, "Closed 4x12", 2f, 42f)
)

fun createLegacyV02Pedalboard(registry: EffectRegistry = EffectRegistry.Default): PedalboardState {
    var state = PedalboardState()
    listOf(EffectModelId.NOISE_GATE, EffectModelId.CLASSIC_OVERDRIVE, EffectModelId.THREE_BAND_EQ, EffectModelId.DIGITAL_DELAY)
        .forEach { state = state.addEffect(it, registry) }
    return state.copy(selectedInstanceId = state.effects.getOrNull(1)?.instanceId)
}


fun createPedalboardFromV02Settings(
    settings: GuitarFxSettings,
    registry: EffectRegistry = EffectRegistry.Default
): PedalboardState {
    var state = createLegacyV02Pedalboard(registry)
    val valuesByModel = mapOf(
        EffectModelId.NOISE_GATE to Triple(
            settings.gateEnabled,
            listOf("threshold_db", "attack_ms", "release_ms"),
            listOf(settings.gateThresholdDb, settings.gateAttackMs, settings.gateReleaseMs)
        ),
        EffectModelId.CLASSIC_OVERDRIVE to Triple(
            settings.driveEnabled,
            listOf("drive", "tone", "level"),
            listOf(settings.driveAmount, settings.driveTone, settings.driveLevel)
        ),
        EffectModelId.THREE_BAND_EQ to Triple(
            settings.eqEnabled,
            listOf("low_db", "mid_db", "high_db"),
            listOf(settings.eqLowDb, settings.eqMidDb, settings.eqHighDb)
        ),
        EffectModelId.DIGITAL_DELAY to Triple(
            settings.delayEnabled,
            listOf("time_ms", "feedback", "mix"),
            listOf(settings.delayTimeMs, settings.delayFeedback, settings.delayMix)
        )
    )
    state.effects.forEach { effect ->
        val data = valuesByModel[effect.modelId] ?: return@forEach
        state = state.setEffectEnabled(effect.instanceId, data.first)
        data.second.zip(data.third).forEach { (parameterId, value) ->
            state = state.setParameter(effect.instanceId, parameterId, value, registry)
        }
    }
    return state
}
