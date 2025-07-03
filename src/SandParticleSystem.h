#pragma once

#include "VisualSystem.h"
#include "ofMain.h"
#include <vector>
#include <deque>

struct SandParticle {
    ofVec2f position;
    ofVec2f velocity;
    ofVec2f acceleration;
    float life;
    float maxLife;
    float size;
    float mass;
    float alpha;
    bool isActive;
    ofColor particleColor;
    
    SandParticle() : position(0, 0), velocity(0, 0), acceleration(0, 0), life(1.0f), maxLife(1.0f), 
                    size(1.0f), mass(1.0f), alpha(255.0f), isActive(true), particleColor(120, 120, 120) {}
};

struct SandDune {
    ofVec2f position;
    float width;
    float height;
    float slope;
    std::vector<ofVec2f> profile;
    float windResistance;
    float stability;
    
    SandDune() : position(0, 0), width(200.0f), height(50.0f), slope(0.3f), 
                windResistance(0.7f), stability(0.8f) {}
};

struct PatternElement {
    std::vector<ofVec2f> points;
    ofColor elementColor;
    float alpha;
    float scale;
    float rotation;
    ofVec2f center;
    float creationTime;
    float lifetime;
    bool isActive;
    
    PatternElement() : elementColor(140, 140, 140), alpha(255.0f), scale(1.0f), rotation(0.0f), 
                      center(0, 0), creationTime(0.0f), lifetime(5.0f), isActive(true) {}
};

struct WindField {
    ofVec2f direction;
    float strength;
    float turbulence;
    float coverage;
    ofVec2f position;
    float radius;
    
    WindField() : direction(1, 0), strength(50.0f), turbulence(0.2f), coverage(1.0f), 
                 position(0, 0), radius(200.0f) {}
};

class SandParticleSystem : public VisualSystem {
private:
    std::vector<SandParticle> particles;
    std::vector<SandDune> dunes;
    std::vector<PatternElement> patterns;
    std::vector<WindField> windFields;
    std::deque<ofVec2f> particleTrails;
    
    // 砂のパラメータ
    float gravityStrength;
    float windStrength;
    float frictionCoefficient;
    float particleInteractionRadius;
    float sandDensity;
    
    // モノトーンカラーパレット
    ofColor sandDark;
    ofColor sandMedium;
    ofColor sandLight;
    ofColor dustColor;
    ofColor shadowColor;
    
    // パターン生成パラメータ
    float patternSpawnRate;
    float patternComplexity;
    float patternSymmetry;
    float patternScale;
    
    // 既視感パターン
    std::vector<std::vector<ofVec2f>> dejavu_patterns;
    float dejavu_trigger_probability;
    float dejavu_fade_rate;
    
    // 物理シミュレーション
    float airResistance;
    float particleCollisionRadius;
    float clusteringTendency;
    float erosionRate;
    
    // 環境効果
    float ambientHeat;
    float mirageEffect;
    float dustStormIntensity;
    
    // MIDI連動
    float kickIntensity;
    float snareIntensity;
    float hihatIntensity;
    float crashIntensity;
    
    void createSandParticle(ofVec2f position, ofVec2f velocity = ofVec2f(0, 0));
    void createParticleCluster(ofVec2f center, int count, float spread);
    void updateParticles(float deltaTime);
    void updateDunes(float deltaTime);
    void updatePatterns(float deltaTime);
    void updateWindFields(float deltaTime);
    void applyWindForce(SandParticle& particle, float deltaTime);
    void applyParticleInteractions(float deltaTime);
    void drawParticles();
    void drawDunes();
    void drawPatterns();
    void drawWindVisualization();
    void drawParticleTrails();
    void generateDejavuPattern();
    void createFractalPattern(ofVec2f center, float scale, int depth);
    void createSpiraPattern(ofVec2f center, float radius, int arms);
    void createMandalaPatter(ofVec2f center, float radius, int segments);
    void simulateErosion();
    void cleanupInactiveElements();
    
public:
    SandParticleSystem();
    void setup() override;
    void update(float deltaTime) override;
    void draw() override;
    void onMidiMessage(ofxMidiMessage& msg) override;
    void onBeatDetected(float velocity);
    void reset();
    
    void setGlobalGrowthLevel(float level);
};