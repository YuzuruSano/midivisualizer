#include "ofApp.h"

void ofApp::setup(){
    ofSetVerticalSync(true);
    ofBackground(0);
    ofSetCircleResolution(64);
    
    // MIDIポートのリスト表示
    cout << "Available MIDI Input Ports:" << endl;
    midiInDrums.listInPorts();
    
    // ポート数の確認
    int numPorts = midiInDrums.getNumInPorts();
    cout << "Number of MIDI ports: " << numPorts << endl;
    
    if (numPorts > 0) {
        // 利用可能なポートを表示
        for (int i = 0; i < numPorts; i++) {
            string portName = midiInDrums.getInPortName(i);
            cout << "Port " << i << ": " << portName << endl;
        }
        
        cout << "=== DUAL MIDI SETUP ===" << endl;
        
        // カスタムリスナーの初期化
        drumListener = std::make_unique<DrumMidiListener>(this);
        push2Listener = std::make_unique<Push2MidiListener>(this);
        
        // IAC ドライバーの検出と接続
        for (int i = 0; i < numPorts; i++) {
            string portName = midiInDrums.getInPortName(i);
            if (portName.find("IAC") != string::npos || 
                portName.find("ドライバ") != string::npos) {
                midiInDrums.openPort(i);
                midiInDrums.ignoreTypes(false, false, false);
                midiInDrums.addListener(drumListener.get());
                drumMidiConnected = true;
                drumPortName = portName;
                cout << "✓ IAC Driver connected on port " << i << ": " << portName << endl;
                break;
            }
        }
        
        // Push2の検出と接続
        for (int i = 0; i < numPorts; i++) {
            string portName = midiInPush2.getInPortName(i);
            if (portName.find("Push 2 Live Port") != string::npos || 
                portName.find("Ableton Push 2 Live Port") != string::npos) {
                midiInPush2.openPort(i);
                midiInPush2.ignoreTypes(false, false, false);
                midiInPush2.addListener(push2Listener.get());
                push2MidiConnected = true;
                push2PortName = portName;
                cout << "✓ Push2 Live Port connected on port " << i << ": " << portName << endl;
                break;
            } else if (portName.find("Push 2") != string::npos || 
                      portName.find("Ableton Push 2") != string::npos) {
                midiInPush2.openPort(i);
                midiInPush2.ignoreTypes(false, false, false);
                midiInPush2.addListener(push2Listener.get());
                push2MidiConnected = true;
                push2PortName = portName;
                cout << "✓ Push2 connected on port " << i << ": " << portName << endl;
            }
        }
        
        cout << "=== CONNECTION SUMMARY ===" << endl;
        cout << "Drum MIDI (IAC): " << (drumMidiConnected ? "✓ CONNECTED" : "✗ NOT FOUND") << endl;
        cout << "Push2 Glitch: " << (push2MidiConnected ? "✓ CONNECTED" : "✗ NOT FOUND") << endl;
        
        if (drumMidiConnected && push2MidiConnected) {
            cout << ">>> DUAL MIDI MODE ACTIVE <<<" << endl;
            cout << "IAC = Drum triggers, Push2 = Glitch effects" << endl;
        } else if (drumMidiConnected) {
            cout << "Drum MIDI only - Connect Push2 for glitch effects" << endl;
        } else if (push2MidiConnected) {
            cout << "Push2 only - Connect IAC Driver for drum MIDI" << endl;
        } else {
            cout << "No compatible MIDI devices found" << endl;
        }
        cout << "=========================" << endl;
    } else {
        cout << "No MIDI input ports available!" << endl;
    }
    
    // ビジュアルシステムの初期化
    visualSystems.push_back(std::make_unique<ParticleSystem>());
    visualSystems.push_back(std::make_unique<FractalSystem>());
    visualSystems.push_back(std::make_unique<WaveSystem>());
    visualSystems.push_back(std::make_unique<FlowFieldSystem>());
    visualSystems.push_back(std::make_unique<LSystemSystem>());
    visualSystems.push_back(std::make_unique<PerlinFlowSystem>());
    visualSystems.push_back(std::make_unique<CurlNoiseSystem>());
    visualSystems.push_back(std::make_unique<InfiniteCorridorSystem>());
    visualSystems.push_back(std::make_unique<BuildingPerspectiveSystem>());
    visualSystems.push_back(std::make_unique<WaterRippleSystem>());
    visualSystems.push_back(std::make_unique<SandParticleSystem>());
    
    for (auto& system : visualSystems) {
        system->setup();
    }
    
    // 最初のシステムをアクティブに
    if (!visualSystems.empty()) {
        visualSystems[currentSystemIndex]->setActive(true);
    }
    
    // グリッチシステムの初期化（高品質）
    glitchAreaSystem.setup(ofGetWidth(), ofGetHeight());
    glitchOutputFbo.allocate(ofGetWidth(), ofGetHeight(), GL_RGBA32F_ARB);
    
    // 再生順序の設定（カラーとモノクロを交互に）
    // 0: Particles (color), 7: Infinite Corridor (mono), 1: Fractals (color), 8: Building Perspective (mono), ...
    playbackOrder = {0, 7, 1, 8, 2, 9, 3, 10, 4, 5, 6};
    
    lastActivityTime = ofGetElapsedTimef();
}

