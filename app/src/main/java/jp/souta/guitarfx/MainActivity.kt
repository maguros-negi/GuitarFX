package jp.souta.guitarfx

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

                StatusCard(
                    stats = stats
                )

                MeterCard(
                    title = "INPUT",
                    peakDb = stats.inputPeakDb,
                    gainDb = inputGain,
                    onGainChanged = { value ->
                        inputGain = value
                        engine.setInputGainDb(value)
                    }
                )

                MeterCard(
                    title = "OUTPUT",
                    peakDb = stats.outputPeakDb,
                    gainDb = outputGain,
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
                            }
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
                        color = Color(0xFFFF8A80),
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
                text =
                    if (stats.running) {
                        "● RUNNING"
                    } else {
                        "○ STOPPED"
                    },
                color =
                    if (stats.running) {
                        Color(0xFF63E6BE)
                    } else {
                        Color.LightGray
                    }
            )

            Text(
                text =
                    "${stats.sampleRate} Hz / " +
                            "${stats.framesPerBurst} frames per burst"
            )

            Text(
                text = "Buffer ${stats.bufferSize} frames"
            )

            Text(
                text =
                    "Input ${stats.inputChannels} ch" +
                            "  →  " +
                            "Output ${stats.outputChannels} ch"
            )

            Text(
                text =
                    "XRuns  IN ${stats.inputXruns}" +
                            " / OUT ${stats.outputXruns}"
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
                        "${((peakDb * 10).roundToInt() / 10f)} dBFS"
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
                valueRange = -60f..12f
            )

            Text(
                text =
                    "Gain " +
                            "${((gainDb * 10).roundToInt() / 10f)} dB"
            )
        }
    }
}