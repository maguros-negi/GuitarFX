package jp.souta.guitarfx

import androidx.compose.foundation.BorderStroke
import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.horizontalScroll
import androidx.compose.foundation.gestures.detectDragGesturesAfterLongPress
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.AlertDialog
import androidx.compose.material3.ButtonDefaults
import androidx.compose.material3.Card
import androidx.compose.material3.CardDefaults
import androidx.compose.material3.OutlinedButton
import androidx.compose.material3.Slider
import androidx.compose.material3.Surface
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.runtime.mutableFloatStateOf
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.graphicsLayer
import androidx.compose.ui.input.pointer.pointerInput
import androidx.compose.ui.platform.LocalDensity
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontFamily
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import kotlin.math.roundToInt

private val BoardPanel = Color(0xFF14181D)
private val BoardPanelLight = Color(0xFF1B2027)
private val BoardBorder = Color(0xFF303740)
private val BoardGreen = Color(0xFF59E3B2)
private val BoardCyan = Color(0xFF55C7F3)
private val BoardOrange = Color(0xFFFFAD42)
private val BoardRed = Color(0xFFFF5D6C)
private val BoardMuted = Color(0xFF929AA5)

@Composable
fun DynamicPedalboardPrototype(
    state: PedalboardState,
    registry: EffectRegistry,
    masterBypassed: Boolean,
    onStructureChange: (PedalboardState) -> Unit,
    onRealtimeChange: (PedalboardState, EffectInstance) -> Unit
) {
    var addDialogVisible by remember { mutableStateOf(false) }
    Card(
        modifier = Modifier.fillMaxWidth(),
        colors = CardDefaults.cardColors(containerColor = BoardPanel),
        border = BorderStroke(1.dp, BoardBorder),
        shape = RoundedCornerShape(12.dp)
    ) {
        Column(modifier = Modifier.padding(12.dp), verticalArrangement = Arrangement.spacedBy(12.dp)) {
            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.SpaceBetween,
                verticalAlignment = Alignment.CenterVertically
            ) {
                Column {
                    Text("DYNAMIC PEDALBOARD", color = Color.White, fontSize = 11.sp, fontWeight = FontWeight.Black)
                    Text("NATIVE DYNAMIC CHAIN", color = BoardGreen, fontSize = 8.sp)
                }
                Text("${state.effects.size} PEDALS", color = BoardMuted, fontSize = 9.sp, fontFamily = FontFamily.Monospace)
            }
            Row(
                modifier = Modifier.fillMaxWidth().horizontalScroll(rememberScrollState()),
                verticalAlignment = Alignment.CenterVertically
            ) {
                Endpoint("IN", BoardGreen)
                Cable()
                state.effects.forEach { effect ->
                    val descriptor = registry.descriptor(effect.modelId) ?: return@forEach
                    PedalBlock(
                        descriptor = descriptor,
                        effect = effect,
                        selected = state.selectedInstanceId == effect.instanceId,
                        masterBypassed = masterBypassed,
                        onClick = {
                            val selectedState = state.selectEffect(effect.instanceId)
                            onRealtimeChange(selectedState, effect)
                        },
                        onMoveBy = { delta ->
                            val currentIndex = state.effects.indexOfFirst { it.instanceId == effect.instanceId }
                            if (currentIndex >= 0 && delta != 0) {
                                onStructureChange(state.moveEffect(effect.instanceId, currentIndex + delta))
                            }
                        }
                    )
                    Cable()
                }
                AddBlock { addDialogVisible = true }
                Cable()
                Endpoint("OUT", BoardCyan)
            }
            state.selectedEffect?.let { selected ->
                val descriptor = registry.descriptor(selected.modelId)
                if (descriptor != null) {
                    SelectedInstanceEditor(
                        state = state,
                        effect = selected,
                        descriptor = descriptor,
                        registry = registry,
                        masterBypassed = masterBypassed,
                        onStructureChange = onStructureChange,
                        onRealtimeChange = onRealtimeChange
                    )
                }
            } ?: Text(
                "＋を押してエフェクターを追加してください。",
                color = BoardMuted,
                modifier = Modifier.fillMaxWidth(),
                textAlign = TextAlign.Center,
                fontSize = 11.sp
            )
        }
    }
    if (addDialogVisible) {
        AlertDialog(
            onDismissRequest = { addDialogVisible = false },
            title = { Text("ADD EFFECT") },
            text = {
                Column(verticalArrangement = Arrangement.spacedBy(12.dp)) {
                    registry.availableEffects()
                        .groupBy { it.category }
                        .forEach { (category, effects) ->
                            Column(verticalArrangement = Arrangement.spacedBy(6.dp)) {
                                Text(
                                    category.displayName,
                                    color = BoardCyan,
                                    fontSize = 10.sp,
                                    fontWeight = FontWeight.Black
                                )
                                effects.forEach { descriptor ->
                                    OutlinedButton(
                                        onClick = {
                                            onStructureChange(state.addEffect(descriptor.modelId, registry))
                                            addDialogVisible = false
                                        },
                                        modifier = Modifier.fillMaxWidth()
                                    ) {
                                        Column(modifier = Modifier.fillMaxWidth()) {
                                            Text(descriptor.displayName, fontWeight = FontWeight.Bold)
                                            Text(descriptor.description, fontSize = 9.sp, color = BoardMuted)
                                        }
                                    }
                                }
                            }
                        }
                }
            },
            confirmButton = {},
            dismissButton = { TextButton(onClick = { addDialogVisible = false }) { Text("CLOSE") } }
        )
    }
}

