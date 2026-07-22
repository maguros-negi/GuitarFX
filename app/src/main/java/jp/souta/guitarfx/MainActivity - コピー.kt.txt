package jp.souta.guitarfx

import android.Manifest
import android.content.pm.PackageManager
import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.rememberLauncherForActivityResult
import androidx.activity.compose.setContent
import androidx.activity.result.contract.ActivityResultContracts
import androidx.compose.animation.animateContentSize
import androidx.compose.foundation.BorderStroke
import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.horizontalScroll
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxHeight
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.heightIn
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.Button
import androidx.compose.material3.ButtonDefaults
import androidx.compose.material3.Card
import androidx.compose.material3.CardDefaults
import androidx.compose.material3.HorizontalDivider
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedButton
import androidx.compose.material3.Slider
import androidx.compose.material3.Surface
import androidx.compose.material3.Text
import androidx.compose.material3.darkColorScheme
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableFloatStateOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.alpha
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.text.font.FontFamily
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.core.content.ContextCompat
import kotlinx.coroutines.delay
import kotlin.math.roundToInt
import kotlin.time.Duration.Companion.milliseconds

private val AppBackground = Color(0xFF090B0E)
private val PanelBackground = Color(0xFF14181D)
private val PanelBackgroundLight = Color(0xFF1B2027)
private val PanelBorder = Color(0xFF303740)
private val PrimaryGreen = Color(0xFF59E3B2)
private val AccentCyan = Color(0xFF55C7F3)
private val WarningOrange = Color(0xFFFFAD42)
private val ErrorRed = Color(0xFFFF5D6C)
private val MutedText = Color(0xFF929AA5)
private val MeterBackground = Color(0xFF272D34)

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

private enum class EffectType(
    val nativeId: Int,
    val shortName: String,
    val fullName: String,
    val description: String,
    val accentColor: Color
) {
    GATE(
        nativeId = 0,
        shortName = "GATE",
        fullName = "NOISE GATE",
        description = "小さい入力音やノイズを抑えるエフェクトです。",
        accentColor = Color(0xFF66D17A)
    ),
    DRIVE(
        nativeId = 1,
        shortName = "DRIVE",
        fullName = "OVERDRIVE",
        description = "ギターサウンドへ歪みと音圧を加えるエフェクトです。",
        accentColor = Color(0xFFFF984F)
    ),
    EQ(
        nativeId = 2,
        shortName = "EQ",
        fullName = "3 BAND EQ",
        description = "低域・中域・高域のバランスを調整するエフェクトです。",
        accentColor = Color(0xFF55B8FF)
    ),
    DELAY(
        nativeId = 3,
        shortName = "DELAY",
        fullName = "DIGITAL DELAY",
        description = "入力音を遅らせて繰り返し再生するエフェクトです。",
        accentColor = Color(0xFFB68CFF)
    )
}

