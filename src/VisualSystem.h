#pragma once

#include "ofMain.h"
#include "ofxMidi.h"

class VisualSystem {
public:
    VisualSystem() {}
    virtual ~VisualSystem() {}
    
    virtual void setup() = 0;
    virtual void update(float deltaTime) = 0;
    virtual void draw() = 0;
    
    virtual void onMidiMessage(ofxMidiMessage& msg) = 0;
    
    void setActive(bool active) { 
        isActive = active; 
        if (active && !isInitialized) {
            setupGlobalEffects();
            isInitialized = true;
        }
    }
    bool getActive() const { return isActive; }
    
protected:
    bool isActive = false;
    bool isInitialized = false;
    
    // 共通のMIDIパラメータ
    float intensity = 0.0f;
    float modulation = 0.0f;
    int currentNote = 0;
    int currentVelocity = 0;
    
    // ドラム用のインパクトパラメータ
    float impactIntensity = 0.0f;
    float impactDecay = 0.95f;
    
    // === 統一成長システム ===
    float globalGrowthLevel = 0.0f;        // 全体成長度 (0.0 - 1.0)
    float growthAcceleration = 0.0f;       // 成長加速度
    bool isCollapsing = false;             // 崩壊状態
    float decayTimer = 0.0f;               // 崩壊タイマー
    float collapseThreshold = 1.0f;        // 崩壊開始閾値
    float collapseDuration = 5.0f;         // 崩壊継続時間
    
    // === 画面全体エフェクト ===
    ofFbo masterBuffer;                    // メインレンダリングバッファ
    ofFbo trailBuffer;                     // 累積トレイルバッファ
    ofFbo distortionBuffer;                // 歪みエフェクトバッファ
    
    float screenShakeIntensity = 0.0f;     // 画面振動強度
    ofVec2f screenOffset;                  // 画面オフセット
    float distortionLevel = 0.0f;          // 歪みレベル
    float chromaticAberration = 0.0f;      // 色収差
    float bloomIntensity = 0.0f;           // ブルーム強度
    
    // === 色彩システム強化 ===
    float globalHueShift = 0.0f;           // 全体的な色相シフト
    float saturationBoost = 1.0f;          // 彩度ブースト
    float contrastLevel = 1.0f;            // コントラストレベル
    float vignette = 0.0f;                 // ビネット効果
    
    // === 時間経過 ===
    float systemTime = 0.0f;               // システム内時間
    float lastMidiTime = 0.0f;             // 最後のMIDI入力時間
    
    // === カラーモード管理 ===
    bool isMonochromeMode = false;         // モノクロモードフラグ
    static bool globalMonochromeMode;       // 全システム共通のモノクロモード
    
    // ドラムタイプの識別（General MIDI準拠）
    enum DrumType {
        KICK = 36,      // Bass Drum
        SNARE = 38,     // Snare Drum
        HIHAT_CLOSED = 42,
        HIHAT_OPEN = 46,
        CRASH = 49,
        RIDE = 51,
        TOM_HIGH = 48,
        TOM_MID = 47,
        TOM_LOW = 45
    };
    
    // === セットアップ関数 ===
    void setupGlobalEffects() {
        int width = ofGetWidth();
        int height = ofGetHeight();
        
        // FBOの初期化
        masterBuffer.allocate(width, height, GL_RGBA);
        trailBuffer.allocate(width, height, GL_RGBA);
        distortionBuffer.allocate(width, height, GL_RGBA);
        
        // バッファをクリア
        masterBuffer.begin();
        ofClear(0, 0);
        masterBuffer.end();
        
        trailBuffer.begin();
        ofClear(0, 0);
        trailBuffer.end();
        
        distortionBuffer.begin();
        ofClear(0, 0);
        distortionBuffer.end();
    }
    
    // === 更新関数 ===
    void updateGlobalEffects(float deltaTime) {
        systemTime += deltaTime;
        
        // 成長システムの更新
        updateGlobalGrowth(deltaTime);
        
        // エフェクトの更新
        updateScreenEffects(deltaTime);
        
        // インパクトの更新
        updateImpact(deltaTime);
        
        // 色彩システムの更新
        updateColorSystem(deltaTime);
    }
    