void ofApp::update(){
    float deltaTime = ofGetLastFrameTime();
    float currentTime = ofGetElapsedTimef();
    
    // トランジションの更新
    if (isTransitioning) {
        updateTransition(deltaTime);
    }
    
    // テンポ同期の自動切替チェック
    if (autoSwitchEnabled && !manualTempoOverride) {
        handleAutoSwitch();
    }
    
    // アクティブなシステムを更新（トランジション中は両方）
    for (auto& system : visualSystems) {
        if (system->getActive() || 
            (isTransitioning && (&system == &visualSystems[nextSystemIndex]))) {
            system->update(deltaTime);
        }
    }
    
    // グリッチシステムの更新
    glitchAreaSystem.update(deltaTime);
    
    // UIのフェードアウト
    float timeSinceActivity = ofGetElapsedTimef() - lastActivityTime;
    if (timeSinceActivity > 3.0f) {
        uiFadeAlpha = ofLerp(uiFadeAlpha, 0, deltaTime * 2.0f);
    } else {
        uiFadeAlpha = ofLerp(uiFadeAlpha, 255, deltaTime * 4.0f);
    }
}

void ofApp::draw(){
    // 一時FBOに通常の描画を行う
    glitchOutputFbo.begin();
    ofClear(0, 0, 0, 255);
    
    if (isTransitioning) {
        // トランジション描画（クロスフェード）
        drawTransition();
    } else {
        // アクティブなビジュアルシステムを描画
        for (auto& system : visualSystems) {
            if (system->getActive()) {
                system->draw();
                break; // 一度に一つだけ描画
            }
        }
    }
    glitchOutputFbo.end();
    
    // グリッチエフェクトを適用（モノクロモードでは無効）
    if (glitchAreaSystem.hasActiveGlitch() && !isMonochromePattern) {
        ofFbo tempFbo;
        tempFbo.allocate(ofGetWidth(), ofGetHeight(), GL_RGBA32F_ARB);
        glitchAreaSystem.applyGlitch(glitchOutputFbo, tempFbo);
        tempFbo.draw(0, 0);
    } else {
        glitchOutputFbo.draw(0, 0);
    }
    
    // UIを描画（フェードアウト付き）
    if (showUI && uiFadeAlpha > 10) {
        drawUI();
    }
}

