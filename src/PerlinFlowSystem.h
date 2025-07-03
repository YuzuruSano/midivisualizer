#pragma once

#include "VisualSystem.h"
#include <vector>
#include <deque>

struct PerlinParticle {
    ofVec2f position;
    ofVec2f velocity;
    ofVec2f previousPosition;
    float age = 0.0f;
    float maxAge = 10.0f;
    float size = 1.0f;
    float speed = 1.0f;
    ofColor color;
    float trail = 0.0f;
    
    PerlinParticle(ofVec2f pos = ofVec2f(0, 0)) {
        position = pos;
        previousPosition = pos;
        velocity = ofVec2f(0, 0);
        color = ofColor::white;
        maxAge = ofRandom(5.0f, 15.0f);
        speed = ofRandom(0.5f, 2.0f);
        size = ofRandom(0.5f, 2.0f);
    }
    
    void update(float deltaTime) {
        age += deltaTime;
        previousPosition = position;
        position += velocity * deltaTime * speed;
        
        // Wrap around screen edges
        if (position.x < 0) position.x = ofGetWidth();
        if (position.x > ofGetWidth()) position.x = 0;
        if (position.y < 0) position.y = ofGetHeight();
        if (position.y > ofGetHeight()) position.y = 0;
        
        trail = ofClamp(trail + deltaTime * 2.0f, 0.0f, 1.0f);
    }
    
    bool isDead() const {
        return age > maxAge;
    }
    
    float getLifeRatio() const {
        return age / maxAge;
    }
};

struct FlowField {
    int cols, rows;
    int resolution = 20;
    std::vector<ofVec2f> field;
    float zOffset = 0.0f;
    float noiseScale = 0.005f;
    
    void setup(int width, int height) {
        cols = std::max(1, width / resolution);
        rows = std::max(1, height / resolution);
        field.clear();
        field.resize(cols * rows);
        // Initialize field with default values
        for (int i = 0; i < cols * rows; i++) {
            field[i] = ofVec2f(0, 0);
        }
    }
    
    void update(float deltaTime, float globalGrowth) {
        if (field.empty() || cols <= 0 || rows <= 0) {
            return;
        }
        zOffset += deltaTime * 0.3f;
        
        for (int y = 0; y < rows; y++) {
            for (int x = 0; x < cols; x++) {
                int index = y * cols + x;
                if (index >= 0 && index < field.size()) {
                    float angle = ofNoise(x * noiseScale, y * noiseScale, zOffset) * TWO_PI * 4;
                    field[index] = ofVec2f(cos(angle), sin(angle));
                }
            }
        }
    }
    
    ofVec2f lookup(ofVec2f position) {
        if (field.empty() || cols <= 0 || rows <= 0) {
            return ofVec2f(0, 0);
        }
        int col = ofClamp((int)(position.x / resolution), 0, cols - 1);
        int row = ofClamp((int)(position.y / resolution), 0, rows - 1);
        int index = row * cols + col;
        if (index >= 0 && index < field.size()) {
            return field[index];
        }
        return ofVec2f(0, 0);
    }
    
    void draw(float alpha = 50) {
        if (field.empty() || cols <= 0 || rows <= 0) {
            return;
        }
        ofSetColor(100, 150, 200, alpha); // 青みがかった色に変更
        for (int y = 0; y < rows; y++) {
            for (int x = 0; x < cols; x++) {
                int index = y * cols + x;
                if (index >= 0 && index < field.size()) {
                    ofVec2f pos(x * resolution + resolution/2, y * resolution + resolution/2);
                    ofVec2f vec = field[index] * resolution * 0.3f;
                    ofDrawLine(pos, pos + vec);
                }
            }
        }
    }
};

class PerlinFlowSystem : public VisualSystem {
private:
    std::deque<PerlinParticle> particles;
    FlowField flowField;
    
    // Visual parameters
    float fieldStrength = 1.0f;
    float noiseFrequency = 0.005f;
    float flowComplexity = 1.0f;
    
    // Color parameters
    float hueBase = 200.0f;
    float hueRange = 60.0f;
    float saturationBase = 100.0f;
    float brightnessBase = 150.0f;
    
    // MIDI response
    float fieldTurbulence = 0.0f;
    float particleEmission = 0.0f;
    ofVec2f impactCenter;
    
    // Effects
    float waveEffect = 0.0f;
    float spiralEffect = 0.0f;
    float flashEffect = 0.0f;
    float flashTimer = 0.0f;
    