    void updateGlobalGrowth(float deltaTime) {
        if (!isCollapsing) {
            // 成長フェーズ
            float baseGrowthRate = 0.03f; // ベース成長率
            float acceleratedGrowth = baseGrowthRate * (1.0f + growthAcceleration);
            
            globalGrowthLevel += deltaTime * acceleratedGrowth;
            
            // 成長加速度の減衰
            growthAcceleration *= 0.98f;
            
            // 成長完了チェック
            if (globalGrowthLevel >= collapseThreshold) {
                triggerCollapse();
            }
        } else {
            // 崩壊フェーズ
            decayTimer += deltaTime;
            
            if (decayTimer >= collapseDuration) {
                resetGrowthSystem();
            }
        }
        
        // 成長レベルのクランプ
        globalGrowthLevel = ofClamp(globalGrowthLevel, 0.0f, 1.2f);
    }
    
    void updateScreenEffects(float deltaTime) {
        // 画面振動の更新
        if (screenShakeIntensity > 0.01f) {
            screenOffset.x = ofRandom(-screenShakeIntensity, screenShakeIntensity) * 10;
            screenOffset.y = ofRandom(-screenShakeIntensity, screenShakeIntensity) * 10;
            screenShakeIntensity *= 0.9f;
        } else {
            screenOffset *= 0.8f;
        }
        
        // 歪みエフェクトの更新
        distortionLevel *= 0.95f;
        chromaticAberration *= 0.98f;
        bloomIntensity *= 0.96f;
        
        // ビネット効果
        vignette = globalGrowthLevel * 0.3f + impactIntensity * 0.2f;
    }
    
    void updateColorSystem(float deltaTime) {
        // 全体的な色相シフト
        globalHueShift += deltaTime * 20.0f * (1.0f + modulation);
        if (globalHueShift > 360.0f) globalHueShift -= 360.0f;
        
        // 彩度とコントラストの動的調整
        saturationBoost = 1.0f + globalGrowthLevel * 0.5f + impactIntensity * 0.3f;
        contrastLevel = 1.0f + globalGrowthLevel * 0.4f + impactIntensity * 0.6f;
    }
    
    // === エフェクト描画 ===
    void beginMasterBuffer() {
        masterBuffer.begin();
        
        // 背景のクリア（完全ではなく、トレイル効果を残す）- ホワイトアウト防止で大幅に暗く
        ofEnableBlendMode(OF_BLENDMODE_MULTIPLY);
        ofColor fadeColor = urbanColor(currentNote, 0.1f);
        fadeColor.setBrightness(ofClamp(60 - globalGrowthLevel * 10, 15, 80)); // 非常に暗い背景に変更
        ofSetColor(fadeColor);
        ofDrawRectangle(0, 0, ofGetWidth(), ofGetHeight());
        ofDisableBlendMode();
    }
    
    void endMasterBuffer() {
        masterBuffer.end();
    }
    
    void drawFullscreenEffects() {
        // 累積トレイルの更新
        updateTrailBuffer();
        
        // メインバッファの描画（エフェクト付き）
        drawBufferWithEffects();
        
        // 追加の全画面エフェクト
        drawAdditionalEffects();
    }
    
    void updateTrailBuffer() {
        trailBuffer.begin();
        
        // 軽い減衰
        ofEnableBlendMode(OF_BLENDMODE_MULTIPLY);
        float fadeAmount = 254 - globalGrowthLevel * 10; // 成長に応じてトレイルを残す
        ofSetColor(fadeAmount);
        ofDrawRectangle(0, 0, ofGetWidth(), ofGetHeight());
        ofDisableBlendMode();
        
        // メインバッファの内容を追加
        ofEnableBlendMode(OF_BLENDMODE_ADD);
        ofSetColor(255, 100 + globalGrowthLevel * 100); // 成長に応じて強く
        masterBuffer.draw(0, 0);
        ofDisableBlendMode();
        
        trailBuffer.end();
    }
    