void ofApp::drawUI(){
    ofPushStyle();
    ofEnableBlendMode(OF_BLENDMODE_ALPHA);
    
    // 背景
    ofSetColor(0, 0, 0, 150 * (uiFadeAlpha / 255.0f));
    ofDrawRectangle(10, 10, 400, 250);
    
    ofSetColor(255, uiFadeAlpha);
    
    int y = 30;
    ofDrawBitmapString("MIDI Generative Art Visualizer", 20, y);
    y += 20;
    
    string systemNames[] = {"Particles", "Fractals", "Waves", "Flow Field", "L-System", "Perlin Flow", "Curl Noise", "Infinite Corridor", "Building Perspective", "Water Ripple", "Sand Particle"};
    string modeStr = isMonochromePattern ? " [MONO]" : " [COLOR]";
    ofDrawBitmapString("System [" + ofToString(currentSystemIndex + 1) + "/" + ofToString(visualSystems.size()) + "]: " + systemNames[currentSystemIndex] + modeStr, 20, y);
    y += 15;
    
    // 複数MIDI接続状況を表示
    string drumStatus = drumMidiConnected ? ("✓ " + drumPortName) : "✗ Not connected";
    string push2Status = push2MidiConnected ? ("✓ " + push2PortName) : "✗ Not connected";
    ofDrawBitmapString("Drum MIDI: " + drumStatus, 20, y);
    y += 15;
    ofDrawBitmapString("Push2 MIDI: " + push2Status, 20, y);
    y += 15;
    
    ofDrawBitmapString("Note: " + ofToString(currentNote) + " Velocity: " + ofToString(currentVelocity), 20, y);
    y += 15;
    
    ofDrawBitmapString("Intensity: " + ofToString(intensity, 2), 20, y);
    y += 15;
    
    ofDrawBitmapString("Keys: Space=Next, 1-9,0,-=Direct System, H=UI, G=Glitch, P=MIDI Status", 20, y);
    y += 15;
    
    // テンポ情報の表示
    ofDrawBitmapString("BPM: " + ofToString(bpm, 1) + " | Beat: " + ofToString(beatCount), 20, y);
    y += 15;
    
    // 自動切替情報
    string autoStatus = autoSwitchEnabled ? "ON" : "OFF";
    if (manualTempoOverride) autoStatus += " (Manual Override)";
    ofDrawBitmapString("Auto Switch: " + autoStatus + " | " + ofToString(barsPerSwitch) + " bars", 20, y);
    y += 15;
    
    // パターンモード情報
    ofDrawBitmapString("Playback Order [" + ofToString(playbackIndex + 1) + "/" + ofToString(playbackOrder.size()) + "] - Mode: " + string(isMonochromePattern ? "MONOCHROME" : "COLOR"), 20, y);
    y += 15;
    
    // トランジション情報
    if (isTransitioning) {
        string systemNames[] = {"Particles", "Fractals", "Waves", "Flow Field", "L-System", "Perlin Flow", "Curl Noise", "Infinite Corridor", "Building Perspective", "Water Ripple", "Sand Particle"};
        ofDrawBitmapString("Transitioning to: " + systemNames[nextSystemIndex] + " (" + ofToString(transitionProgress * 100, 1) + "%)", 20, y);
        y += 15;
    }
    
    // グリッチシステム情報
    if (!push2MidiConnected) {
        ofSetColor(150, 150, 150, uiFadeAlpha);
        ofDrawBitmapString("Glitch: Disabled (Push2 not connected)", 20, y);
        ofSetColor(255, uiFadeAlpha);
    } else if (isMonochromePattern) {
        ofSetColor(150, 150, 150, uiFadeAlpha);
        ofDrawBitmapString("Glitch: Disabled (Monochrome Mode)", 20, y);
        ofSetColor(255, uiFadeAlpha);
    } else if (glitchAreaSystem.hasActiveGlitch()) {
        ofSetColor(255, 100, 100, uiFadeAlpha);
        ofDrawBitmapString("GLITCH ACTIVE: " + ofToString(glitchAreaSystem.getActiveAreaCount()) + " areas", 20, y);
        ofSetColor(255, uiFadeAlpha);
    } else {
        ofSetColor(100, 255, 100, uiFadeAlpha);
        ofDrawBitmapString("Glitch: Ready (Push2 pads 36-99)", 20, y);
        ofSetColor(255, uiFadeAlpha);
    }
    y += 15;
    
    ofDrawBitmapString("Channels 1-3: Switch Systems", 20, y);
    y += 20;
    
    // MIDIメッセージの表示（最新のみ）
    if (!midiMessages.empty()) {
        ofxMidiMessage &message = midiMessages.back();
        string text = "Last MIDI: ";
        if(message.status == MIDI_NOTE_ON){
            text += "Note ON ";
        }else if(message.status == MIDI_NOTE_OFF){
            text += "Note OFF ";
        }else if(message.status == MIDI_CONTROL_CHANGE){
            text += "CC ";
        }
        text += ofToString(message.pitch) + " vel:" + ofToString(message.velocity);
        text += " ch:" + ofToString(message.channel);
        
        ofDrawBitmapString(text, 20, y);
    }
    
    ofDisableBlendMode();
    ofPopStyle();
}