@Composable
private fun GuitarFxApp(engine: AudioEngine) {
    val context = LocalContext.current

    var hasPermission by remember {
        mutableStateOf(
            ContextCompat.checkSelfPermission(
                context,
                Manifest.permission.RECORD_AUDIO
            ) == PackageManager.PERMISSION_GRANTED
        )
    }

    val permissionLauncher = rememberLauncherForActivityResult(
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

    var selectedEffect by remember {
        mutableStateOf(EffectType.DRIVE)
    }

    var diagnosticsExpanded by remember {
        mutableStateOf(false)
    }
    var gateThresholdDb by remember { mutableFloatStateOf(-50f) }
    var gateAttackMs by remember { mutableFloatStateOf(5f) }
    var gateReleaseMs by remember { mutableFloatStateOf(120f) }
    var eqLowDb by remember { mutableFloatStateOf(0f) }
    var eqMidDb by remember { mutableFloatStateOf(0f) }
    var eqHighDb by remember { mutableFloatStateOf(0f) }
    var driveAmount by remember { mutableFloatStateOf(35f) }
    var driveTone by remember { mutableFloatStateOf(50f) }
    var driveLevel by remember { mutableFloatStateOf(50f) }
    var delayTimeMs by remember { mutableFloatStateOf(350f) }
    var delayFeedback by remember { mutableFloatStateOf(35f) }
    var delayMix by remember { mutableFloatStateOf(25f) }

    LaunchedEffect(Unit) {
        while (true) {
            stats = engine.readStats()
            delay(50.milliseconds)
        }
    }

    MaterialTheme(
        colorScheme = darkColorScheme(
            primary = PrimaryGreen,
            secondary = AccentCyan,
            background = AppBackground,
            surface = PanelBackground,
            error = ErrorRed,
            onPrimary = AppBackground,
            onBackground = Color.White,
            onSurface = Color.White
        )
    ) {
        Surface(
            modifier = Modifier.fillMaxSize(),
            color = AppBackground
        ) {
            Column(
                modifier = Modifier
                    .fillMaxSize()
                    .verticalScroll(rememberScrollState())
                    .padding(
                        start = 14.dp,
                        top = 12.dp,
                        end = 14.dp,
                        bottom = 24.dp
                    ),
                verticalArrangement = Arrangement.spacedBy(12.dp)
            ) {
                AppHeader(stats = stats)

                CompactLevelPanel(
                    inputPeakDb = stats.inputPeakDb,
                    outputPeakDb = stats.outputPeakDb
                )

                SectionLabel(
                    title = "SIGNAL CHAIN",
                    subtitle = "INPUT  →  EFFECTS  →  OUTPUT"
                )

                EffectChainPanel(
                    selectedEffect = selectedEffect,
                    stats = stats,
                    masterBypassed = bypassed,
                    onEffectSelected = { effect ->
                        selectedEffect = effect
                    }
                )

                SelectedEffectPanel(
                    selectedEffect = selectedEffect,
                    enabled = isEffectEnabled(stats, selectedEffect),
                    masterBypassed = bypassed,
                    gateThresholdDb = gateThresholdDb,
                    gateAttackMs = gateAttackMs,
                    gateReleaseMs = gateReleaseMs,
                    onGateThresholdChange = { value ->
                        gateThresholdDb = value
                        engine.setNoiseGateParameters(value, gateAttackMs, gateReleaseMs)
                    },
                    onGateAttackChange = { value ->
                        gateAttackMs = value
                        engine.setNoiseGateParameters(gateThresholdDb, value, gateReleaseMs)
                    },
                    onGateReleaseChange = { value ->
                        gateReleaseMs = value
                        engine.setNoiseGateParameters(gateThresholdDb, gateAttackMs, value)
                    },
                    eqLowDb = eqLowDb,
                    eqMidDb = eqMidDb,
                    eqHighDb = eqHighDb,
                    onEqLowChange = { value ->
                        eqLowDb = value
                        engine.setThreeBandEqGains(value, eqMidDb, eqHighDb)
                    },
                    onEqMidChange = { value ->
                        eqMidDb = value
                        engine.setThreeBandEqGains(eqLowDb, value, eqHighDb)
                    },
                    onEqHighChange = { value ->
                        eqHighDb = value
                        engine.setThreeBandEqGains(eqLowDb, eqMidDb, value)
                    },
                    driveAmount = driveAmount,
                    driveTone = driveTone,
                    driveLevel = driveLevel,
                    onDriveAmountChange = { value ->
                        driveAmount = value
                        engine.setOverdriveParameters(value, driveTone, driveLevel)
                    },
                    onDriveToneChange = { value ->
                        driveTone = value
                        engine.setOverdriveParameters(driveAmount, value, driveLevel)
                    },
                    onDriveLevelChange = { value ->
                        driveLevel = value
                        engine.setOverdriveParameters(driveAmount, driveTone, value)
                    },
                    delayTimeMs = delayTimeMs,
                    delayFeedback = delayFeedback,
                    delayMix = delayMix,
                    onDelayTimeChange = { value ->
                        delayTimeMs = value
                        engine.setDelayParameters(value, delayFeedback, delayMix)
                    },
                    onDelayFeedbackChange = { value ->
                        delayFeedback = value
                        engine.setDelayParameters(delayTimeMs, value, delayMix)
                    },
                    onDelayMixChange = { value ->
                        delayMix = value
                        engine.setDelayParameters(delayTimeMs, delayFeedback, value)
                    },
                    onEnabledChange = { enabled ->
                        if (selectedEffect == EffectType.GATE) {
                            engine.setNoiseGateParameters(
                                gateThresholdDb,
                                gateAttackMs,
                                gateReleaseMs
                            )
                        }
                        if (selectedEffect == EffectType.EQ) {
                            engine.setThreeBandEqGains(eqLowDb, eqMidDb, eqHighDb)
                        }
                        if (selectedEffect == EffectType.DRIVE) {
                            engine.setOverdriveParameters(driveAmount, driveTone, driveLevel)
                        }
                        if (selectedEffect == EffectType.DELAY) {
                            engine.setDelayParameters(delayTimeMs, delayFeedback, delayMix)
                        }
                        engine.setEffectEnabled(selectedEffect.nativeId, enabled)
                    }
                )

                SectionLabel(
                    title = "GLOBAL CONTROL",
                    subtitle = "INPUT / MASTER"
                )

                GlobalGainPanel(
                    inputGain = inputGain,
                    outputGain = outputGain,
                    inputPeakDb = stats.inputPeakDb,
                    outputPeakDb = stats.outputPeakDb,
                    enabled = !bypassed,
                    onInputGainChanged = { value ->
                        inputGain = value
                        engine.setInputGainDb(value)
                    },
                    onOutputGainChanged = { value ->
                        outputGain = value
                        engine.setOutputGainDb(value)
                    }
                )

                FootSwitchPanel(
                    hasPermission = hasPermission,
                    stats = stats,
                    muted = muted,
                    bypassed = bypassed,
                    onMuteClick = {
                        muted = !muted
                        engine.setMuted(muted)
                    },
                    onBypassClick = {
                        bypassed = !bypassed
                        engine.setBypassed(bypassed)
                    },
                    onStartStopClick = {
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
                            engine.setNoiseGateParameters(
                                gateThresholdDb,
                                gateAttackMs,
                                gateReleaseMs
                            )
                            engine.setThreeBandEqGains(eqLowDb, eqMidDb, eqHighDb)
                            engine.setOverdriveParameters(driveAmount, driveTone, driveLevel)
                            engine.setDelayParameters(delayTimeMs, delayFeedback, delayMix)
                            engine.start()
                        }
                    }
                )

                if (stats.error.isNotBlank()) {
                    ErrorPanel(error = stats.error)
                }

                DiagnosticsPanel(
                    stats = stats,
                    expanded = diagnosticsExpanded,
                    onExpandedChange = {
                        diagnosticsExpanded = !diagnosticsExpanded
                    }
                )
            }
        }
    }
}

@Composable
private fun AppHeader(stats: AudioStats) {
    Row(
        modifier = Modifier.fillMaxWidth(),
        verticalAlignment = Alignment.CenterVertically,
        horizontalArrangement = Arrangement.SpaceBetween
    ) {
        Column {
            Row(
                verticalAlignment = Alignment.CenterVertically
            ) {
                Text(
                    text = "Guitar",
                    color = Color.White,
                    fontSize = 25.sp,
                    fontWeight = FontWeight.Black
                )

                Text(
                    text = "FX",
                    color = PrimaryGreen,
                    fontSize = 25.sp,
                    fontWeight = FontWeight.Black
                )
            }

            Text(
                text = "MOBILE MULTI EFFECTS",
                color = MutedText,
                fontSize = 10.sp,
                letterSpacing = 1.6.sp,
                fontWeight = FontWeight.Bold
            )
        }

        Column(
            horizontalAlignment = Alignment.End
        ) {
            EngineStateIndicator(stats.state)

            Spacer(
                modifier = Modifier.height(3.dp)
            )

            Text(
                text = "v0.2 EFFECT CHAIN",
                color = MutedText,
                fontSize = 9.sp,
                letterSpacing = 1.sp
            )
        }
    }
}

