package jp.souta.guitarfx

import kotlin.time.Duration.Companion.milliseconds
import android.Manifest
import android.content.pm.PackageManager
import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.rememberLauncherForActivityResult
import androidx.activity.compose.setContent
import androidx.activity.result.contract.ActivityResultContracts
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.*
import androidx.compose.runtime.*
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

        setContent {
            GuitarFxApp(engine)
        }
    }

    override fun onDestroy() {
        engine.stop()
        engine.destroy()

        super.onDestroy()
    }
}

@Composable
private fun GuitarFxApp(engine: AudioEngine) {
    val context =
        androidx.compose.ui.platform.LocalContext.current

    var hasPermission by remember {
        mutableStateOf(
            ContextCompat.checkSelfPermission(
                context,
                Manifest.permission.RECORD_AUDIO
            ) == PackageManager.PERMISSION_GRANTED
        )
    }

    val permissionLauncher =
        rememberLauncherForActivityResult(
            ActivityResultContracts.RequestPermission()
        ) { granted ->
            hasPermission = granted
        }

    var stats by remember {
        mutableStateOf(AudioStats())
    }

    var inputGain by remember {
        mutableFloatStateOf(0f)
    }

    var outputGain by remember {
        mutableFloatStateOf(-6f)
    }

    var muted by remember {
        mutableStateOf(false)
    }

    var bypassed by remember {
        mutableStateOf(false)
    }

    LaunchedEffect(Unit) {
        while (true) {
            stats = engine.readStats()
            delay(50.milliseconds)
        }
    }

    MaterialTheme(
        colorScheme = darkColorScheme(
            primary = Color(0xFF63E6BE),
            background = Color(0xFF101318),
            surface = Color(0xFF1A1F26),
            error = Color(0xFFFF8A80)
        )
    ) {
        Surface(
            modifier = Modifier.fillMaxSize()
        ) {
            Column(
                modifier = Modifier
                    .fillMaxSize()
                    .padding(16.dp),
                verticalArrangement =
                    Arrangement.spacedBy(12.dp)
            ) {
                Row(
                    modifier = Modifier.fillMaxWidth(),
                    horizontalArrangement =
                        Arrangement.SpaceBetween
                ) {
                    Text(
                        text = "GuitarFX",
                        style =
                            MaterialTheme.typography.headlineMedium,
                        fontWeight = FontWeight.Bold
                    )

                    Text(
                        text = "v0.1",
                        color = MaterialTheme.colorScheme.primary
                    )
                }

                StatusCard(stats)

                MeterCard(
                    title = "INPUT",
                    peakDb = stats.inputPeakDb,
                    gainDb = inputGain,
                    enabled = !bypassed,
                    onGainChanged = { value ->
                        inputGain = value
                        engine.setInputGainDb(value)
                    }
                )

                MeterCard(
                    title = "OUTPUT",
                    peakDb = stats.outputPeakDb,
                    gainDb = outputGain,
                    enabled = !bypassed,
                    onGainChanged = { value ->
                        outputGain = value
                        engine.setOutputGainDb(value)
                    }
                )

                Button(
                    onClick = {
                        bypassed = !bypassed
                        engine.setBypassed(bypassed)
                    },
                    modifier = Modifier.fillMaxWidth(),
                    colors = ButtonDefaults.buttonColors(
                        containerColor =
                            if (bypassed) {
                                Color(0xFFFFB74D)
                            } else {
                                MaterialTheme.colorScheme.surface
                            },
                        contentColor =
                            if (bypassed) {
                                Color(0xFF101318)
                            } else {
                                Color.White
                            }
                    )
                ) {
                    Text(
                        text =
                            if (bypassed) {
                                "BYPASS ON"
                            } else {
                                "BYPASS OFF"
                            }
                    )
                }

                Button(
                    onClick = {
                        muted = !muted
                        engine.setMuted(muted)
                    },
                    modifier = Modifier.fillMaxWidth(),
                    colors = ButtonDefaults.buttonColors(
                        containerColor =
                            if (muted) {
                                Color(0xFFE85D75)
                            } else {
                                MaterialTheme.colorScheme.surface
                            },
                        contentColor = Color.White
                    )
                ) {
                    Text(
                        text =
                            if (muted) {
                                "MUTED"
                            } else {
                                "MUTE"
                            }
                    )
                }

                Button(
                    onClick = {
                        if (!hasPermission) {
                            permissionLauncher.launch(
                                Manifest.permission.RECORD_AUDIO
                            )
                        } else if (stats.running) {
                            engine.stop()
                        } else {
                            engine.setInputGainDb(inputGain)
                            engine.setOutputGainDb(outputGain)
                            engine.setMuted(muted)
                            engine.setBypassed(bypassed)
                            engine.start()
                        }
                    },
                    modifier = Modifier
                        .fillMaxWidth()
                        .height(56.dp)
                ) {
                    Text(
                        text =
                            if (!hasPermission) {
                                "マイク権限を許可"
                            } else if (stats.running) {
                                "STOP"
                            } else {
                                "START"
                            }
                    )
                }

                if (stats.error.isNotBlank()) {
                    Text(
                        text = stats.error,
                        color = MaterialTheme.colorScheme.error,
                        style =
                            MaterialTheme.typography.bodySmall
                    )
                }
            }
        }
    }
}