void ofApp::exit(){
    if (drumMidiConnected) {
        midiInDrums.closePort();
        midiInDrums.removeListener(drumListener.get());
    }
    if (push2MidiConnected) {
        midiInPush2.closePort();
        midiInPush2.removeListener(push2Listener.get());
    }
}

void ofApp::newMidiMessage(ofxMidiMessage& msg){
    // このメソッドは現在使用されていません（カスタムリスナー使用中）
    midiMessages.push_back(msg);
    while(midiMessages.size() > maxMessages){
        midiMessages.erase(midiMessages.begin());
    }
    lastActivityTime = ofGetElapsedTimef();
}

void ofApp::onDrumMidiMessage(ofxMidiMessage& msg) {
    cout << "=== DRUM MIDI ===" << endl;
    cout << "Pitch: " << msg.pitch << ", Velocity: " << msg.velocity << ", Port: " << drumPortName << endl;
    
    // MIDIメッセージ履歴に追加
    midiMessages.push_back(msg);
    while(midiMessages.size() > maxMessages){
        midiMessages.erase(midiMessages.begin());
    }
    lastActivityTime = ofGetElapsedTimef();
    
    // レガシー情報の更新
    if(msg.status == MIDI_NOTE_ON && msg.velocity > 0){
        currentNote = msg.pitch;
        currentVelocity = msg.velocity;
        intensity = ofMap(msg.velocity, 0, 127, 0.0f, 1.0f);
        
        // KICKでテンポトラッキング（ドラムの4つ打ちをビートとして認識）
        if (msg.pitch == 36 || msg.pitch == 35) {  // 一般的なKICKのNOTE番号
            cout << "KICK detected (pitch " << msg.pitch << ") - updating tempo tracking" << endl;
            updateTempoTracking(ofGetElapsedTimef());
        }
    }else if(msg.status == MIDI_NOTE_OFF || (msg.status == MIDI_NOTE_ON && msg.velocity == 0)){
        if(msg.pitch == currentNote){
            currentVelocity = 0;
            intensity *= 0.7f;
        }
    }
    
    // アクティブなシステムにMIDIメッセージを送信（トランジション中は両方に送信）
    for (auto& system : visualSystems) {
        if (system->getActive() || 
            (isTransitioning && (&system == &visualSystems[nextSystemIndex]))) {
            system->onMidiMessage(msg);
        }
    }
    cout << "=================" << endl;
}

void ofApp::onPush2MidiMessage(ofxMidiMessage& msg) {
    cout << "=== PUSH2 MIDI ===" << endl;
    cout << "Pitch: " << msg.pitch << ", Velocity: " << msg.velocity << ", Port: " << push2PortName << endl;
    
    // MIDIメッセージ履歴に追加（Push2メッセージも履歴に残す）
    midiMessages.push_back(msg);
    while(midiMessages.size() > maxMessages){
        midiMessages.erase(midiMessages.begin());
    }
    lastActivityTime = ofGetElapsedTimef();
    
    // Push2のパッド範囲からのグリッチトリガー
    if (msg.status == MIDI_NOTE_ON && msg.velocity > 0 && msg.pitch >= 36 && msg.pitch <= 99) {
        cout << ">>> PUSH2 PAD DETECTED <<<" << endl;
        
        // モノクロモードではグリッチを無効化
        if (isMonochromePattern) {
            cout << "GLITCH BLOCKED: Monochrome mode active (System " << (currentSystemIndex + 1) << ")" << endl;
        } else {
            // 厳格な安全チェック
            if (glitchSystemBusy) {
                cout << "GLITCH BLOCKED: System busy" << endl;
                return;
            }
            
            float currentTime = ofGetElapsedTimef();
            if (currentTime - lastGlitchTime >= glitchCooldown) {
                // 複数の安全性チェック
                int currentAreas = glitchAreaSystem.getActiveAreaCount();
                if (currentAreas >= 2) { // さらに厳格に：最大2個まで
                    cout << "GLITCH BLOCKED: Too many active areas (" << currentAreas << ")" << endl;
                    return;
                }
                
                // グリッチシステムをビジー状態にマーク
                glitchSystemBusy = true;
                
                try {
                    cout << ">>> GLITCH TRIGGERED! (Safe Mode) <<<" << endl;
                    // 軽量モード：1個のエリアのみ
                    glitchAreaSystem.triggerGlitch(1);
                    lastGlitchTime = currentTime;
                } catch (const std::exception& e) {
                    cout << "Error in glitch trigger: " << e.what() << endl;
                } catch (...) {
                    cout << "Unknown error in glitch trigger" << endl;
                }
                
                // ビジー状態を解除（0.5秒後）
                glitchSystemBusy = false;
            } else {
                cout << "GLITCH BLOCKED: Cooldown active (" << (glitchCooldown - (currentTime - lastGlitchTime)) << "s remaining)" << endl;
            }
        }
    }
    cout << "==================" << endl;
}