@Composable
private fun EngineStateIndicator(state: AudioEngineState) {
    val stateText: String
    val stateColor: Color

    when (state) {
        AudioEngineState.RUNNING -> {
            stateText = "RUNNING"
            stateColor = PrimaryGreen
        }

        AudioEngineState.DISCONNECTED -> {
            stateText = "DISCONNECTED"
            stateColor = WarningOrange
        }

        AudioEngineState.ERROR -> {
            stateText = "ERROR"
            stateColor = ErrorRed
        }

        AudioEngineState.STOPPED -> {
            stateText = "STOPPED"
            stateColor = MutedText
        }
    }

    Surface(
        color = stateColor.copy(alpha = 0.12f),
        shape = RoundedCornerShape(50),
        border = BorderStroke(
            width = 1.dp,
            color = stateColor.copy(alpha = 0.65f)
        )
    ) {
        Row(
            modifier = Modifier.padding(
                horizontal = 10.dp,
                vertical = 5.dp
            ),
            verticalAlignment = Alignment.CenterVertically
        ) {
            Box(
                modifier = Modifier
                    .size(7.dp)
                    .background(
                        color = stateColor,
                        shape = CircleShape
                    )
            )

            Spacer(
                modifier = Modifier.width(6.dp)
            )

            Text(
                text = stateText,
                color = stateColor,
                fontSize = 11.sp,
                fontWeight = FontWeight.Bold,
                letterSpacing = 0.8.sp
            )
        }
    }
}

@Composable
private fun CompactLevelPanel(
    inputPeakDb: Float,
    outputPeakDb: Float
) {
    Card(
        modifier = Modifier.fillMaxWidth(),
        colors = CardDefaults.cardColors(
            containerColor = PanelBackground
        ),
        border = BorderStroke(
            width = 1.dp,
            color = PanelBorder
        ),
        shape = RoundedCornerShape(12.dp)
    ) {
        Column(
            modifier = Modifier.padding(12.dp),
            verticalArrangement = Arrangement.spacedBy(9.dp)
        ) {
            CompactMeter(
                label = "IN",
                peakDb = inputPeakDb,
                accentColor = PrimaryGreen
            )

            CompactMeter(
                label = "OUT",
                peakDb = outputPeakDb,
                accentColor = AccentCyan
            )
        }
    }
}

@Composable
private fun CompactMeter(
    label: String,
    peakDb: Float,
    accentColor: Color
) {
    val fraction = ((peakDb + 60f) / 60f)
        .coerceIn(0f, 1f)

    val meterColor = when {
        peakDb > -1f -> ErrorRed
        peakDb > -6f -> WarningOrange
        else -> accentColor
    }

    Row(
        modifier = Modifier.fillMaxWidth(),
        verticalAlignment = Alignment.CenterVertically
    ) {
        Text(
            text = label,
            modifier = Modifier.width(34.dp),
            color = Color.White,
            fontSize = 11.sp,
            fontWeight = FontWeight.Black
        )

        Box(
            modifier = Modifier
                .weight(1f)
                .height(9.dp)
                .background(
                    color = MeterBackground,
                    shape = RoundedCornerShape(2.dp)
                )
        ) {
            Box(
                modifier = Modifier
                    .fillMaxWidth(fraction)
                    .fillMaxHeight()
                    .background(
                        color = meterColor,
                        shape = RoundedCornerShape(2.dp)
                    )
            )
        }

        Text(
            text = "${formatOneDecimal(peakDb)} dBFS",
            modifier = Modifier.width(82.dp),
            color = meterColor,
            textAlign = TextAlign.End,
            fontSize = 11.sp,
            fontFamily = FontFamily.Monospace
        )
    }
}

@Composable
private fun SectionLabel(
    title: String,
    subtitle: String
) {
    Row(
        modifier = Modifier
            .fillMaxWidth()
            .padding(
                top = 2.dp,
                start = 2.dp,
                end = 2.dp
            ),
        horizontalArrangement = Arrangement.SpaceBetween,
        verticalAlignment = Alignment.Bottom
    ) {
        Text(
            text = title,
            color = Color.White,
            fontSize = 12.sp,
            fontWeight = FontWeight.Black,
            letterSpacing = 1.3.sp
        )

        Text(
            text = subtitle,
            color = MutedText,
            fontSize = 9.sp,
            letterSpacing = 0.8.sp
        )
    }
}

@Composable
private fun EffectChainPanel(
    selectedEffect: EffectType,
    stats: AudioStats,
    masterBypassed: Boolean,
    onEffectSelected: (EffectType) -> Unit
) {
    Card(
        modifier = Modifier.fillMaxWidth(),
        colors = CardDefaults.cardColors(
            containerColor = PanelBackground
        ),
        border = BorderStroke(
            width = 1.dp,
            color = PanelBorder
        ),
        shape = RoundedCornerShape(12.dp)
    ) {
        Row(
            modifier = Modifier
                .fillMaxWidth()
                .horizontalScroll(rememberScrollState())
                .padding(12.dp),
            verticalAlignment = Alignment.CenterVertically
        ) {
            SignalEndpoint(
                text = "IN",
                accentColor = PrimaryGreen
            )

            SignalCable()

            EffectType.entries.forEachIndexed { index, effect ->
                EffectBlock(
                    effect = effect,
                    selected = selectedEffect == effect,
                    enabled = isEffectEnabled(stats, effect),
                    masterBypassed = masterBypassed,
                    onClick = {
                        onEffectSelected(effect)
                    }
                )

                if (index != EffectType.entries.lastIndex) {
                    SignalCable()
                }
            }

            SignalCable()

            SignalEndpoint(
                text = "OUT",
                accentColor = AccentCyan
            )
        }
    }
}

