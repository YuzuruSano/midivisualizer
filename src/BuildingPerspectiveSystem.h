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

// 建物の成長タイプ
enum BuildingGrowthType {
    FOUNDATION,     // 基礎段階
    LOW_RISE,       // 低層階
    MID_RISE,       // 中層階
    HIGH_RISE,      // 高層階
    SKYSCRAPER,     // 超高層
    COMPLEX         // 複合建物
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
    
    // 成長システム
    BuildingGrowthType growthType;
    int growthLevel;        // 0-5の成長レベル
    float growthProgress;   // 0.0-1.0の成長進行度
    float age;              // 建物の年齢
    std::vector<Building*> children;  // 派生した子建物
    Building* parent;       // 親建物
    
    // 成長パラメータ
    float growthRate;       // 成長速度
    float maxHeight;        // 最大高さ
    float spawnProbability; // 派生確率
    bool canSpawnChildren;  // 子建物を生成できるか
    
    Building() : position(0, 0, 0), size(100, 100, 100), rotationY(0.0f), 
                depth(0.0f), isActive(true), spawnTime(0.0f),
                growthType(FOUNDATION), growthLevel(0), growthProgress(0.0f), age(0.0f),
                parent(nullptr), growthRate(1.0f), maxHeight(200.0f),
                spawnProbability(0.1f), canSpawnChildren(true) {}
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
    
    // 成長システムパラメータ
    float globalGrowthRate;      // 全体成長速度
    float spawnCooldown;         // 派生クールダウン
    float lastSpawnTime;         // 最後の派生時刻
    int maxBuildingsPerArea;     // エリア当たりの最大建物数
    
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
    
    // 成長システム
    void updateBuildingGrowth(Building& building, float deltaTime);
    void spawnChildBuilding(Building& parent);
    void growBuilding(Building& building, float deltaTime);
    BuildingGrowthType getNextGrowthType(BuildingGrowthType current);
    void updateBuildingType(Building& building);
    ofColor getBuildingColorByType(BuildingGrowthType type, int level);
    void generateBuildingByType(Building& building, BuildingGrowthType type);
    
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