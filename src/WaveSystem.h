#pragma once

#include "VisualSystem.h"
#include <deque>

class WaveSystem : public VisualSystem {
private:
    // 波形データ
    std::deque<float> waveHistory;
    int maxHistorySize = 512;
    
    // 波のパラメータ
    float waveAmplitude = 100.0f;
    float waveSpeed = 1.0f;
    float phaseOffset = 0.0f;
    
    // 複数の波レイヤー
    struct WaveLayer {
        float amplitude;
        float frequency;
        float speed;
        float phase;
        ofColor color;
        float growthPhase;  // 成長位相
    };
    std::vector<WaveLayer> waveLayers;
    
    // メッシュ
    ofMesh waveMesh;
    
    // 累積波形システム
    std::vector<ofPolyline> waveTrails;
    // float trailDecay = 0.998f;  // Currently unused
    int maxTrails = 20;
    
    // 都市的波動効果
    float urbanPulse = 0.0f;
    float structuralResonance = 0.0f;
    
    // 相互作用するベクターフィールド
    struct VectorNode {
        ofVec2f position;
        ofVec2f velocity;
        float phase;
        float influence;
        float lifespan;
    };
    std::vector<VectorNode> vectorField;
    
    // 流体的背景帯のための相互作用ポイント
    struct FluidPoint {
        ofVec2f position;
        ofVec2f velocity;
        float phase;
        float influence;
        ofColor color;
    };
    std::vector<FluidPoint> fluidPoints;
    
public:
    void setup() override {
        // 初期波レイヤーの設定（密度削減）
        for (int i = 0; i < 3; i++) {
            WaveLayer layer;
            layer.amplitude = ofRandom(30, 120);
            layer.frequency = ofRandom(0.003f, 0.025f);
            layer.speed = ofRandom(0.3f, 2.5f);
            layer.phase = ofRandom(TWO_PI);
            layer.growthPhase = ofRandom(TWO_PI);
            layer.color = urbanColor(i * 18, 0.8f);
            waveLayers.push_back(layer);
        }
        
        waveMesh.setMode(OF_PRIMITIVE_LINE_STRIP);
        
        // 初期トレイルの設定
        waveTrails.resize(maxTrails);
        
        // ベクターフィールドの初期化
        vectorField.clear();
        for (int i = 0; i < 8; i++) {
            VectorNode node;
            node.position = ofVec2f(ofRandom(ofGetWidth()), ofRandom(ofGetHeight()));
            node.velocity = ofVec2f(ofRandom(-0.5f, 0.5f), ofRandom(-0.5f, 0.5f));
            node.phase = ofRandom(TWO_PI);
            node.influence = ofRandom(0.3f, 0.8f);
            node.lifespan = 1.0f;
            vectorField.push_back(node);
        }
        
        // 流体的背景ポイントの初期化
        fluidPoints.clear();
        for (int i = 0; i < 12; i++) {
            FluidPoint point;
            point.position = ofVec2f(ofGetWidth() * (i / 12.0f), ofGetHeight() * 0.5f + ofRandom(-100, 100));
            point.velocity = ofVec2f(ofRandom(-0.3f, 0.3f), ofRandom(-0.2f, 0.2f));
            point.phase = ofRandom(TWO_PI);
            point.influence = ofRandom(0.4f, 0.9f);
            point.color = urbanColor(i * 15, 0.6f);
            fluidPoints.push_back(point);
        }
    }
    
