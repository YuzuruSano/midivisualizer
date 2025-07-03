#include "SandParticleSystem.h"

SandParticleSystem::SandParticleSystem() {
    gravityStrength = 200.0f;
    windStrength = 80.0f;
    frictionCoefficient = 0.95f;
    particleInteractionRadius = 15.0f;
    sandDensity = 0.8f;
    
    // モノトーンカラーパレット
    sandDark = ofColor(40, 40, 40);
    sandMedium = ofColor(90, 90, 90);
    sandLight = ofColor(140, 140, 140);
    dustColor = ofColor(180, 180, 180);
    shadowColor = ofColor(20, 20, 20);
    
    patternSpawnRate = 0.1f;
    patternComplexity = 0.7f;
    patternSymmetry = 0.8f;
    patternScale = 100.0f;
    
    dejavu_trigger_probability = 0.05f;
    dejavu_fade_rate = 0.02f;
    
    airResistance = 0.98f;
    particleCollisionRadius = 3.0f;
    clusteringTendency = 0.3f;
    erosionRate = 0.01f;
    
    ambientHeat = 0.5f;
    mirageEffect = 0.2f;
    dustStormIntensity = 0.0f;
    
    kickIntensity = 0.0f;
    snareIntensity = 0.0f;
    hihatIntensity = 0.0f;
    crashIntensity = 0.0f;
}

void SandParticleSystem::setup() {
    // 初期砂丘の生成
    for (int i = 0; i < 5; i++) {
        SandDune dune;
        dune.position.set(ofRandom(ofGetWidth()), ofRandom(ofGetHeight() * 0.7f, ofGetHeight()));
        dune.width = ofRandom(150, 300);
        dune.height = ofRandom(30, 80);
        dune.slope = ofRandom(0.2f, 0.5f);
        dune.windResistance = ofRandom(0.5f, 0.9f);
        dune.stability = ofRandom(0.6f, 1.0f);
        
        // 砂丘プロファイルの生成
        for (int j = 0; j < 20; j++) {
            float x = (float)j / 19.0f * dune.width - dune.width * 0.5f;
            float y = -dune.height * exp(-x * x / (dune.width * dune.width * 0.25f));
            dune.profile.push_back(ofVec2f(x, y));
        }
        
        dunes.push_back(dune);
    }
    
    // 初期風場の生成
    for (int i = 0; i < 3; i++) {
        WindField field;
        field.position.set(ofRandom(ofGetWidth()), ofRandom(ofGetHeight()));
        field.direction.set(ofRandom(-1, 1), ofRandom(-1, 1));
        field.direction.normalize();
        field.strength = ofRandom(30, 80);
        field.turbulence = ofRandom(0.1f, 0.4f);
        field.radius = ofRandom(150, 300);
        windFields.push_back(field);
    }
    
    // 初期粒子の生成
    for (int i = 0; i < 150; i++) {
        ofVec2f pos(ofRandom(ofGetWidth()), ofRandom(ofGetHeight()));
        createSandParticle(pos);
    }
    
    // 既視感パターンの事前生成
    for (int i = 0; i < 5; i++) {
        std::vector<ofVec2f> pattern;
        
        // フラクタル的な既視感パターン
        int numPoints = 12 + i * 8;
        for (int j = 0; j < numPoints; j++) {
            float angle = (float)j / numPoints * TWO_PI;
            float radius = 50.0f + sin(angle * 3.0f) * 20.0f;
            pattern.push_back(ofVec2f(cos(angle) * radius, sin(angle) * radius));
        }
        
        dejavu_patterns.push_back(pattern);
    }
}

