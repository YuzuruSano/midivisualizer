#include "WaterRippleSystem.h"

WaterRippleSystem::WaterRippleSystem() {
    waterLevel = ofGetHeight() * 0.5f;
    waterOpacity = 80.0f;
    surfaceTension = 0.8f;
    waveAmplitude = 10.0f;
    waveFrequency = 0.02f;
    
    // モノトーンカラーパレット
    waterDark = ofColor(20, 20, 20);
    waterMedium = ofColor(60, 60, 60);
    waterLight = ofColor(100, 100, 100);
    rippleColor = ofColor(140, 140, 140);
    foamColor = ofColor(180, 180, 180);
    
    rippleSpawnRate = 0.3f;
    rippleLifetime = 4.0f;
    rippleSpeed = 100.0f;
    rippleIntensity = 1.0f;
    
    gravitationalAnomalyStrength = 0.5f;
    timeDistortionFactor = 1.0f;
    rippleInteractionStrength = 0.7f;
    quantumFluctuationRate = 0.1f;
    
    ambientFlow = 0.3f;
    turbulenceStrength = 0.2f;
    interferencePattern = 0.0f;
    
    kickIntensity = 0.0f;
    snareIntensity = 0.0f;
    hihatIntensity = 0.0f;
    crashIntensity = 0.0f;
    
    autonomousMovementSpeed = 30.0f;
}

void WaterRippleSystem::setup() {
    // 自律的な波紋中心点の初期化
    for (int i = 0; i < 6; i++) {
        ofVec2f center(
            ofRandom(ofGetWidth() * 0.2f, ofGetWidth() * 0.8f),
            ofRandom(ofGetHeight() * 0.2f, ofGetHeight() * 0.8f)
        );
        autonomousRippleCenters.push_back(center);
    }
    
    // 初期波紋の生成
    for (int i = 0; i < 3; i++) {
        createRipple(ofVec2f(ofRandom(ofGetWidth()), ofRandom(ofGetHeight())), 0.5f);
    }
}

void WaterRippleSystem::update(float deltaTime) {
    float currentTime = ofGetElapsedTimef();
    
    // 時間歪曲効果
    float adjustedDeltaTime = deltaTime * timeDistortionFactor;
    
    // 波紋の更新
    updateRipples(adjustedDeltaTime);
    
    // 水滴パーティクルの更新
    updateWaterParticles(adjustedDeltaTime);
    
    // 波紋クラスターの更新
    updateRippleClusters(adjustedDeltaTime);
    
    // 自律的な波紋移動の更新
    updateAutonomousRipples(adjustedDeltaTime);
    
    // 波紋の相互作用計算
    calculateRippleInteraction();
    
    // ランダムな波紋生成
    if (ofRandom(1.0f) < rippleSpawnRate * adjustedDeltaTime) {
        ofVec2f randomPos(ofRandom(ofGetWidth()), ofRandom(ofGetHeight()));
        createRipple(randomPos, ofRandom(0.3f, 0.8f));
    }
    
    // 量子揺らぎによる波紋
    if (ofRandom(1.0f) < quantumFluctuationRate * adjustedDeltaTime) {
        ofVec2f quantumPos(ofRandom(ofGetWidth()), ofRandom(ofGetHeight()));
        createRippleCluster(quantumPos, ofRandom(3, 7), ofRandom(50, 120));
    }
    
    // 環境効果の更新
    interferencePattern += adjustedDeltaTime * 0.8f;
    turbulenceStrength = 0.2f + sin(currentTime * 0.5f) * 0.15f;
    
    // MIDI連動強度の減衰
    kickIntensity *= 0.90f;
    snareIntensity *= 0.85f;
    hihatIntensity *= 0.80f;
    crashIntensity *= 0.75f;
    
    // 成長レベルに基づく時間歪曲
    timeDistortionFactor = 1.0f + globalGrowthLevel * 0.5f;
    
    // 非活性要素の削除
    cleanupInactiveElements();
}

void WaterRippleSystem::draw() {
    ofPushMatrix();
    
    // 水面の背景描画
    drawWaterSurface();
    
    // 干渉パターンの描画
    drawInterferencePattern();
    
    // 量子揺らぎの描画
    drawQuantumFluctuations();
    
    // 波紋クラスターの描画
    drawRippleClusters();
    
    // 波紋の描画
    drawRipples();
    
    // 水滴パーティクルの描画
    drawWaterParticles();
    
    ofPopMatrix();
}

