#pragma once

#include "VisualSystem.h"
#include <vector>

class Particle {
public:
    ofVec2f position;
    ofVec2f velocity;
    ofVec2f acceleration;
    float life;
    float maxLife;
    ofColor color;
    float size;
    float mass;
    bool isUrbanElement; // 都市要素かどうか
    
    Particle(ofVec2f pos, ofVec2f vel, float lifespan, ofColor col, bool urban = false) {
        position = pos;
        velocity = vel;
        acceleration = ofVec2f(0, 0);
        life = lifespan;
        maxLife = lifespan;
        color = col;
        size = ofRandom(0.5, 4);
        mass = ofRandom(0.5f, 2.0f);
        isUrbanElement = urban;
    }
    
    void applyForce(ofVec2f force) {
        acceleration += force / mass;
    }
    
    void update(float deltaTime, float globalGrowth) {
        velocity += acceleration * deltaTime;
        
        // 空気抵抗
        velocity *= 0.999f;
        
        position += velocity * deltaTime;
        acceleration *= 0;
        
        // 成長に応じた寿命変化
        float lifeLoss = deltaTime * (1.0f - globalGrowth * 0.5f);
        life -= lifeLoss;
        
        // 都市要素は成長と共にサイズ増加
        if (isUrbanElement) {
            size *= (1.0f + globalGrowth * deltaTime * 0.05f);
        }
    }
    
    bool isDead() {
        return life <= 0;
    }
    
    void draw(float globalGrowth, float impactIntensity) {
        float alpha = ofMap(life, 0, maxLife, 0, 255);
        alpha *= (0.7f + globalGrowth * 0.3f); // 成長で明るく
        
        ofColor drawColor = color;
        
        // インパクト時の色彩強化
        if (impactIntensity > 0.3f) {
            drawColor.setSaturation(drawColor.getSaturation() * (1.0f + impactIntensity));
            drawColor.setBrightness(drawColor.getBrightness() * (1.0f + impactIntensity * 0.5f));
        }
        
        ofSetColor(drawColor, alpha);
        
        float drawSize = size * (life / maxLife);
        
        // 成長レベルに応じたエフェクト
        if (globalGrowth > 0.5f) {
            // 高成長時はグローエフェクト
            ofSetColor(drawColor, alpha * 0.3f);
            ofDrawCircle(position, drawSize * 1.3);
            ofSetColor(drawColor, alpha);
        }
        
        if (isUrbanElement) {
            // 都市要素は矩形で描画
            ofDrawRectangle(position.x - drawSize/2, position.y - drawSize/2, drawSize, drawSize);
            
            // 建物の窓のような効果
            if (drawSize > 4) {
                ofSetColor(255, alpha * 0.8f);
                float windowSize = drawSize * 0.15f;
                for (int i = 0; i < 3; i++) {
                    for (int j = 0; j < 3; j++) {
                        float x = position.x - drawSize/2 + (i + 0.5f) * drawSize/3;
                        float y = position.y - drawSize/2 + (j + 0.5f) * drawSize/3;
                        ofDrawRectangle(x - windowSize/2, y - windowSize/2, windowSize, windowSize);
                    }
                }
            }
        } else {
            ofDrawCircle(position, drawSize);
        }
    }
};

class ParticleSystem : public VisualSystem {
private:
    std::vector<Particle> particles;
    ofVec2f gravity;
    ofVec2f wind;
    float particleRate = 5.0f;
    float particleTimer = 0.0f;
    
    // 強化されたアトラクター
    std::vector<ofVec2f> attractors;
    std::vector<float> attractorStrengths;
    float baseAttractorStrength = 150.0f;
    
    // 都市パーティクル生成
    std::vector<ofVec2f> urbanSpawnPoints;
    float urbanParticleChance = 0.3f;
    