@Composable
private fun SignalEndpoint(
    text: String,
    accentColor: Color
) {
    Column(
        modifier = Modifier.width(42.dp),
        horizontalAlignment = Alignment.CenterHorizontally
    ) {
        Box(
            modifier = Modifier
                .size(30.dp)
                .background(
                    color = accentColor.copy(alpha = 0.14f),
                    shape = CircleShape
                ),
            contentAlignment = Alignment.Center
        ) {
            Box(
                modifier = Modifier
                    .size(10.dp)
                    .background(
                        color = accentColor,
                        shape = CircleShape
                    )
            )
        }

        Spacer(
            modifier = Modifier.height(5.dp)
        )

        Text(
            text = text,
            color = accentColor,
            fontSize = 9.sp,
            fontWeight = FontWeight.Bold
        )
    }
}

@Composable
private fun SignalCable() {
    Box(
        modifier = Modifier
            .width(14.dp)
            .height(2.dp)
            .background(PanelBorder)
    )
}

@Composable
private fun EffectBlock(
    effect: EffectType,
    selected: Boolean,
    enabled: Boolean,
    masterBypassed: Boolean,
    onClick: () -> Unit
) {
    val borderColor = if (selected) {
        effect.accentColor
    } else {
        PanelBorder
    }

    val backgroundColor = if (selected) {
        effect.accentColor.copy(alpha = 0.11f)
    } else {
        PanelBackgroundLight
    }

    Card(
        modifier = Modifier
            .width(78.dp)
            .height(94.dp)
            .clickable(onClick = onClick),
        colors = CardDefaults.cardColors(
            containerColor = backgroundColor
        ),
        border = BorderStroke(
            width = if (selected) 2.dp else 1.dp,
            color = borderColor
        ),
        shape = RoundedCornerShape(9.dp)
    ) {
        Column(
            modifier = Modifier
                .fillMaxSize()
                .padding(8.dp),
            horizontalAlignment = Alignment.CenterHorizontally,
            verticalArrangement = Arrangement.SpaceBetween
        ) {
            Box(
                modifier = Modifier
                    .size(9.dp)
                    .background(
                        color = when {
                            masterBypassed -> WarningOrange.copy(alpha = 0.45f)
                            enabled -> effect.accentColor
                            else -> MutedText.copy(alpha = 0.5f)
                        },
                        shape = CircleShape
                    )
            )

            Text(
                text = effect.shortName,
                color = if (selected) {
                    effect.accentColor
                } else {
                    Color.White
                },
                fontSize = 11.sp,
                fontWeight = FontWeight.Black,
                textAlign = TextAlign.Center
            )

            Text(
                text = when {
                    masterBypassed && enabled -> "BYPASS"
                    enabled -> "ON"
                    else -> "OFF"
                },
                color = when {
                    masterBypassed && enabled -> WarningOrange
                    enabled -> effect.accentColor
                    else -> MutedText
                },
                fontSize = 9.sp,
                fontWeight = FontWeight.Bold
            )
        }
    }
}

