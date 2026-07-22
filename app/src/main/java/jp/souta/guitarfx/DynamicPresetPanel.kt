package jp.souta.guitarfx

import androidx.compose.animation.animateContentSize
import androidx.compose.foundation.BorderStroke
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.AlertDialog
import androidx.compose.material3.Card
import androidx.compose.material3.CardDefaults
import androidx.compose.material3.HorizontalDivider
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontFamily
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp

@Composable
fun DynamicPresetPanel(
    repository: DynamicPedalboardRepository,
    registry: EffectRegistry,
    currentBoard: PedalboardState,
    inputGainDb: Float,
    outputGainDb: Float,
    limiterEnabled: Boolean,
    onLoad: (DynamicPresetData) -> Unit
) {
    var names by remember { mutableStateOf(repository.presetNames(registry)) }
    var expanded by remember { mutableStateOf(false) }
    var saveSlot by remember { mutableStateOf<Int?>(null) }
    var presetName by remember { mutableStateOf("") }

    Card(
        modifier = Modifier.fillMaxWidth().animateContentSize(),
        colors = CardDefaults.cardColors(containerColor = Color(0xFF14181D)),
        border = BorderStroke(1.dp, Color(0xFF303740)),
        shape = RoundedCornerShape(12.dp)
    ) {
        Column(modifier = Modifier.padding(12.dp)) {
            Row(
                modifier = Modifier.fillMaxWidth().clickable { expanded = !expanded },
                horizontalArrangement = Arrangement.SpaceBetween,
                verticalAlignment = Alignment.CenterVertically
            ) {
                Column {
                    Text("DYNAMIC PRESETS", color = Color.White, fontSize = 11.sp, fontWeight = FontWeight.Black)
                    Text("${names.count { it != null }} / 5 SAVED", color = Color(0xFF929AA5), fontSize = 9.sp, fontFamily = FontFamily.Monospace)
                }
                Text(if (expanded) "▲ CLOSE" else "▼ OPEN", color = Color(0xFF55C7F3), fontSize = 9.sp, fontWeight = FontWeight.Bold)
            }
            if (expanded) {
                Spacer(Modifier.height(10.dp))
                HorizontalDivider(color = Color(0xFF303740))
                Spacer(Modifier.height(6.dp))
                Column(verticalArrangement = Arrangement.spacedBy(6.dp)) {
                    (1..5).forEach { slot ->
                        val name = names[slot - 1]
                        Row(modifier = Modifier.fillMaxWidth(), verticalAlignment = Alignment.CenterVertically) {
                            Text(
                                name ?: "PRESET $slot • EMPTY",
                                modifier = Modifier.weight(1f),
                                color = if (name == null) Color(0xFF929AA5) else Color.White,
                                fontSize = 10.sp,
                                fontWeight = FontWeight.Bold,
                                maxLines = 1
                            )
                            TextButton(onClick = { saveSlot = slot; presetName = name ?: "PRESET $slot" }) { Text("SAVE", fontSize = 9.sp) }
                            TextButton(onClick = { repository.loadPreset(slot, registry)?.let(onLoad) }, enabled = name != null) { Text("LOAD", fontSize = 9.sp) }
                            TextButton(onClick = { repository.deletePreset(slot); names = repository.presetNames(registry) }, enabled = name != null) { Text("DELETE", fontSize = 9.sp) }
                        }
                    }
                }
            }
        }
    }

    saveSlot?.let { slot ->
        AlertDialog(
            onDismissRequest = { saveSlot = null },
            title = { Text("SAVE PRESET $slot") },
            text = { OutlinedTextField(value = presetName, onValueChange = { presetName = it }, singleLine = true, label = { Text("Preset name") }) },
            confirmButton = {
                TextButton(onClick = {
                    repository.savePreset(slot, presetName, currentBoard, inputGainDb, outputGainDb, limiterEnabled)
                    names = repository.presetNames(registry)
                    saveSlot = null
                }) { Text("SAVE") }
            },
            dismissButton = { TextButton(onClick = { saveSlot = null }) { Text("CANCEL") } }
        )
    }
}