void ofApp::keyPressed(int key){
    lastActivityTime = ofGetElapsedTimef();
    
    cout << "Key pressed: " << key << " (char: '" << (char)key << "')" << endl;
    cout << "Checking conditions..." << endl;
    
    if (key == ' ' || key == 32) {
        // スペースキーで次のシステムに切り替え（再生順序に従う）
        cout << "Space key pressed - switching system" << endl;
        playbackIndex = (playbackIndex + 1) % playbackOrder.size();
        int nextSystem = playbackOrder[playbackIndex];
        switchToSystem(nextSystem);
        cout << "Switched to system: " << nextSystem << " (playback index: " << playbackIndex << ")" << endl;
    } else if (key == 'h' || key == 'H') {
        // UIの表示切り替え
        cout << "Toggle UI" << endl;
        showUI = !showUI;
    } else if (key >= '1' && key <= '9') {
        // 直接システム切り替え（優先）
        int systemIndex = key - '1';
        if (systemIndex < visualSystems.size()) {
            cout << "Direct system switch to: " << systemIndex << endl;
            // 直接選択時は再生位置も更新
            for (int i = 0; i < playbackOrder.size(); i++) {
                if (playbackOrder[i] == systemIndex) {
                    playbackIndex = i;
                    break;
                }
            }
            switchToSystem(systemIndex);
        }
    } else if (key == '0') {
        // 10番目のシステム（Water Ripple）へ直接切り替え
        if (9 < visualSystems.size()) {
            cout << "Direct system switch to: 9 (Water Ripple)" << endl;
            for (int i = 0; i < playbackOrder.size(); i++) {
                if (playbackOrder[i] == 9) {
                    playbackIndex = i;
                    break;
                }
            }
            switchToSystem(9);
        }
    } else if (key == '-' || key == '_') {
        // 11番目のシステム（Sand Particle）へ直接切り替え  
        if (10 < visualSystems.size()) {
            cout << "Direct system switch to: 10 (Sand Particle)" << endl;
            for (int i = 0; i < playbackOrder.size(); i++) {
                if (playbackOrder[i] == 10) {
                    playbackIndex = i;
                    break;
                }
            }
            switchToSystem(10);
        }
    } else if (key == 'p' || key == 'P') {
        // MIDI接続状況の表示
        cout << "=== MIDI CONNECTION STATUS ===" << endl;
        cout << "Drum MIDI (IAC): " << (drumMidiConnected ? "✓ CONNECTED" : "✗ NOT CONNECTED") << endl;
        if (drumMidiConnected) {
            cout << "  Port: " << drumPortName << endl;
        }
        cout << "Push2 Glitch: " << (push2MidiConnected ? "✓ CONNECTED" : "✗ NOT CONNECTED") << endl;
        if (push2MidiConnected) {
            cout << "  Port: " << push2PortName << endl;
        }
        
        if (drumMidiConnected && push2MidiConnected) {
            cout << ">>> DUAL MIDI MODE ACTIVE <<<" << endl;
            cout << "Both drum triggers and glitch effects available simultaneously" << endl;
        }
        cout << "==============================" << endl;
    } else if (key == 'g' || key == 'G') {
        // グリッチエフェクトのテストトリガー（モノクロモードでは無効）
        cout << "=== MANUAL GLITCH TEST (G KEY) ===" << endl;
        cout << "Current System: " << (currentSystemIndex + 1) << endl;
        cout << "Monochrome Mode: " << (isMonochromePattern ? "YES" : "NO") << endl;
        
        if (isMonochromePattern) {
            cout << "GLITCH BLOCKED: Monochrome mode active" << endl;
            cout << "=================================" << endl;
            return;
        }
        
        // 厳格な安全チェック
        if (glitchSystemBusy) {
            cout << "GLITCH BLOCKED: System busy" << endl;
            cout << "=================================" << endl;
            return;
        }
        
        float currentTime = ofGetElapsedTimef();
        if (currentTime - lastGlitchTime >= glitchCooldown) {
            // 複数の安全性チェック
            int currentAreas = glitchAreaSystem.getActiveAreaCount();
            if (currentAreas >= 2) { // さらに厳格に：最大2個まで
                cout << "GLITCH BLOCKED: Too many active areas (" << currentAreas << ")" << endl;
                cout << "=================================" << endl;
                return;
            }
            
            // グリッチシステムをビジー状態にマーク
            glitchSystemBusy = true;
            
            try {
                cout << ">>> MANUAL GLITCH TRIGGERED! (Safe Mode) <<<" << endl;
                // 軽量モード：1個のエリアのみ
                glitchAreaSystem.triggerGlitch(1);
                lastGlitchTime = currentTime;
                cout << "Glitch areas created: " << glitchAreaSystem.getActiveAreaCount() << endl;
            } catch (const std::exception& e) {
                cout << "Error in manual glitch trigger: " << e.what() << endl;
            } catch (...) {
                cout << "Unknown error in manual glitch trigger" << endl;
            }
            
            // ビジー状態を解除
            glitchSystemBusy = false;
        } else {
            cout << "GLITCH BLOCKED: Cooldown active (" << (glitchCooldown - (currentTime - lastGlitchTime)) << "s remaining)" << endl;
        }
        cout << "=================================" << endl;
    }
}