@Composable
private fun StatusCard(stats: AudioStats) {
    val stateText = when (stats.state) {
        AudioEngineState.RUNNING ->
            "● RUNNING"

        AudioEngineState.DISCONNECTED ->
            "○ DISCONNECTED"

        AudioEngineState.ERROR ->
            "○ ERROR"

        AudioEngineState.STOPPED ->
            "○ STOPPED"
    }

    val stateColor = when (stats.state) {
        AudioEngineState.RUNNING ->
            Color(0xFF63E6BE)

        AudioEngineState.DISCONNECTED ->
            Color(0xFFFFB74D)

        AudioEngineState.ERROR ->
            Color(0xFFFF8A80)

        AudioEngineState.STOPPED ->
            Color.LightGray
    }

    Card(
        colors = CardDefaults.cardColors(
            containerColor =
                MaterialTheme.colorScheme.surface
        )
    ) {
        Column(
            modifier = Modifier
                .fillMaxWidth()
                .padding(14.dp),
            verticalArrangement =
                Arrangement.spacedBy(5.dp)
        ) {
            Text(
                text = stateText,
                color = stateColor,
                fontWeight = FontWeight.Bold
            )

            Text(
                text = "Sample Rate: ${stats.sampleRate} Hz"
            )

            Text(
                text =
                    "Burst: IN ${stats.inputFramesPerBurst}" +
                            " / OUT ${stats.outputFramesPerBurst}"
            )

            Text(
                text =
                    "Buffer Size: IN ${stats.inputBufferSize}" +
                            " / OUT ${stats.outputBufferSize}"
            )

            Text(
                text =
                    "Buffer Capacity: " +
                            "IN ${stats.inputBufferCapacity}" +
                            " / OUT ${stats.outputBufferCapacity}"
            )

            Text(
                text =
                    "Channels: IN ${stats.inputChannels}" +
                            " → OUT ${stats.outputChannels}"
            )

            Text(
                text =
                    "XRuns: IN ${stats.inputXruns}" +
                            " / OUT ${stats.outputXruns}"
            )

            Text(
                text =
                    "Adaptive Buffer Adjustments: " +
                            stats.bufferAdjustments,
                color =
                    if (stats.bufferAdjustments > 0) {
                        Color(0xFFFFB74D)
                    } else {
                        Color.LightGray
                    }
            )

            Text(
                text =
                    if (stats.bypassed) {
                        "Bypass ON"
                    } else {
                        "Bypass OFF"
                    },
                color =
                    if (stats.bypassed) {
                        Color(0xFFFFB74D)
                    } else {
                        Color.LightGray
                    }
            )
        }
    }
}

@Composable
private fun MeterCard(
    title: String,
    peakDb: Float,
    gainDb: Float,
    enabled: Boolean,
    onGainChanged: (Float) -> Unit
) {
    Card(
        colors = CardDefaults.cardColors(
            containerColor =
                MaterialTheme.colorScheme.surface
        )
    ) {
        Column(
            modifier = Modifier
                .fillMaxWidth()
                .padding(14.dp)
        ) {
            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement =
                    Arrangement.SpaceBetween
            ) {
                Text(
                    text = title,
                    fontWeight = FontWeight.Bold
                )

                Text(
                    text =
                        "${formatOneDecimal(peakDb)} dBFS"
                )
            }

            Spacer(
                modifier = Modifier.height(8.dp)
            )

            val fraction =
                ((peakDb + 60f) / 60f)
                    .coerceIn(0f, 1f)

            Box(
                modifier = Modifier
                    .fillMaxWidth()
                    .height(12.dp)
                    .background(
                        color = Color(0xFF2A3038),
                        shape = RoundedCornerShape(6.dp)
                    )
            ) {
                Box(
                    modifier = Modifier
                        .fillMaxWidth(fraction)
                        .fillMaxHeight()
                        .background(
                            color =
                                if (peakDb > -1f) {
                                    Color.Red
                                } else {
                                    Color(0xFF63E6BE)
                                },
                            shape = RoundedCornerShape(6.dp)
                        )
                )
            }

            Spacer(
                modifier = Modifier.height(8.dp)
            )

            Slider(
                value = gainDb,
                onValueChange = onGainChanged,
                valueRange = -60f..12f,
                enabled = enabled
            )

            Text(
                text =
                    if (enabled) {
                        "Gain ${formatOneDecimal(gainDb)} dB"
                    } else {
                        "Gain ${formatOneDecimal(gainDb)} dB " +
                                "(Bypassed)"
                    },
                color =
                    if (enabled) {
                        Color.Unspecified
                    } else {
                        Color.LightGray
                    }
            )
        }
    }
}

private fun formatOneDecimal(value: Float): String {
    return (
            (value * 10f).roundToInt() / 10f
            ).toString()
}