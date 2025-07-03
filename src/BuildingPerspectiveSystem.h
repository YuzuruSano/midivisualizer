#pragma once

#include "VisualSystem.h"
#include "ofMain.h"
#include <vector>

struct BuildingEdge {
    ofVec3f start;
    ofVec3f end;
    float intensity;
    float width;
    bool isVisible;
    
    BuildingEdge() : start(0, 0, 0), end(0, 0, 0), intensity(255.0f), width(1.0f), isVisible(true) {}
};

struct BuildingFace {
    std::vector<ofVec3f> vertices;
    ofColor faceColor;
    float alpha;
    bool isFloor;
    float windowDensity;
    
    BuildingFace() : faceColor(80, 80, 80), alpha(255.0f), isFloor(false), windowDensity(0.0f) {}
};

struct Building {
    std::vector<BuildingFace> faces;
    std::vector<BuildingEdge> edges;
    ofVec3f position;
    ofVec3f size;
    float rotationY;
    float depth;
    bool isActive;
    float spawnTime;
    
    Building() : position(0, 0, 0), size(100, 100, 100), rotationY(0.0f), 
                depth(0.0f), isActive(true), spawnTime(0.0f) {}
};

class BuildingPerspectiveSystem : public VisualSystem {
private:
    std::vector<Building> buildings;
    ofVec3f cameraPosition;
    ofVec3f cameraTarget;
    float cameraSpeed;
    float cameraRotation;
    float perspectiveAngle;
    
    // 一人称視点パラメータ
    float walkBobbing;
    float walkSpeed;
    float headSway;
    float breathingOffset;
    
    // モノトーンカラーパレット
    ofColor buildingDark;
    ofColor buildingMedium;
    ofColor buildingLight;
    ofColor edgeColor;
    ofColor floorColor;
    ofColor windowColor;
    
    // 建物生成パラメータ
    float buildingSpawnRate;
    float buildingDensity;
    float streetWidth;
    float buildingHeight;
    float generationDistance;
    
    // 環境効果
    float fogDensity;
    float ambientLight;
    float shadowIntensity;
    
    // MIDI連動
    float kickIntensity;
    float snareIntensity;
    float hihatIntensity;
    float crashIntensity;
    
    void generateBuilding(float depth);
    void updateCameraMovement(float deltaTime);
    void drawBuilding(const Building& building);
    void drawBuildingEdges(const Building& building);
    void drawWindows(const Building& building);
    void drawStreetElements();
    void drawPerspectiveGrid();
    void projectPoint(const ofVec3f& point3D, ofVec2f& point2D);
    void createBuildingGeometry(Building& building);
    void cleanupDistantBuildings();
    
public:
    BuildingPerspectiveSystem();
    void setup() override;
    void update(float deltaTime) override;
    void draw() override;
    void onMidiMessage(ofxMidiMessage& msg) override;
    void onBeatDetected(float velocity);
    void reset();
    
    void setGlobalGrowthLevel(float level);
};