    void update(float deltaTime) override {
        // 統一エフェクトシステムの更新
        updateGlobalEffects(deltaTime);
        
        phaseOffset += deltaTime * waveSpeed * (1.0f + globalGrowthLevel);
        urbanPulse += deltaTime * 2.0f;
        
        // 構造的共鳴の更新
        structuralResonance = globalGrowthLevel * sin(systemTime * 3.0f) * 0.5f + 0.5f;
        
        // 波の履歴を更新（成長に応じて保持量増加）
        if (currentVelocity > 0) {
            waveHistory.push_back(mapVelocity(currentVelocity) * (1.0f + globalGrowthLevel));
            int currentMax = maxHistorySize + globalGrowthLevel * 256;
            if (waveHistory.size() > currentMax) {
                waveHistory.pop_front();
            }
        } else if (!waveHistory.empty()) {
            float decay = 0.95f - globalGrowthLevel * 0.05f; // 成長時は減衰を遅く
            waveHistory.push_back(waveHistory.back() * decay);
            if (waveHistory.size() > maxHistorySize) {
                waveHistory.pop_front();
            }
        }
        
        // 各レイヤーの更新
        for (auto& layer : waveLayers) {
            layer.phase += deltaTime * layer.speed * (1.0f + modulation + globalGrowthLevel * 0.5f);
            layer.growthPhase += deltaTime * (1.0f + globalGrowthLevel * 2.0f);
            
            // 成長による振幅の動的変化
            layer.amplitude *= (1.0f + globalGrowthLevel * 0.001f);
        }
        
        // 波の振幅を音の強さと成長で変調
        waveAmplitude = 50.0f + intensity * 200.0f + globalGrowthLevel * 150.0f;
        
        // トレイルの更新
        updateWaveTrails();
        
        // ベクターフィールドの更新
        updateVectorField(deltaTime);
        
        // 流体的背景ポイントの更新
        updateFluidPoints(deltaTime);
    }
    
    void draw() override {
        // マスターバッファに描画開始
        beginMasterBuffer();
        
        // 都市的な背景波形の描画
        drawUrbanWaveBackground();
        
        // メイン波形の描画
        drawMainWaves();
        
        // 累積トレイルの描画
        drawWaveTrails();
        
        // 相互作用ベクターフィールドの描画
        drawVectorField();
        
        // 成長による追加エフェクト
        if (globalGrowthLevel > 0.6f) {
            drawAdvancedWaveEffects();
        }
        
        endMasterBuffer();
        
        // 全画面エフェクトの描画
        drawFullscreenEffects();
    }
    