void SandParticleSystem::update(float deltaTime) {
    // 粒子の更新
    updateParticles(deltaTime);
    
    // 砂丘の更新
    updateDunes(deltaTime);
    
    // パターンの更新
    updatePatterns(deltaTime);
    
    // 風場の更新
    updateWindFields(deltaTime);
    
    // 粒子間相互作用の適用
    applyParticleInteractions(deltaTime);
    
    // 侵食シミュレーション
    simulateErosion();
    
    // 既視感パターンの生成
    if (ofRandom(1.0f) < dejavu_trigger_probability * deltaTime) {
        generateDejavuPattern();
    }
    
    // 新しい粒子の生成
    if (ofRandom(1.0f) < 0.3f * deltaTime) {
        ofVec2f spawnPos(ofRandom(ofGetWidth()), ofRandom(ofGetHeight() * 0.3f));
        createSandParticle(spawnPos, ofVec2f(ofRandom(-30, 30), ofRandom(10, 50)));
    }
    
    // パターンの自動生成
    if (ofRandom(1.0f) < patternSpawnRate * deltaTime) {
        ofVec2f patternCenter(ofRandom(ofGetWidth()), ofRandom(ofGetHeight()));
        int patternType = (int)ofRandom(3);
        
        switch (patternType) {
            case 0: createFractalPattern(patternCenter, patternScale, 3); break;
            case 1: createSpiraPattern(patternCenter, patternScale * 0.8f, 5); break;
            case 2: createMandalaPatter(patternCenter, patternScale * 0.6f, 8); break;
        }
    }
    
    // MIDI連動強度の減衰
    kickIntensity *= 0.88f;
    snareIntensity *= 0.85f;
    hihatIntensity *= 0.90f;
    crashIntensity *= 0.82f;
    
    // 成長レベルに基づく環境調整
    dustStormIntensity = globalGrowthLevel * 0.5f;
    patternComplexity = 0.7f + globalGrowthLevel * 0.4f;
    
    // 非活性要素の削除
    cleanupInactiveElements();
}

void SandParticleSystem::draw() {
    ofPushMatrix();
    
    // 背景（砂漠の空）
    ofBackground(sandDark.r * 0.8f, sandDark.g * 0.8f, sandDark.b * 0.8f);
    
    // 風の可視化
    drawWindVisualization();
    
    // 砂丘の描画
    drawDunes();
    
    // 粒子軌跡の描画
    drawParticleTrails();
    
    // 既視感パターンの描画
    drawPatterns();
    
    // 砂粒子の描画
    drawParticles();
    
    ofPopMatrix();
}

void SandParticleSystem::createSandParticle(ofVec2f position, ofVec2f velocity) {
    SandParticle particle;
    particle.position = position;
    particle.velocity = velocity;
    particle.acceleration.set(0, 0);
    particle.life = ofRandom(3.0f, 8.0f);
    particle.maxLife = particle.life;
    particle.size = ofRandom(1.0f, 3.0f);
    particle.mass = ofRandom(0.8f, 1.2f);
    particle.alpha = 255.0f;
    particle.isActive = true;
    
    // 色のバリエーション
    int colorChoice = (int)ofRandom(3);
    switch (colorChoice) {
        case 0: particle.particleColor = sandDark; break;
        case 1: particle.particleColor = sandMedium; break;
        case 2: particle.particleColor = sandLight; break;
    }
    
    particles.push_back(particle);
}

void SandParticleSystem::createParticleCluster(ofVec2f center, int count, float spread) {
    for (int i = 0; i < count; i++) {
        float angle = ofRandom(TWO_PI);
        float distance = ofRandom(0, spread);
        ofVec2f pos = center + ofVec2f(cos(angle) * distance, sin(angle) * distance);
        
        ofVec2f vel(ofRandom(-20, 20), ofRandom(-30, 10));
        createSandParticle(pos, vel);
    }
}

