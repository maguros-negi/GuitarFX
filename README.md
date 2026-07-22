# GuitarFX

Android端末を、低遅延USBオーディオ対応のギター用マルチエフェクターとして使用するためのアプリです。

ギターをUSBオーディオインターフェース経由でAndroid端末へ接続し、Noise Gate、Overdrive、3 Band EQ、Digital Delayなどのエフェクトをリアルタイムに適用します。

現在の安定版候補は **v0.2.0** です。

## 主な機能

- Oboe / AAudioを使用した低遅延オーディオ処理
- USBオーディオインターフェースからのモノラル入力
- ステレオ出力
- Input Gain / Master Gain
- Noise Gate
- Overdrive
- 3 Band EQ
- Digital Delay
- Output Limiter
- Limiter適用前のCLIP検出
- Gain Reduction表示
- 完全原音のMASTER BYPASS
- Mute
- 入出力レベルメーター
- xRun監視
- xRun増加時の適応的出力バッファ調整
- USB切断時の安全停止
- 現在設定の自動保存と起動時復元
- 5スロットのユーザープリセット
- プリセット名の設定、読込、上書き、削除
- 折りたたみ可能なプリセットパネル
- 没入型フルスクリーンUI

## 接続構成

```text
Guitar
  ↓
USB Audio Interface
  ↓ USB Type-C
Android Device
  ↓
USB Audio Interface
  ↓
Headphones / Speaker
```

端末、USBオーディオインターフェース、変換アダプターなどの組み合わせによっては正常に動作しない場合があります。

## 現在の信号経路

通常時の信号経路は次のとおりです。

```text
Input
→ Input Gain
→ Noise Gate
→ Overdrive
→ 3 Band EQ
→ Digital Delay
→ Output Gain
→ CLIP Detection
→ Output Limiter
→ MASTER BYPASS Crossfade
→ Mute
→ Output
```

MASTER BYPASS時は、入力原音がInput Gain、Effect Chain、Output Gain、CLIP保護用Limiterを迂回します。

```text
Input Raw Signal
→ MASTER BYPASS Crossfade
→ Mute
→ Output
```

Muteは最終段で適用され、通常経路とMASTER BYPASS経路の両方に対して常に最優先です。

## エフェクト

### Noise Gate

小さい入力音やノイズを抑えるエフェクトです。

- Threshold: -80 ～ -10 dBFS
- Attack: 1 ～ 100 ms
- Release: 10 ～ 1000 ms

### Overdrive

Soft Clippingを使用して、ギターサウンドへ歪みと音圧を加えます。

- Drive: 0 ～ 100%
- Tone: 0 ～ 100%
- Level: 0 ～ 100%
- DC Blocker
- Tone Low-pass
- ON / OFF時のWet / Dryクロスフェード

### 3 Band EQ

低域、中域、高域を個別に調整します。

- Low: -12 ～ +12 dB / 250 Hz Low Shelf
- Mid: -12 ～ +12 dB / 1.0 kHz Peaking / Q = 0.8
- High: -12 ～ +12 dB / 2.5 kHz High Shelf

### Digital Delay

入力音を遅らせて繰り返し再生します。

- Time: 20 ～ 1000 ms
- Feedback: 0 ～ 90%
- Mix: 0 ～ 100%
- Linear Interpolation
- Feedback経路のSoft Saturation
- ON / OFF時のクロスフェード
- OFF後のDelay余韻処理

## Output Protection

### Output Limiter

Effect ChainとOutput Gainの後段に、出力保護用Limiterを配置しています。

- Threshold: 約 -1.0 dBFS
- Attack: 瞬時
- Release: 約80 ms
- Lookahead: なし
- Makeup Gain: なし
- 初期状態: ON
- OFF時: 完全パススルー

Gain Reductionのピーク値は、視認しやすいように約250 ms保持します。

### CLIP Detection

Limiterを適用する前の信号で、絶対値が1.0以上になった場合にCLIPを検出します。

CLIP表示は約1秒間保持されます。

MASTER BYPASS中はLimiterを迂回するため、接続機器や音量設定に注意してください。

## MASTER BYPASSとMute

### MASTER BYPASS

MASTER BYPASSは、処理済み信号と入力原音を約10 msでクロスフェードします。

次の処理をすべて迂回します。

- Input Gain
- Effect Chain
- Output Gain
- Output Limiter

### Mute

Muteは出力直前の最終段で適用されます。

MASTER BYPASS中でもMuteは有効です。

## 設定の自動保存

アプリ内で変更した設定は自動保存され、次回起動時に復元されます。

保存対象は次のとおりです。

- Input Gain
- Master Gain
- Mute
- MASTER BYPASS
- Limiter ON / OFF
- 各エフェクトのON / OFF
- 各エフェクトのパラメーター
- 最後に選択していたエフェクト

Slider操作中の過剰な保存を避けるため、設定変更後に短い待機時間を設けて保存します。

安全のため、RUNNING状態は保存・復元しません。アプリ起動直後は必ずSTOPPEDとなり、ユーザーがSTARTを押したときに復元済み設定がオーディオエンジンへ反映されます。

## プリセット

5つのユーザープリセットスロットを利用できます。

各スロットでは次の操作が可能です。

- 現在の音色を保存
- プリセット名の設定と変更
- プリセットの読込
- プリセットの上書き
- プリセットの削除

プリセットパネルは、画面を広く使えるように開閉できます。

プリセットには次の設定が含まれます。

- Input Gain
- Master Gain
- Limiter ON / OFF
- 各エフェクトのON / OFF
- 各エフェクトの全パラメーター