    // 大規模エフェクト
    float explosionTimer = 0.0f;
    bool massExplosionActive = false;
    
public:
    void setup() override {
        gravity = ofVec2f(0, 80);
        wind = ofVec2f(0, 0);
        
        // 初期アトラクターを画面全体に配置（間隔を拡大）
        attractors.push_back(ofVec2f(ofGetWidth() * 0.15f, ofGetHeight() * 0.25f));
        attractors.push_back(ofVec2f(ofGetWidth() * 0.85f, ofGetHeight() * 0.25f));
        attractors.push_back(ofVec2f(ofGetWidth() * 0.5f, ofGetHeight() * 0.75f));
        
        for (int i = 0; i < attractors.size(); i++) {
            attractorStrengths.push_back(baseAttractorStrength);
        }
        
        // 都市スポーン地点（密度を削減）
        for (int i = 0; i < 4; i++) {
            urbanSpawnPoints.push_back(ofVec2f(
                ofRandom(50, ofGetWidth() - 50),
                ofRandom(50, ofGetHeight() - 50)
            ));
        }
    }
    
    void update(float deltaTime) override {
        // 統一エフェクトシステムの更新
        updateGlobalEffects(deltaTime);
        
        particleTimer += deltaTime;
        explosionTimer += deltaTime;
        
        // 成長レベルに応じたパーティクル生成率
        float adjustedRate = particleRate * (1.0f + globalGrowthLevel * 3.0f);
        
        // パーティクル生成
        if (particleTimer >= 1.0f / adjustedRate) {
            generateParticles();
            particleTimer = 0.0f;
        }
        
        // 大規模爆発エフェクト
        if (massExplosionActive) {
            generateExplosionParticles();
            if (explosionTimer > 2.0f) {
                massExplosionActive = false;
                explosionTimer = 0.0f;
            }
        }
        
        // アトラクターの動的更新
        updateAttractors(deltaTime);
        
        // パーティクルの更新
        for (auto it = particles.begin(); it != particles.end();) {
            // 重力とエアレジスタンス
            it->applyForce(gravity * (1.0f - modulation * 0.5f));
            it->applyForce(wind * (1.0f + impactIntensity));
            
            // アトラクターの影響
            for (int i = 0; i < attractors.size(); i++) {
                ofVec2f force = attractors[i] - it->position;
                float distance = force.length();
                
                if (distance > 1.0f && distance < 300.0f) {
                    force.normalize();
                    float strength = attractorStrengths[i] / (distance * distance);
                    strength *= (1.0f + globalGrowthLevel); // 成長で強化
                    it->applyForce(force * strength);
                }
            }
            
            // 境界反発（画面全体を活用）
            if (it->position.x < 0 || it->position.x > ofGetWidth()) {
                it->velocity.x *= -0.8f;
                it->position.x = ofClamp(it->position.x, 0, ofGetWidth());
            }
            if (it->position.y < 0 || it->position.y > ofGetHeight()) {
                it->velocity.y *= -0.8f;
                it->position.y = ofClamp(it->position.y, 0, ofGetHeight());
            }
            
            it->update(deltaTime, globalGrowthLevel);
            
            if (it->isDead()) {
                it = particles.erase(it);
            } else {
                ++it;
            }
        }
        
        // パーティクル数制限（成長に応じて増加、大幅削減）
        int maxParticles = 250 + globalGrowthLevel * 350;
        while (particles.size() > maxParticles) {
            particles.pop_back();
        }
        
        // 風の更新
        wind.x = sin(systemTime * 0.5f) * modulation * 30.0f;
        wind.y = cos(systemTime * 0.3f) * modulation * 15.0f;
    }
    
    void draw() override {
        // マスターバッファに描画開始
        beginMasterBuffer();
        
        // パーティクルの描画
        drawParticles();
        
        // アトラクターの描画
        drawAttractors();
        
        // 都市構造の描画
        drawUrbanStructures();
        
        endMasterBuffer();
        
        // 全画面エフェクトの描画
        drawFullscreenEffects();
        
        // 統計情報
        drawDebugInfo();
    }
    
