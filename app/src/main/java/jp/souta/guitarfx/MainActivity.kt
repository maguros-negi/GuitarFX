package jp.souta.guitarfx

import android.Manifest
import android.content.pm.PackageManager
import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.rememberLauncherForActivityResult
import androidx.activity.result.contract.ActivityResultContracts
import androidx.activity.compose.setContent
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.core.content.ContextCompat
import kotlinx.coroutines.delay
import kotlin.math.roundToInt

class MainActivity : ComponentActivity() {
    private val engine = AudioEngine()

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        engine.create()
        setContent { GuitarFxApp(engine) }
    }

    override fun onDestroy() {
        engine.stop()
        engine.destroy()
        super.onDestroy()
    }
}

@Composable
private fun GuitarFxApp(engine: AudioEngine) {
    val context = androidx.compose.ui.platform.LocalContext.current
    var hasPermission by remember {
        mutableStateOf(ContextCompat.checkSelfPermission(context, Manifest.permission.RECORD_AUDIO) == PackageManager.PERMISSION_GRANTED)
    }
    val permissionLauncher = rememberLauncherForActivityResult(ActivityResultContracts.RequestPermission()) {
        hasPermission = it
    }
    var stats by remember { mutableStateOf(AudioStats()) }
    var inputGain by remember { mutableFloatStateOf(0f) }
    var outputGain by remember { mutableFloatStateOf(-6f) }
    var muted by remember { mutableStateOf(false) }

    LaunchedEffect(Unit) {
        while (true) {
            stats = engine.readStats()
            delay(50)
        }
    }

    MaterialTheme(
        colorScheme = darkColorScheme(
            primary = Color(0xFF63E6BE),
            background = Color(0xFF101318),
            surface = Color(0xFF1A1F26)
        )
    ) {
        Surface(Modifier.fillMaxSize()) {
            Column(
                modifier = Modifier.fillMaxSize().padding(16.dp),
                verticalArrangement = Arrangement.spacedBy(12.dp)
            ) {
                Row(Modifier.fillMaxWidth(), horizontalArrangement = Arrangement.SpaceBetween) {
                    Text("GuitarFX", style = MaterialTheme.typography.headlineMedium, fontWeight = FontWeight.Bold)
                    Text("v0.1", color = MaterialTheme.colorScheme.primary)
                }

                StatusCard(stats)
                MeterCard("INPUT", stats.inputPeakDb, inputGain) {
                    inputGain = it
                    engine.setInputGainDb(it)
                }
                MeterCard("OUTPUT", stats.outputPeakDb, outputGain) {
                    outputGain = it
                    engine.setOutputGainDb(it)
                }

                Button(
                    onClick = {
                        muted = !muted
                        engine.setMuted(muted)
                    },
                    modifier = Modifier.fillMaxWidth(),
                    colors = ButtonDefaults.buttonColors(
                        containerColor = if (muted) Color(0xFFE85D75) else MaterialTheme.colorScheme.surface
                    )
                ) { Text(if (muted) "MUTED" else "MUTE") }

                Button(
                    onClick = {
                        if (!hasPermission) {
                            permissionLauncher.launch(Manifest.permission.RECORD_AUDIO)
                        } else if (stats.running) {
                            engine.stop()
                        } else {
                            engine.setInputGainDb(inputGain)
                            engine.setOutputGainDb(outputGain)
                            engine.setMuted(muted)
                            engine.start()
                        }
                    },
                    modifier = Modifier.fillMaxWidth().height(56.dp)
                ) {
                    Text(if (!hasPermission) "マイク権限を許可" else if (stats.running) "STOP" else "START")
                }

                if (stats.error.isNotBlank()) {
                    Text(stats.error, color = Color(0xFFFF8A80), style = MaterialTheme.typography.bodySmall)
                }
            }
        }
    }
}

@Composable
private fun StatusCard(stats: AudioStats) {
    Card(colors = CardDefaults.cardColors(containerColor = MaterialTheme.colorScheme.surface)) {
        Column(Modifier.fillMaxWidth().padding(14.dp), verticalArrangement = Arrangement.spacedBy(5.dp)) {
            Text(if (stats.running) "● RUNNING" else "○ STOPPED", color = if (stats.running) Color(0xFF63E6BE) else Color.LightGray)
            Text("${stats.sampleRate} Hz / ${stats.framesPerBurst} frames per burst")
            Text("Buffer ${stats.bufferSize} frames")
            Text("Input ${stats.inputChannels} ch  →  Output ${stats.outputChannels} ch")
            Text("XRuns  IN ${stats.inputXruns} / OUT ${stats.outputXruns}")
        }
    }
}

@Composable
private fun MeterCard(title: String, peakDb: Float, gainDb: Float, onGainChanged: (Float) -> Unit) {
    Card(colors = CardDefaults.cardColors(containerColor = MaterialTheme.colorScheme.surface)) {
        Column(Modifier.fillMaxWidth().padding(14.dp)) {
            Row(Modifier.fillMaxWidth(), horizontalArrangement = Arrangement.SpaceBetween) {
                Text(title, fontWeight = FontWeight.Bold)
                Text("${((peakDb * 10).roundToInt() / 10f)} dBFS")
            }
            Spacer(Modifier.height(8.dp))
            val fraction = ((peakDb + 60f) / 60f).coerceIn(0f, 1f)
            Box(Modifier.fillMaxWidth().height(12.dp).background(Color(0xFF2A3038), RoundedCornerShape(6.dp))) {
                Box(Modifier.fillMaxWidth(fraction).fillMaxHeight().background(if (peakDb > -1f) Color.Red else Color(0xFF63E6BE), RoundedCornerShape(6.dp)))
            }
            Spacer(Modifier.height(8.dp))
            Slider(value = gainDb, onValueChange = onGainChanged, valueRange = -60f..12f)
            Text("Gain ${((gainDb * 10).roundToInt() / 10f)} dB")
        }
    }
}