    // Trail system
    struct Trail {
        std::deque<ofVec2f> points;
        ofColor color;
        float width;
    };
    std::vector<Trail> trails;
    
public:
    void setup() override {
        // Ensure valid dimensions
        int width = std::max(100, ofGetWidth());
        int height = std::max(100, ofGetHeight());
        flowField.setup(width, height);
        
        // Initial particles
        for (int i = 0; i < 100; i++) {
            ofVec2f pos(ofRandom(ofGetWidth()), ofRandom(ofGetHeight()));
            particles.push_back(PerlinParticle(pos));
        }
        
        // Initialize impact center
        impactCenter = ofVec2f(ofGetWidth() * 0.5f, ofGetHeight() * 0.5f);
        
        // Set initial growth level
        globalGrowthLevel = 0.3f;
    }
    
    void update(float deltaTime) override {
        updateGlobalEffects(deltaTime);
        
        // Ensure flow field is initialized
        if (flowField.cols == 0 || flowField.rows == 0) {
            int width = std::max(100, ofGetWidth());
            int height = std::max(100, ofGetHeight());
            flowField.setup(width, height);
        }
        
        // Update flow field
        flowField.noiseScale = noiseFrequency * (1.0f + fieldTurbulence * 2.0f);
        flowField.update(deltaTime, globalGrowthLevel);
        
        // Update particles
        for (auto& particle : particles) {
            // Apply flow field force
            ofVec2f force = flowField.lookup(particle.position);
            force *= fieldStrength * (1.0f + globalGrowthLevel);
            
            // Add spiral force
            if (spiralEffect > 0.1f) {
                ofVec2f center(ofGetWidth() * 0.5f, ofGetHeight() * 0.5f);
                ofVec2f toCenter = center - particle.position;
                float dist = toCenter.length();
                if (dist > 0) {
                    toCenter.normalize();
                    ofVec2f spiral = ofVec2f(-toCenter.y, toCenter.x);
                    force += spiral * spiralEffect * 2.0f;
                }
            }
            
            // Add wave distortion
            if (waveEffect > 0.1f) {
                float waveX = sin(particle.position.y * 0.01f + systemTime * 2.0f) * waveEffect * 10.0f;
                float waveY = cos(particle.position.x * 0.01f + systemTime * 1.5f) * waveEffect * 10.0f;
                force += ofVec2f(waveX, waveY);
            }
            
            particle.velocity = particle.velocity * 0.9f + force * 0.1f;
            particle.update(deltaTime);
            
            // Update color based on velocity and position
            float speed = particle.velocity.length();
            float hue = hueBase + ofNoise(particle.position.x * 0.001f, particle.position.y * 0.001f, systemTime * 0.1f) * hueRange;
            float saturation = saturationBase + speed * 20.0f;
            float brightness = 200 + globalGrowthLevel * 50.0f; // より明るく
            
            particle.color = ofColor::fromHsb(
                fmod(hue, 255),
                ofClamp(saturation, 0, 255),
                ofClamp(brightness, 0, 255)
            );
        }
        
        // Remove dead particles
        particles.erase(
            std::remove_if(particles.begin(), particles.end(),
                [](const PerlinParticle& p) { return p.isDead(); }),
            particles.end()
        );
        
        // Add new particles
        float emissionRate = 2.0f + particleEmission * 20.0f + globalGrowthLevel * 5.0f;
        while (particles.size() < 200 && ofRandom(1.0f) < emissionRate * deltaTime) {
            ofVec2f pos;
            if (impactIntensity > 0.5f) {
                // Emit from impact center
                float angle = ofRandom(TWO_PI);
                float radius = ofRandom(50.0f);
                pos = impactCenter + ofVec2f(cos(angle), sin(angle)) * radius;
            } else {
                // Random position
                pos = ofVec2f(ofRandom(ofGetWidth()), ofRandom(ofGetHeight()));
            }
            particles.push_back(PerlinParticle(pos));
        }
        
        // Update effects
        fieldTurbulence *= 0.95f;
        particleEmission *= 0.9f;
        waveEffect *= 0.92f;
        spiralEffect *= 0.93f;
        
        // Flash effect
        flashTimer += deltaTime;
        flashEffect *= 0.9f;
        if (flashTimer > 3.0f && ofRandom(1.0f) < 0.01f) {
            flashEffect = 1.0f;
            flashTimer = 0.0f;
        }
        
        // Update trails
        updateTrails();
    }
    