void SandParticleSystem::updateParticles(float deltaTime) {
    for (auto& particle : particles) {
        if (!particle.isActive) continue;
        
        particle.life -= deltaTime;
        if (particle.life <= 0.0f) {
            particle.isActive = false;
            continue;
        }
        
        // 重力の適用
        particle.acceleration.y += gravityStrength * deltaTime;
        
        // 風力の適用
        applyWindForce(particle, deltaTime);
        
        // 速度の更新
        particle.velocity += particle.acceleration * deltaTime;
        
        // 空気抵抗
        particle.velocity *= airResistance;
        
        // 位置の更新
        particle.position += particle.velocity * deltaTime;
        
        // 加速度のリセット
        particle.acceleration.set(0, 0);
        
        // 地面との衝突
        if (particle.position.y > ofGetHeight()) {
            particle.position.y = ofGetHeight();
            particle.velocity.y *= -0.3f;
            particle.velocity.x *= frictionCoefficient;
        }
        
        // 画面端の処理
        if (particle.position.x < 0) {
            particle.position.x = 0;
            particle.velocity.x *= -0.5f;
        } else if (particle.position.x > ofGetWidth()) {
            particle.position.x = ofGetWidth();
            particle.velocity.x *= -0.5f;
        }
        
        // 透明度の更新
        float lifeRatio = particle.life / particle.maxLife;
        particle.alpha = 255.0f * lifeRatio;
        
        // 軌跡の記録
        if (particle.velocity.length() > 10.0f) {
            particleTrails.push_back(particle.position);
            if (particleTrails.size() > 200) {
                particleTrails.pop_front();
            }
        }
    }
}

void SandParticleSystem::updateDunes(float deltaTime) {
    for (auto& dune : dunes) {
        // 風による砂丘の変形
        for (auto& field : windFields) {
            float distance = dune.position.distance(field.position);
            if (distance < field.radius) {
                float windEffect = (field.radius - distance) / field.radius;
                windEffect *= field.strength * deltaTime * 0.01f;
                
                // 砂丘の高さと形状を調整
                dune.height += windEffect * (1.0f - dune.windResistance);
                dune.height = ofClamp(dune.height, 10.0f, 120.0f);
                
                // 砂丘の移動
                ofVec2f moveDirection = field.direction * windEffect * 0.1f;
                dune.position += moveDirection;
            }
        }
        
        // 砂丘の安定性チェック
        if (dune.stability < 0.3f) {
            // 不安定な砂丘から粒子を放出
            for (int i = 0; i < 5; i++) {
                ofVec2f emitPos = dune.position + ofVec2f(ofRandom(-dune.width * 0.5f, dune.width * 0.5f), 0);
                createSandParticle(emitPos, ofVec2f(ofRandom(-50, 50), ofRandom(-30, 0)));
            }
        }
        
        // 砂丘の境界制限
        dune.position.x = ofClamp(dune.position.x, 50, ofGetWidth() - 50);
        dune.position.y = ofClamp(dune.position.y, ofGetHeight() * 0.5f, ofGetHeight() - 20);
    }
}

void SandParticleSystem::updatePatterns(float deltaTime) {
    for (auto& pattern : patterns) {
        if (!pattern.isActive) continue;
        
        float age = ofGetElapsedTimef() - pattern.creationTime;
        if (age > pattern.lifetime) {
            pattern.isActive = false;
            continue;
        }
        
        // パターンの回転
        pattern.rotation += deltaTime * 0.5f;
        
        // パターンのスケール変化
        float scalePhase = sin(age * 2.0f) * 0.1f + 1.0f;
        pattern.scale = scalePhase;
        
        // 透明度の変化
        float fadeRatio = 1.0f - (age / pattern.lifetime);
        pattern.alpha = 255.0f * fadeRatio * fadeRatio;
        
        // 既視感効果のためのゆらぎ
        pattern.center.x += sin(age * 3.0f) * 0.5f;
        pattern.center.y += cos(age * 2.5f) * 0.3f;
    }
}

void SandParticleSystem::updateWindFields(float deltaTime) {
    for (auto& field : windFields) {
        // 風向きの変化
        float directionChange = ofNoise(ofGetElapsedTimef() * 0.3f, field.position.x * 0.001f) * 0.2f - 0.1f;
        field.direction.rotate(directionChange);
        
        // 風力の変化
        field.strength += sin(ofGetElapsedTimef() * 0.7f + field.position.y * 0.001f) * 10.0f * deltaTime;
        field.strength = ofClamp(field.strength, 20.0f, 100.0f);
        
        // 乱流の更新
        field.turbulence = 0.2f + ofNoise(ofGetElapsedTimef() * 0.5f, field.position.x * 0.002f) * 0.3f;
        
        // 風場の移動
        field.position.x += field.direction.x * 20.0f * deltaTime;
        field.position.y += field.direction.y * 10.0f * deltaTime;
        
        // 境界での反射
        if (field.position.x < 0 || field.position.x > ofGetWidth()) {
            field.direction.x *= -1;
            field.position.x = ofClamp(field.position.x, 0, ofGetWidth());
        }
        if (field.position.y < 0 || field.position.y > ofGetHeight()) {
            field.direction.y *= -1;
            field.position.y = ofClamp(field.position.y, 0, ofGetHeight());
        }
    }
}

