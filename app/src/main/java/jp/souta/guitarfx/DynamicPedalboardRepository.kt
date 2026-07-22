package jp.souta.guitarfx

import android.content.Context
import org.json.JSONArray
import org.json.JSONObject

data class DynamicPresetData(
    val name: String,
    val pedalboard: PedalboardState,
    val inputGainDb: Float,
    val outputGainDb: Float,
    val limiterEnabled: Boolean
)

class DynamicPedalboardRepository(context: Context) {
    private val prefs = context.getSharedPreferences("guitar_fx_dynamic_v2", Context.MODE_PRIVATE)

    fun loadCurrent(registry: EffectRegistry): PedalboardState? =
        decodePedalboard(prefs.getString(KEY_CURRENT_BOARD, null), registry)

    fun saveCurrent(state: PedalboardState) {
        prefs.edit().putString(KEY_CURRENT_BOARD, encodePedalboard(state).toString()).apply()
    }

    fun savePreset(
        slot: Int,
        name: String,
        state: PedalboardState,
        inputGainDb: Float,
        outputGainDb: Float,
        limiterEnabled: Boolean
    ) {
        if (slot !in 1..5) return
        val root = JSONObject()
            .put("formatVersion", PRESET_FORMAT_VERSION)
            .put("name", name.trim().ifBlank { "PRESET $slot" })
            .put("inputGainDb", inputGainDb)
            .put("outputGainDb", outputGainDb)
            .put("limiterEnabled", limiterEnabled)
            .put("pedalboard", encodePedalboard(state))
        prefs.edit().putString("preset_$slot", root.toString()).apply()
    }

    fun loadPreset(slot: Int, registry: EffectRegistry): DynamicPresetData? {
        if (slot !in 1..5) return null
        return try {
            val root = JSONObject(prefs.getString("preset_$slot", null) ?: return null)
            val board = decodePedalboard(root.optJSONObject("pedalboard"), registry) ?: return null
            DynamicPresetData(
                name = root.optString("name", "PRESET $slot"),
                pedalboard = board,
                inputGainDb = root.safeFloat("inputGainDb", 0f, -60f, 12f),
                outputGainDb = root.safeFloat("outputGainDb", -6f, -60f, 12f),
                limiterEnabled = root.optBoolean("limiterEnabled", true)
            )
        } catch (_: Exception) { null }
    }

    fun deletePreset(slot: Int) {
        if (slot in 1..5) prefs.edit().remove("preset_$slot").apply()
    }

    fun presetNames(registry: EffectRegistry): List<String?> =
        (1..5).map { loadPreset(it, registry)?.name }

    private fun encodePedalboard(state: PedalboardState): JSONObject {
        val effects = JSONArray()
        state.effects.forEach { effect ->
            val parameters = JSONObject()
            effect.parameterValues.forEach { (key, value) -> parameters.put(key, value) }
            effects.put(
                JSONObject()
                    .put("instanceId", effect.instanceId.value)
                    .put("modelId", effect.modelId.persistentId)
                    .put("enabled", effect.enabled)
                    .put("customName", effect.customName ?: JSONObject.NULL)
                    .put("parameters", parameters)
            )
        }
        return JSONObject()
            .put("formatVersion", PedalboardState.CURRENT_FORMAT_VERSION)
            .put("selectedInstanceId", state.selectedInstanceId?.value ?: JSONObject.NULL)
            .put("effects", effects)
    }

    private fun decodePedalboard(text: String?, registry: EffectRegistry): PedalboardState? =
        try { text?.let { decodePedalboard(JSONObject(it), registry) } } catch (_: Exception) { null }

    private fun decodePedalboard(root: JSONObject?, registry: EffectRegistry): PedalboardState? {
        if (root == null) return null
        return try {
            val effectsJson = root.optJSONArray("effects") ?: JSONArray()
            val effects = buildList {
                val seen = mutableSetOf<String>()
                for (index in 0 until effectsJson.length()) {
                    val item = effectsJson.optJSONObject(index) ?: continue
                    val instanceText = item.optString("instanceId").trim()
                    if (instanceText.isBlank() || !seen.add(instanceText)) continue
                    val model = EffectModelId.fromPersistentId(item.optString("modelId")) ?: continue
                    val descriptor = registry.descriptor(model) ?: continue
                    val parameterJson = item.optJSONObject("parameters") ?: JSONObject()
                    val values = descriptor.parameters.associate { parameter ->
                        parameter.parameterId to parameter.normalize(
                            parameterJson.optDouble(parameter.parameterId, parameter.defaultValue.toDouble()).toFloat()
                        )
                    }
                    add(
                        EffectInstance(
                            instanceId = EffectInstanceId(instanceText),
                            modelId = model,
                            enabled = item.optBoolean("enabled", false),
                            parameterValues = values,
                            customName = item.optString("customName").trim().takeIf { it.isNotBlank() && it != "null" }
                        )
                    )
                }
            }
            val selectedText = root.optString("selectedInstanceId").takeIf { it.isNotBlank() && it != "null" }
            val selected = selectedText?.let(::EffectInstanceId)?.takeIf { id -> effects.any { it.instanceId == id } }
            PedalboardState(
                formatVersion = PedalboardState.CURRENT_FORMAT_VERSION,
                effects = effects,
                selectedInstanceId = selected ?: effects.firstOrNull()?.instanceId
            )
        } catch (_: Exception) { null }
    }

    private fun JSONObject.safeFloat(key: String, default: Float, min: Float, max: Float): Float {
        val value = optDouble(key, default.toDouble()).toFloat()
        return (if (value.isFinite()) value else default).coerceIn(min, max)
    }

    companion object {
        private const val KEY_CURRENT_BOARD = "current_pedalboard"
        private const val PRESET_FORMAT_VERSION = 2
    }
}