    void draw() override {
        beginMasterBuffer();
        
        // Background gradient
        drawBackground();
        
        // Draw flow field (always visible for debugging)
        ofEnableBlendMode(OF_BLENDMODE_ALPHA);
        flowField.draw(30 + 20 * globalGrowthLevel);
        ofDisableBlendMode();
        
        // Draw trails
        drawTrails();
        
        // Draw particles
        drawParticles();
        
        // Draw effects
        if (globalGrowthLevel > 0.5f) {
            drawAdvancedEffects();
        }
        
        endMasterBuffer();
        drawFullscreenEffects();
        
        // Stats
        if (getTimeSinceLastMidi() < 5.0f) {
            ofSetColor(200);
            ofDrawBitmapString("Perlin Flow System", 20, ofGetHeight() - 80);
            ofDrawBitmapString("Particles: " + ofToString(particles.size()), 20, ofGetHeight() - 60);
            ofDrawBitmapString("Field Strength: " + ofToString(fieldStrength, 2), 20, ofGetHeight() - 40);
            ofDrawBitmapString("Turbulence: " + ofToString(fieldTurbulence, 2), 20, ofGetHeight() - 20);
        }
    }
    
    void onMidiMessage(ofxMidiMessage& msg) override {
        if (msg.status == MIDI_NOTE_ON && msg.velocity > 0) {
            currentNote = msg.pitch;
            currentVelocity = msg.velocity;
            
            triggerImpact(msg.pitch, msg.velocity);
            
            // Set impact center
            impactCenter = ofVec2f(
                ofMap(msg.pitch % 12, 0, 12, 100, ofGetWidth() - 100),
                ofMap(msg.pitch / 12, 0, 10, 100, ofGetHeight() - 100)
            );
            
            switch(msg.pitch) {
                case KICK:
                    // Increase field turbulence
                    fieldTurbulence = impactIntensity * 2.0f;
                    fieldStrength = 1.0f + impactIntensity * 3.0f;
                    
                    // Burst of particles
                    for (int i = 0; i < impactIntensity * 20; i++) {
                        float angle = ofRandom(TWO_PI);
                        float radius = ofRandom(100.0f);
                        ofVec2f pos = impactCenter + ofVec2f(cos(angle), sin(angle)) * radius;
                        particles.push_back(PerlinParticle(pos));
                    }
                    break;
                    
                case SNARE:
                    // Wave effect
                    waveEffect = impactIntensity * 2.0f;
                    particleEmission = impactIntensity * 3.0f;
                    break;
                    
                case HIHAT_CLOSED:
                    // Subtle turbulence
                    fieldTurbulence += impactIntensity * 0.5f;
                    noiseFrequency = 0.005f + impactIntensity * 0.01f;
                    break;
                    
                case CRASH:
                    // Major disruption
                    spiralEffect = impactIntensity * 3.0f;
                    flashEffect = 1.0f;
                    
                    // Clear and respawn particles
                    particles.clear();
                    for (int i = 0; i < 100; i++) {
                        ofVec2f pos(ofRandom(ofGetWidth()), ofRandom(ofGetHeight()));
                        particles.push_back(PerlinParticle(pos));
                    }
                    break;
                    
                default:
                    // Localized disturbance
                    particleEmission = impactIntensity * 2.0f;
                    break;
            }
            
        } else if (msg.status == MIDI_CONTROL_CHANGE) {
            if (msg.control == 1) { // Mod wheel
                modulation = mapCC(msg.value);
                flowComplexity = 1.0f + modulation * 2.0f;
            }
        }
    }
    
private:
    void updateTrails() {
        // Add trails for fast-moving particles
        trails.clear();
        for (auto& particle : particles) {
            if (particle.velocity.length() > 5.0f && particle.trail > 0.5f) {
                Trail trail;
                trail.color = particle.color;
                trail.width = particle.size * 0.5f;
                
                // Create trail points
                ofVec2f dir = particle.velocity;
                if (dir.length() > 0.001f) {
                    dir = dir.getNormalized();
                    for (int i = 0; i < 10; i++) {
                        float t = i / 10.0f;
                        ofVec2f pos = particle.position - dir * t * 20.0f;
                        trail.points.push_back(pos);
                    }
                }
                
                trails.push_back(trail);
            }
        }
    }
    
    void drawBackground() {
        // beginMasterBuffer()で既に背景が描画されているため、
        // 追加の背景は描画しない
        // 必要に応じてエフェクトのみ追加
    }
    