void SandParticleSystem::applyWindForce(SandParticle& particle, float deltaTime) {
    for (const auto& field : windFields) {
        float distance = particle.position.distance(field.position);
        if (distance < field.radius) {
            float windEffect = (field.radius - distance) / field.radius;
            windEffect *= field.strength / particle.mass;
            
            // 基本的な風力
            ofVec2f windForce = field.direction * windEffect;
            
            // 乱流効果
            float turbulenceX = ofNoise(particle.position.x * 0.01f, ofGetElapsedTimef() * 2.0f) * 2.0f - 1.0f;
            float turbulenceY = ofNoise(particle.position.y * 0.01f, ofGetElapsedTimef() * 2.0f + 100) * 2.0f - 1.0f;
            windForce += ofVec2f(turbulenceX, turbulenceY) * field.turbulence * windEffect;
            
            particle.acceleration += windForce * deltaTime;
        }
    }
}

void SandParticleSystem::applyParticleInteractions(float deltaTime) {
    for (int i = 0; i < particles.size(); i++) {
        if (!particles[i].isActive) continue;
        
        for (int j = i + 1; j < particles.size(); j++) {
            if (!particles[j].isActive) continue;
            
            float distance = particles[i].position.distance(particles[j].position);
            if (distance < particleInteractionRadius && distance > 0) {
                // 粒子間の相互作用力（クラスタリング）
                ofVec2f direction = particles[j].position - particles[i].position;
                direction.normalize();
                
                float interactionStrength = (particleInteractionRadius - distance) / particleInteractionRadius;
                interactionStrength *= clusteringTendency;
                
                // 引力（クラスタリング）
                particles[i].acceleration += direction * interactionStrength * 10.0f * deltaTime;
                particles[j].acceleration -= direction * interactionStrength * 10.0f * deltaTime;
                
                // 衝突回避
                if (distance < particleCollisionRadius) {
                    float pushForce = (particleCollisionRadius - distance) / particleCollisionRadius * 50.0f;
                    particles[i].acceleration -= direction * pushForce * deltaTime;
                    particles[j].acceleration += direction * pushForce * deltaTime;
                }
            }
        }
    }
}

void SandParticleSystem::drawParticles() {
    ofFill();
    
    for (const auto& particle : particles) {
        if (!particle.isActive) continue;
        
        ofSetColor(particle.particleColor.r, particle.particleColor.g, particle.particleColor.b, particle.alpha);
        ofDrawCircle(particle.position.x, particle.position.y, particle.size);
        
        // 高速移動時の軌跡エフェクト
        if (particle.velocity.length() > 20.0f) {
            ofSetColor(particle.particleColor.r, particle.particleColor.g, particle.particleColor.b, particle.alpha * 0.3f);
            ofVec2f trailEnd = particle.position - particle.velocity.getNormalized() * 10.0f;
            ofDrawLine(particle.position.x, particle.position.y, trailEnd.x, trailEnd.y);
        }
    }
}

void SandParticleSystem::drawDunes() {
    ofSetColor(sandMedium.r, sandMedium.g, sandMedium.b, 150);
    ofFill();
    
    for (const auto& dune : dunes) {
        ofPushMatrix();
        ofTranslate(dune.position.x, dune.position.y);
        
        // 砂丘の形状描画
        ofBeginShape();
        for (const auto& point : dune.profile) {
            ofVertex(point.x, point.y);
        }
        ofEndShape(true);
        
        // 砂丘の影
        ofSetColor(shadowColor.r, shadowColor.g, shadowColor.b, 80);
        ofPushMatrix();
        ofTranslate(5, 5);
        ofBeginShape();
        for (const auto& point : dune.profile) {
            ofVertex(point.x, point.y);
        }
        ofEndShape(true);
        ofPopMatrix();
        
        ofPopMatrix();
    }
}