    void drawBufferWithEffects() {
        ofPushMatrix();
        
        // 画面振動の適用
        ofTranslate(screenOffset.x, screenOffset.y);
        
        // 歪み効果
        if (distortionLevel > 0.1f) {
            // 簡易的な歪み（拡大縮小）
            float scale = 1.0f + distortionLevel * 0.1f;
            ofTranslate(ofGetWidth() * 0.5f, ofGetHeight() * 0.5f);
            ofScale(scale, scale);
            ofTranslate(-ofGetWidth() * 0.5f, -ofGetHeight() * 0.5f);
        }
        
        // 色収差効果
        if (chromaticAberration > 0.1f) {
            float offset = chromaticAberration * 5;
            
            // 赤チャンネル
            ofEnableBlendMode(OF_BLENDMODE_ADD);
            ofSetColor(255, 0, 0, 180);
            masterBuffer.draw(-offset, 0);
            
            // 緑チャンネル
            ofSetColor(0, 255, 0, 180);
            masterBuffer.draw(0, 0);
            
            // 青チャンネル
            ofSetColor(0, 0, 255, 180);
            masterBuffer.draw(offset, 0);
            ofDisableBlendMode();
        } else {
            // 通常描画
            ofSetColor(255);
            masterBuffer.draw(0, 0);
        }
        
        // トレイルバッファの合成
        ofEnableBlendMode(OF_BLENDMODE_ADD);
        ofSetColor(255, 150 + globalGrowthLevel * 80);
        trailBuffer.draw(0, 0);
        ofDisableBlendMode();
        
        ofPopMatrix();
    }
    
    void drawAdditionalEffects() {
        // ビネット効果
        if (vignette > 0.1f) {
            ofEnableBlendMode(OF_BLENDMODE_MULTIPLY);
            
            ofMesh vignetteMesh;
            vignetteMesh.setMode(OF_PRIMITIVE_TRIANGLE_FAN);
            
            float centerX = ofGetWidth() * 0.5f;
            float centerY = ofGetHeight() * 0.5f;
            float maxRadius = sqrt(centerX * centerX + centerY * centerY);
            
            // 中心点
            vignetteMesh.addVertex(ofVec3f(centerX, centerY));
            vignetteMesh.addColor(ofColor(255));
            
            // 外周
            int numPoints = 32;
            for (int i = 0; i <= numPoints; i++) {
                float angle = (i / float(numPoints)) * TWO_PI;
                float x = centerX + cos(angle) * maxRadius;
                float y = centerY + sin(angle) * maxRadius;
                
                vignetteMesh.addVertex(ofVec3f(x, y));
                
                float darkness = 255 * (1.0f - vignette);
                vignetteMesh.addColor(ofColor(darkness));
            }
            
            vignetteMesh.draw();
            ofDisableBlendMode();
        }
        
        // グローバルな色彩調整
        if (saturationBoost > 1.1f || contrastLevel > 1.1f) {
            // これは簡易版 - 実際にはシェーダーで行うべき
            ofEnableBlendMode(OF_BLENDMODE_ADD);
            
            ofColor boostColor = accentColor(globalGrowthLevel);
            boostColor.a = (saturationBoost - 1.0f) * 30;
            
            ofSetColor(boostColor);
            ofDrawRectangle(0, 0, ofGetWidth(), ofGetHeight());
            
            ofDisableBlendMode();
        }
        
        // 成長インジケーター
        drawGrowthIndicator();
    }
    
    void drawGrowthIndicator() {
        if (globalGrowthLevel > 0.1f) {
            // 画面端のグロー効果
            ofEnableBlendMode(OF_BLENDMODE_ADD);
            
            ofColor glowColor = isCollapsing ? 
                ofColor(255, 100, 100) : // 崩壊時は赤
                accentColor(globalGrowthLevel); // 成長時はアクセント
            
            glowColor.a = globalGrowthLevel * 80;
            ofSetColor(glowColor);
            
            float glowThickness = globalGrowthLevel * 20;
            
            // 上下のグロー
            ofDrawRectangle(0, 0, ofGetWidth(), glowThickness);
            ofDrawRectangle(0, ofGetHeight() - glowThickness, ofGetWidth(), glowThickness);
            
            // 左右のグロー
            ofDrawRectangle(0, 0, glowThickness, ofGetHeight());
            ofDrawRectangle(ofGetWidth() - glowThickness, 0, glowThickness, ofGetHeight());
            
            ofDisableBlendMode();
        }
    }
    