    void onMidiMessage(ofxMidiMessage& msg) override {
        if (msg.status == MIDI_NOTE_ON && msg.velocity > 0) {
            currentNote = msg.pitch;
            currentVelocity = msg.velocity;
            
            triggerImpact(msg.pitch, msg.velocity);
            
            // ドラムタイプに応じたエフェクト
            switch(msg.pitch) {
                case KICK:
                    // 中央から大爆発
                    triggerExplosion(ofVec2f(ofGetWidth() * 0.5f, ofGetHeight() * 0.5f), 
                                   impactIntensity * 200, true);
                    // アトラクター強化
                    for (auto& strength : attractorStrengths) {
                        strength += impactIntensity * 100;
                    }
                    break;
                    
                case SNARE:
                    // 画面四隅から爆発
                    triggerExplosion(ofVec2f(0, 0), impactIntensity * 100, false);
                    triggerExplosion(ofVec2f(ofGetWidth(), 0), impactIntensity * 100, false);
                    triggerExplosion(ofVec2f(0, ofGetHeight()), impactIntensity * 100, false);
                    triggerExplosion(ofVec2f(ofGetWidth(), ofGetHeight()), impactIntensity * 100, false);
                    break;
                    
                case HIHAT_CLOSED:
                    // 細かいスパーク
                    for (int i = 0; i < 3; i++) {
                        ofVec2f pos(ofRandom(ofGetWidth()), ofRandom(ofGetHeight()));
                        triggerExplosion(pos, impactIntensity * 30, false);
                    }
                    break;
                    
                case CRASH:
                    // 大規模爆発モード
                    massExplosionActive = true;
                    explosionTimer = 0.0f;
                    // 全アトラクターを強化
                    for (auto& strength : attractorStrengths) {
                        strength += impactIntensity * 300;
                    }
                    break;
                    
                default:
                    // 音程に基づく位置での爆発
                    ofVec2f pos(
                        ofMap(msg.pitch % 12, 0, 12, 0, ofGetWidth()),
                        ofMap(msg.pitch / 12, 0, 10, 0, ofGetHeight())
                    );
                    triggerExplosion(pos, impactIntensity * 80, false);
            }
            
        } else if (msg.status == MIDI_CONTROL_CHANGE) {
            if (msg.control == 1) { // Mod wheel
                modulation = mapCC(msg.value);
                // モジュレーションでアトラクター移動
                for (int i = 0; i < attractors.size(); i++) {
                    attractors[i].x += ofRandom(-modulation * 50, modulation * 50);
                    attractors[i].y += ofRandom(-modulation * 50, modulation * 50);
                    attractors[i].x = ofClamp(attractors[i].x, 0, ofGetWidth());
                    attractors[i].y = ofClamp(attractors[i].y, 0, ofGetHeight());
                }
            }
        }
    }
    
private:
    void generateParticles() {
        int numParticles = 1 + globalGrowthLevel * 3; // 成長で生成数増加（大幅削減）
        
        for (int i = 0; i < numParticles; i++) {
            ofVec2f spawnPos;
            ofColor color;
            bool isUrban = false;
            
            if (ofRandom(1.0f) < urbanParticleChance + globalGrowthLevel * 0.3f) {
                // 都市パーティクル
                spawnPos = urbanSpawnPoints[ofRandom(urbanSpawnPoints.size())];
                spawnPos += ofVec2f(ofRandom(-30, 30), ofRandom(-30, 30));
                color = urbanColor(currentNote, 0.8f + globalGrowthLevel * 0.2f);
                isUrban = true;
            } else {
                // 通常パーティクル
                spawnPos = ofVec2f(ofRandom(ofGetWidth()), ofRandom(ofGetHeight()));
                color = accentColor(impactIntensity + globalGrowthLevel * 0.5f);
            }
            
            ofVec2f velocity(
                ofRandom(-50, 50) * (1.0f + impactIntensity),
                ofRandom(-80, 20) * (1.0f + impactIntensity)
            );
            
            float lifespan = ofRandom(2, 8) * (1.0f + globalGrowthLevel);
            
            particles.push_back(Particle(spawnPos, velocity, lifespan, color, isUrban));
        }
    }
    
    void generateExplosionParticles() {
        // 大規模爆発時のパーティクル生成（密度削減）
        int explosionCount = 5 + impactIntensity * 10;
        
        for (int i = 0; i < explosionCount; i++) {
            ofVec2f center(ofGetWidth() * 0.5f, ofGetHeight() * 0.5f);
            center += ofVec2f(ofRandom(-100, 100), ofRandom(-100, 100));
            
            float angle = ofRandom(TWO_PI);
            float speed = ofRandom(100, 400);
            ofVec2f velocity(cos(angle) * speed, sin(angle) * speed);
            
            ofColor explosionColor = accentColor(1.0f);
            explosionColor.setBrightness(255);
            
            particles.push_back(Particle(center, velocity, ofRandom(1, 4), explosionColor, false));
        }
    }
    
