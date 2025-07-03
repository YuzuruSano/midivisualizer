#pragma once

#include "VisualSystem.h"
#include <vector>
#include <deque>

struct CurlParticle {
    ofVec2f position;
    ofVec2f velocity;
    float age = 0.0f;
    float maxAge = 8.0f;
    float size = 1.0f;
    ofColor color;
    float energy = 1.0f;
    int trailLength = 20;
    std::deque<ofVec2f> trail;
    
    CurlParticle(ofVec2f pos = ofVec2f(0, 0)) {
        position = pos;
        velocity = ofVec2f(0, 0);
        color = ofColor::white;
        maxAge = ofRandom(5.0f, 12.0f);
        size = ofRandom(0.3f, 1.5f);
        energy = ofRandom(0.5f, 1.0f);
        trailLength = ofRandom(10, 30);
    }
    
    void update(float deltaTime) {
        age += deltaTime;
        position += velocity * deltaTime;
        
        // Add to trail
        trail.push_back(position);
        if (trail.size() > trailLength) {
            trail.pop_front();
        }
        
        // Energy decay
        energy *= 0.995f;
        
        // Boundary wrapping
        if (position.x < 0) position.x = ofGetWidth();
        if (position.x > ofGetWidth()) position.x = 0;
        if (position.y < 0) position.y = ofGetHeight();
        if (position.y > ofGetHeight()) position.y = 0;
    }
    
    bool isDead() const {
        return age > maxAge || energy < 0.01f;
    }
    
    float getLifeRatio() const {
        return age / maxAge;
    }
};

struct VortexCore {
    ofVec2f position;
    float strength;
    float radius;
    float rotation;
    float oscillation;
    
    VortexCore(ofVec2f pos) {
        position = pos;
        strength = ofRandom(0.5f, 2.0f);
        radius = ofRandom(50.0f, 150.0f);
        rotation = 0.0f;
        oscillation = ofRandom(0.1f, 0.5f);
    }
    
    void update(float deltaTime) {
        rotation += deltaTime * strength;
        position.x += sin(rotation * oscillation) * deltaTime * 10.0f;
        position.y += cos(rotation * oscillation * 0.7f) * deltaTime * 10.0f;
        
        // Keep within bounds
        if (position.x < radius) position.x = radius;
        if (position.x > ofGetWidth() - radius) position.x = ofGetWidth() - radius;
        if (position.y < radius) position.y = radius;
        if (position.y > ofGetHeight() - radius) position.y = ofGetHeight() - radius;
    }
};

class CurlNoiseSystem : public VisualSystem {
private:
    std::vector<CurlParticle> particles;
    std::vector<VortexCore> vortices;
    
    // Noise field parameters
    float noiseScale = 0.003f;
    float curlScale = 2.0f;
    float timeScale = 0.2f;
    float zOffset = 0.0f;
    
    // Visual parameters
    float flowSpeed = 100.0f;
    float particleDensity = 1.0f;
    float trailOpacity = 0.5f;
    
    // Color scheme
    float hueShift = 0.0f;
    float colorComplexity = 1.0f;
    
    // Effects
    float turbulence = 0.0f;
    float vortexStrength = 1.0f;
    float fieldDistortion = 0.0f;
    float flashEffect = 0.0f;
    float flashTimer = 0.0f;
    ofVec2f impactCenter;
    
    // Advanced effects
    std::vector<ofVec2f> attractors;
    std::vector<ofVec2f> repellers;
    float attractorStrength = 0.0f;
    
public:
    void setup() override {
        // Initialize particles
        for (int i = 0; i < 150; i++) {
            ofVec2f pos(ofRandom(ofGetWidth()), ofRandom(ofGetHeight()));
            particles.push_back(CurlParticle(pos));
        }
        
        // Initialize vortices
        for (int i = 0; i < 3; i++) {
            ofVec2f pos(
                ofRandom(100, ofGetWidth() - 100),
                ofRandom(100, ofGetHeight() - 100)
            );
            vortices.push_back(VortexCore(pos));
        }
        
        // Initialize impact center
        impactCenter = ofVec2f(ofGetWidth() * 0.5f, ofGetHeight() * 0.5f);
        
        // Set initial growth level
        globalGrowthLevel = 0.3f;
    }
    