    // === インパクトエフェクトの更新 ===
    void updateImpact(float deltaTime) {
        impactIntensity *= pow(impactDecay, deltaTime * 60.0f);
        if (impactIntensity < 0.01f) impactIntensity = 0.0f;
        
        // intensityも徐々に減衰
        intensity *= pow(0.98f, deltaTime * 60.0f);
    }
    
    // === ドラムヒット時のインパクト ===
    void triggerImpact(int note, int velocity) {
        impactIntensity = mapVelocity(velocity);
        intensity = impactIntensity;
        lastMidiTime = systemTime;
        
        // 成長システムへの影響
        growthAcceleration += impactIntensity * 0.5f;
        growthAcceleration = ofClamp(growthAcceleration, 0.0f, 3.0f);
        
        // 画面エフェクトの強化
        screenShakeIntensity += impactIntensity * 0.3f;
        distortionLevel += impactIntensity * 0.4f;
        chromaticAberration += impactIntensity * 0.2f;
        bloomIntensity += impactIntensity * 0.5f;
        
        // ドラムタイプに応じて減衰率を調整
        switch(note) {
            case KICK:
                impactDecay = 0.88f;  // キックは非常に長め
                screenShakeIntensity *= 1.5f; // 強い振動
                break;
            case SNARE:
                impactDecay = 0.92f;
                chromaticAberration *= 1.3f; // 強い色収差
                break;
            case HIHAT_CLOSED:
                impactDecay = 0.98f;  // ハイハットは短め
                break;
            case CRASH:
                impactDecay = 0.85f;  // クラッシュは最も長め
                triggerMassiveEffect(); // 特別エフェクト
                break;
            default:
                impactDecay = 0.95f;
        }
    }
    
    void triggerMassiveEffect() {
        // クラッシュ時の大規模エフェクト
        screenShakeIntensity = 1.0f;
        distortionLevel = 0.8f;
        chromaticAberration = 0.6f;
        bloomIntensity = 1.0f;
        
        // 成長の急激な促進
        growthAcceleration += 1.0f;
    }
    
    void triggerCollapse() {
        isCollapsing = true;
        decayTimer = 0.0f;
        
        // 崩壊エフェクト
        screenShakeIntensity = 0.8f;
        distortionLevel = 1.0f;
        chromaticAberration = 0.5f;
    }
    
    void resetGrowthSystem() {
        // システムリセット
        isCollapsing = false;
        globalGrowthLevel = 0.0f;
        decayTimer = 0.0f;
        growthAcceleration = 0.0f;
        
        // エフェクトリセット
        screenShakeIntensity = 0.0f;
        distortionLevel = 0.0f;
        chromaticAberration = 0.0f;
        bloomIntensity = 0.0f;
        
        // バッファクリア
        trailBuffer.begin();
        ofClear(0, 0);
        trailBuffer.end();
        
        masterBuffer.begin();
        ofClear(0, 0);
        masterBuffer.end();
    }
    
    // === ユーティリティ関数 ===
    float mapVelocity(int velocity) {
        return ofMap(velocity, 0, 127, 0.0f, 1.0f);
    }
    
    float mapCC(int value) {
        return ofMap(value, 0, 127, 0.0f, 1.0f);
    }
    
    ofColor noteToColor(int note) {
        float hue = ofMap(note % 12, 0, 12, 0, 255);
        float brightness = ofMap(note, 0, 127, 150, 255);
        ofColor color;
        color.setHsb(hue, 200, brightness);
        return color;
    }
    