    void onMidiMessage(ofxMidiMessage& msg) override {
        if (msg.status == MIDI_NOTE_ON && msg.velocity > 0) {
            currentNote = msg.pitch;
            currentVelocity = msg.velocity;
            
            triggerImpact(msg.pitch, msg.velocity);
            
            // ドラムタイプに応じた波形効果
            switch(msg.pitch) {
                case KICK:
                    // 低周波の強い波
                    addWaveLayer(0.002f, impactIntensity * 3.0f, urbanColor(msg.pitch, 1.0f));
                    waveSpeed += impactIntensity;
                    break;
                    
                case SNARE:
                    // 中周波の鋭い波
                    addWaveLayer(0.008f, impactIntensity * 2.0f, accentColor(impactIntensity));
                    structuralResonance += impactIntensity * 0.5f;
                    addVectorNode(); // ベクターフィールドにノード追加
                    break;
                    
                case HIHAT_CLOSED:
                    // 高周波の細かい波
                    addWaveLayer(0.02f, impactIntensity * 1.5f, urbanColor(msg.pitch, 0.8f));
                    break;
                    
                case CRASH:
                    // 全周波数帯域の爆発的波
                    triggerWaveExplosion();
                    break;
                    
                default:
                    // 音程に基づく波形
                    float noteFreq = ofMap(msg.pitch, 0, 127, 0.001f, 0.05f);
                    addWaveLayer(noteFreq, impactIntensity, urbanColor(msg.pitch, impactIntensity));
            }
            
            // トレイルの追加
            addWaveTrail();
            
        } else if (msg.status == MIDI_NOTE_OFF) {
            if (msg.pitch == currentNote) {
                currentVelocity = 0;
                intensity *= 0.8f;
            }
        } else if (msg.status == MIDI_CONTROL_CHANGE) {
            if (msg.control == 1) { // Mod wheel
                modulation = mapCC(msg.value);
                waveSpeed = 0.5f + modulation * 3.0f;
            } else if (msg.control == 7) { // Volume
                float vol = mapCC(msg.value);
                for (auto& layer : waveLayers) {
                    layer.amplitude = ofRandom(30, 120) * vol * (1.0f + globalGrowthLevel);
                }
            }
        }
    }
    
private:
    void drawUrbanWaveBackground() {
        // 流体的相互作用背景帯
        ofEnableBlendMode(OF_BLENDMODE_ADD);
        
        float width = ofGetWidth();
        float height = ofGetHeight();
        
        // 流体ポイント間の接続による帯の形成
        for (int i = 0; i < fluidPoints.size() - 1; i++) {
            auto& pointA = fluidPoints[i];
            auto& pointB = fluidPoints[i + 1];
            
            // 流体的な帯状メッシュの生成
            ofMesh fluidBand;
            fluidBand.setMode(OF_PRIMITIVE_TRIANGLE_STRIP);
            
            int resolution = 20; // 解像度
            for (int j = 0; j <= resolution; j++) {
                float t = j / float(resolution);
                
                // ベジエ曲線的な補間で滑らかな接続
                ofVec2f basePos = pointA.position.getInterpolated(pointB.position, t);
                
                // 相互作用による波状変形（ゆっくり）
                float waveOffset = 0;
                for (auto& other : fluidPoints) {
                    float distance = basePos.distance(other.position);
                    if (distance > 0 && distance < 200) {
                        float influence = sin(other.phase + systemTime * 0.3f) * other.influence;
                        waveOffset += influence * (200 - distance) / 200.0f * 15.0f;
                    }
                }
                
                // 波形からの影響も追加
                for (auto& layer : waveLayers) {
                    waveOffset += sin(basePos.x * layer.frequency + layer.phase) * layer.amplitude * 0.02f;
                }
                
                // 帯の上下端を計算
                float bandHeight = 40 + globalGrowthLevel * 30 + intensity * 20;
                ofVec2f topPos = basePos + ofVec2f(0, -bandHeight + waveOffset);
                ofVec2f bottomPos = basePos + ofVec2f(0, bandHeight + waveOffset);
                
                // メッシュに頂点追加
                fluidBand.addVertex(ofVec3f(topPos.x, topPos.y));
                fluidBand.addVertex(ofVec3f(bottomPos.x, bottomPos.y));
                
                // 色の補間（ホワイトアウト防止）
                ofColor topColor = pointA.color.getLerped(pointB.color, t);
                ofColor bottomColor = topColor;
                
                topColor.setBrightness(ofClamp(topColor.getBrightness() * (0.3f + globalGrowthLevel * 0.2f), 20, 100));
                bottomColor.setBrightness(ofClamp(bottomColor.getBrightness() * (0.5f + globalGrowthLevel * 0.3f), 30, 120));
                
                topColor.a = 80 * (0.6f + globalGrowthLevel * 0.3f);
                bottomColor.a = 120 * (0.6f + globalGrowthLevel * 0.3f);
                
                fluidBand.addColor(topColor);
                fluidBand.addColor(bottomColor);
            }
            
            fluidBand.draw();
        }
        
        // 流体ポイント自体の描画（小さな円）
        for (auto& point : fluidPoints) {
            ofColor pointColor = point.color;
            pointColor.a = 60 + sin(point.phase) * 40;
            pointColor.setBrightness(ofClamp(pointColor.getBrightness() * 0.7f, 20, 80));
            ofSetColor(pointColor);
            
            float size = 3 + point.influence * 2 + sin(point.phase) * 1;
            ofDrawCircle(point.position.x, point.position.y, size);
        }
        
        ofDisableBlendMode();
    }
    