void ofApp::switchToSystem(int systemIndex) {
    cout << "switchToSystem called with index: " << systemIndex << endl;
    cout << "Current system index: " << currentSystemIndex << endl;
    cout << "Total systems: " << visualSystems.size() << endl;
    
    if (systemIndex >= 0 && systemIndex < visualSystems.size() && systemIndex != currentSystemIndex) {
        // システム番号に基づいてモノクロ/カラーを設定
        // 7以上（Infinite Corridor以降）はモノクロ
        isMonochromePattern = (systemIndex >= 7);
        VisualSystem::setGlobalMonochromeMode(isMonochromePattern);
        
        cout << "System: " << systemIndex << " - Mode: " << (isMonochromePattern ? "MONOCHROME" : "COLOR") << endl;
        
        if (isTransitioning) {
            // 既にトランジション中の場合は即座切替
            visualSystems[currentSystemIndex]->setActive(false);
            visualSystems[nextSystemIndex]->setActive(false);
            currentSystemIndex = systemIndex;
            visualSystems[currentSystemIndex]->setActive(true);
            isTransitioning = false;
            manualTempoOverride = true;  // 手動切替時は自動を一時停止
        } else {
            // トランジションを開始
            startTransition(systemIndex);
            manualTempoOverride = true;  // 手動切替時は自動を一時停止
        }
        cout << "Started transition to system: " << systemIndex << endl;
    } else if (systemIndex == currentSystemIndex) {
        cout << "Already on system: " << systemIndex << endl;
    } else {
        cout << "Invalid system index: " << systemIndex << endl;
    }
}

void ofApp::startTransition(int targetSystemIndex) {
    if (targetSystemIndex >= 0 && targetSystemIndex < visualSystems.size() && targetSystemIndex != currentSystemIndex) {
        nextSystemIndex = targetSystemIndex;
        isTransitioning = true;
        transitionStartTime = ofGetElapsedTimef();
        transitionProgress = 0.0f;
        
        // ターゲットシステムをアクティブにして更新を開始
        visualSystems[nextSystemIndex]->setActive(true);
    }
}

void ofApp::updateTransition(float deltaTime) {
    float currentTime = ofGetElapsedTimef();
    transitionProgress = (currentTime - transitionStartTime) / transitionDuration;
    
    if (transitionProgress >= 1.0f) {
        // トランジション完了
        visualSystems[currentSystemIndex]->setActive(false);
        currentSystemIndex = nextSystemIndex;
        isTransitioning = false;
        transitionProgress = 1.0f;
        cout << "Transition completed to system: " << currentSystemIndex << endl;
    }
}