@Composable
private fun SelectedEffectPanel(
    selectedEffect: EffectType,
    enabled: Boolean,
    masterBypassed: Boolean,
    gateThresholdDb: Float,
    gateAttackMs: Float,
    gateReleaseMs: Float,
    onGateThresholdChange: (Float) -> Unit,
    onGateAttackChange: (Float) -> Unit,
    onGateReleaseChange: (Float) -> Unit,
    eqLowDb: Float,
    eqMidDb: Float,
    eqHighDb: Float,
    onEqLowChange: (Float) -> Unit,
    onEqMidChange: (Float) -> Unit,
    onEqHighChange: (Float) -> Unit,
    driveAmount: Float,
    driveTone: Float,
    driveLevel: Float,
    onDriveAmountChange: (Float) -> Unit,
    onDriveToneChange: (Float) -> Unit,
    onDriveLevelChange: (Float) -> Unit,
    delayTimeMs: Float,
    delayFeedback: Float,
    delayMix: Float,
    onDelayTimeChange: (Float) -> Unit,
    onDelayFeedbackChange: (Float) -> Unit,
    onDelayMixChange: (Float) -> Unit,
    onEnabledChange: (Boolean) -> Unit
) {
    Card(
        modifier = Modifier.fillMaxWidth(),
        colors = CardDefaults.cardColors(
            containerColor = PanelBackgroundLight
        ),
        border = BorderStroke(
            width = 1.dp,
            color = selectedEffect.accentColor.copy(alpha = 0.55f)
        ),
        shape = RoundedCornerShape(12.dp)
    ) {
        Column(
            modifier = Modifier.padding(14.dp)
        ) {
            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.SpaceBetween,
                verticalAlignment = Alignment.Top
            ) {
                Column(
                    modifier = Modifier.weight(1f)
                ) {
                    Text(
                        text = "SELECTED EFFECT",
                        color = MutedText,
                        fontSize = 9.sp,
                        fontWeight = FontWeight.Bold,
                        letterSpacing = 1.2.sp
                    )
                    Spacer(modifier = Modifier.height(4.dp))
                    Text(
                        text = selectedEffect.fullName,
                        color = selectedEffect.accentColor,
                        fontSize = 18.sp,
                        fontWeight = FontWeight.Black
                    )
                }

                OutlinedButton(
                    onClick = { onEnabledChange(!enabled) },
                    shape = RoundedCornerShape(50),
                    border = BorderStroke(
                        width = if (enabled) 2.dp else 1.dp,
                        color = if (enabled) {
                            selectedEffect.accentColor
                        } else {
                            PanelBorder
                        }
                    ),
                    colors = ButtonDefaults.outlinedButtonColors(
                        containerColor = if (enabled) {
                            selectedEffect.accentColor.copy(alpha = 0.16f)
                        } else {
                            PanelBackground
                        },
                        contentColor = if (enabled) {
                            selectedEffect.accentColor
                        } else {
                            MutedText
                        }
                    )
                ) {
                    Text(
                        text = if (enabled) "EFFECT ON" else "EFFECT OFF",
                        fontSize = 9.sp,
                        fontWeight = FontWeight.Black
                    )
                }
            }

            Spacer(modifier = Modifier.height(8.dp))

            Text(
                text = selectedEffect.description,
                color = Color(0xFFD2D6DB),
                fontSize = 12.sp
            )

            if (masterBypassed && enabled) {
                Spacer(modifier = Modifier.height(8.dp))
                Text(
                    text = "MASTER BYPASSにより現在は迂回中です。",
                    color = WarningOrange,
                    fontSize = 10.sp,
                    fontWeight = FontWeight.Bold
                )
            }

            Spacer(modifier = Modifier.height(12.dp))

            if (selectedEffect == EffectType.GATE) {
                GateParameterSlider(
                    label = "THRESHOLD",
                    valueText = "${formatOneDecimal(gateThresholdDb)} dBFS",
                    value = gateThresholdDb,
                    valueRange = -80f..-10f,
                    enabled = !masterBypassed,
                    accentColor = selectedEffect.accentColor,
                    onValueChange = onGateThresholdChange
                )
                GateParameterSlider(
                    label = "ATTACK",
                    valueText = "${formatOneDecimal(gateAttackMs)} ms",
                    value = gateAttackMs,
                    valueRange = 1f..100f,
                    enabled = !masterBypassed,
                    accentColor = selectedEffect.accentColor,
                    onValueChange = onGateAttackChange
                )
                GateParameterSlider(
                    label = "RELEASE",
                    valueText = "${gateReleaseMs.roundToInt()} ms",
                    value = gateReleaseMs,
                    valueRange = 10f..1000f,
                    enabled = !masterBypassed,
                    accentColor = selectedEffect.accentColor,
                    onValueChange = onGateReleaseChange
                )
            } else if (selectedEffect == EffectType.DRIVE) {
                PercentParameterSlider(
                    label = "DRIVE",
                    hint = "SATURATION",
                    value = driveAmount,
                    enabled = !masterBypassed,
                    accentColor = selectedEffect.accentColor,
                    onValueChange = onDriveAmountChange
                )
                PercentParameterSlider(
                    label = "TONE",
                    hint = "DARK  ←  →  BRIGHT",
                    value = driveTone,
                    enabled = !masterBypassed,
                    accentColor = selectedEffect.accentColor,
                    onValueChange = onDriveToneChange
                )
                PercentParameterSlider(
                    label = "LEVEL",
                    hint = "OUTPUT",
                    value = driveLevel,
                    enabled = !masterBypassed,
                    accentColor = selectedEffect.accentColor,
                    onValueChange = onDriveLevelChange
                )
            } else if (selectedEffect == EffectType.EQ) {
                EqParameterSlider(
                    label = "LOW",
                    frequencyText = "250 Hz SHELF",
                    value = eqLowDb,
                    enabled = !masterBypassed,
                    accentColor = selectedEffect.accentColor,
                    onValueChange = onEqLowChange
                )
                EqParameterSlider(
                    label = "MID",
                    frequencyText = "1.0 kHz PEAK",
                    value = eqMidDb,
                    enabled = !masterBypassed,
                    accentColor = selectedEffect.accentColor,
                    onValueChange = onEqMidChange
                )
                EqParameterSlider(
                    label = "HIGH",
                    frequencyText = "2.5 kHz SHELF",
                    value = eqHighDb,
                    enabled = !masterBypassed,
                    accentColor = selectedEffect.accentColor,
                    onValueChange = onEqHighChange
                )
                Spacer(modifier = Modifier.height(4.dp))
                Text(
                    text = "Boost時はクリップを避けるためMASTERを下げてください。",
                    modifier = Modifier.fillMaxWidth(),
                    color = WarningOrange,
                    textAlign = TextAlign.Center,
                    fontSize = 8.sp
                )
            } else if (selectedEffect == EffectType.DELAY) {
                DelayTimeSlider(
                    value = delayTimeMs,
                    enabled = !masterBypassed,
                    accentColor = selectedEffect.accentColor,
                    onValueChange = onDelayTimeChange
                )
                PercentParameterSlider(
                    label = "FEEDBACK",
                    hint = "REPEATS 0–90%",
                    value = delayFeedback,
                    enabled = !masterBypassed,
                    accentColor = selectedEffect.accentColor,
                    valueRange = 0f..90f,
                    onValueChange = onDelayFeedbackChange
                )
                PercentParameterSlider(
                    label = "MIX",
                    hint = "DRY / WET",
                    value = delayMix,
                    enabled = !masterBypassed,
                    accentColor = selectedEffect.accentColor,
                    onValueChange = onDelayMixChange
                )
            } else {
                Row(
                    modifier = Modifier.fillMaxWidth(),
                    horizontalArrangement = Arrangement.spacedBy(8.dp)
                ) {
                    ParameterPlaceholder(firstParameterName(selectedEffect), Modifier.weight(1f))
                    ParameterPlaceholder(secondParameterName(selectedEffect), Modifier.weight(1f))
                    ParameterPlaceholder(thirdParameterName(selectedEffect), Modifier.weight(1f))
                }
                Spacer(modifier = Modifier.height(8.dp))
                Text(
                    text = "DSP PARAMETERS COMING NEXT",
                    modifier = Modifier.fillMaxWidth(),
                    color = MutedText,
                    textAlign = TextAlign.Center,
                    fontSize = 8.sp,
                    fontWeight = FontWeight.Bold,
                    letterSpacing = 0.8.sp
                )
            }
        }
    }
}

@Composable
private fun DelayTimeSlider(
    value: Float,
    enabled: Boolean,
    accentColor: Color,
    onValueChange: (Float) -> Unit
) {
    Column(modifier = Modifier.fillMaxWidth()) {
        Row(
            modifier = Modifier.fillMaxWidth(),
            horizontalArrangement = Arrangement.SpaceBetween
        ) {
            Column {
                Text("TIME", color = MutedText, fontSize = 9.sp, fontWeight = FontWeight.Bold)
                Text("20–1000 ms", color = MutedText, fontSize = 8.sp)
            }
            Text(
                text = "${value.roundToInt()} ms",
                color = accentColor,
                fontSize = 10.sp,
                fontFamily = FontFamily.Monospace
            )
        }
        Slider(
            value = value,
            onValueChange = onValueChange,
            valueRange = 20f..1000f,
            enabled = enabled
        )
    }
}