    void drawMainWaves() {
        ofEnableBlendMode(OF_BLENDMODE_ADD);
        
        float width = ofGetWidth();
        float height = ofGetHeight();
        
        for (int layerIdx = 0; layerIdx < waveLayers.size(); layerIdx++) {
            auto& layer = waveLayers[layerIdx];
            
            waveMesh.clear();
            
            int numPoints = 80 + globalGrowthLevel * 40; // さらに簡略化
            for (int i = 0; i < numPoints; i++) {
                float x = ofMap(i, 0, numPoints - 1, 0, width);
                // ランダムなベースY位置（中央集中を防ぐ）
                float baseY = height * (0.3f + layerIdx * 0.15f + ofNoise(layerIdx * 0.5f) * 0.4f);
                
                // 複合波形の計算
                float y = baseY;
                
                // 基本波（成長により複雑化）
                float growthComplexity = 1.0f + globalGrowthLevel * 3.0f;
                float impactBoost = 1.0f + impactIntensity * (3.0f + globalGrowthLevel * 2.0f);
                
                y += sin(x * layer.frequency + layer.phase) * layer.amplitude * waveAmplitude / 100.0f * impactBoost * growthComplexity;
                y += sin(x * layer.frequency * 2.1f + layer.phase * 1.3f) * layer.amplitude * 0.4f * waveAmplitude / 100.0f * impactBoost;
                y += sin(x * layer.frequency * 4.3f + layer.phase * 2.1f) * layer.amplitude * 0.2f * waveAmplitude / 100.0f * impactBoost;
                
                // 成長による高次波の追加
                if (globalGrowthLevel > 0.3f) {
                    y += sin(x * layer.frequency * 8.7f + layer.growthPhase) * layer.amplitude * 0.1f * globalGrowthLevel * impactBoost;
                    y += sin(x * layer.frequency * 16.1f + layer.growthPhase * 2.1f) * layer.amplitude * 0.05f * globalGrowthLevel * impactBoost;
                }
                
                // 履歴データの影響（累積効果）
                if (!waveHistory.empty()) {
                    int histIdx = ofMap(i, 0, numPoints - 1, 0, waveHistory.size() - 1);
                    y += waveHistory[histIdx] * layer.amplitude * (0.5f + globalGrowthLevel * 0.3f);
                }
                
                // 構造的共鳴の影響
                y += sin(x * 0.01f + structuralResonance * TWO_PI) * 30.0f * globalGrowthLevel;
                
                // ノートに基づく変調（強化）
                float noteModulation = sin(x * 0.005f + currentNote * 0.1f + urbanPulse) * (20.0f + globalGrowthLevel * 40.0f);
                y += noteModulation * intensity;
                
                waveMesh.addVertex(ofVec3f(x, y, 0));
                
                // 色の設定（ホワイトアウト防止）
                ofColor color = layer.color;
                float brightness = ofMap(abs(y - baseY), 0, waveAmplitude * 3, 180, 60); // 最大輝度を下げる
                brightness *= (0.4f + 0.3f * intensity + globalGrowthLevel * 0.2f); // 全体的に暗く
                brightness = ofClamp(brightness, 20, 150); // 輝度範囲制限
                color.setBrightness(brightness);
                
                // 成長による色の強化（控えめに）
                if (globalGrowthLevel > 0.5f) {
                    color = accentColor(globalGrowthLevel);
                    color.setBrightness(ofClamp(brightness * 0.8f, 20, 120)); // さらに抑制
                }
                
                waveMesh.addColor(color);
            }
            
            // 波を描画（成長により太く）
            float lineWidth = 0.8f + layerIdx * 0.15f + globalGrowthLevel * 0.8f;
            ofSetLineWidth(lineWidth);
            waveMesh.draw();
            
            // 成長による反射効果の強化
            if (globalGrowthLevel > 0.2f) {
                ofPushMatrix();
                ofTranslate(0, height);
                ofScale(1, -0.3f - globalGrowthLevel * 0.2f);
                ofTranslate(0, -height);
                
                for (int i = 0; i < waveMesh.getNumColors(); i++) {
                    ofColor c = waveMesh.getColor(i);
                    c.a *= (0.3f + globalGrowthLevel * 0.2f); // より透明に
                    c.setBrightness(c.getBrightness() * 0.6f); // 反射を暗く
                    waveMesh.setColor(i, c);
                }
                waveMesh.draw();
                
                ofPopMatrix();
            }
        }
        
        ofDisableBlendMode();
    }
    