void WaterRippleSystem::createRipple(ofVec2f position, float intensity) {
    Ripple ripple;
    ripple.center = position;
    ripple.radius = 0.0f;
    ripple.maxRadius = 150.0f + intensity * 100.0f;
    ripple.intensity = intensity;
    ripple.speed = rippleSpeed + ofRandom(-20, 20);
    ripple.creationTime = ofGetElapsedTimef();
    ripple.lifetime = rippleLifetime + ofRandom(-1, 1);
    ripple.isActive = true;
    ripple.rippleColor = ofColor(
        rippleColor.r + ofRandom(-20, 20),
        rippleColor.g + ofRandom(-20, 20),
        rippleColor.b + ofRandom(-20, 20)
    );
    
    ripples.push_back(ripple);
    
    // 波紋に伴う水滴パーティクルの生成
    spawnWaterParticles(position, intensity);
}

void WaterRippleSystem::createRippleCluster(ofVec2f center, int count, float spread) {
    RippleCluster cluster;
    cluster.center = center;
    cluster.clusterRadius = spread;
    cluster.activationTime = ofGetElapsedTimef();
    cluster.intensity = ofRandom(0.5f, 1.2f);
    cluster.isExpanding = true;
    
    for (int i = 0; i < count; i++) {
        float angle = (float)i / count * TWO_PI;
        float distance = ofRandom(0, spread);
        ofVec2f ripplePos = center + ofVec2f(
            cos(angle) * distance,
            sin(angle) * distance
        );
        
        Ripple ripple;
        ripple.center = ripplePos;
        ripple.radius = 0.0f;
        ripple.maxRadius = 80.0f + distance * 0.5f;
        ripple.intensity = cluster.intensity * ofRandom(0.7f, 1.3f);
        ripple.speed = rippleSpeed * ofRandom(0.8f, 1.2f);
        ripple.creationTime = ofGetElapsedTimef() + i * 0.1f;
        ripple.lifetime = rippleLifetime;
        ripple.isActive = true;
        ripple.rippleColor = rippleColor;
        
        cluster.ripples.push_back(ripple);
    }
    
    rippleClusters.push_back(cluster);
}

void WaterRippleSystem::updateRipples(float deltaTime) {
    for (auto& ripple : ripples) {
        if (!ripple.isActive) continue;
        
        float age = ofGetElapsedTimef() - ripple.creationTime;
        if (age > ripple.lifetime) {
            ripple.isActive = false;
            continue;
        }
        
        // 波紋の拡大
        ripple.radius += ripple.speed * deltaTime;
        
        // 異常な物理: 重力異常による波紋の歪み
        if (gravitationalAnomalyStrength > 0.0f) {
            ofVec2f screenCenter(ofGetWidth() * 0.5f, ofGetHeight() * 0.5f);
            ofVec2f toCenter = screenCenter - ripple.center;
            float distance = toCenter.length();
            
            if (distance > 0) {
                float gravitationalPull = gravitationalAnomalyStrength * 50.0f / distance;
                ripple.center += toCenter.getNormalized() * gravitationalPull * deltaTime;
            }
        }
        
        // 波紋の強度減衰
        float fadeRatio = 1.0f - (age / ripple.lifetime);
        ripple.intensity = fadeRatio * fadeRatio;
        
        // 最大半径に達したら非活性化
        if (ripple.radius > ripple.maxRadius) {
            ripple.isActive = false;
        }
    }
}

void WaterRippleSystem::updateWaterParticles(float deltaTime) {
    for (auto& particle : waterParticles) {
        if (!particle.isActive) continue;
        
        particle.life -= deltaTime;
        if (particle.life <= 0.0f) {
            particle.isActive = false;
            continue;
        }
        
        // パーティクルの移動
        particle.position += particle.velocity * deltaTime;
        
        // 重力の影響
        particle.velocity.y += 200.0f * deltaTime;
        
        // 空気抵抗
        particle.velocity *= 0.98f;
        
        // 透明度の計算
        particle.alpha = 255.0f * (particle.life / particle.maxLife);
        
        // サイズの変化
        particle.size = 2.0f + (1.0f - particle.life / particle.maxLife) * 3.0f;
        
        // 画面外で非活性化
        if (particle.position.x < -50 || particle.position.x > ofGetWidth() + 50 ||
            particle.position.y < -50 || particle.position.y > ofGetHeight() + 50) {
            particle.isActive = false;
        }
    }
}