void SandParticleSystem::drawPatterns() {
    ofNoFill();
    ofSetLineWidth(2.0f);
    
    for (const auto& pattern : patterns) {
        if (!pattern.isActive) continue;
        
        ofPushMatrix();
        ofTranslate(pattern.center.x, pattern.center.y);
        ofRotateDeg(pattern.rotation * 180.0f / PI);
        ofScale(pattern.scale, pattern.scale);
        
        ofSetColor(pattern.elementColor.r, pattern.elementColor.g, pattern.elementColor.b, pattern.alpha);
        
        // パターンの描画
        if (pattern.points.size() > 2) {
            ofBeginShape();
            for (const auto& point : pattern.points) {
                ofVertex(point.x, point.y);
            }
            ofEndShape(true);
        }
        
        // 既視感効果のための重複描画
        ofSetColor(pattern.elementColor.r, pattern.elementColor.g, pattern.elementColor.b, pattern.alpha * 0.5f);
        ofPushMatrix();
        ofTranslate(sin(ofGetElapsedTimef() * 2.0f) * 3.0f, cos(ofGetElapsedTimef() * 1.5f) * 2.0f);
        ofScale(0.95f, 0.95f);
        if (pattern.points.size() > 2) {
            ofBeginShape();
            for (const auto& point : pattern.points) {
                ofVertex(point.x, point.y);
            }
            ofEndShape(true);
        }
        ofPopMatrix();
        
        ofPopMatrix();
    }
}

void SandParticleSystem::drawWindVisualization() {
    ofSetColor(dustColor.r, dustColor.g, dustColor.b, 30);
    ofSetLineWidth(1.0f);
    
    for (const auto& field : windFields) {
        // 風場の可視化
        int numLines = 12;
        for (int i = 0; i < numLines; i++) {
            float angle = (float)i / numLines * TWO_PI;
            float innerRadius = field.radius * 0.3f;
            float outerRadius = field.radius * 0.8f;
            
            ofVec2f start = field.position + ofVec2f(cos(angle) * innerRadius, sin(angle) * innerRadius);
            ofVec2f end = field.position + ofVec2f(cos(angle) * outerRadius, sin(angle) * outerRadius);
            
            // 風向きに基づく偏向
            ofVec2f windOffset = field.direction * field.strength * 0.3f;
            end += windOffset;
            
            ofDrawLine(start.x, start.y, end.x, end.y);
        }
        
        // 風場の中心
        ofSetColor(dustColor.r, dustColor.g, dustColor.b, 60);
        ofDrawCircle(field.position.x, field.position.y, 5);
    }
}

void SandParticleSystem::drawParticleTrails() {
    if (particleTrails.size() < 2) return;
    
    ofSetColor(dustColor.r, dustColor.g, dustColor.b, 40);
    ofSetLineWidth(1.0f);
    
    for (int i = 1; i < particleTrails.size(); i++) {
        float alpha = (float)i / particleTrails.size() * 40.0f;
        ofSetColor(dustColor.r, dustColor.g, dustColor.b, alpha);
        
        ofDrawLine(particleTrails[i-1].x, particleTrails[i-1].y,
                   particleTrails[i].x, particleTrails[i].y);
    }
}

void SandParticleSystem::generateDejavuPattern() {
    if (dejavu_patterns.empty()) return;
    
    PatternElement pattern;
    pattern.center.set(ofRandom(ofGetWidth()), ofRandom(ofGetHeight()));
    pattern.creationTime = ofGetElapsedTimef();
    pattern.lifetime = ofRandom(4.0f, 8.0f);
    pattern.scale = ofRandom(0.5f, 1.5f);
    pattern.rotation = ofRandom(TWO_PI);
    pattern.elementColor = ofColor(120 + ofRandom(-30, 30), 120 + ofRandom(-30, 30), 120 + ofRandom(-30, 30));
    pattern.alpha = 255.0f;
    pattern.isActive = true;
    
    // 既視感パターンから選択
    int patternIndex = (int)ofRandom(dejavu_patterns.size());
    pattern.points = dejavu_patterns[patternIndex];
    
    patterns.push_back(pattern);
}

