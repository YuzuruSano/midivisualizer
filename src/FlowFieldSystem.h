#pragma once

#include "VisualSystem.h"
#include <vector>

class FlowParticle {
public:
    ofVec2f position;
    ofVec2f velocity;
    ofVec2f previousPosition;
    ofColor color;
    float life;
    float maxLife;
    float size;
    bool active;
    float growthPhase;  // 成長位相
    
    FlowParticle() {
        reset();
    }
    
    void reset() {
        position = ofVec2f(ofRandom(ofGetWidth()), ofRandom(ofGetHeight()));
        velocity = ofVec2f(0, 0);
        previousPosition = position;
        life = ofRandom(100, 400);  // 長寿命化
        maxLife = life;
        size = ofRandom(0.4f, 1.5f);
        active = true;
        growthPhase = ofRandom(TWO_PI);
        
        // 都市的な初期色
        color = ofColor::fromHsb(ofRandom(200, 240), 120, 180);
    }
    
    void update(ofVec2f force, float deltaTime, float globalGrowth) {
        if (!active) return;
        
        previousPosition = position;
        
        // 成長による力の増幅
        ofVec2f enhancedForce = force * (1.0f + globalGrowth * 2.0f);
        
        // 力を速度に加算
        velocity += enhancedForce * deltaTime * (50.0f + globalGrowth * 30.0f);
        velocity *= (0.99f - globalGrowth * 0.01f); // 成長により持続性向上
        
        // 速度を制限（成長により高速化）
        float maxSpeed = 100.0f + globalGrowth * 80.0f;
        if (velocity.length() > maxSpeed) {
            velocity.normalize();
            velocity *= maxSpeed;
        }
        
        position += velocity * deltaTime;
        
        // 境界での処理（ラップアラウンド）
        if (position.x < 0) { position.x = ofGetWidth(); previousPosition.x = position.x; }
        if (position.x > ofGetWidth()) { position.x = 0; previousPosition.x = position.x; }
        if (position.y < 0) { position.y = ofGetHeight(); previousPosition.y = position.y; }
        if (position.y > ofGetHeight()) { position.y = 0; previousPosition.y = position.y; }
        
        // 成長位相の更新
        growthPhase += deltaTime * (1.0f + globalGrowth * 3.0f);
        
        // 寿命の減少（成長により長寿命化）
        life -= deltaTime * (10.0f - globalGrowth * 3.0f);
        if (life <= 0) {
            reset();
        }
    }
    
    void draw(float globalGrowth, float intensity) {
        if (!active) return;
        
        float alpha = ofMap(life, 0, maxLife, 0, 255) * (0.7f + globalGrowth * 0.3f);
        
        // 成長による色の強化
        ofColor drawColor = color;
        if (globalGrowth > 0.4f) {
            drawColor.setHue(color.getHue() + sin(growthPhase) * 30);
            drawColor.setSaturation(color.getSaturation() + globalGrowth * 80);
        }
        
        ofSetColor(drawColor, alpha);
        
        // 流体的な軌跡を描画
        float lineWidth = size * (0.3f + globalGrowth * 0.4f);  // より細い線
        ofSetLineWidth(lineWidth);
        ofDrawLine(previousPosition, position);
        
        // 流体エフェクト（白い塗りつぶしを除去）
        if (globalGrowth > 0.4f && intensity > 0.2f) {
            // 流体の渦巻きエフェクト
            ofColor flowColor;
            flowColor.setHsb(180 + intensity * 60, 100, 200);  // 青緑系の色
            flowColor.a = alpha * 0.3f;
            ofSetColor(flowColor);
            
            // 小さな流体点を描画
            ofDrawCircle(position, size * 0.3f);
        }
    }
};

class FlowFieldSystem : public VisualSystem {
private:
    std::vector<FlowParticle> particles;
    int baseParticleCount = 320;  // ベース粒子数（60%削減）
    
    // フローフィールド
    int cols, rows;
    float scale = 40.0f;  // グリッドスペーシングを増加
    std::vector<std::vector<ofVec2f>> field;
    