@Composable
private fun PedalBlock(
    descriptor: EffectDescriptor,
    effect: EffectInstance,
    selected: Boolean,
    masterBypassed: Boolean,
    onClick: () -> Unit,
    onMoveBy: (Int) -> Unit
) {
    val accent = modelColor(effect.modelId)
    var dragOffsetX by remember(effect.instanceId) { mutableFloatStateOf(0f) }
    var dragging by remember(effect.instanceId) { mutableStateOf(false) }
    val pedalStepPx = with(LocalDensity.current) { 96.dp.toPx() }
    Card(
        modifier = Modifier
            .width(82.dp)
            .height(98.dp)
            .graphicsLayer {
                translationX = dragOffsetX
                scaleX = if (dragging) 1.06f else 1f
                scaleY = if (dragging) 1.06f else 1f
                shadowElevation = if (dragging) 14f else 0f
            }
            .pointerInput(effect.instanceId) {
                detectDragGesturesAfterLongPress(
                    onDragStart = { dragging = true; dragOffsetX = 0f },
                    onDragCancel = { dragging = false; dragOffsetX = 0f },
                    onDragEnd = {
                        val delta = (dragOffsetX / pedalStepPx).roundToInt()
                        dragging = false
                        dragOffsetX = 0f
                        if (delta != 0) onMoveBy(delta)
                    },
                    onDrag = { change, dragAmount ->
                        change.consume()
                        dragOffsetX += dragAmount.x
                    }
                )
            }
            .clickable(onClick = onClick),
        colors = CardDefaults.cardColors(containerColor = if (selected) accent.copy(alpha = 0.13f) else BoardPanelLight),
        border = BorderStroke(if (selected) 2.dp else 1.dp, if (selected) accent else BoardBorder),
        shape = RoundedCornerShape(9.dp)
    ) {
        Column(
            modifier = Modifier.padding(8.dp),
            horizontalAlignment = Alignment.CenterHorizontally,
            verticalArrangement = Arrangement.SpaceBetween
        ) {
            Box(Modifier.size(9.dp).background(if (effect.enabled) accent else BoardMuted, CircleShape))
            Text(descriptor.shortName, color = if (selected) accent else Color.White, fontSize = 11.sp, fontWeight = FontWeight.Black)
            Text(
                if (masterBypassed && effect.enabled) "BYPASS" else if (effect.enabled) "ON" else "OFF",
                color = if (masterBypassed && effect.enabled) BoardOrange else if (effect.enabled) accent else BoardMuted,
                fontSize = 8.sp,
                fontWeight = FontWeight.Bold
            )
            Text(effect.instanceId.value.take(4).uppercase(), color = BoardMuted, fontSize = 7.sp, fontFamily = FontFamily.Monospace)
        }
    }
}