void SandParticleSystem::createFractalPattern(ofVec2f center, float scale, int depth) {
    if (depth <= 0) return;
    
    PatternElement pattern;
    pattern.center = center;
    pattern.creationTime = ofGetElapsedTimef();
    pattern.lifetime = ofRandom(5.0f, 10.0f);
    pattern.scale = 1.0f;
    pattern.rotation = ofRandom(TWO_PI);
    pattern.elementColor = sandLight;
    pattern.alpha = 255.0f;
    pattern.isActive = true;
    
    // フラクタル形状の生成
    int numPoints = 8;
    for (int i = 0; i < numPoints; i++) {
        float angle = (float)i / numPoints * TWO_PI;
        float radius = scale * (0.5f + 0.5f * sin(angle * 3.0f));
        pattern.points.push_back(ofVec2f(cos(angle) * radius, sin(angle) * radius));
    }
    
    patterns.push_back(pattern);
    
    // 再帰的にサブパターンを生成
    for (int i = 0; i < 3; i++) {
        float subAngle = (float)i / 3.0f * TWO_PI;
        ofVec2f subCenter = center + ofVec2f(cos(subAngle) * scale * 0.7f, sin(subAngle) * scale * 0.7f);
        createFractalPattern(subCenter, scale * 0.4f, depth - 1);
    }
}

void SandParticleSystem::createSpiraPattern(ofVec2f center, float radius, int arms) {
    PatternElement pattern;
    pattern.center = center;
    pattern.creationTime = ofGetElapsedTimef();
    pattern.lifetime = ofRandom(6.0f, 12.0f);
    pattern.scale = 1.0f;
    pattern.rotation = 0.0f;
    pattern.elementColor = sandMedium;
    pattern.alpha = 255.0f;
    pattern.isActive = true;
    
    // スパイラルパターンの生成
    float totalAngle = arms * TWO_PI;
    int numPoints = arms * 16;
    
    for (int i = 0; i < numPoints; i++) {
        float t = (float)i / numPoints;
        float angle = t * totalAngle;
        float r = radius * t;
        pattern.points.push_back(ofVec2f(cos(angle) * r, sin(angle) * r));
    }
    
    patterns.push_back(pattern);
}

void SandParticleSystem::createMandalaPatter(ofVec2f center, float radius, int segments) {
    PatternElement pattern;
    pattern.center = center;
    pattern.creationTime = ofGetElapsedTimef();
    pattern.lifetime = ofRandom(8.0f, 15.0f);
    pattern.scale = 1.0f;
    pattern.rotation = 0.0f;
    pattern.elementColor = sandDark;
    pattern.alpha = 255.0f;
    pattern.isActive = true;
    
    // マンダラパターンの生成
    for (int layer = 0; layer < 3; layer++) {
        float layerRadius = radius * (0.3f + 0.35f * layer);
        int layerSegments = segments * (layer + 1);
        
        for (int i = 0; i < layerSegments; i++) {
            float angle = (float)i / layerSegments * TWO_PI;
            float r = layerRadius * (0.8f + 0.2f * sin(angle * segments));
            pattern.points.push_back(ofVec2f(cos(angle) * r, sin(angle) * r));
        }
    }
    
    patterns.push_back(pattern);
}

void SandParticleSystem::simulateErosion() {
    // 簡単な侵食シミュレーション
    for (auto& dune : dunes) {
        dune.stability -= erosionRate * 0.1f;
        if (dune.stability < 0.1f) {
            dune.stability = 0.1f;
        }
        
        // 侵食により砂丘が縮小
        dune.height *= (1.0f - erosionRate * 0.5f);
        if (dune.height < 20.0f) {
            dune.height = 20.0f;
        }
    }
}

void SandParticleSystem::cleanupInactiveElements() {
    // 非活性な粒子を削除
    particles.erase(std::remove_if(particles.begin(), particles.end(),
                                   [](const SandParticle& p) { return !p.isActive; }),
                    particles.end());
    
    // 非活性なパターンを削除
    patterns.erase(std::remove_if(patterns.begin(), patterns.end(),
                                  [](const PatternElement& p) { return !p.isActive; }),
                   patterns.end());
}