    // ノイズパラメータ
    float zOffset = 0.0f;
    float noiseScale = 0.01f;
    float timeSpeed = 0.002f;
    
    // 累積成長システム（統一）
    std::vector<ofVec2f> growthCenters; // 成長の中心点
    std::vector<float> centerIntensities; // 各中心の強度
    
    // 都市的な表現
    float concreteNoise = 0.0f;
    float infrastructureLevel = 0.0f;
    
    // 追加エフェクト
    float turbulence = 0.0f;
    float magneticField = 0.0f;
    
public:
    void setup() override {
        // パーティクルの初期化
        particles.resize(baseParticleCount);
        for (auto& p : particles) {
            p.reset();
        }
        
        // フローフィールドの初期化
        updateFieldDimensions();
        
        // 初期成長中心を設定
        growthCenters.push_back(ofVec2f(ofGetWidth() * 0.5f, ofGetHeight() * 0.5f));
        centerIntensities.push_back(1.0f);
        
        updateFlowField();
    }
    
    void update(float deltaTime) override {
        // 統一エフェクトシステムの更新
        updateGlobalEffects(deltaTime);
        
        zOffset += timeSpeed * (1.0f + modulation * 3.0f + globalGrowthLevel);
        concreteNoise += deltaTime * 0.1f;
        
        // インフラレベルの更新
        infrastructureLevel = globalGrowthLevel * (1.0f + sin(systemTime * 0.5f) * 0.2f);
        
        // 乱流の更新
        turbulence += deltaTime * 0.3f;
        magneticField = sin(systemTime * 1.2f + globalGrowthLevel * PI) * 0.5f + 0.5f;
        
        // 成長に応じてパーティクル数を動的調整（密度削減）
        int targetParticleCount = baseParticleCount + globalGrowthLevel * 160;
        if (particles.size() < targetParticleCount) {
            particles.resize(targetParticleCount);
            for (int i = baseParticleCount; i < targetParticleCount; i++) {
                particles[i].reset();
            }
        }
        
        // フローフィールドの更新
        updateFlowField();
        
        // パーティクルの更新
        updateParticles(deltaTime);
        
        // 成長中心の管理
        updateGrowthCenters(deltaTime);
        
        // 崩壊時の効果
        if (isCollapsing) {
            applyCollapseEffects();
        }
    }
    
    void draw() override {
        // マスターバッファに描画開始
        beginMasterBuffer();
        
        // インフラストラクチャの背景
        drawInfrastructureBackground();
        
        // フローフィールドの可視化（デバッグ用）
        if (globalGrowthLevel > 0.8f) {
            drawFlowField();
        }
        
        // パーティクルの描画
        drawParticles();
        
        // 成長中心の可視化
        drawGrowthCenters();
        
        // 都市的エフェクト
        if (globalGrowthLevel > 0.5f) {
            drawUrbanEffects();
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
            
            // 新しい成長中心を追加
            ofVec2f newCenter;
            float intensity = impactIntensity;
            
            switch(msg.pitch) {
                case KICK:
                    newCenter = ofVec2f(ofGetWidth() * 0.5f, ofGetHeight() * 0.8f);
                    zOffset += 10.0f; // 大きな流れの変化
                    magneticField += impactIntensity;
                    intensity *= 1.5f;
                    break;
                    
                case SNARE:
                    newCenter = ofVec2f(ofGetWidth() * 0.5f, ofGetHeight() * 0.3f);
                    noiseScale += 0.003f; // 乱流を増加
                    turbulence += impactIntensity * 2.0f;
                    break;
                    
                case HIHAT_CLOSED:
                    newCenter = ofVec2f(ofRandom(100, ofGetWidth()-100), ofRandom(100, ofGetHeight()-100));
                    timeSpeed += impactIntensity * 0.001f;
                    break;
                    
                case CRASH:
                    // 画面全体に影響
                    triggerMassiveFlow();
                    newCenter = ofVec2f(ofGetWidth() * 0.5f, ofGetHeight() * 0.5f);
                    intensity *= 2.0f;
                    break;
                    
                default:
                    newCenter = ofVec2f(
                        ofMap(msg.pitch % 12, 0, 12, 100, ofGetWidth() - 100),
                        ofMap(msg.pitch / 12, 0, 10, 100, ofGetHeight() - 100)
                    );
            }
            
            // 成長中心を追加（最大8個まで）
            addGrowthCenter(newCenter, intensity);
            
            // パーティクルを新しい中心から放出
            spawnParticlesFromCenter(newCenter, impactIntensity);
            
        } else if (msg.status == MIDI_CONTROL_CHANGE) {
            if (msg.control == 1) { // Mod wheel
                modulation = mapCC(msg.value);
                timeSpeed = 0.001f + modulation * 0.015f;
            } else if (msg.control == 7) { // Volume
                float vol = mapCC(msg.value);
                scale = 15 + vol * 25;
                updateFieldDimensions();
            }
        }
    }
    
private:
    void updateFieldDimensions() {
        cols = ceil(ofGetWidth() / scale) + 1;
        rows = ceil(ofGetHeight() / scale) + 1;
        field.clear();
        field.resize(cols, std::vector<ofVec2f>(rows));
    }
    