    void drawWaveTrails() {
        if (globalGrowthLevel > 0.1f) {
            ofEnableBlendMode(OF_BLENDMODE_ADD);
            
            for (int i = 0; i < waveTrails.size(); i++) {
                auto& trail = waveTrails[i];
                if (trail.size() > 1) {
                    ofColor trailColor = urbanColor(currentNote + i * 10, globalGrowthLevel * 0.7f);
                    trailColor.a = 150 * globalGrowthLevel;
                    ofSetColor(trailColor);
                    ofSetLineWidth(0.3f + globalGrowthLevel * 0.6f);
                    trail.draw();
                }
            }
            
            ofDisableBlendMode();
        }
    }
    
    void updateVectorField(float deltaTime) {
        for (auto& node : vectorField) {
            // フェーズ更新（ゆっくり）
            node.phase += deltaTime * 0.5f * (1.0f + globalGrowthLevel * 0.3f);
            
            // 相互作用力の計算
            ofVec2f totalForce(0, 0);
            for (auto& other : vectorField) {
                if (&node != &other) {
                    ofVec2f distance = other.position - node.position;
                    float dist = distance.length();
                    if (dist > 0 && dist < 150) {
                        // 近接時の引力/斥力
                        distance.normalize();
                        float strength = (dist < 80) ? -0.1f : 0.05f; // 近すぎると反発
                        totalForce += distance * strength * other.influence;
                    }
                }
            }
            
            // 波形からの影響
            float waveInfluence = 0;
            for (auto& layer : waveLayers) {
                waveInfluence += sin(node.position.x * layer.frequency + layer.phase) * 0.3f;
            }
            totalForce.y += waveInfluence * intensity;
            
            // 速度更新（慣性あり）
            node.velocity += totalForce * deltaTime;
            node.velocity *= 0.98f; // 抵抗
            node.velocity.limit(1.0f); // 速度制限
            
            // 位置更新
            node.position += node.velocity * deltaTime * 20.0f;
            
            // 境界での反射
            if (node.position.x < 0 || node.position.x > ofGetWidth()) {
                node.velocity.x *= -0.8f;
                node.position.x = ofClamp(node.position.x, 0, ofGetWidth());
            }
            if (node.position.y < 0 || node.position.y > ofGetHeight()) {
                node.velocity.y *= -0.8f;
                node.position.y = ofClamp(node.position.y, 0, ofGetHeight());
            }
            
            // 寿命管理
            node.lifespan = ofClamp(node.lifespan + deltaTime * 0.1f, 0.5f, 1.0f);
        }
    }
    