void ofApp::drawTransition() {
    ofPushStyle();
    ofEnableBlendMode(OF_BLENDMODE_ALPHA);
    
    // スムーズなイージング（コサインカーブ）を適用
    float easedProgress = (1.0f - cos(transitionProgress * PI)) * 0.5f;
    
    // 現在のシステムをフェードアウト
    float currentAlpha = 1.0f - easedProgress;
    ofSetColor(255, 255, 255, currentAlpha * 255);
    visualSystems[currentSystemIndex]->draw();
    
    // 次のシステムをフェードイン
    float nextAlpha = easedProgress;
    ofSetColor(255, 255, 255, nextAlpha * 255);
    visualSystems[nextSystemIndex]->draw();
    
    ofDisableBlendMode();
    ofPopStyle();
}

void ofApp::updateTempoTracking(float currentTime) {
    // ビート間隔の記録（最新8ビート）
    if (lastBeatTime > 0) {
        float interval = currentTime - lastBeatTime;
        recentBeatIntervals.push_back(interval);
        if (recentBeatIntervals.size() > 8) {
            recentBeatIntervals.erase(recentBeatIntervals.begin());
        }
        calculateBPM();
    }
    lastBeatTime = currentTime;
    beatCount++;
}

void ofApp::calculateBPM() {
    if (recentBeatIntervals.size() >= 4) {
        float avgInterval = 0;
        for (float interval : recentBeatIntervals) {
            avgInterval += interval;
        }
        avgInterval /= recentBeatIntervals.size();
        
        // BPMを計算（一分間のビート数）
        if (avgInterval > 0.1f && avgInterval < 2.0f) {  // 合理的な範囲内
            bpm = 60.0f / avgInterval;
            bpm = ofClamp(bpm, 60.0f, 200.0f);  // BPM範囲制限
        }
    }
}

bool ofApp::shouldAutoSwitch() {
    if (!autoSwitchEnabled || manualTempoOverride || isTransitioning) {
        return false;
    }
    
    // 8小節（基本的に32ビート）をチェック
    int beatsPerBar = 4;  // 4/4拍子
    int totalBeats = barsPerSwitch * beatsPerBar;
    
    return (beatCount % totalBeats == 0) && (beatCount > 0);
}

void ofApp::handleAutoSwitch() {
    if (shouldAutoSwitch()) {
        // 再生順序に従って次のシステムに切り替え
        playbackIndex = (playbackIndex + 1) % playbackOrder.size();
        int nextSystem = playbackOrder[playbackIndex];
        cout << "Auto-switching to system: " << nextSystem << " (playback index: " << playbackIndex << ", Beat: " << beatCount << ", BPM: " << bpm << ")" << endl;
        startTransition(nextSystem);
        
        // 手動オーバーライドをリセット（次の自動切替を有効に）
        manualTempoOverride = false;
    }
}

void ofApp::keyReleased(int key){}
void ofApp::mouseMoved(int x, int y){}
void ofApp::mouseDragged(int x, int y, int button){}
void ofApp::mousePressed(int x, int y, int button){}
void ofApp::mouseReleased(int x, int y, int button){}
void ofApp::mouseEntered(int x, int y){}
void ofApp::mouseExited(int x, int y){}
void ofApp::windowResized(int w, int h){
    // グリッチシステムのFBOをリサイズ
    glitchAreaSystem.setup(w, h);
    glitchOutputFbo.allocate(w, h, GL_RGBA32F_ARB);
    
    // 各ビジュアルシステムのリサイズ処理（必要に応じて）
    for (auto& system : visualSystems) {
        // システムがリサイズ対応している場合はここで処理
    }
}
void ofApp::dragEvent(ofDragInfo dragInfo){}
void ofApp::gotMessage(ofMessage msg){}

// カスタムMIDIリスナーの実装
void DrumMidiListener::newMidiMessage(ofxMidiMessage& msg) {
    app->onDrumMidiMessage(msg);
}

void Push2MidiListener::newMidiMessage(ofxMidiMessage& msg) {
    app->onPush2MidiMessage(msg);
}