    void updateFlowField() {
        for (int x = 0; x < cols; x++) {
            for (int y = 0; y < rows; y++) {
                // Perlin noiseベースの角度
                float angle = ofNoise(x * noiseScale, y * noiseScale, zOffset) * TWO_PI * 4;
                
                // 乱流の追加
                angle += sin(x * 0.1f + turbulence) * cos(y * 0.1f + turbulence) * globalGrowthLevel;
                
                // 成長中心からの影響
                ofVec2f pos(x * scale, y * scale);
                for (int i = 0; i < growthCenters.size(); i++) {
                    auto& center = growthCenters[i];
                    float dist = pos.distance(center);
                    float influence = centerIntensities[i] * exp(-dist / (200.0f + globalGrowthLevel * 100.0f));
                    
                    if (influence > 0.1f) {
                        ofVec2f direction = (center - pos).getNormalized();
                        float centerAngle = atan2(direction.y, direction.x);
                        angle = ofLerpRadians(angle, centerAngle, influence * globalGrowthLevel);
                    }
                }
                
                // 磁場効果
                if (magneticField > 0.1f) {
                    float magneticAngle = atan2(pos.y - ofGetHeight() * 0.5f, pos.x - ofGetWidth() * 0.5f);
                    angle += sin(magneticAngle * 2 + magneticField * TWO_PI) * magneticField * 0.5f;
                }
                
                field[x][y] = ofVec2f(cos(angle), sin(angle));
                field[x][y] *= (intensity + 0.3f) * (1.0f + globalGrowthLevel * 0.8f);
            }
        }
    }
    
    void updateParticles(float deltaTime) {
        for (auto& particle : particles) {
            if (particle.active) {
                ofVec2f force = getForceAtPosition(particle.position);
                particle.update(force, deltaTime, globalGrowthLevel);
            }
        }
    }
    
    void updateGrowthCenters(float deltaTime) {
        // 中心の強度を時間とともに減衰
        for (int i = centerIntensities.size() - 1; i >= 0; i--) {
            centerIntensities[i] *= (0.998f - globalGrowthLevel * 0.0005f);
            
            if (centerIntensities[i] < 0.1f) {
                growthCenters.erase(growthCenters.begin() + i);
                centerIntensities.erase(centerIntensities.begin() + i);
            }
        }
        
        // 新しい中心をランダムに生成（成長時）
        if (globalGrowthLevel > 0.6f && ofRandom(1.0f) < 0.002f) {
            ofVec2f randomCenter(ofRandom(100, ofGetWidth() - 100), ofRandom(100, ofGetHeight() - 100));
            addGrowthCenter(randomCenter, 0.3f);
        }
    }
    