void SandParticleSystem::onMidiMessage(ofxMidiMessage& msg) {
    if (msg.status == MIDI_NOTE_ON) {
        int note = msg.pitch;
        float velocity = msg.velocity / 127.0f;
        
        if (note == 36) {  // KICK
            kickIntensity = velocity;
            
            // 強力な粒子クラスターを生成
            ofVec2f kickPos(ofRandom(ofGetWidth()), ofRandom(ofGetHeight()));
            createParticleCluster(kickPos, 30 * velocity, 80.0f * velocity);
        }
        else if (note == 38) {  // SNARE
            snareIntensity = velocity;
            
            // 風場の強度を一時的に増加
            for (auto& field : windFields) {
                field.strength += velocity * 50.0f;
            }
            
            // 複数の小さなクラスターを生成
            for (int i = 0; i < 3; i++) {
                ofVec2f snarePos(ofRandom(ofGetWidth()), ofRandom(ofGetHeight()));
                createParticleCluster(snarePos, 15 * velocity, 40.0f * velocity);
            }
        }
        else if (note == 42) {  // HIHAT_CLOSED
            hihatIntensity = velocity;
            
            // 細かい粒子を連続生成
            for (int i = 0; i < 10; i++) {
                ofVec2f hihatPos(ofRandom(ofGetWidth()), ofRandom(ofGetHeight() * 0.5f));
                createSandParticle(hihatPos, ofVec2f(ofRandom(-20, 20), ofRandom(-10, 10)));
            }
        }
        else if (note == 49) {  // CRASH
            crashIntensity = velocity;
            
            // 大規模な砂嵐効果
            dustStormIntensity = velocity;
            
            // 全ての風場を活性化
            for (auto& field : windFields) {
                field.strength = 100.0f * velocity;
                field.turbulence = 0.5f * velocity;
            }
            
            // 既視感パターンを複数生成
            for (int i = 0; i < 5; i++) {
                generateDejavuPattern();
            }
            
            // 大量の粒子を生成
            for (int i = 0; i < 50; i++) {
                ofVec2f crashPos(ofRandom(ofGetWidth()), ofRandom(ofGetHeight()));
                createSandParticle(crashPos, ofVec2f(ofRandom(-100, 100), ofRandom(-50, 50)));
            }
        }
    }
}

void SandParticleSystem::onBeatDetected(float velocity) {
    // ビート検出で風場を同期
    for (auto& field : windFields) {
        field.strength += velocity * 20.0f;
        
        // 風向きを変更
        float angleChange = velocity * 0.5f;
        field.direction.rotate(angleChange);
    }
    
    // パターン生成頻度を一時的に上げる
    patternSpawnRate = 0.1f + velocity * 0.3f;
    
    // 既視感パターンの生成確率を上げる
    dejavu_trigger_probability = 0.05f + velocity * 0.1f;
}

void SandParticleSystem::reset() {
    particles.clear();
    patterns.clear();
    particleTrails.clear();
    
    dustStormIntensity = 0.0f;
    
    setup();
}

void SandParticleSystem::setGlobalGrowthLevel(float level) {
    globalGrowthLevel = level;
    
    // 成長レベルに基づく色彩の調整
    sandDark = ofColor(40 + level * 20, 40 + level * 20, 40 + level * 20);
    sandMedium = ofColor(90 + level * 30, 90 + level * 30, 90 + level * 30);
    sandLight = ofColor(140 + level * 25, 140 + level * 25, 140 + level * 25);
    
    // 物理パラメータの調整
    windStrength = 80.0f + level * 40.0f;
    particleInteractionRadius = 15.0f + level * 10.0f;
    clusteringTendency = 0.3f + level * 0.4f;
    
    // パターンの複雑度調整
    patternComplexity = 0.7f + level * 0.4f;
    patternSpawnRate = 0.1f + level * 0.2f;
    
    // 既視感効果の強度調整
    dejavu_trigger_probability = 0.05f + level * 0.1f;
    dejavu_fade_rate = 0.02f + level * 0.03f;
}