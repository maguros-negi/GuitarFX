package jp.souta.guitarfx

import java.util.UUID

enum class EffectModelId(val persistentId: String) {
    NOISE_GATE("noise_gate"),
    CLASSIC_OVERDRIVE("classic_overdrive"),
    THREE_BAND_EQ("three_band_eq"),
    DIGITAL_DELAY("digital_delay"),
    DISTORTION("distortion"),
    VINTAGE_FUZZ("vintage_fuzz"),
    PREAMP("preamp"),
    CABINET("cabinet");

    companion object {
        fun fromPersistentId(value: String): EffectModelId? =
            entries.firstOrNull { it.persistentId == value }
    }
}

enum class EffectCategory(val displayName: String) {
    DYNAMICS("DYNAMICS"), DRIVE("DRIVE"), EQUALIZER("EQUALIZER"),
    DELAY("DELAY"), PREAMP("PREAMP"), CABINET("CABINET")
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

private fun defaultDescriptors() = listOf(
    EffectDescriptor(EffectModelId.NOISE_GATE, "Noise Gate", "GATE", EffectCategory.DYNAMICS,
        "小さい入力音やノイズを抑えます。", listOf(
            p("threshold_db", "THRESHOLD", -80f, -10f, -50f, ParameterValueType.DECIBEL_FS, "dBFS"),
            p("attack_ms", "ATTACK", 1f, 100f, 5f, ParameterValueType.MILLISECONDS, "ms"),
            p("release_ms", "RELEASE", 10f, 1000f, 120f, ParameterValueType.MILLISECONDS, "ms"))),
    EffectDescriptor(EffectModelId.CLASSIC_OVERDRIVE, "Classic Overdrive", "OD", EffectCategory.DRIVE,
        "滑らかなソフトクリッピングのオーバードライブです。", listOf(
            p("drive", "DRIVE", 0f, 100f, 35f, ParameterValueType.PERCENT, "%"),
            p("tone", "TONE", 0f, 100f, 50f, ParameterValueType.PERCENT, "%"),
            p("level", "LEVEL", 0f, 100f, 50f, ParameterValueType.PERCENT, "%"))),
    EffectDescriptor(EffectModelId.THREE_BAND_EQ, "3 Band EQ", "EQ", EffectCategory.EQUALIZER,
        "低域、中域、高域のバランスを調整します。", listOf(
            p("low_db", "LOW", -12f, 12f, 0f, ParameterValueType.DECIBEL, "dB"),
            p("mid_db", "MID", -12f, 12f, 0f, ParameterValueType.DECIBEL, "dB"),
            p("high_db", "HIGH", -12f, 12f, 0f, ParameterValueType.DECIBEL, "dB"))),
    EffectDescriptor(EffectModelId.DIGITAL_DELAY, "Digital Delay", "DELAY", EffectCategory.DELAY,
        "入力音を遅らせて繰り返し再生します。", listOf(
            p("time_ms", "TIME", 20f, 1000f, 350f, ParameterValueType.MILLISECONDS, "ms"),
            p("feedback", "FEEDBACK", 0f, 90f, 35f, ParameterValueType.PERCENT, "%"),
            p("mix", "MIX", 0f, 100f, 25f, ParameterValueType.PERCENT, "%"))),
    EffectDescriptor(EffectModelId.DISTORTION, "Distortion", "DIST", EffectCategory.DRIVE, "将来追加予定です。", emptyList(), false),
    EffectDescriptor(EffectModelId.VINTAGE_FUZZ, "Vintage Fuzz", "FUZZ", EffectCategory.DRIVE, "将来追加予定です。", emptyList(), false),
    EffectDescriptor(EffectModelId.PREAMP, "Preamp", "PRE", EffectCategory.PREAMP, "将来追加予定です。", emptyList(), false),
    EffectDescriptor(EffectModelId.CABINET, "Cabinet", "CAB", EffectCategory.CABINET, "将来追加予定です。", emptyList(), false)
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