    void drawInfrastructureBackground() {
        // インフラストラクチャグリッド
        if (infrastructureLevel > 0.2f) {
            ofEnableBlendMode(OF_BLENDMODE_ADD);
            
            ofColor gridColor = urbanColor(currentNote, infrastructureLevel * 0.5f);
            gridColor.a = 60 + infrastructureLevel * 80;
            ofSetColor(gridColor);
            
            float gridSpacing = 120 - globalGrowthLevel * 20;  // グリッド間隔を大幅に拡大
            ofSetLineWidth(0.3f + globalGrowthLevel * 0.5);
            
            // インフラ不規則配置
            ofNoFill();
            
            // インフラ施設として散在配置
            int numInfra = (ofGetWidth() * ofGetHeight()) / (gridSpacing * gridSpacing * 1.8); // インフラ密度
            
            for (int i = 0; i < numInfra; i++) {
                // インフラノードの不規則配置
                float nodeX = ofRandom(0, ofGetWidth());
                float nodeY = ofRandom(0, ofGetHeight());
                
                // インフラ風多角形（様々なサイズ）
                ofBeginShape();
                int numVertices = 3 + (int)(ofRandom(6)); // 3-8角形（インフラの多様性）
                float radius = ofRandom(gridSpacing * 0.05, gridSpacing * 0.45);
                
                // コンクリートノイズで位置・形状変形
                float xOffset = sin(nodeX * 0.01f + concreteNoise) * globalGrowthLevel * 20;
                float yOffset = cos(nodeY * 0.01f + concreteNoise) * globalGrowthLevel * 15;
                nodeX += xOffset;
                nodeY += yOffset;
                
                for (int j = 0; j < numVertices; j++) {
                    float angle = (j * TWO_PI / numVertices) + ofRandom(-0.4, 0.4);
                    float r = radius + ofRandom(-radius * 0.4, radius * 0.4);
                    float vx = nodeX + cos(angle) * r;
                    float vy = nodeY + sin(angle) * r;
                    ofVertex(vx, vy);
                }
                ofEndShape(true);
            }
            ofFill();
            
            ofDisableBlendMode();
        }
    }
    
    void drawParticles() {
        ofEnableBlendMode(OF_BLENDMODE_ADD);
        
        for (auto& particle : particles) {
            particle.draw(globalGrowthLevel, intensity);
        }
        
        ofDisableBlendMode();
    }
    
    void drawGrowthCenters() {
        if (globalGrowthLevel > 0.3f) {
            ofEnableBlendMode(OF_BLENDMODE_ADD);
            
            for (int i = 0; i < growthCenters.size(); i++) {
                auto& center = growthCenters[i];
                float intensity = centerIntensities[i];
                
                ofColor centerColor = accentColor(intensity);
                centerColor.a = 150 * intensity * globalGrowthLevel;
                ofSetColor(centerColor);
                
                float radius = 4 + intensity * 10 + globalGrowthLevel * 6;  // 半分に縮小
                
                // 流体的な中心エフェクト
                for (int ring = 0; ring < 3; ring++) {
                    float ringRadius = radius * (0.3f + ring * 0.2f);
                    float ringAlpha = centerColor.a * (1.0f - ring * 0.3f);
                    ofColor ringColor = centerColor;
                    ringColor.a = ringAlpha;
                    ofSetColor(ringColor);
                    ofDrawCircle(center, ringRadius);
                }
                
                // 流体ラインで影響範囲を表現
                ofSetLineWidth(0.5f + globalGrowthLevel * 0.8f);
                ofNoFill();
                ofDrawCircle(center, radius);
                ofFill();
                
                // 放射状エフェクト
                int numRays = 8 + globalGrowthLevel * 8;
                for (int j = 0; j < numRays; j++) {
                    float angle = (j / float(numRays)) * TWO_PI + systemTime;
                    ofVec2f rayEnd = center + ofVec2f(cos(angle), sin(angle)) * radius * 0.7f;
                    
                    ofSetLineWidth(0.3f + intensity * 0.5);
                    ofDrawLine(center, rayEnd);
                }
            }
            
            ofDisableBlendMode();
        }
    }
    