    void update(float deltaTime) override {
        updateGlobalEffects(deltaTime);
        
        zOffset += deltaTime * timeScale;
        hueShift += deltaTime * 10.0f;
        
        // Update vortices
        for (auto& vortex : vortices) {
            vortex.update(deltaTime);
        }
        
        // Update particles
        for (auto& particle : particles) {
            // Calculate curl noise force
            ofVec2f force = calculateCurlNoise(particle.position);
            force *= flowSpeed * (1.0f + turbulence);
            
            // Add vortex influences
            for (auto& vortex : vortices) {
                float dist = particle.position.distance(vortex.position);
                if (dist < vortex.radius && dist > 0) {
                    ofVec2f toVortex = vortex.position - particle.position;
                    ofVec2f tangent(-toVortex.y, toVortex.x);
                    if (tangent.length() > 0.001f) {
                        tangent.normalize();
                    }
                    
                    float influence = (1.0f - dist / vortex.radius) * vortex.strength * vortexStrength;
                    force += tangent * influence * 50.0f;
                    
                    // Inward pull
                    if (toVortex.length() > 0.001f) {
                        force += toVortex.getNormalized() * influence * 10.0f;
                    }
                }
            }
            
            // Add attractor/repeller forces
            if (attractorStrength > 0.1f) {
                for (auto& attractor : attractors) {
                    ofVec2f toAttractor = attractor - particle.position;
                    float dist = toAttractor.length();
                    if (dist > 0.001f && dist < 200) {
                        toAttractor.normalize();
                        force += toAttractor * (200 - dist) * attractorStrength * 0.5f;
                    }
                }
                
                for (auto& repeller : repellers) {
                    ofVec2f fromRepeller = particle.position - repeller;
                    float dist = fromRepeller.length();
                    if (dist > 0.001f && dist < 100) {
                        fromRepeller.normalize();
                        force += fromRepeller * (100 - dist) * attractorStrength;
                    }
                }
            }
            
            // Apply force
            particle.velocity = particle.velocity * 0.9f + force * 0.1f;
            particle.update(deltaTime);
            
            // Update color
            float speed = particle.velocity.length();
            float hue = fmod(hueShift + 
                ofNoise(particle.position.x * 0.002f, particle.position.y * 0.002f, zOffset) * 100.0f * colorComplexity, 
                255);
            float saturation = 100 + speed * 0.5f;
            float brightness = 200 + globalGrowthLevel * 50.0f; // より明るく
            
            particle.color = ofColor::fromHsb(hue, ofClamp(saturation, 0, 255), ofClamp(brightness, 0, 255));
        }
        
        // Remove dead particles
        particles.erase(
            std::remove_if(particles.begin(), particles.end(),
                [](const CurlParticle& p) { return p.isDead(); }),
            particles.end()
        );
        
        // Spawn new particles
        float spawnRate = particleDensity * (2.0f + globalGrowthLevel * 3.0f);
        while (particles.size() < 300 && ofRandom(1.0f) < spawnRate * deltaTime) {
            ofVec2f pos;
            if (impactIntensity > 0.5f) {
                float angle = ofRandom(TWO_PI);
                float radius = ofRandom(100.0f);
                pos = impactCenter + ofVec2f(cos(angle), sin(angle)) * radius;
            } else {
                pos = ofVec2f(ofRandom(ofGetWidth()), ofRandom(ofGetHeight()));
            }
            particles.push_back(CurlParticle(pos));
        }
        
        // Update effects
        turbulence *= 0.93f;
        fieldDistortion *= 0.95f;
        attractorStrength *= 0.9f;
        
        // Flash effect
        flashTimer += deltaTime;
        flashEffect *= 0.92f;
        if (flashTimer > 3.0f && ofRandom(1.0f) < 0.015f) {
            flashEffect = 1.0f;
            flashTimer = 0.0f;
        }
        
        // Update attractors/repellers
        for (int i = attractors.size() - 1; i >= 0; i--) {
            if (ofRandom(1.0f) < 0.02f) {
                attractors.erase(attractors.begin() + i);
            }
        }
        for (int i = repellers.size() - 1; i >= 0; i--) {
            if (ofRandom(1.0f) < 0.02f) {
                repellers.erase(repellers.begin() + i);
            }
        }
    }
    