@Composable
private fun PercentParameterSlider(
    label: String,
    hint: String,
    value: Float,
    enabled: Boolean,
    accentColor: Color,
    valueRange: ClosedFloatingPointRange<Float> = 0f..100f,
    onValueChange: (Float) -> Unit
) {
    Column(modifier = Modifier.fillMaxWidth()) {
        Row(
            modifier = Modifier.fillMaxWidth(),
            horizontalArrangement = Arrangement.SpaceBetween
        ) {
            Column {
                Text(label, color = MutedText, fontSize = 9.sp, fontWeight = FontWeight.Bold)
                Text(hint, color = MutedText, fontSize = 8.sp)
            }
            Text(
                text = "${value.roundToInt()} %",
                color = accentColor,
                fontSize = 10.sp,
                fontFamily = FontFamily.Monospace
            )
        }
        Slider(
            value = value,
            onValueChange = onValueChange,
            valueRange = valueRange,
            enabled = enabled
        )
    }
}

@Composable
private fun EqParameterSlider(
    label: String,
    frequencyText: String,
    value: Float,
    enabled: Boolean,
    accentColor: Color,
    onValueChange: (Float) -> Unit
) {
    Column(modifier = Modifier.fillMaxWidth()) {
        Row(
            modifier = Modifier.fillMaxWidth(),
            horizontalArrangement = Arrangement.SpaceBetween
        ) {
            Column {
                Text(label, color = MutedText, fontSize = 9.sp, fontWeight = FontWeight.Bold)
                Text(frequencyText, color = MutedText, fontSize = 8.sp)
            }
            Text(
                text = "${formatSignedOneDecimal(value)} dB",
                color = accentColor,
                fontSize = 10.sp,
                fontFamily = FontFamily.Monospace
            )
        }
        Slider(
            value = value,
            onValueChange = onValueChange,
            valueRange = -12f..12f,
            enabled = enabled
        )
    }
}

@Composable
private fun GateParameterSlider(
    label: String,
    valueText: String,
    value: Float,
    valueRange: ClosedFloatingPointRange<Float>,
    enabled: Boolean,
    accentColor: Color,
    onValueChange: (Float) -> Unit
) {
    Column(modifier = Modifier.fillMaxWidth()) {
        Row(
            modifier = Modifier.fillMaxWidth(),
            horizontalArrangement = Arrangement.SpaceBetween
        ) {
            Text(label, color = MutedText, fontSize = 9.sp, fontWeight = FontWeight.Bold)
            Text(valueText, color = accentColor, fontSize = 10.sp, fontFamily = FontFamily.Monospace)
        }
        Slider(
            value = value,
            onValueChange = onValueChange,
            valueRange = valueRange,
            enabled = enabled
        )
    }
}

@Composable
private fun ParameterPlaceholder(
    label: String,
    modifier: Modifier = Modifier
) {
    Column(
        modifier = modifier
            .alpha(0.55f)
            .background(
                color = AppBackground.copy(alpha = 0.7f),
                shape = RoundedCornerShape(8.dp)
            )
            .padding(
                horizontal = 6.dp,
                vertical = 10.dp
            ),
        horizontalAlignment = Alignment.CenterHorizontally
    ) {
        Box(
            modifier = Modifier
                .size(27.dp)
                .background(
                    color = PanelBorder,
                    shape = CircleShape
                )
        )

        Spacer(
            modifier = Modifier.height(6.dp)
        )

        Text(
            text = label,
            color = MutedText,
            textAlign = TextAlign.Center,
            fontSize = 8.sp,
            fontWeight = FontWeight.Bold,
            maxLines = 1
        )
    }
}

private fun isEffectEnabled(
    stats: AudioStats,
    effect: EffectType
): Boolean {
    return when (effect) {
        EffectType.GATE -> stats.gateEnabled
        EffectType.DRIVE -> stats.driveEnabled
        EffectType.EQ -> stats.eqEnabled
        EffectType.DELAY -> stats.delayEnabled
    }
}

private fun firstParameterName(effect: EffectType): String {
    return when (effect) {
        EffectType.GATE -> "THRESHOLD"
        EffectType.DRIVE -> "DRIVE"
        EffectType.EQ -> "LOW"
        EffectType.DELAY -> "TIME"
    }
}

private fun secondParameterName(effect: EffectType): String {
    return when (effect) {
        EffectType.GATE -> "ATTACK"
        EffectType.DRIVE -> "TONE"
        EffectType.EQ -> "MID"
        EffectType.DELAY -> "FEEDBACK"
    }
}

private fun thirdParameterName(effect: EffectType): String {
    return when (effect) {
        EffectType.GATE -> "RELEASE"
        EffectType.DRIVE -> "LEVEL"
        EffectType.EQ -> "HIGH"
        EffectType.DELAY -> "MIX"
    }
}

@Composable
private fun GlobalGainPanel(
    inputGain: Float,
    outputGain: Float,
    inputPeakDb: Float,
    outputPeakDb: Float,
    enabled: Boolean,
    onInputGainChanged: (Float) -> Unit,
    onOutputGainChanged: (Float) -> Unit
) {
    Row(
        modifier = Modifier.fillMaxWidth(),
        horizontalArrangement = Arrangement.spacedBy(10.dp)
    ) {
        GainControlCard(
            title = "INPUT",
            subtitle = "PRE GAIN",
            peakDb = inputPeakDb,
            gainDb = inputGain,
            enabled = enabled,
            accentColor = PrimaryGreen,
            modifier = Modifier.weight(1f),
            onGainChanged = onInputGainChanged
        )

        GainControlCard(
            title = "MASTER",
            subtitle = "OUTPUT",
            peakDb = outputPeakDb,
            gainDb = outputGain,
            enabled = enabled,
            accentColor = AccentCyan,
            modifier = Modifier.weight(1f),
            onGainChanged = onOutputGainChanged
        )
    }
}

