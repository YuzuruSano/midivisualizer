#pragma once

#include "VisualSystem.h"
#include "ofMain.h"
#include <vector>
#include <queue>

struct Ripple {
    ofVec2f center;
    float radius;
    float maxRadius;
    float intensity;
    float speed;
    float creationTime;
    float lifetime;
    bool isActive;
    ofColor rippleColor;
    
    Ripple() : center(0, 0), radius(0.0f), maxRadius(200.0f), intensity(1.0f), 
               speed(80.0f), creationTime(0.0f), lifetime(3.0f), isActive(true),
               rippleColor(120, 120, 120) {}
};

struct WaterParticle {
    ofVec2f position;
    ofVec2f velocity;
    float life;
    float maxLife;
    float size;
    float alpha;
    bool isActive;
    
    WaterParticle() : position(0, 0), velocity(0, 0), life(1.0f), maxLife(1.0f), 
                     size(2.0f), alpha(255.0f), isActive(true) {}
};

struct RippleCluster {
    ofVec2f center;
    std::vector<Ripple> ripples;
    float clusterRadius;
    float activationTime;
    float intensity;
    bool isExpanding;
    
    RippleCluster() : center(0, 0), clusterRadius(0.0f), activationTime(0.0f), 
                     intensity(1.0f), isExpanding(true) {}
};

class WaterRippleSystem : public VisualSystem {
private:
    std::vector<Ripple> ripples;
    std::vector<WaterParticle> waterParticles;
    std::vector<RippleCluster> rippleClusters;
    std::queue<ofVec2f> rippleQueue;
    
    // 水面パラメータ
    float waterLevel;
    float waterOpacity;
    float surfaceTension;
    float waveAmplitude;
    float waveFrequency;
    
    // モノトーンカラーパレット
    ofColor waterDark;
    ofColor waterMedium;
    ofColor waterLight;
    ofColor rippleColor;
    ofColor foamColor;
    
    // 波紋生成パラメータ
    float rippleSpawnRate;
    float rippleLifetime;
    float rippleSpeed;
    float rippleIntensity;
    
    // 異常な物理パラメータ
    float gravitationalAnomalyStrength;
    float timeDistortionFactor;
    float rippleInteractionStrength;
    float quantumFluctuationRate;
    
    // 環境効果
    float ambientFlow;
    float turbulenceStrength;
    float interferencePattern;
    
    // MIDI連動
    float kickIntensity;
    float snareIntensity;
    float hihatIntensity;
    float crashIntensity;
    
    // 動的波紋移動
    std::vector<ofVec2f> autonomousRippleCenters;
    float autonomousMovementSpeed;
    
    void createRipple(ofVec2f position, float intensity = 1.0f);
    void createRippleCluster(ofVec2f center, int count, float spread);
    void updateRipples(float deltaTime);
    void updateWaterParticles(float deltaTime);
    void updateRippleClusters(float deltaTime);
    void updateAutonomousRipples(float deltaTime);
    void drawWaterSurface();
    void drawRipples();
    void drawWaterParticles();
    void drawRippleClusters();
    void drawInterferencePattern();
    void drawQuantumFluctuations();
    void calculateRippleInteraction();
    void spawnWaterParticles(ofVec2f position, float intensity);
    void cleanupInactiveElements();
    
public:
    WaterRippleSystem();
    void setup() override;
    void update(float deltaTime) override;
    void draw() override;
    void onMidiMessage(ofxMidiMessage& msg) override;
    void onBeatDetected(float velocity);
    void reset();
    
    void setGlobalGrowthLevel(float level);
};