    void draw() override {
        beginMasterBuffer();
        
        // Background
        drawBackground();
        
        // Draw curl noise field (always visible for debugging)
        drawNoiseField();
        
        // Draw vortices
        drawVortices();
        
        // Draw particles and trails
        drawParticles();
        
        // Advanced effects
        if (globalGrowthLevel > 0.6f) {
            drawAdvancedEffects();
        }
        
        endMasterBuffer();
        drawFullscreenEffects();
        
        // Stats
        if (getTimeSinceLastMidi() < 5.0f) {
            ofSetColor(200);
            ofDrawBitmapString("Curl Noise System", 20, ofGetHeight() - 80);
            ofDrawBitmapString("Particles: " + ofToString(particles.size()), 20, ofGetHeight() - 60);
            ofDrawBitmapString("Vortices: " + ofToString(vortices.size()), 20, ofGetHeight() - 40);
            ofDrawBitmapString("Turbulence: " + ofToString(turbulence, 2), 20, ofGetHeight() - 20);
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
                    // Major turbulence
                    turbulence = impactIntensity * 3.0f;
                    fieldDistortion = impactIntensity * 2.0f;
                    
                    // Add new vortex
                    if (vortices.size() < 6) {
                        vortices.push_back(VortexCore(impactCenter));
                    }
                    
                    // Particle burst
                    for (int i = 0; i < impactIntensity * 30; i++) {
                        float angle = ofRandom(TWO_PI);
                        float speed = ofRandom(50, 150);
                        ofVec2f pos = impactCenter;
                        CurlParticle p(pos);
                        p.velocity = ofVec2f(cos(angle), sin(angle)) * speed;
                        particles.push_back(p);
                    }
                    break;
                    
                case SNARE:
                    // Add attractors
                    attractors.push_back(impactCenter);
                    attractorStrength = impactIntensity * 2.0f;
                    
                    // Color shift
                    colorComplexity = 1.0f + impactIntensity * 2.0f;
                    break;
                    
                case HIHAT_CLOSED:
                    // Subtle field modulation
                    noiseScale = 0.003f + impactIntensity * 0.002f;
                    timeScale = 0.2f + impactIntensity * 0.3f;
                    break;
                    
                case CRASH:
                    // Major disruption
                    flashEffect = 1.0f;
                    
                    // Clear and reset
                    vortices.clear();
                    for (int i = 0; i < 4; i++) {
                        ofVec2f pos(
                            ofRandom(100, ofGetWidth() - 100),
                            ofRandom(100, ofGetHeight() - 100)
                        );
                        VortexCore v(pos);
                        v.strength *= 2.0f;
                        vortices.push_back(v);
                    }
                    
                    // Add repellers
                    for (int i = 0; i < 3; i++) {
                        repellers.push_back(ofVec2f(
                            ofRandom(ofGetWidth()),
                            ofRandom(ofGetHeight())
                        ));
                    }
                    attractorStrength = 3.0f;
                    break;
                    
                default:
                    // Local disturbance
                    turbulence += impactIntensity * 0.5f;
                    particleDensity = 1.0f + impactIntensity;
                    break;
            }
            
        } else if (msg.status == MIDI_CONTROL_CHANGE) {
            if (msg.control == 1) { // Mod wheel
                modulation = mapCC(msg.value);
                vortexStrength = 1.0f + modulation * 2.0f;
                curlScale = 2.0f + modulation * 3.0f;
            }
        }
    }
    