@Composable
private fun GainControlCard(
    title: String,
    subtitle: String,
    peakDb: Float,
    gainDb: Float,
    enabled: Boolean,
    accentColor: Color,
    modifier: Modifier = Modifier,
    onGainChanged: (Float) -> Unit
) {
    Card(
        modifier = modifier,
        colors = CardDefaults.cardColors(
            containerColor = PanelBackground
        ),
        border = BorderStroke(
            width = 1.dp,
            color = PanelBorder
        ),
        shape = RoundedCornerShape(12.dp)
    ) {
        Column(
            modifier = Modifier.padding(12.dp)
        ) {
            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.SpaceBetween,
                verticalAlignment = Alignment.Top
            ) {
                Column {
                    Text(
                        text = title,
                        color = accentColor,
                        fontSize = 13.sp,
                        fontWeight = FontWeight.Black
                    )

                    Text(
                        text = subtitle,
                        color = MutedText,
                        fontSize = 8.sp,
                        letterSpacing = 0.8.sp
                    )
                }

                Text(
                    text = "${formatOneDecimal(peakDb)} dBFS",
                    color = MutedText,
                    fontSize = 9.sp,
                    fontFamily = FontFamily.Monospace
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
                text = if (enabled) {
                    "${formatOneDecimal(gainDb)} dB"
                } else {
                    "${formatOneDecimal(gainDb)} dB  BYPASSED"
                },
                modifier = Modifier.fillMaxWidth(),
                color = if (enabled) {
                    Color.White
                } else {
                    WarningOrange
                },
                fontSize = 11.sp,
                fontWeight = FontWeight.Bold,
                fontFamily = FontFamily.Monospace,
                textAlign = TextAlign.Center
            )
        }
    }
}

@Composable
private fun FootSwitchPanel(
    hasPermission: Boolean,
    stats: AudioStats,
    muted: Boolean,
    bypassed: Boolean,
    onMuteClick: () -> Unit,
    onBypassClick: () -> Unit,
    onStartStopClick: () -> Unit
) {
    Card(
        modifier = Modifier.fillMaxWidth(),
        colors = CardDefaults.cardColors(
            containerColor = Color(0xFF111419)
        ),
        border = BorderStroke(
            width = 1.dp,
            color = PanelBorder
        ),
        shape = RoundedCornerShape(14.dp)
    ) {
        Column(
            modifier = Modifier.padding(12.dp)
        ) {
            Text(
                text = "FOOT CONTROLS",
                color = MutedText,
                fontSize = 9.sp,
                fontWeight = FontWeight.Bold,
                letterSpacing = 1.4.sp
            )

            Spacer(
                modifier = Modifier.height(10.dp)
            )

            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.spacedBy(8.dp)
            ) {
                FootSwitchButton(
                    label = if (muted) {
                        "MUTED"
                    } else {
                        "MUTE"
                    },
                    active = muted,
                    activeColor = ErrorRed,
                    modifier = Modifier.weight(1f),
                    onClick = onMuteClick
                )

                FootSwitchButton(
                    label = if (bypassed) {
                        "BYPASS ON"
                    } else {
                        "BYPASS"
                    },
                    active = bypassed,
                    activeColor = WarningOrange,
                    modifier = Modifier.weight(1f),
                    onClick = onBypassClick
                )

                StartStopButton(
                    hasPermission = hasPermission,
                    running = stats.running,
                    modifier = Modifier.weight(1f),
                    onClick = onStartStopClick
                )
            }
        }
    }
}

@Composable
private fun FootSwitchButton(
    label: String,
    active: Boolean,
    activeColor: Color,
    modifier: Modifier = Modifier,
    onClick: () -> Unit
) {
    val backgroundColor = if (active) {
        activeColor.copy(alpha = 0.18f)
    } else {
        PanelBackgroundLight
    }

    val borderColor = if (active) {
        activeColor
    } else {
        PanelBorder
    }

    OutlinedButton(
        onClick = onClick,
        modifier = modifier.height(67.dp),
        shape = RoundedCornerShape(10.dp),
        border = BorderStroke(
            width = if (active) 2.dp else 1.dp,
            color = borderColor
        ),
        colors = ButtonDefaults.outlinedButtonColors(
            containerColor = backgroundColor,
            contentColor = if (active) {
                activeColor
            } else {
                Color.White
            }
        ),
        contentPadding = androidx.compose.foundation.layout.PaddingValues(
            horizontal = 4.dp,
            vertical = 6.dp
        )
    ) {
        Column(
            horizontalAlignment = Alignment.CenterHorizontally
        ) {
            Box(
                modifier = Modifier
                    .size(8.dp)
                    .background(
                        color = if (active) {
                            activeColor
                        } else {
                            MutedText.copy(alpha = 0.35f)
                        },
                        shape = CircleShape
                    )
            )

            Spacer(
                modifier = Modifier.height(7.dp)
            )

            Text(
                text = label,
                fontSize = 9.sp,
                fontWeight = FontWeight.Black,
                textAlign = TextAlign.Center,
                maxLines = 1
            )
        }
    }
}

@Composable
private fun StartStopButton(
    hasPermission: Boolean,
    running: Boolean,
    modifier: Modifier = Modifier,
    onClick: () -> Unit
) {
    val buttonText = when {
        !hasPermission -> "PERMISSION"
        running -> "STOP"
        else -> "START"
    }

    val buttonColor = when {
        !hasPermission -> WarningOrange
        running -> ErrorRed
        else -> PrimaryGreen
    }

    Button(
        onClick = onClick,
        modifier = modifier.height(67.dp),
        shape = RoundedCornerShape(10.dp),
        colors = ButtonDefaults.buttonColors(
            containerColor = buttonColor,
            contentColor = AppBackground
        ),
        contentPadding = androidx.compose.foundation.layout.PaddingValues(
            horizontal = 4.dp,
            vertical = 6.dp
        )
    ) {
        Column(
            horizontalAlignment = Alignment.CenterHorizontally
        ) {
            Box(
                modifier = Modifier
                    .size(8.dp)
                    .background(
                        color = AppBackground.copy(alpha = 0.75f),
                        shape = CircleShape
                    )
            )

            Spacer(
                modifier = Modifier.height(7.dp)
            )

            Text(
                text = buttonText,
                fontSize = 9.sp,
                fontWeight = FontWeight.Black,
                textAlign = TextAlign.Center,
                maxLines = 1
            )
        }
    }
}

