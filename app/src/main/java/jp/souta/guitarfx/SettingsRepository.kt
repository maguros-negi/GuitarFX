package jp.souta.guitarfx

import android.content.Context
import org.json.JSONObject

class SettingsRepository(context: Context) {
    private val prefs = context.getSharedPreferences("guitar_fx_settings", Context.MODE_PRIVATE)
    fun loadCurrent() = decode(prefs.getString("current", null)) ?: GuitarFxSettings()
    fun saveCurrent(s: GuitarFxSettings) { prefs.edit().putString("current", encode(s).toString()).apply() }
    fun loadPreset(slot: Int): GuitarFxPreset? {
        return try {
            val json = prefs.getString("preset_$slot", null) ?: return null
            val root = JSONObject(json)
            val settings = decode(root.optJSONObject("settings")) ?: return null
            GuitarFxPreset(
                name = root.optString("name", "PRESET $slot"),
                settings = settings
            )
        } catch (_: Exception) {
            null
        }
    }
    fun savePreset(slot: Int, name: String, s: GuitarFxSettings) {
        val o = JSONObject().put("name", name.trim().ifBlank { "PRESET $slot" })
            .put("settings", encode(s.copy(muted = false, bypassed = false)))
        prefs.edit().putString("preset_$slot", o.toString()).apply()
    }
    fun deletePreset(slot: Int) { prefs.edit().remove("preset_$slot").apply() }
    fun presetNames() = (1..5).map { loadPreset(it)?.name }
    private fun encode(s: GuitarFxSettings) = JSONObject()
        .put("version",s.version).put("inputGainDb",s.inputGainDb).put("outputGainDb",s.outputGainDb)
        .put("muted",s.muted).put("bypassed",s.bypassed).put("limiterEnabled",s.limiterEnabled)
        .put("gateEnabled",s.gateEnabled).put("gateThresholdDb",s.gateThresholdDb).put("gateAttackMs",s.gateAttackMs).put("gateReleaseMs",s.gateReleaseMs)
        .put("driveEnabled",s.driveEnabled).put("driveAmount",s.driveAmount).put("driveTone",s.driveTone).put("driveLevel",s.driveLevel)
        .put("eqEnabled",s.eqEnabled).put("eqLowDb",s.eqLowDb).put("eqMidDb",s.eqMidDb).put("eqHighDb",s.eqHighDb)
        .put("delayEnabled",s.delayEnabled).put("delayTimeMs",s.delayTimeMs).put("delayFeedback",s.delayFeedback).put("delayMix",s.delayMix)
        .put("selectedEffectId",s.selectedEffectId)
    private fun decode(text: String?) = try { text?.let { decode(JSONObject(it)) } } catch (_: Exception) { null }
    private fun decode(o: JSONObject?): GuitarFxSettings? { if (o == null) return null; val d=GuitarFxSettings(); return GuitarFxSettings(
        version=o.optInt("version",1), inputGainDb=o.f("inputGainDb",d.inputGainDb,-60f,12f), outputGainDb=o.f("outputGainDb",d.outputGainDb,-60f,12f),
        muted=o.optBoolean("muted",false), bypassed=o.optBoolean("bypassed",false), limiterEnabled=o.optBoolean("limiterEnabled",true),
        gateEnabled=o.optBoolean("gateEnabled",false), gateThresholdDb=o.f("gateThresholdDb",d.gateThresholdDb,-80f,-10f), gateAttackMs=o.f("gateAttackMs",d.gateAttackMs,1f,100f), gateReleaseMs=o.f("gateReleaseMs",d.gateReleaseMs,10f,1000f),
        driveEnabled=o.optBoolean("driveEnabled",false), driveAmount=o.f("driveAmount",d.driveAmount,0f,100f), driveTone=o.f("driveTone",d.driveTone,0f,100f), driveLevel=o.f("driveLevel",d.driveLevel,0f,100f),
        eqEnabled=o.optBoolean("eqEnabled",false), eqLowDb=o.f("eqLowDb",d.eqLowDb,-12f,12f), eqMidDb=o.f("eqMidDb",d.eqMidDb,-12f,12f), eqHighDb=o.f("eqHighDb",d.eqHighDb,-12f,12f),
        delayEnabled=o.optBoolean("delayEnabled",false), delayTimeMs=o.f("delayTimeMs",d.delayTimeMs,20f,1000f), delayFeedback=o.f("delayFeedback",d.delayFeedback,0f,90f), delayMix=o.f("delayMix",d.delayMix,0f,100f), selectedEffectId=o.optInt("selectedEffectId",1).coerceIn(0,3)) }
    private fun JSONObject.f(k:String,d:Float,min:Float,max:Float)=optDouble(k,d.toDouble()).toFloat().coerceIn(min,max)
}