private:
    ofVec2f calculateCurlNoise(ofVec2f pos) {
        float eps = 1.0f;
        
        // Sample noise at neighboring points
        float n1 = ofNoise(pos.x * noiseScale, (pos.y - eps) * noiseScale, zOffset);
        float n2 = ofNoise(pos.x * noiseScale, (pos.y + eps) * noiseScale, zOffset);
        float n3 = ofNoise((pos.x - eps) * noiseScale, pos.y * noiseScale, zOffset);
        float n4 = ofNoise((pos.x + eps) * noiseScale, pos.y * noiseScale, zOffset);
        
        // Calculate curl
        float a = (n2 - n1) / (2 * eps);
        float b = (n4 - n3) / (2 * eps);
        
        // Apply field distortion
        if (fieldDistortion > 0.1f) {
            a += sin(pos.x * 0.01f + systemTime) * fieldDistortion * 0.5f;
            b += cos(pos.y * 0.01f + systemTime * 1.3f) * fieldDistortion * 0.5f;
        }
        
        return ofVec2f(a, -b) * curlScale;
    }
    
    void drawBackground() {
        // beginMasterBuffer()で既に背景が描画されているため、
        // 追加の背景は描画しない
        // 必要に応じてエフェクトのみ追加
    }
    
    void drawNoiseField() {
        ofEnableBlendMode(OF_BLENDMODE_ALPHA);
        
        int step = 30;
        for (int y = 0; y < ofGetHeight(); y += step) {
            for (int x = 0; x < ofGetWidth(); x += step) {
                ofVec2f pos(x, y);
                ofVec2f curl = calculateCurlNoise(pos);
                
                ofSetColor(150, 200, 255, 40 + 30 * globalGrowthLevel);
                ofSetLineWidth(0.5f);
                ofDrawLine(pos, pos + curl * 10);
            }
        }
        
        ofDisableBlendMode();
    }
    
    void drawVortices() {
        ofEnableBlendMode(OF_BLENDMODE_ALPHA);
        
        for (auto& vortex : vortices) {
            // Vortex core
            ofNoFill();
            ofSetColor(100, 150, 200, 100);
            ofSetLineWidth(2.0f);
            
            // Spiral lines
            for (int i = 0; i < 3; i++) {
                ofBeginShape();
                for (float angle = 0; angle < TWO_PI * 3; angle += 0.1f) {
                    float r = (angle / (TWO_PI * 3)) * vortex.radius;
                    float spiralAngle = angle + vortex.rotation + i * TWO_PI / 3.0f;
                    float x = vortex.position.x + cos(spiralAngle) * r;
                    float y = vortex.position.y + sin(spiralAngle) * r;
                    ofVertex(x, y);
                }
                ofEndShape();
            }
            
            // Center glow
            ofFill();
            ofSetColor(200, 220, 255, 150);
            ofDrawCircle(vortex.position, 5);
        }
        
        ofDisableBlendMode();
    }
    
    void drawParticles() {
        ofEnableBlendMode(OF_BLENDMODE_ALPHA);
        
        for (auto& particle : particles) {
            float alpha = (1.0f - particle.getLifeRatio() * 0.5f) * 255.0f * particle.energy; // より不透明に
            
            // Draw trail
            if (particle.trail.size() > 1) {
                ofNoFill();
                ofSetLineWidth(particle.size * 0.5f);
                
                for (int i = 0; i < particle.trail.size() - 1; i++) {
                    float trailAlpha = (i / float(particle.trail.size())) * alpha * trailOpacity;
                    
                    if (flashEffect > 0.5f) {
                        ofSetColor(255, 255, 255, trailAlpha * flashEffect);
                    } else {
                        ofSetColor(particle.color.r, particle.color.g, particle.color.b, trailAlpha);
                    }
                    
                    ofDrawLine(particle.trail[i], particle.trail[i + 1]);
                }
                ofFill();
            }
            
            // Draw particle
            if (flashEffect > 0.5f) {
                ofSetColor(255, 255, 255, alpha * flashEffect);
            } else {
                ofSetColor(particle.color.r, particle.color.g, particle.color.b, alpha);
            }
            
            float size = particle.size * (1.0f + particle.velocity.length() * 0.01f);
            ofDrawCircle(particle.position, size);
        }
        
        ofDisableBlendMode();
    }
    
    void drawAdvancedEffects() {
        ofEnableBlendMode(OF_BLENDMODE_ALPHA);
        
        // Attractor/Repeller visualization
        if (attractorStrength > 0.1f) {
            ofNoFill();
            ofSetLineWidth(1.5f);
            
            // Attractors
            for (auto& attractor : attractors) {
                ofSetColor(100, 255, 150, 100 * attractorStrength);
                for (int i = 0; i < 3; i++) {
                    float radius = 20 + i * 20 * attractorStrength;
                    ofDrawCircle(attractor, radius);
                }
            }
            
            // Repellers
            for (auto& repeller : repellers) {
                ofSetColor(255, 100, 100, 100 * attractorStrength);
                
                // Draw radiating lines
                for (int i = 0; i < 12; i++) {
                    float angle = (i / 12.0f) * TWO_PI;
                    ofVec2f lineEnd = repeller + ofVec2f(cos(angle), sin(angle)) * 50 * attractorStrength;
                    ofDrawLine(repeller, lineEnd);
                }
            }
            
            ofFill();
        }
        
        // Energy waves at high growth
        if (globalGrowthLevel > 0.8f) {
            float waveTime = systemTime * 2.0f;
            
            ofSetColor(150, 200, 255, 50 * globalGrowthLevel);
            ofNoFill();
            ofSetLineWidth(2.0f);
            
            for (int wave = 0; wave < 3; wave++) {
                float phase = waveTime + wave * 0.5f;
                float radius = fmod(phase * 100, ofGetWidth() * 0.5f);
                float alpha = (1.0f - radius / (ofGetWidth() * 0.5f)) * 100 * globalGrowthLevel;
                
                ofSetColor(200, 220, 255, alpha);
                ofDrawCircle(ofGetWidth() * 0.5f, ofGetHeight() * 0.5f, radius);
            }
            
            ofFill();
        }
        
        ofDisableBlendMode();
    }
};