    void drawTrails() {
        ofEnableBlendMode(OF_BLENDMODE_ALPHA);
        
        for (auto& trail : trails) {
            ofSetColor(trail.color.r, trail.color.g, trail.color.b, 60);
            ofSetLineWidth(trail.width);
            
            ofBeginShape();
            for (auto& point : trail.points) {
                ofVertex(point.x, point.y);
            }
            ofEndShape(false);
        }
        
        ofDisableBlendMode();
    }
    
    void drawParticles() {
        ofEnableBlendMode(OF_BLENDMODE_ALPHA);
        
        for (auto& particle : particles) {
            float alpha = (1.0f - particle.getLifeRatio() * 0.5f) * 255.0f; // より不透明に
            
            if (flashEffect > 0.5f) {
                ofSetColor(255, 255, 255, alpha * flashEffect);
            } else {
                ofSetColor(particle.color.r, particle.color.g, particle.color.b, alpha);
            }
            
            float size = particle.size * (1.0f + particle.velocity.length() * 0.1f);
            
            // Draw as stretched ellipse in direction of motion
            if (particle.velocity.length() > 1.0f) {
                ofPushMatrix();
                ofTranslate(particle.position.x, particle.position.y);
                
                float angle = atan2(particle.velocity.y, particle.velocity.x);
                ofRotateDeg(ofRadToDeg(angle));
                
                float stretch = 1.0f + particle.velocity.length() * 0.2f;
                ofDrawEllipse(0, 0, size * stretch, size);
                
                ofPopMatrix();
            } else {
                ofDrawCircle(particle.position, size);
            }
            
            // Motion blur trail
            if (particle.velocity.length() > 3.0f) {
                ofSetColor(particle.color.r, particle.color.g, particle.color.b, alpha * 0.3f);
                ofSetLineWidth(size * 0.5f);
                ofDrawLine(particle.previousPosition, particle.position);
            }
        }
        
        ofDisableBlendMode();
    }
    
    void drawAdvancedEffects() {
        ofEnableBlendMode(OF_BLENDMODE_ALPHA);
        
        // Vortex visualization
        if (spiralEffect > 0.5f) {
            ofVec2f center(ofGetWidth() * 0.5f, ofGetHeight() * 0.5f);
            
            ofNoFill();
            ofSetColor(255, 100 * spiralEffect);
            ofSetLineWidth(2.0f);
            
            for (int i = 0; i < 5; i++) {
                float radius = 50 + i * 40 * spiralEffect;
                float startAngle = systemTime * (i + 1) * 0.5f;
                
                ofBeginShape();
                for (int j = 0; j <= 36; j++) {
                    float angle = startAngle + (j / 36.0f) * TWO_PI;
                    float r = radius + sin(angle * 3 + systemTime) * 20 * spiralEffect;
                    float x = center.x + cos(angle) * r;
                    float y = center.y + sin(angle) * r;
                    ofVertex(x, y);
                }
                ofEndShape();
            }
            
            ofFill();
        }
        
        // Wave distortion overlay
        if (waveEffect > 0.3f) {
            ofSetColor(100, 150, 200, 50 * waveEffect);
            
            for (int y = 0; y < ofGetHeight(); y += 20) {
                ofBeginShape();
                for (int x = 0; x <= ofGetWidth(); x += 10) {
                    float offset = sin(x * 0.01f + systemTime * 2.0f) * 10 * waveEffect;
                    ofVertex(x, y + offset);
                }
                for (int x = ofGetWidth(); x >= 0; x -= 10) {
                    float offset = sin(x * 0.01f + systemTime * 2.0f) * 10 * waveEffect;
                    ofVertex(x, y + 20 + offset);
                }
                ofEndShape();
            }
        }
        
        // Energy bursts at high growth
        if (globalGrowthLevel > 0.7f) {
            for (int i = 0; i < 3; i++) {
                float phase = systemTime * 0.5f + i * TWO_PI / 3.0f;
                float x = ofGetWidth() * 0.5f + cos(phase) * 200;
                float y = ofGetHeight() * 0.5f + sin(phase) * 200;
                
                ofSetColor(200, 150, 255, 100 * globalGrowthLevel);
                float size = 20 + sin(systemTime * 3 + i) * 10;
                ofDrawCircle(x, y, size);
                
                // Radiating lines
                ofSetLineWidth(1.0f);
                for (int j = 0; j < 8; j++) {
                    float angle = (j / 8.0f) * TWO_PI + systemTime;
                    ofVec2f lineEnd(
                        x + cos(angle) * size * 2,
                        y + sin(angle) * size * 2
                    );
                    ofDrawLine(x, y, lineEnd.x, lineEnd.y);
                }
            }
        }
        
        ofDisableBlendMode();
    }
};