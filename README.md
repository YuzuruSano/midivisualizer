# MIDI Visualizer for Ableton Live

## セットアップ手順

### 1. ofxMidiアドオンのインストール
```bash
cd path/to/openFrameworks/addons
git clone https://github.com/danomatika/ofxMidi.git
```

### 2. プロジェクトのビルド
- openFrameworks Project Generatorを使用してプロジェクトを生成
- `ofxMidi`アドオンを追加
- XcodeまたはMakefileでビルド

#### コンパイルコマンド
```bash
make MAC_OS_CPP_VER="-std=c++17"
```

#### 起動コマンド
```bash
# 方法1: 直接実行
./bin/midiVisualizer.app/Contents/MacOS/midiVisualizer

# 方法2: Finderから起動
open bin/midiVisualizer.app
```

### 3. macOSでの仮想MIDIポート設定

1. **Audio MIDI設定**を開く（アプリケーション > ユーティリティ）
2. メニューから「ウインドウ」>「MIDIスタジオを表示」
3. 「IAC Driver」をダブルクリック
4. 「装置はオンライン」にチェック
5. ポートを追加（例：「OF-Live」）

### 4. Ableton Liveの設定

1. 環境設定 > Link/Tempo/MIDI
2. MIDI出力で「IAC Driver (OF-Live)」を有効化
3. トラックのMIDI出力先を「IAC Driver」に設定

### 5. アプリケーションの使用方法

- 起動すると利用可能なMIDIポートが表示される
- 数字キー（0-9）でポートを切り替え
- MIDIノート：円の色と大きさが変化
- CC1（モジュレーションホイール）：基本円サイズを調整

## ビジュアライゼーション

### ビジュアルシステムの再生順序
1. **ParticleSystem** - パーティクルシステム（都市の塵、光の粒子）
2. **FractalSystem** - フラクタルシステム（都市の自己相似構造）
3. **WaveSystem** - ウェーブシステム（都市の波動、振動）
4. **FlowFieldSystem** - フローフィールドシステム（都市の流れ、交通）
5. **LSystemSystem** - L-システム（建設現場、都市開発、クレーン）
6. **DifferentialGrowthSystem** - 差分成長システム（メトロポリタン発展）
7. **ReactionDiffusionSystem** - 反応拡散システム（都市セルラーオートマトン）

### 操作方法
- **スペースキー**: 次のシステムへ切り替え（4秒間のクロスフェード）
- **1-7キー**: システムを直接選択
- **Hキー**: UI表示/非表示
- **0,8-9キー**: MIDIポート切り替え

### 自動切替機能
- **MIDIテンポ検出**: KICKドラム（NOTE 35/36）からBPMを自動検出
- **8小節自動切替**: テンポに同期して8小節（32ビート）ごとに次のシステムへ自動切替
- **手動オーバーライド**: 手動切替後は次の自動切替タイミングまで自動切替が一時停止

### MIDI反応
- **ノートオン**：音程に応じた色相、ベロシティに応じたサイズ
- **ノートオフ**：円が元のサイズに戻る
- **コントロールチェンジ**：CC1で基本サイズを調整