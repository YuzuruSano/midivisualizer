#pragma once

#include "VisualSystem.h"
#include "ofMain.h"
#include <vector>

struct WalkingFigure {
    ofVec2f position;
    float walkSpeed;
    float walkCycle;
    float shadowOffset;
    float scale;
    bool isActive;
    float fadeAlpha;
    
    WalkingFigure() : position(0, 0), walkSpeed(50.0f), walkCycle(0.0f), 
                     shadowOffset(0.0f), scale(1.0f), isActive(true), fadeAlpha(255.0f) {}
};

struct CorridorSegment {
    float depth;
    float width;
    float height;
    float perspective;
    ofVec2f vanishingPoint;
    float wallIntensity;
    float floorIntensity;
    
    CorridorSegment() : depth(0.0f), width(800.0f), height(600.0f), 
                       perspective(0.8f), vanishingPoint(400, 300), 
                       wallIntensity(80.0f), floorIntensity(40.0f) {}
};

class InfiniteCorridorSystem : public VisualSystem {
private:
    std::vector<WalkingFigure> figures;
    std::vector<CorridorSegment> corridorSegments;
    float corridorDepth;
    float walkingSpeed;
    float perspectiveShift;
    ofVec2f vanishingPoint;
    float ambientIntensity;
    float corridorWidth;
    float corridorHeight;
    
    // モノトーンカラーパレット
    ofColor darkGray;
    ofColor mediumGray;
    ofColor lightGray;
    ofColor shadowColor;
    
    // アニメーション
    float walkCycleTime;
    float perspectiveOscillation;
    float corridorSway;
    
    // MIDI連動
    float kickIntensity;
    float snareIntensity;
    float hihatIntensity;
    float crashIntensity;
    
    void drawCorridor();
    void drawWalkingFigures();
    void drawPerspectiveLines();
    void drawFloorPattern();
    void drawWallDetails();
    void updateFigureWalk(WalkingFigure& figure, float deltaTime);
    void createNewFigure();
    void updateCorridorPerspective(float deltaTime);
    
public:
    InfiniteCorridorSystem();
    void setup() override;
    void update(float deltaTime) override;
    void draw() override;
    void onMidiMessage(ofxMidiMessage& msg) override;
    void onBeatDetected(float velocity);
    void reset();
    
    void setGlobalGrowthLevel(float level);
};