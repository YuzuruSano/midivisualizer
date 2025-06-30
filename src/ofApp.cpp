#include "ofApp.h"

void ofApp::setup(){
    ofSetVerticalSync(true);
    ofBackground(0);
    ofSetCircleResolution(64);
    
    midiIn.listInPorts();
    
    midiIn.openPort(0);
    
    midiIn.ignoreTypes(false, false, false);
    
    midiIn.addListener(this);
    
    // ビジュアルシステムの初期化
    visualSystems.push_back(std::make_unique<ParticleSystem>());
    visualSystems.push_back(std::make_unique<FractalSystem>());
    visualSystems.push_back(std::make_unique<WaveSystem>());
    visualSystems.push_back(std::make_unique<FlowFieldSystem>());
    visualSystems.push_back(std::make_unique<LSystemSystem>());
    visualSystems.push_back(std::make_unique<DifferentialGrowthSystem>());
    visualSystems.push_back(std::make_unique<ReactionDiffusionSystem>());
    
    for (auto& system : visualSystems) {
        system->setup();
    }
    
    // 最初のシステムをアクティブに
    if (!visualSystems.empty()) {
        visualSystems[currentSystemIndex]->setActive(true);
    }
    
    // グリッチシステムの初期化
    glitchAreaSystem.setup(ofGetWidth(), ofGetHeight());
    glitchOutputFbo.allocate(ofGetWidth(), ofGetHeight(), GL_RGBA32F_ARB);
    
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
    
    // グリッチエフェクトを適用
    if (glitchAreaSystem.hasActiveGlitch()) {
        ofFbo tempFbo;
        tempFbo.allocate(ofGetWidth(), ofGetHeight(), GL_RGBA);
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
    
    string systemNames[] = {"Particles", "Fractals", "Waves", "Flow Field", "L-System", "Differential Growth", "Reaction-Diffusion"};
    ofDrawBitmapString("System [" + ofToString(currentSystemIndex + 1) + "/" + ofToString(visualSystems.size()) + "]: " + systemNames[currentSystemIndex], 20, y);
    y += 15;
    
    ofDrawBitmapString("Current Port: " + midiIn.getName(), 20, y);
    y += 15;
    
    ofDrawBitmapString("Note: " + ofToString(currentNote) + " Velocity: " + ofToString(currentVelocity), 20, y);
    y += 15;
    
    ofDrawBitmapString("Intensity: " + ofToString(intensity, 2), 20, y);
    y += 15;
    
    ofDrawBitmapString("Keys: Space=Next, 1-7=Direct System, H=UI, G=Glitch, 0,8-9=MIDI Port", 20, y);
    y += 15;
    
    // テンポ情報の表示
    ofDrawBitmapString("BPM: " + ofToString(bpm, 1) + " | Beat: " + ofToString(beatCount), 20, y);
    y += 15;
    
    // 自動切替情報
    string autoStatus = autoSwitchEnabled ? "ON" : "OFF";
    if (manualTempoOverride) autoStatus += " (Manual Override)";
    ofDrawBitmapString("Auto Switch: " + autoStatus + " | " + ofToString(barsPerSwitch) + " bars", 20, y);
    y += 15;
    
    // トランジション情報
    if (isTransitioning) {
        string systemNames[] = {"Particles", "Fractals", "Waves", "Flow Field", "L-System", "Differential Growth", "Reaction-Diffusion"};
        ofDrawBitmapString("Transitioning to: " + systemNames[nextSystemIndex] + " (" + ofToString(transitionProgress * 100, 1) + "%)", 20, y);
        y += 15;
    }
    
    // グリッチシステム情報
    if (glitchAreaSystem.hasActiveGlitch()) {
        ofSetColor(255, 100, 100, uiFadeAlpha);
        ofDrawBitmapString("GLITCH ACTIVE: " + ofToString(glitchAreaSystem.getActiveAreaCount()) + " areas", 20, y);
        ofSetColor(255, uiFadeAlpha);
    } else {
        ofDrawBitmapString("Glitch: Ready (Push2 pads 92-99)", 20, y);
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
    midiIn.closePort();
    midiIn.removeListener(this);
}

void ofApp::newMidiMessage(ofxMidiMessage& msg){
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
            updateTempoTracking(ofGetElapsedTimef());
        }
    }else if(msg.status == MIDI_NOTE_OFF || (msg.status == MIDI_NOTE_ON && msg.velocity == 0)){
        if(msg.pitch == currentNote){
            currentVelocity = 0;
            intensity *= 0.7f;
        }
    }
    
    // MIDIチャンネルによるシステム切り替え（無効化中）
    /*
    if (msg.status == MIDI_NOTE_ON && msg.velocity > 0) {
        int targetSystem = -1;
        if (msg.channel == 1) targetSystem = 0; // Particles
        else if (msg.channel == 2) targetSystem = 1; // Fractals
        else if (msg.channel == 3) targetSystem = 2; // Waves
        
        if (targetSystem != -1 && targetSystem != currentSystemIndex && targetSystem < visualSystems.size()) {
            cout << "MIDI channel switch to system: " << targetSystem << endl;
            switchToSystem(targetSystem);
        }
    }
    */
    
    // アクティブなシステムにMIDIメッセージを送信（トランジション中は両方に送信）
    for (auto& system : visualSystems) {
        if (system->getActive() || 
            (isTransitioning && (&system == &visualSystems[nextSystemIndex]))) {
            system->onMidiMessage(msg);
        }
    }
    
    // Push2からのグリッチトリガー
    if (msg.status == MIDI_NOTE_ON && msg.velocity > 0) {
        // Push2のパッド（36-99）の特定のノートでグリッチトリガー
        // 例: 上段のパッド（92-99）をグリッチトリガーに使用
        if (msg.pitch >= 92 && msg.pitch <= 99) {
            glitchAreaSystem.triggerGlitch(ofRandom(2, 4)); // 2-3個のランダムエリア
            cout << "Glitch triggered from Push2 pad: " << msg.pitch << endl;
        }
    }
}

