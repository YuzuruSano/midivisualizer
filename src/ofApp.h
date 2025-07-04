#pragma once

#include "ofMain.h"
#include <sstream>  // ofxMidiのコンパイルエラー対策
#include "ofxMidi.h"
#include "VisualSystem.h"
#include "ParticleSystem.h"
#include "FractalSystem.h"
#include "WaveSystem.h"
#include "FlowFieldSystem.h"
#include "LSystemSystem.h"
#include "PerlinFlowSystem.h"
#include "CurlNoiseSystem.h"
#include "InfiniteCorridorSystem.h"
#include "BuildingPerspectiveSystem.h"
#include "WaterRippleSystem.h"
#include "SandParticleSystem.h"
#include "GlitchAreaSystem.h"
#include <memory>

// 前方宣言
class ofApp;

// カスタムMIDIリスナー（ドラム用）
class DrumMidiListener : public ofxMidiListener {
    ofApp* app;
public:
    DrumMidiListener(ofApp* a) : app(a) {}
    void newMidiMessage(ofxMidiMessage& msg) override;
};

// カスタムMIDIリスナー（Push2用）
class Push2MidiListener : public ofxMidiListener {
    ofApp* app;
public:
    Push2MidiListener(ofApp* a) : app(a) {}
    void newMidiMessage(ofxMidiMessage& msg) override;
};

class ofApp : public ofBaseApp, public ofxMidiListener {
public:
    void setup();
    void update();
    void draw();
    void exit();
    
    void keyPressed(int key);
    void keyReleased(int key);
    void mouseMoved(int x, int y);
    void mouseDragged(int x, int y, int button);
    void mousePressed(int x, int y, int button);
    void mouseReleased(int x, int y, int button);
    void mouseEntered(int x, int y);
    void mouseExited(int x, int y);
    void windowResized(int w, int h);
    void dragEvent(ofDragInfo dragInfo);
    void gotMessage(ofMessage msg);
    
    void newMidiMessage(ofxMidiMessage& eventArgs);  // デフォルトMIDIコールバック
    void onDrumMidiMessage(ofxMidiMessage& msg);     // ドラムMIDI処理
    void onPush2MidiMessage(ofxMidiMessage& msg);    // Push2 MIDI処理
    void drawUI();
    void switchToSystem(int systemIndex);
    void startTransition(int targetSystemIndex);
    void updateTransition(float deltaTime);
    void drawTransition();
    void updateTempoTracking(float currentTime);
    void calculateBPM();
    bool shouldAutoSwitch();
    void handleAutoSwitch();
    
    // 複数MIDI入力（同時受信）
    ofxMidiIn midiInDrums;    // IAC ドライバー用（ドラムMIDI）
    ofxMidiIn midiInPush2;    // Push2用（グリッチトリガー）
    std::vector<ofxMidiMessage> midiMessages;
    std::size_t maxMessages = 10;
    
    // カスタムMIDIリスナー
    std::unique_ptr<DrumMidiListener> drumListener;
    std::unique_ptr<Push2MidiListener> push2Listener;
    
    // MIDI接続状況
    bool drumMidiConnected = false;
    bool push2MidiConnected = false;
    string drumPortName = "";
    string push2PortName = "";
    
    // ビジュアルシステム
    std::vector<std::unique_ptr<VisualSystem>> visualSystems;
    int currentSystemIndex = 0;
    
    // クロスフェード機能
    bool isTransitioning = false;
    int nextSystemIndex = 0;
    float transitionDuration = 4.0f;  // 4秒間のゆったりクロスフェード
    float transitionStartTime = 0.0f;
    float transitionProgress = 0.0f;
    
    // MIDIテンポ同期
    float bpm = 120.0f;  // デフォルトBPM
    float lastBeatTime = 0.0f;
    int beatCount = 0;
    bool autoSwitchEnabled = true;
    int barsPerSwitch = 8;  // 8小節ごとに切替
    float timingAccumulator = 0.0f;
    std::vector<float> recentBeatIntervals;
    bool manualTempoOverride = false;
    
    // レガシー用（デバッグ表示）
    int currentNote = 0;
    int currentVelocity = 0;
    float intensity = 0.0f;
    
    // UI
    bool showUI = true;
    float uiFadeAlpha = 255;
    float lastActivityTime = 0;
    
    // カラーモード管理
    bool isMonochromePattern = false;  // 現在のパターンがモノクロかどうか
    int patternCount = 0;              // パターンの通し番号
    
    // システムの再生順序マッピング
    std::vector<int> playbackOrder;     // 実際の再生順序
    int playbackIndex = 0;             // 現在の再生位置
    
    // グリッチシステム
    GlitchAreaSystem glitchAreaSystem;
    ofFbo glitchOutputFbo;
    float lastGlitchTime = 0.0f;
    float glitchCooldown = 2.0f;  // 2.0秒のクールダウンに拡大（安全性最優先）
    bool glitchSystemBusy = false;  // グリッチシステムのビジー状態
    
    // Push2 MIDI設定（グリッチトリガー用）
    const int PUSH2_NOTE_OFFSET = 36;  // Push2のノート開始位置
    const int PUSH2_GLITCH_TRIGGER_NOTE = 48;  // グリッチトリガー用のノート番号（例）
};