@Composable
private fun ErrorPanel(error: String) {
    Card(
        modifier = Modifier.fillMaxWidth(),
        colors = CardDefaults.cardColors(
            containerColor = ErrorRed.copy(alpha = 0.1f)
        ),
        border = BorderStroke(
            width = 1.dp,
            color = ErrorRed.copy(alpha = 0.7f)
        ),
        shape = RoundedCornerShape(10.dp)
    ) {
        Column(
            modifier = Modifier.padding(12.dp)
        ) {
            Text(
                text = "AUDIO ENGINE ERROR",
                color = ErrorRed,
                fontSize = 10.sp,
                fontWeight = FontWeight.Black,
                letterSpacing = 1.sp
            )

            Spacer(
                modifier = Modifier.height(5.dp)
            )

            Text(
                text = error,
                color = Color(0xFFFFC8CD),
                fontSize = 11.sp
            )
        }
    }
}

@Composable
private fun DiagnosticsPanel(
    stats: AudioStats,
    expanded: Boolean,
    onExpandedChange: () -> Unit
) {
    Card(
        modifier = Modifier
            .fillMaxWidth()
            .animateContentSize()
            .clickable(onClick = onExpandedChange),
        colors = CardDefaults.cardColors(
            containerColor = PanelBackground
        ),
        border = BorderStroke(
            width = 1.dp,
            color = PanelBorder
        ),
        shape = RoundedCornerShape(12.dp)
    ) {
        Column(
            modifier = Modifier.padding(13.dp)
        ) {
            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.SpaceBetween,
                verticalAlignment = Alignment.CenterVertically
            ) {
                Column {
                    Text(
                        text = "AUDIO DIAGNOSTICS",
                        color = Color.White,
                        fontSize = 11.sp,
                        fontWeight = FontWeight.Black,
                        letterSpacing = 1.1.sp
                    )

                    Text(
                        text = "XRuns IN ${stats.inputXruns} / OUT ${stats.outputXruns}",
                        color = if (
                            stats.inputXruns > 0 ||
                            stats.outputXruns > 0
                        ) {
                            WarningOrange
                        } else {
                            MutedText
                        },
                        fontSize = 9.sp,
                        fontFamily = FontFamily.Monospace
                    )
                }

                Text(
                    text = if (expanded) {
                        "▲ CLOSE"
                    } else {
                        "▼ DETAILS"
                    },
                    color = AccentCyan,
                    fontSize = 9.sp,
                    fontWeight = FontWeight.Bold
                )
            }

            if (expanded) {
                Spacer(
                    modifier = Modifier.height(12.dp)
                )

                HorizontalDivider(
                    color = PanelBorder
                )

                Spacer(
                    modifier = Modifier.height(10.dp)
                )

                DiagnosticRow(
                    label = "Sample Rate",
                    value = "${stats.sampleRate} Hz"
                )

                DiagnosticRow(
                    label = "Frames Per Burst",
                    value = "IN ${stats.inputFramesPerBurst} / " +
                            "OUT ${stats.outputFramesPerBurst}"
                )

                DiagnosticRow(
                    label = "Buffer Size",
                    value = "IN ${stats.inputBufferSize} / " +
                            "OUT ${stats.outputBufferSize}"
                )

                DiagnosticRow(
                    label = "Buffer Capacity",
                    value = "IN ${stats.inputBufferCapacity} / " +
                            "OUT ${stats.outputBufferCapacity}"
                )

                DiagnosticRow(
                    label = "Channels",
                    value = "IN ${stats.inputChannels} / " +
                            "OUT ${stats.outputChannels}"
                )

                DiagnosticRow(
                    label = "XRuns",
                    value = "IN ${stats.inputXruns} / " +
                            "OUT ${stats.outputXruns}",
                    warning = stats.inputXruns > 0 ||
                            stats.outputXruns > 0
                )

                DiagnosticRow(
                    label = "Buffer Adjustments",
                    value = stats.bufferAdjustments.toString(),
                    warning = stats.bufferAdjustments > 0
                )

                DiagnosticRow(
                    label = "Effects",
                    value = "G ${if (stats.gateEnabled) 1 else 0} / " +
                            "D ${if (stats.driveEnabled) 1 else 0} / " +
                            "E ${if (stats.eqEnabled) 1 else 0} / " +
                            "DL ${if (stats.delayEnabled) 1 else 0}"
                )
                DiagnosticRow(
                    label = "Master Bypass",
                    value = if (stats.bypassed) {
                        "ON"
                    } else {
                        "OFF"
                    },
                    warning = stats.bypassed
                )

                DiagnosticRow(
                    label = "Mute",
                    value = if (stats.muted) {
                        "ON"
                    } else {
                        "OFF"
                    },
                    warning = stats.muted
                )
            }
        }
    }
}

@Composable
private fun DiagnosticRow(
    label: String,
    value: String,
    warning: Boolean = false
) {
    Row(
        modifier = Modifier
            .fillMaxWidth()
            .heightIn(min = 27.dp),
        horizontalArrangement = Arrangement.SpaceBetween,
        verticalAlignment = Alignment.CenterVertically
    ) {
        Text(
            text = label,
            color = MutedText,
            fontSize = 10.sp
        )

        Text(
            text = value,
            color = if (warning) {
                WarningOrange
            } else {
                Color(0xFFDFE3E7)
            },
            fontSize = 10.sp,
            fontWeight = FontWeight.Bold,
            fontFamily = FontFamily.Monospace,
            textAlign = TextAlign.End
        )
    }
}

private fun formatSignedOneDecimal(value: Float): String {
    val formatted = formatOneDecimal(value)
    return if (value > 0f) "+$formatted" else formatted
}

private fun formatOneDecimal(value: Float): String {
    return ((value * 10f).roundToInt() / 10f).toString()
}