    void triggerExplosion(ofVec2f center, float force, bool isUrban) {
        int numParticles = force / 8 + 4;
        
        for (int i = 0; i < numParticles; i++) {
            float angle = ofRandom(TWO_PI);
            float speed = ofRandom(force * 0.5f, force);
            ofVec2f velocity(cos(angle) * speed, sin(angle) * speed);
            
            ofColor explosionColor = isUrban ? 
                urbanColor(currentNote, 1.0f) : 
                accentColor(impactIntensity);
            
            particles.push_back(Particle(center, velocity, ofRandom(1, 5), explosionColor, isUrban));
        }
    }
    
    void updateAttractors(float deltaTime) {
        // アトラクターの動的移動
        for (int i = 0; i < attractors.size(); i++) {
            // 円運動
            float angle = systemTime * 0.3f + i * TWO_PI / attractors.size();
            float radius = 100 + globalGrowthLevel * 150;
            
            ofVec2f center(ofGetWidth() * 0.5f, ofGetHeight() * 0.5f);
            attractors[i] = center + ofVec2f(cos(angle) * radius, sin(angle) * radius);
            
            // 強度の減衰
            attractorStrengths[i] *= 0.99f;
            attractorStrengths[i] = ofClamp(attractorStrengths[i], 
                                          baseAttractorStrength, 
                                          baseAttractorStrength * 5);
        }
    }
    
    void drawParticles() {
        ofEnableBlendMode(OF_BLENDMODE_ADD);
        
        for (auto& particle : particles) {
            particle.draw(globalGrowthLevel, impactIntensity);
        }
        
        ofDisableBlendMode();
    }
    
    void drawAttractors() {
        ofEnableBlendMode(OF_BLENDMODE_ADD);
        
        for (int i = 0; i < attractors.size(); i++) {
            float strength = attractorStrengths[i] / baseAttractorStrength;
            
            ofColor attractorColor = accentColor(strength);
            attractorColor.a = 100 + strength * 50;
            ofSetColor(attractorColor);
            
            float size = 4 + strength * 6 + globalGrowthLevel * 8;
            
            // 脈動効果
            size += sin(systemTime * 3 + i) * 2;
            
            ofDrawCircle(attractors[i], size);
            
            // リング エフェクト
            ofNoFill();
            ofSetLineWidth(0.8 + strength * 0.4);
            ofDrawCircle(attractors[i], size * 1.5);
            ofFill();
        }
        
        ofDisableBlendMode();
    }
    
    void drawUrbanStructures() {
        // 都市スポーン地点での建造物表現
        ofEnableBlendMode(OF_BLENDMODE_ADD);
        
        for (auto& spawnPoint : urbanSpawnPoints) {
            ofColor structureColor = urbanColor(currentNote, globalGrowthLevel);
            structureColor.a = 150 + globalGrowthLevel * 80;
            ofSetColor(structureColor);
            
            float height = 20 + globalGrowthLevel * 60;
            float width = 8 + globalGrowthLevel * 12;
            
            // 建物のシルエット
            ofDrawRectangle(spawnPoint.x - width/2, spawnPoint.y - height, width, height);
            
            // 建物の明かり
            if (globalGrowthLevel > 0.3f) {
                ofSetColor(255, 200);
                for (int i = 0; i < 3; i++) {
                    float lightY = spawnPoint.y - height + (i + 1) * height / 4;
                    ofDrawRectangle(spawnPoint.x - 2, lightY - 1, 4, 2);
                }
            }
        }
        
        ofDisableBlendMode();
    }
    
    void drawDebugInfo() {
        if (getTimeSinceLastMidi() < 5.0f) { // 最近MIDI入力があった場合のみ表示
            ofSetColor(200);
            ofDrawBitmapString("Particles: " + ofToString(particles.size()), 20, ofGetHeight() - 80);
            ofDrawBitmapString("Growth: " + ofToString(globalGrowthLevel * 100, 1) + "%", 20, ofGetHeight() - 60);
            ofDrawBitmapString("Impact: " + ofToString(impactIntensity, 2), 20, ofGetHeight() - 40);
            if (isCollapsing) {
                ofSetColor(255, 100, 100);
                ofDrawBitmapString("URBAN COLLAPSE", 20, ofGetHeight() - 20);
            }
        }
    }
};