void WaterRippleSystem::updateRippleClusters(float deltaTime) {
    for (auto& cluster : rippleClusters) {
        // クラスター内の波紋を更新
        for (auto& ripple : cluster.ripples) {
            if (!ripple.isActive) continue;
            
            float rippleAge = ofGetElapsedTimef() - ripple.creationTime;
            if (rippleAge > ripple.lifetime) {
                ripple.isActive = false;
                continue;
            }
            
            ripple.radius += ripple.speed * deltaTime;
            
            float fadeRatio = 1.0f - (rippleAge / ripple.lifetime);
            ripple.intensity = fadeRatio * cluster.intensity;
            
            if (ripple.radius > ripple.maxRadius) {
                ripple.isActive = false;
            }
        }
        
        // クラスター半径の変化
        if (cluster.isExpanding) {
            cluster.clusterRadius += 30.0f * deltaTime;
            if (cluster.clusterRadius > 200.0f) {
                cluster.isExpanding = false;
            }
        }
    }
}

void WaterRippleSystem::updateAutonomousRipples(float deltaTime) {
    for (auto& center : autonomousRippleCenters) {
        // 自律的な移動
        float moveAngle = ofNoise(center.x * 0.01f, center.y * 0.01f, ofGetElapsedTimef() * 0.5f) * TWO_PI;
        ofVec2f moveDir(cos(moveAngle), sin(moveAngle));
        
        center += moveDir * autonomousMovementSpeed * deltaTime;
        
        // 画面境界での反射
        if (center.x < 50 || center.x > ofGetWidth() - 50) {
            center.x = ofClamp(center.x, 50, ofGetWidth() - 50);
        }
        if (center.y < 50 || center.y > ofGetHeight() - 50) {
            center.y = ofClamp(center.y, 50, ofGetHeight() - 50);
        }
        
        // 自律的な波紋生成
        if (ofRandom(1.0f) < 0.02f) {
            createRipple(center, ofRandom(0.4f, 0.9f));
        }
    }
}

void WaterRippleSystem::drawWaterSurface() {
    ofSetColor(waterDark.r, waterDark.g, waterDark.b, waterOpacity);
    ofDrawRectangle(0, 0, ofGetWidth(), ofGetHeight());
    
    // 水面の波の描画
    ofSetColor(waterMedium.r, waterMedium.g, waterMedium.b, waterOpacity * 0.7f);
    ofSetLineWidth(2.0f);
    
    for (float y = 0; y < ofGetHeight(); y += 30) {
        ofBeginShape();
        for (float x = 0; x <= ofGetWidth(); x += 10) {
            float waveHeight = sin(x * waveFrequency + ofGetElapsedTimef() * 2.0f) * waveAmplitude;
            waveHeight += sin(x * waveFrequency * 2.3f + ofGetElapsedTimef() * 1.5f) * waveAmplitude * 0.5f;
            
            ofVertex(x, y + waveHeight);
        }
        ofEndShape(false);
    }
}

void WaterRippleSystem::drawRipples() {
    ofSetLineWidth(2.0f);
    ofNoFill();
    
    for (const auto& ripple : ripples) {
        if (!ripple.isActive || ripple.radius <= 0) continue;
        
        float alpha = ripple.intensity * 200.0f;
        ofSetColor(ripple.rippleColor.r, ripple.rippleColor.g, ripple.rippleColor.b, alpha);
        
        // メインの波紋
        ofDrawCircle(ripple.center.x, ripple.center.y, ripple.radius);
        
        // 内側の波紋
        if (ripple.radius > 20) {
            ofSetColor(ripple.rippleColor.r, ripple.rippleColor.g, ripple.rippleColor.b, alpha * 0.6f);
            ofDrawCircle(ripple.center.x, ripple.center.y, ripple.radius * 0.7f);
        }
        
        // 外側の波紋
        if (ripple.radius > 40) {
            ofSetColor(ripple.rippleColor.r, ripple.rippleColor.g, ripple.rippleColor.b, alpha * 0.3f);
            ofDrawCircle(ripple.center.x, ripple.center.y, ripple.radius * 1.3f);
        }
    }
}