    void drawVectorField() {
        if (globalGrowthLevel > 0.1f) {
            ofEnableBlendMode(OF_BLENDMODE_ADD);
            
            // ノード間の接続線を描画
            for (int i = 0; i < vectorField.size(); i++) {
                auto& nodeA = vectorField[i];
                
                for (int j = i + 1; j < vectorField.size(); j++) {
                    auto& nodeB = vectorField[j];
                    
                    float distance = nodeA.position.distance(nodeB.position);
                    if (distance < 120) {
                        // 接続線の描画
                        float alpha = ofMap(distance, 0, 120, 80, 10) * nodeA.lifespan * nodeB.lifespan;
                        alpha *= globalGrowthLevel * 0.6f; // ホワイトアウト防止
                        
                        ofColor lineColor = urbanColor(i * 20, 0.7f);
                        lineColor.a = alpha;
                        ofSetColor(lineColor);
                        
                        ofSetLineWidth(0.5f + globalGrowthLevel * 0.3f);
                        
                        // 波状の接続線
                        ofBeginShape();
                        ofNoFill();
                        
                        int segments = 8;
                        for (int s = 0; s <= segments; s++) {
                            float t = s / float(segments);
                            ofVec2f pos = nodeA.position.getInterpolated(nodeB.position, t);
                            
                            // 波状の変形
                            float waveOffset = sin(t * PI * 2 + nodeA.phase) * 15 * nodeA.influence;
                            pos.y += waveOffset;
                            
                            ofVertex(pos.x, pos.y);
                        }
                        ofEndShape();
                    }
                }
                
                // ノード自体の描画
                float nodeAlpha = 40 + nodeA.influence * 60;
                nodeAlpha *= globalGrowthLevel * 0.5f; // ホワイトアウト防止
                
                ofColor nodeColor = accentColor(nodeA.influence);
                nodeColor.a = nodeAlpha;
                nodeColor.setBrightness(ofClamp(nodeColor.getBrightness() * 0.7f, 30, 100)); // 輝度制限
                ofSetColor(nodeColor);
                
                float size = 2 + nodeA.influence * 4 + sin(nodeA.phase) * 2;
                ofDrawCircle(nodeA.position.x, nodeA.position.y, size);
                
                // ベクター方向の表示
                ofVec2f direction = nodeA.velocity.getNormalized() * 20;
                ofDrawLine(nodeA.position.x, nodeA.position.y, 
                          nodeA.position.x + direction.x, nodeA.position.y + direction.y);
            }
            
            ofDisableBlendMode();
        }
    }
    
    void drawAdvancedWaveEffects() {
        ofEnableBlendMode(OF_BLENDMODE_ADD);
        
        // 高成長時の粒子効果（密度削減）
        int numParticles = 8 + globalGrowthLevel * 12;
        for (int i = 0; i < numParticles; i++) {
            float x = ofRandom(ofGetWidth());
            // ランダムな散在配置（中央集中を避ける）
            float baseY = ofRandom(ofGetHeight() * 0.2f, ofGetHeight() * 0.8f);
            
            // 波形に沿った粒子
            float waveY = 0;
            for (auto& layer : waveLayers) {
                waveY += sin(x * layer.frequency + layer.phase) * layer.amplitude * waveAmplitude / 300.0f; // 振幅を縮小
            }
            
            float y = baseY + waveY + ofRandom(-30, 30);
            
            ofColor particleColor = accentColor(globalGrowthLevel);
            particleColor.a = ofRandom(30, 100) * globalGrowthLevel; // 透明度を下げる
            particleColor.setBrightness(ofClamp(particleColor.getBrightness() * 0.6f, 20, 80)); // 輝度制限
            ofSetColor(particleColor);
            
            float size = ofRandom(0.3, 1.2) * globalGrowthLevel; // サイズを小さく
            ofDrawCircle(x, y, size);
        }
        
        ofDisableBlendMode();
    }
    
    void updateWaveTrails() {
        // トレイルの自然減衰
        for (auto& trail : waveTrails) {
            if (trail.size() > 1) {
                // 末尾の点を少しずつ削除
                if (ofRandom(1.0f) < 0.02f + globalGrowthLevel * 0.01f) {
                    if (trail.size() > 0) {
                        trail.getVertices().pop_back();
                    }
                }
            }
        }
    }
    
    void addWaveLayer(float frequency, float amplitude, ofColor color) {
        if (waveLayers.size() < 15) { // 最大レイヤー数制限
            WaveLayer newLayer;
            newLayer.amplitude = amplitude * 80;
            newLayer.frequency = frequency;
            newLayer.speed = ofRandom(0.5f, 2.0f);
            newLayer.phase = ofRandom(TWO_PI);
            newLayer.growthPhase = 0;
            newLayer.color = color;
            waveLayers.push_back(newLayer);
        }
    }
    