    // === 強化された都市カラーシステム ===
    ofColor urbanColor(int note, float intensity = 1.0f) {
        if (globalMonochromeMode) {
            // モノクロモード: 純粋なグレースケール
            float base = ofMap(note % 12, 0, 12, 30, 120);
            float brightness = base + intensity * 60;
            brightness += globalGrowthLevel * 30;
            
            if (isCollapsing) {
                brightness *= 0.7f;
            }
            
            brightness = ofClamp(brightness, 0, 255);
            return ofColor(brightness, brightness, brightness);
        } else {
            // カラーモード: 既存の都市カラー
            float base = ofMap(note % 12, 0, 12, 15, 70);
            float brightness = base + intensity * 80;
            brightness += globalGrowthLevel * 40;
            
            if (isCollapsing) {
                brightness *= 0.7f;
            }
            
            ofColor color;
            color.r = brightness * 0.92f;
            color.g = brightness * 0.95f;
            color.b = brightness * 1.08f;
            
            color.setHue(color.getHue() + globalHueShift);
            color.setSaturation(color.getSaturation() * saturationBoost);
            
            return color;
        }
    }
    
    // 都市的アクセントカラー（大幅強化）
    ofColor accentColor(float intensity = 1.0f) {
        if (globalMonochromeMode) {
            // モノクロモード: 白〜グレーのアクセント
            float brightness = 150 + intensity * 105;
            brightness = ofClamp(brightness, 0, 255);
            return ofColor(brightness, brightness, brightness);
        } else {
            // カラーモード: 既存のアクセントカラー
            static std::vector<ofVec3f> urbanAccents = {
                ofVec3f(255, 85, 0),    // 交通コーン・オレンジ
                ofVec3f(0, 255, 255),   // 蛍光ブルー（地下鉄）
                ofVec3f(255, 255, 0),   // 工事現場・イエロー
                ofVec3f(255, 20, 147),  // ネオンピンク
                ofVec3f(50, 205, 50),   // 緊急出口・グリーン
                ofVec3f(255, 69, 0),    // 危険・レッド
                ofVec3f(138, 43, 226),  // 電気・パープル
                ofVec3f(255, 140, 0),   // 街灯・アンバー
            };
            
            static int lastIndex = 0;
            if (intensity > 0.7f || impactIntensity > 0.8f) {
                lastIndex = ofRandom(urbanAccents.size());
            }
            
            ofVec3f accent = urbanAccents[lastIndex];
            ofColor color;
            
            float boostFactor = 1.0f + globalGrowthLevel * 0.5f + impactIntensity * 0.8f;
            color.r = accent.x * intensity * boostFactor;
            color.g = accent.y * intensity * boostFactor;
            color.b = accent.z * intensity * boostFactor;
            
            color.setHue(color.getHue() + globalHueShift);
            color.setSaturation(color.getSaturation() * saturationBoost);
            
            return color;
        }
    }
    
    // 深度ベースの都市カラー（強化版）
    ofColor depthUrbanColor(int note, float depth, float intensity = 1.0f) {
        float baseTemp = ofMap(depth, 0, 1, 80, 15);
        float variance = ofMap(note % 12, 0, 12, -20, 20);
        float brightness = baseTemp + variance + intensity * 60;
        brightness += globalGrowthLevel * 30;
        
        if (globalMonochromeMode) {
            // モノクロモード: 深度による明度変化のみ
            brightness = ofClamp(brightness, 0, 255);
            return ofColor(brightness, brightness, brightness);
        } else {
            // カラーモード: 既存の深度カラー
            ofColor color;
            if (depth < 0.3f) {
                color.r = brightness * 1.3f;
                color.g = brightness * 0.7f;
                color.b = brightness * 0.4f;
            } else if (depth < 0.7f) {
                color.r = brightness * 0.8f;
                color.g = brightness;
                color.b = brightness * 1.1f;
            } else {
                color.r = brightness * 0.5f;
                color.g = brightness * 0.8f;
                color.b = brightness * 1.4f;
            }
            
            color.setHue(color.getHue() + globalHueShift);
            color.setSaturation(color.getSaturation() * saturationBoost);
            
            return color;
        }
    }
    
    // === 状態取得関数 ===
    float getGlobalGrowthLevel() const { return globalGrowthLevel; }
    bool getIsCollapsing() const { return isCollapsing; }
    float getSystemTime() const { return systemTime; }
    float getTimeSinceLastMidi() const { return systemTime - lastMidiTime; }
    
public:
    // カラーモード管理（publicに変更）
    static void setGlobalMonochromeMode(bool mono) { globalMonochromeMode = mono; }
    static bool getGlobalMonochromeMode() { return globalMonochromeMode; }
};