@Composable
private fun SelectedInstanceEditor(
    state: PedalboardState,
    effect: EffectInstance,
    descriptor: EffectDescriptor,
    registry: EffectRegistry,
    masterBypassed: Boolean,
    onStructureChange: (PedalboardState) -> Unit,
    onRealtimeChange: (PedalboardState, EffectInstance) -> Unit
) {
    val index = state.effects.indexOfFirst { it.instanceId == effect.instanceId }
    val accent = modelColor(effect.modelId)
    Card(
        colors = CardDefaults.cardColors(containerColor = BoardPanelLight),
        border = BorderStroke(1.dp, accent.copy(alpha = 0.6f)),
        shape = RoundedCornerShape(10.dp)
    ) {
        Column(modifier = Modifier.padding(12.dp), verticalArrangement = Arrangement.spacedBy(8.dp)) {
            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.SpaceBetween,
                verticalAlignment = Alignment.CenterVertically
            ) {
                Column(modifier = Modifier.weight(1f)) {
                    Text(descriptor.displayName, color = accent, fontSize = 16.sp, fontWeight = FontWeight.Black)
                    Text("INSTANCE ${effect.instanceId.value.take(8).uppercase()} • NATIVE", color = BoardGreen, fontSize = 8.sp)
                }
                OutlinedButton(
                    onClick = {
                        val updated = state.setEffectEnabled(effect.instanceId, !effect.enabled)
                        updated.effects.firstOrNull { it.instanceId == effect.instanceId }?.let {
                            onRealtimeChange(updated, it)
                        }
                    },
                    enabled = !masterBypassed,
                    colors = ButtonDefaults.outlinedButtonColors(contentColor = accent)
                ) { Text(if (effect.enabled) "ON" else "OFF", fontSize = 9.sp, fontWeight = FontWeight.Black) }
            }
            Text(descriptor.description, color = Color(0xFFD2D6DB), fontSize = 11.sp)
            descriptor.parameters.forEach { parameter ->
                val value = effect.parameterValues[parameter.parameterId] ?: parameter.defaultValue
                DynamicParameterSlider(parameter, value, accent, !masterBypassed) { changed ->
                    val updated = state.setParameter(effect.instanceId, parameter.parameterId, changed, registry)
                    updated.effects.firstOrNull { it.instanceId == effect.instanceId }?.let {
                        onRealtimeChange(updated, it)
                    }
                }
            }
            Row(modifier = Modifier.fillMaxWidth(), horizontalArrangement = Arrangement.spacedBy(6.dp)) {
                TextButton(
                    onClick = { onStructureChange(state.moveEffectLeft(effect.instanceId)) },
                    enabled = index > 0,
                    modifier = Modifier.weight(1f)
                ) { Text("← LEFT", fontSize = 9.sp) }
                TextButton(
                    onClick = { onStructureChange(state.moveEffectRight(effect.instanceId)) },
                    enabled = index in 0 until state.effects.lastIndex,
                    modifier = Modifier.weight(1f)
                ) { Text("RIGHT →", fontSize = 9.sp) }
                TextButton(
                    onClick = { onStructureChange(state.removeEffect(effect.instanceId)) },
                    modifier = Modifier.weight(1f)
                ) { Text("DELETE", color = BoardRed, fontSize = 9.sp) }
            }
        }
    }
}

@Composable
private fun DynamicParameterSlider(
    parameter: ParameterDescriptor,
    value: Float,
    accent: Color,
    enabled: Boolean,
    onValueChange: (Float) -> Unit
) {
    Column {
        Row(modifier = Modifier.fillMaxWidth(), horizontalArrangement = Arrangement.SpaceBetween) {
            Text(parameter.displayName, color = BoardMuted, fontSize = 9.sp, fontWeight = FontWeight.Bold)
            Text(formatValue(value, parameter), color = accent, fontSize = 10.sp, fontFamily = FontFamily.Monospace)
        }
        Slider(value = value, onValueChange = onValueChange, valueRange = parameter.minimumValue..parameter.maximumValue, enabled = enabled)
    }
}

private fun formatValue(value: Float, parameter: ParameterDescriptor): String = when (parameter.valueType) {
    ParameterValueType.INTEGER, ParameterValueType.PERCENT, ParameterValueType.MILLISECONDS -> "${value.roundToInt()} ${parameter.unit}".trim()
    else -> "${((value * 10f).roundToInt() / 10f)} ${parameter.unit}".trim()
}

@Composable private fun Endpoint(text: String, color: Color) {
    Column(modifier = Modifier.width(42.dp), horizontalAlignment = Alignment.CenterHorizontally) {
        Box(Modifier.size(30.dp).background(color.copy(alpha = 0.14f), CircleShape), contentAlignment = Alignment.Center) {
            Box(Modifier.size(10.dp).background(color, CircleShape))
        }
        Spacer(Modifier.height(5.dp)); Text(text, color = color, fontSize = 9.sp, fontWeight = FontWeight.Bold)
    }
}

@Composable private fun Cable() { Box(Modifier.width(14.dp).height(2.dp).background(BoardBorder)) }

@Composable private fun AddBlock(onClick: () -> Unit) {
    Surface(
        modifier = Modifier.size(58.dp).clickable(onClick = onClick),
        color = BoardGreen.copy(alpha = 0.10f),
        shape = RoundedCornerShape(10.dp),
        border = BorderStroke(1.dp, BoardGreen.copy(alpha = 0.7f))
    ) { Box(contentAlignment = Alignment.Center) { Text("＋", color = BoardGreen, fontSize = 24.sp, fontWeight = FontWeight.Bold) } }
}

private fun modelColor(id: EffectModelId): Color = when (id) {
    EffectModelId.NOISE_GATE -> Color(0xFF66D17A)
    EffectModelId.CLASSIC_OVERDRIVE, EffectModelId.DISTORTION, EffectModelId.VINTAGE_FUZZ -> Color(0xFFFF984F)
    EffectModelId.THREE_BAND_EQ -> Color(0xFF55B8FF)
    EffectModelId.DIGITAL_DELAY -> Color(0xFFB68CFF)
    EffectModelId.PREAMP -> Color(0xFFFFD166)
    EffectModelId.CABINET -> Color(0xFF7BDFF2)
}
