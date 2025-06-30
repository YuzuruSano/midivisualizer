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
#include "DifferentialGrowthSystem.h"
#include "ReactionDiffusionSystem.h"
#include <memory>

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
    
    void newMidiMessage(ofxMidiMessage& eventArgs);
    void drawUI();
    void switchToSystem(int systemIndex);
    void startTransition(int targetSystemIndex);
    void updateTransition(float deltaTime);
    void drawTransition();
    void updateTempoTracking(float currentTime);
    void calculateBPM();
    bool shouldAutoSwitch();
    void handleAutoSwitch();
    
    ofxMidiIn midiIn;
    std::vector<ofxMidiMessage> midiMessages;
    std::size_t maxMessages = 10;
    
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
};