void ofApp::keyPressed(int key){
    lastActivityTime = ofGetElapsedTimef();
    
    cout << "Key pressed: " << key << " (char: '" << (char)key << "')" << endl;
    cout << "Checking conditions..." << endl;
    
    if (key == ' ' || key == 32) {
        // スペースキーで次のシステムに切り替え
        cout << "Space key pressed - switching system" << endl;
        int nextSystem = (currentSystemIndex + 1) % visualSystems.size();
        switchToSystem(nextSystem);
        cout << "Switched to system: " << nextSystem << endl;
    } else if (key == 'h' || key == 'H') {
        // UIの表示切り替え
        cout << "Toggle UI" << endl;
        showUI = !showUI;
    } else if (key >= '1' && key <= '7') {
        // 直接システム切り替え（優先）
        int systemIndex = key - '1';
        if (systemIndex < visualSystems.size()) {
            cout << "Direct system switch to: " << systemIndex << endl;
            switchToSystem(systemIndex);
        }
    } else if (key >= '8' && key <= '9') {
        // MIDIポート切り替え（8-9のみ）
        int port = key - '0';
        cout << "Switching to MIDI port: " << port << endl;
        if(port < midiIn.getNumInPorts()){
            midiIn.closePort();
            midiIn.openPort(port);
            cout << "Successfully switched to port: " << port << endl;
        } else {
            cout << "Port " << port << " not available" << endl;
        }
    } else if (key == '0') {
        // ポート0への切り替え
        cout << "Switching to MIDI port: 0" << endl;
        if(midiIn.getNumInPorts() > 0){
            midiIn.closePort();
            midiIn.openPort(0);
            cout << "Successfully switched to port: 0" << endl;
        } else {
            cout << "No MIDI ports available" << endl;
        }
    } else if (key == 'g' || key == 'G') {
        // グリッチエフェクトのトリガー（テスト用）
        cout << "Triggering glitch effect" << endl;
        glitchAreaSystem.triggerGlitch(ofRandom(2, 4));
    }
}

void ofApp::switchToSystem(int systemIndex) {
    cout << "switchToSystem called with index: " << systemIndex << endl;
    cout << "Current system index: " << currentSystemIndex << endl;
    cout << "Total systems: " << visualSystems.size() << endl;
    
    if (systemIndex >= 0 && systemIndex < visualSystems.size() && systemIndex != currentSystemIndex) {
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
        int nextSystem = (currentSystemIndex + 1) % visualSystems.size();
        cout << "Auto-switching to system: " << nextSystem << " (Beat: " << beatCount << ", BPM: " << bpm << ")" << endl;
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
    glitchOutputFbo.allocate(w, h, GL_RGBA);
    
    // 各ビジュアルシステムのリサイズ処理（必要に応じて）
    for (auto& system : visualSystems) {
        // システムがリサイズ対応している場合はここで処理
    }
}
void ofApp::dragEvent(ofDragInfo dragInfo){}
void ofApp::gotMessage(ofMessage msg){}