void WaterRippleSystem::drawWaterParticles() {
    ofFill();
    
    for (const auto& particle : waterParticles) {
        if (!particle.isActive) continue;
        
        ofSetColor(foamColor.r, foamColor.g, foamColor.b, particle.alpha);
        ofDrawCircle(particle.position.x, particle.position.y, particle.size);
    }
}

void WaterRippleSystem::drawRippleClusters() {
    ofSetLineWidth(1.5f);
    ofNoFill();
    
    for (const auto& cluster : rippleClusters) {
        for (const auto& ripple : cluster.ripples) {
            if (!ripple.isActive || ripple.radius <= 0) continue;
            
            float alpha = ripple.intensity * 150.0f;
            ofSetColor(ripple.rippleColor.r, ripple.rippleColor.g, ripple.rippleColor.b, alpha);
            
            ofDrawCircle(ripple.center.x, ripple.center.y, ripple.radius);
        }
    }
}

void WaterRippleSystem::drawInterferencePattern() {
    ofSetColor(waterLight.r, waterLight.g, waterLight.b, 40);
    
    // 波紋同士の干渉パターンを描画
    for (int i = 0; i < ripples.size(); i++) {
        for (int j = i + 1; j < ripples.size(); j++) {
            if (!ripples[i].isActive || !ripples[j].isActive) continue;
            
            float distance = ripples[i].center.distance(ripples[j].center);
            if (distance < 200.0f) {
                float interferenceStrength = (200.0f - distance) / 200.0f;
                
                ofSetColor(waterLight.r, waterLight.g, waterLight.b, 
                          interferenceStrength * 30.0f);
                ofDrawLine(ripples[i].center.x, ripples[i].center.y,
                          ripples[j].center.x, ripples[j].center.y);
            }
        }
    }
}

void WaterRippleSystem::drawQuantumFluctuations() {
    ofSetColor(waterLight.r, waterLight.g, waterLight.b, 20);
    
    // 量子揺らぎによる微細な波紋
    for (int i = 0; i < 20; i++) {
        float noiseX = ofNoise(i * 0.1f, ofGetElapsedTimef() * 0.3f) * ofGetWidth();
        float noiseY = ofNoise(i * 0.1f + 100, ofGetElapsedTimef() * 0.3f) * ofGetHeight();
        float noiseRadius = ofNoise(i * 0.1f + 200, ofGetElapsedTimef() * 0.5f) * 30.0f + 5.0f;
        
        ofDrawCircle(noiseX, noiseY, noiseRadius);
    }
}

void WaterRippleSystem::calculateRippleInteraction() {
    // 波紋同士の相互作用を計算
    for (auto& ripple1 : ripples) {
        if (!ripple1.isActive) continue;
        
        for (auto& ripple2 : ripples) {
            if (!ripple2.isActive || &ripple1 == &ripple2) continue;
            
            float distance = ripple1.center.distance(ripple2.center);
            if (distance < 100.0f && distance > 0) {
                float interactionStrength = (100.0f - distance) / 100.0f;
                
                // 相互作用による速度変化
                ripple1.speed *= (1.0f + interactionStrength * 0.1f);
                ripple2.speed *= (1.0f + interactionStrength * 0.1f);
                
                // 強度の増幅
                ripple1.intensity *= (1.0f + interactionStrength * 0.05f);
                ripple2.intensity *= (1.0f + interactionStrength * 0.05f);
            }
        }
    }
}

void WaterRippleSystem::spawnWaterParticles(ofVec2f position, float intensity) {
    int particleCount = (int)(intensity * 15.0f);
    
    for (int i = 0; i < particleCount; i++) {
        WaterParticle particle;
        particle.position = position + ofVec2f(ofRandom(-10, 10), ofRandom(-10, 10));
        
        float angle = ofRandom(TWO_PI);
        float speed = ofRandom(50, 150) * intensity;
        particle.velocity.set(cos(angle) * speed, sin(angle) * speed - 100);
        
        particle.life = ofRandom(0.5f, 2.0f);
        particle.maxLife = particle.life;
        particle.size = ofRandom(1.0f, 4.0f);
        particle.alpha = 255.0f;
        particle.isActive = true;
        
        waterParticles.push_back(particle);
    }
}