    void addWaveTrail() {
        // 新しいトレイルを追加
        int trailIndex = ofRandom(waveTrails.size());
        auto& trail = waveTrails[trailIndex];
        
        trail.clear();
        
        // ランダムなベース位置（中央帯を避ける）
        float baseY = ofRandom(ofGetHeight() * 0.2f, ofGetHeight() * 0.8f);
        int numPoints = 15 + globalGrowthLevel * 25;  // 点数を削減
        
        for (int i = 0; i < numPoints; i++) {
            float x = ofMap(i, 0, numPoints - 1, 0, ofGetWidth());
            float y = baseY;
            
            // 現在の波形に基づいてトレイルを生成
            for (auto& layer : waveLayers) {
                y += sin(x * layer.frequency + layer.phase) * layer.amplitude * 0.3f;
            }
            
            trail.addVertex(x, y);
        }
    }
    
    void triggerWaveExplosion() {
        // クラッシュ時の波形爆発
        for (auto& layer : waveLayers) {
            layer.amplitude *= 1.5f;
            layer.speed += ofRandom(0.5f, 1.5f);
            layer.color = accentColor(1.0f);
        }
        
        // 複数のトレイルを同時生成
        for (int i = 0; i < 5; i++) {
            addWaveTrail();
        }
        
        waveSpeed *= 1.8f;
        structuralResonance = 1.0f;
    }
    
    void updateFluidPoints(float deltaTime) {
        for (auto& point : fluidPoints) {
            // フェーズ更新（ゆっくり）
            point.phase += deltaTime * 0.4f * (1.0f + globalGrowthLevel * 0.2f);
            
            // 相互作用力の計算
            ofVec2f totalForce(0, 0);
            for (auto& other : fluidPoints) {
                if (&point != &other) {
                    ofVec2f distance = other.position - point.position;
                    float dist = distance.length();
                    if (dist > 0 && dist < 120) {
                        distance.normalize();
                        // 近すぎると反発、適度な距離で引力
                        float strength = (dist < 60) ? -0.08f : 0.03f;
                        totalForce += distance * strength * other.influence;
                    }
                }
            }
            
            // 画面中央への復帰力（帯状を維持）
            float centerY = ofGetHeight() * 0.5f;
            float restoreForce = (centerY - point.position.y) * 0.002f;
            totalForce.y += restoreForce;
            
            // 波形からの影響
            float waveInfluence = 0;
            for (auto& layer : waveLayers) {
                waveInfluence += sin(point.position.x * layer.frequency + layer.phase) * 0.1f;
            }
            totalForce.y += waveInfluence * intensity;
            
            // 速度更新（慣性あり、ゆっくり）
            point.velocity += totalForce * deltaTime;
            point.velocity *= 0.99f; // 高い抵抗でゆっくりした動き
            point.velocity.limit(0.5f); // 低い速度制限
            
            // 位置更新
            point.position += point.velocity * deltaTime * 15.0f; // ゆっくりした移動
            
            // 水平方向の境界処理（循環）
            if (point.position.x < -50) {
                point.position.x = ofGetWidth() + 50;
            } else if (point.position.x > ofGetWidth() + 50) {
                point.position.x = -50;
            }
            
            // 垂直方向の境界処理（反射）
            if (point.position.y < ofGetHeight() * 0.2f || point.position.y > ofGetHeight() * 0.8f) {
                point.velocity.y *= -0.7f;
                point.position.y = ofClamp(point.position.y, ofGetHeight() * 0.2f, ofGetHeight() * 0.8f);
            }
            
            // 色の更新
            point.color = urbanColor(point.position.x * 0.1f, point.influence);
        }
    }
    
    void addVectorNode() {
        if (vectorField.size() < 12) { // 最大数制限
            VectorNode newNode;
            newNode.position = ofVec2f(ofRandom(ofGetWidth()), ofRandom(ofGetHeight()));
            newNode.velocity = ofVec2f(ofRandom(-1, 1), ofRandom(-1, 1));
            newNode.phase = ofRandom(TWO_PI);
            newNode.influence = ofRandom(0.4f, 0.9f);
            newNode.lifespan = 1.0f;
            vectorField.push_back(newNode);
        }
    }
};