次の項目は演奏中の安全操作であり、音色設定ではないためプリセットには含まれません。

- Mute
- MASTER BYPASS
- RUNNING / STOPPED

## Audio Diagnostics

折りたたみ式のAudio Diagnosticsで、次の情報を確認できます。

- Sample Rate
- Input / Output Frames Per Burst
- Input / Output Buffer Size
- Input / Output Buffer Capacity
- Input / Output Channels
- Input / Output XRuns
- Adaptive Buffer Adjustments
- 各エフェクトのON / OFF
- Output Limiter状態
- Gain Reduction
- Clip Hold
- MASTER BYPASS
- Mute

CPU / DSP負荷表示は、UI更新による描画負荷を避けるため現在は搭載していません。

## オーディオストリーム

- Input / OutputにOboeを使用
- Exclusive Modeを要求
- Exclusive Modeを利用できない場合はShared Modeへフォールバック
- Low Latency Modeを要求
- Float Audio Format
- Output CallbackからInput Streamをノンブロッキングで読込
- モノラル入力をステレオ出力へ複製
- 約10 msのGain Smoothing

xRunが増加した場合は、出力バッファを1 Burstずつ拡大します。バッファ容量の上限を超えて拡大することはありません。

## USB切断時の動作

USBオーディオ機器の切断を検出すると、オーディオ処理を安全に停止します。

再接続後は自動再開せず、ユーザーがSTARTを押して再開します。

## 技術構成

- Android Studio
- Kotlin
- Jetpack Compose
- Android NDK
- C++20
- JNI
- CMake 3.22.1
- Google Oboe 1.10.0
- AAudio

## Android設定

- compileSdk: 35
- minSdk: 27
- targetSdk: 35
- ABI: arm64-v8a
- Java JVM Target: 17
- Kotlin JVM Target: 17

## ビルド方法

### 必要な環境

- Android Studio
- Android SDK 35
- Android NDK
- CMake 3.22.1
- JDK 17
- arm64-v8a対応Android端末
- USBオーディオインターフェース

### 手順

1. リポジトリを取得します。

```bash
git clone https://github.com/maguros-negi/GuitarFX.git
cd GuitarFX
```

2. Android Studioでプロジェクトを開きます。
3. Gradle Syncを実行します。
4. 必要に応じて、Linked C++ Projectsを更新します。
5. arm64-v8a対応Android端末を接続します。
6. アプリをビルドして実行します。
7. マイク権限を許可します。
8. USBオーディオインターフェースを接続し、STARTを押します。

## C++変更後の完全再ビルド

JNI、C++、Native Statsの形式を変更した場合は、古いネイティブライブラリが残らないように次の手順を実行してください。

1. Android Studioを終了します。
2. 次のディレクトリを削除します。

```text
app/.cxx
app/build
```

3. Android Studioを起動します。
4. Gradle Syncを実行します。
5. 必要に応じてRefresh Linked C++ Projectsを実行します。
6. Rebuild Projectを実行します。
7. 実機へ再インストールします。

Kotlinのみを変更した場合、通常は`app/.cxx`の削除は不要です。

## リアルタイム処理上の原則

音声コールバック内では、リアルタイム安全性を維持するため次の処理を行いません。

- 動的メモリ確保
- `vector`の`resize`、`assign`、`push_back`
- Mutex待機
- ファイルI/O
- SharedPreferencesへのアクセス
- ネットワーク処理
- 大量のログ出力
- JNI呼び出し
- Streamのclose
- UIへの直接通知

Delay Bufferなどのメモリは`prepare()`で事前確保します。UIから受け取るパラメーターはatomicを介してオーディオ処理へ反映します。

## 現在の制限

- Effect Chainの順序は固定です。
- 同じエフェクトを複数配置することはできません。
- エフェクトの追加、削除、並べ替えには未対応です。
- Amp / Preamp Modelは未実装です。
- Cabinet Simulationは未実装です。
- IR Convolutionは未実装です。
- 対応状況はAndroid端末とUSBオーディオ機器の組み合わせに依存します。
- MASTER BYPASSはOutput Limiterも迂回します。

## 今後の予定

固定Effect Chain版を安定化した後、動的ペダルボード方式への移行を予定しています。

目標とする操作イメージは次のとおりです。

```text
INPUT
→ Noise Gate
→ Classic Overdrive
→ Vintage Fuzz
→ Digital Delay
→ Preamp
→ Cabinet
→ OUTPUT
```

予定している機能は次のとおりです。

- エフェクター一覧からの追加
- 同じモデルの複数配置
- Effect Instanceごとの独立したON / OFF
- Effect Instanceごとの独立したパラメーター
- 並べ替え
- 削除
- 名前変更
- Overdrive / Distortion / Fuzzなどの複数モデル
- Preamp選択
- Cabinet Simulation
- 動的Effect Chain全体の保存と復元
- Versioned Preset

Output Limiterはユーザーが並べ替えるエフェクターではなく、動的Effect Chainの後段に固定された出力保護機能として維持する予定です。

## 動作確認済み項目

Android実機で、次の項目を確認しています。

- 低遅延USBオーディオ入出力
- Noise Gate
- Overdrive
- 3 Band EQ
- Digital Delay
- Output Limiter
- CLIP検出
- Gain Reduction表示
- Mute
- 完全原音MASTER BYPASS
- START / STOP
- USB切断時の安全停止
- 設定の自動保存と起動時復元
- プリセットの保存、読込、削除
- プリセットパネルの開閉
- 没入型フルスクリーン

## ライセンス

ライセンスは現在未設定です。公開・配布方法を決定したうえで、適切なライセンスファイルを追加してください。