    void drawUrbanEffects() {
        ofEnableBlendMode(OF_BLENDMODE_ADD);
        
        // 都市のパルス効果
        float pulse = sin(systemTime * 2.0f + globalGrowthLevel * PI) * 0.5f + 0.5f;
        
        ofColor pulseColor = accentColor(pulse);
        pulseColor.a = 50 * globalGrowthLevel;
        ofSetColor(pulseColor);
        
        // エッジグロー
        float glowWidth = globalGrowthLevel * 20;
        
        ofDrawRectangle(0, 0, ofGetWidth(), glowWidth);
        ofDrawRectangle(0, ofGetHeight() - glowWidth, ofGetWidth(), glowWidth);
        ofDrawRectangle(0, 0, glowWidth, ofGetHeight());
        ofDrawRectangle(ofGetWidth() - glowWidth, 0, glowWidth, ofGetHeight());
        
        ofDisableBlendMode();
    }
    
    void drawFlowField() {
        ofEnableBlendMode(OF_BLENDMODE_ADD);
        ofSetColor(255, 30);
        ofSetLineWidth(0.5f);
        
        for (int x = 0; x < cols; x += 4) {  // 間引きをさらに増加して描画
            for (int y = 0; y < rows; y += 4) {
                ofVec2f pos(x * scale, y * scale);
                ofVec2f force = field[x][y];
                
                ofVec2f end = pos + force * scale * 0.8f;
                
                ofDrawLine(pos, end);
                ofDrawCircle(end, 1);
            }
        }
        
        ofDisableBlendMode();
    }
    
    void spawnParticlesFromCenter(ofVec2f center, float intensity) {
        int spawnCount = intensity * 30 + globalGrowthLevel * 15;  // スポーン率を大幅削減
        
        for (int i = 0; i < spawnCount && i < particles.size(); i++) {
            if (!particles[i].active || particles[i].life < 20) {
                particles[i].reset();
                particles[i].position = center + ofVec2f(
                    ofRandom(-50, 50),
                    ofRandom(-50, 50)
                );
                particles[i].color = urbanColor(currentNote, intensity);
                particles[i].size = ofRandom(1.0f, 3.0f) * (1.0f + globalGrowthLevel);
                particles[i].life = 200 + intensity * 200; // 長寿命
            }
        }
    }
    
    void addGrowthCenter(ofVec2f center, float intensity) {
        growthCenters.push_back(center);
        centerIntensities.push_back(intensity);
        
        // 最大数制限
        if (growthCenters.size() > 8) {
            growthCenters.erase(growthCenters.begin());
            centerIntensities.erase(centerIntensities.begin());
        }
    }
    
    void triggerMassiveFlow() {
        // クラッシュ時の大規模フロー効果
        zOffset += 20.0f;
        turbulence += 3.0f;
        magneticField = 1.0f;
        
        // 全パーティクルにエネルギー注入
        for (auto& particle : particles) {
            if (particle.active) {
                particle.velocity *= 2.0f;
                particle.life += 100;
                particle.color = accentColor(1.0f);
            }
        }
        
        // 複数の成長中心を同時生成
        for (int i = 0; i < 4; i++) {
            ofVec2f explosionCenter(
                ofGetWidth() * 0.25f + i * ofGetWidth() * 0.25f,
                ofGetHeight() * 0.5f + ofRandom(-100, 100)
            );
            addGrowthCenter(explosionCenter, 1.0f);
        }
    }
    
    void applyCollapseEffects() {
        // 崩壊時のエフェクト
        for (auto& particle : particles) {
            if (particle.active && ofRandom(1.0f) < 0.1f) {
                // ランダムにパーティクルを削除
                particle.active = false;
            }
        }
        
        // フローフィールドの乱れ
        turbulence += 0.1f;
        noiseScale += 0.001f;
    }
    
    ofVec2f getForceAtPosition(ofVec2f pos) {
        int col = floor(pos.x / scale);
        int row = floor(pos.y / scale);
        
        // 境界チェック
        col = ofClamp(col, 0, cols - 1);
        row = ofClamp(row, 0, rows - 1);
        
        return field[col][row];
    }
};