void WaterRippleSystem::cleanupInactiveElements() {
    // 非活性な波紋を削除
    ripples.erase(std::remove_if(ripples.begin(), ripples.end(),
                                [](const Ripple& r) { return !r.isActive; }),
                  ripples.end());
    
    // 非活性な水滴パーティクルを削除
    waterParticles.erase(std::remove_if(waterParticles.begin(), waterParticles.end(),
                                       [](const WaterParticle& p) { return !p.isActive; }),
                        waterParticles.end());
    
    // 非活性なクラスターを削除
    rippleClusters.erase(std::remove_if(rippleClusters.begin(), rippleClusters.end(),
                                       [](const RippleCluster& c) {
                                           return std::all_of(c.ripples.begin(), c.ripples.end(),
                                                             [](const Ripple& r) { return !r.isActive; });
                                       }),
                        rippleClusters.end());
}

void WaterRippleSystem::onMidiMessage(ofxMidiMessage& msg) {
    if (msg.status == MIDI_NOTE_ON) {
        int note = msg.pitch;
        float velocity = msg.velocity / 127.0f;
        
        if (note == 36) {  // KICK
            kickIntensity = velocity;
            
            // 強力な波紋を生成
            ofVec2f kickPos(ofRandom(ofGetWidth()), ofRandom(ofGetHeight()));
            createRipple(kickPos, velocity * 1.5f);
        }
        else if (note == 38) {  // SNARE
            snareIntensity = velocity;
            
            // 複数の波紋を同時生成
            for (int i = 0; i < 3; i++) {
                ofVec2f snarePos(ofRandom(ofGetWidth()), ofRandom(ofGetHeight()));
                createRipple(snarePos, velocity * 0.8f);
            }
        }
        else if (note == 42) {  // HIHAT_CLOSED
            hihatIntensity = velocity;
            
            // 小さな波紋を高頻度で生成
            ofVec2f hihatPos(ofRandom(ofGetWidth()), ofRandom(ofGetHeight()));
            createRipple(hihatPos, velocity * 0.5f);
        }
        else if (note == 49) {  // CRASH
            crashIntensity = velocity;
            
            // 大規模な波紋クラスターを生成
            ofVec2f crashPos(ofRandom(ofGetWidth() * 0.3f, ofGetWidth() * 0.7f),
                            ofRandom(ofGetHeight() * 0.3f, ofGetHeight() * 0.7f));
            createRippleCluster(crashPos, 8, 200.0f * velocity);
            
            // 時間歪曲効果を一時的に強化
            timeDistortionFactor = 1.0f + velocity * 2.0f;
        }
    }
}

void WaterRippleSystem::onBeatDetected(float velocity) {
    // ビート検出で自律的な波紋中心を移動
    for (auto& center : autonomousRippleCenters) {
        float angle = ofRandom(TWO_PI);
        float distance = velocity * 50.0f;
        center += ofVec2f(cos(angle) * distance, sin(angle) * distance);
        
        // 画面内に制限
        center.x = ofClamp(center.x, 50, ofGetWidth() - 50);
        center.y = ofClamp(center.y, 50, ofGetHeight() - 50);
    }
    
    // 波紋の生成頻度を一時的に上げる
    rippleSpawnRate = 0.3f + velocity * 0.5f;
}

void WaterRippleSystem::reset() {
    ripples.clear();
    waterParticles.clear();
    rippleClusters.clear();
    autonomousRippleCenters.clear();
    
    timeDistortionFactor = 1.0f;
    interferencePattern = 0.0f;
    
    setup();
}

void WaterRippleSystem::setGlobalGrowthLevel(float level) {
    globalGrowthLevel = level;
    
    // 成長レベルに基づく異常現象の強化
    gravitationalAnomalyStrength = 0.5f + level * 0.8f;
    quantumFluctuationRate = 0.1f + level * 0.2f;
    rippleInteractionStrength = 0.7f + level * 0.5f;
    
    // 色彩の調整
    waterDark = ofColor(20 + level * 15, 20 + level * 15, 20 + level * 15);
    waterMedium = ofColor(60 + level * 25, 60 + level * 25, 60 + level * 25);
    waterLight = ofColor(100 + level * 30, 100 + level * 30, 100 + level * 30);
    rippleColor = ofColor(140 + level * 20, 140 + level * 20, 140 + level * 20);
    
    // 物理パラメータの調整
    rippleSpeed = 100.0f + level * 50.0f;
    rippleSpawnRate = 0.3f + level * 0.4f;
    autonomousMovementSpeed = 30.0f + level * 40.0f;
}