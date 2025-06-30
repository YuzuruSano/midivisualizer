#pragma once

#include "VisualSystem.h"
#include <string>
#include <map>
#include <stack>
#include <vector>

struct UrbanStructure {
    ofVec2f position;
    ofVec2f direction;
    float size;
    float age;
    ofColor color;
    string type; // "foundation", "pillar", "beam", "detail", "pendulum", "lever"
    int generation;
    bool isConnected;
    float stability;
    
    // 振り子・線分パーツ用
    float pendulumAngle;
    float angularVelocity;
    float length;
    ofVec2f anchor;
    
    UrbanStructure(ofVec2f pos, ofVec2f dir, float s, string t, int gen) :
        position(pos), direction(dir), size(s), type(t), generation(gen) {
        age = 0.0f;
        isConnected = false;
        stability = 1.0f;
        color = ofColor::white;
        
        // 振り子・線分パーツの初期化
        pendulumAngle = ofRandom(-PI/3, PI/3);
        angularVelocity = ofRandom(-0.5f, 0.5f);
        length = s * (1.5f + ofRandom(1.0f));
        anchor = pos;
    }
    
    void update(float deltaTime, float growthLevel) {
        age += deltaTime * (1.0f + growthLevel);
        
        // 安定性の変化
        if (type == "foundation") {
            stability = 1.0f; // 基礎は常に安定
        } else {
            stability *= (0.999f + growthLevel * 0.0005f);
        }
        
        // サイズの成長
        if (age < 3.0f) {
            size *= (1.0f + deltaTime * 0.1f * growthLevel);
            length = size * (1.5f + sin(age) * 0.5f);
        }
        
        // 振り子の物理演算
        if (type == "pendulum" || type == "lever") {
            // 重力による角速度変化
            float gravity = 0.5f;
            float dampening = 0.99f;
            
            angularVelocity += -(gravity / length) * sin(pendulumAngle) * deltaTime;
            angularVelocity *= dampening;
            pendulumAngle += angularVelocity * deltaTime;
            
            // 振り子先端位置の更新
            position = anchor + ofVec2f(sin(pendulumAngle), cos(pendulumAngle)) * length;
        }
    }
};

class LSystemSystem : public VisualSystem {
private:
    // 構造システム
    std::vector<UrbanStructure> structures;
    std::vector<ofVec2f> constructionSites;
    std::vector<std::vector<UrbanStructure*>> buildingClusters;
    
    // 建設パラメータ
    float constructionProgress = 0.0f;
    float architecturalComplexity = 0.0f;
    float urbanPlanning = 0.0f;
    
    // 視覚的エフェクト
    std::vector<ofPolyline> scaffoldLines;
    std::vector<ofVec2f> cranePositions;
    float constructionNoise = 0.0f;
    
    // 有機的成長システム
    struct GrowthVector {
        ofVec2f position;
        ofVec2f direction;
        float intensity;
        float age;
        ofColor color;
    };
    std::vector<GrowthVector> growthVectors;
    
    // 建設波形効果
    struct ConstructionWave {
        ofVec2f center;
        float radius;
        float intensity;
        float age;
    };
    std::vector<ConstructionWave> constructionWaves;
    
    // 繊維束システム
    struct Fiber {
        ofVec2f startPos;
        ofVec2f endPos;
        ofVec2f velocity;
        float thickness;
        float age;
        ofColor color;
        float phase;
    };
    std::vector<Fiber> fibers;
    
    // 建物タイプ
    enum BuildingType {
        RESIDENTIAL,
        COMMERCIAL, 
        INDUSTRIAL,
        INFRASTRUCTURE,
        MONUMENT
    };
    
    BuildingType currentBuildingType = RESIDENTIAL;
    float constructionIntensity = 0.0f;
    
    // プロシージャル生成
    struct BluePrint {
        ofVec2f origin;
        float scale;
        BuildingType type;
        std::vector<ofVec2f> nodes;
        std::vector<std::pair<int, int>> connections;
    };
    
    std::vector<BluePrint> blueprints;
    int activeBlueprintIndex = 0;
    
public:
    void setup() override {
        // 初期建設サイトの設定（スペーシング拡大）
        constructionSites.push_back(ofVec2f(ofGetWidth() * 0.5f, ofGetHeight() * 0.8f));
        constructionSites.push_back(ofVec2f(ofGetWidth() * 0.25f, ofGetHeight() * 0.5f));
        constructionSites.push_back(ofVec2f(ofGetWidth() * 0.75f, ofGetHeight() * 0.5f));
        
        // 初期青写真の生成
        generateInitialBlueprints();
        
        // 基礎構造の配置
        for (auto& site : constructionSites) {
            createFoundation(site);
        }
        
        // 初期足場の設定（密度削減）
        scaffoldLines.resize(3);
        cranePositions.resize(2);
        for (int i = 0; i < 2; i++) {
            cranePositions[i] = ofVec2f(ofRandom(100, ofGetWidth()-100), ofRandom(50, 200));
        }
        
        // 成長ベクターの初期化
        for (int i = 0; i < 6; i++) {
            GrowthVector gv;
            gv.position = ofVec2f(ofRandom(ofGetWidth()), ofRandom(ofGetHeight()));
            gv.direction = ofVec2f(cos(ofRandom(TWO_PI)), sin(ofRandom(TWO_PI)));
            gv.intensity = ofRandom(0.3f, 0.8f);
            gv.age = 0.0f;
            gv.color = urbanColor(i * 30, 0.6f);
            growthVectors.push_back(gv);
        }
        
        // 繊維束の初期化
        initializeFibers();
    }
    
    void update(float deltaTime) override {
        // 統一エフェクトシステムの更新
        updateGlobalEffects(deltaTime);
        
        constructionNoise += deltaTime * 0.5f;
        
        // 建設進行度の更新
        constructionProgress += deltaTime * 0.03f * (1.0f + globalGrowthLevel);
        architecturalComplexity = globalGrowthLevel * (1.0f + sin(systemTime * 0.3f) * 0.2f);
        urbanPlanning += deltaTime * 0.02f;
        
        // 構造物の更新
        updateStructures(deltaTime);
        
        // プロシージャル建設（密度制限を大幅削減）
        if (constructionProgress > 0.5f && structures.size() < 150) {
            proceduralConstruction();
        }
        
        // 建設強度の減衰
        constructionIntensity *= 0.98f;
        
        // 崩壊時の効果
        if (isCollapsing) {
            applyStructuralFailure();
        }
        
        // 足場システムの更新
        updateScaffolding(deltaTime);
        
        // 有機的成長システムの更新
        updateGrowthVectors(deltaTime);
        
        // 建設波形の更新
        updateConstructionWaves(deltaTime);
        
        // 繊維束の更新
        updateFibers(deltaTime);
    }
    
    void draw() override {
        // マスターバッファに描画開始
        beginMasterBuffer();
        
        // 建設現場の背景
        drawConstructionBackground();
        
        // 青写真の表示
        if (globalGrowthLevel > 0.4f) {
            drawBlueprints();
        }
        
        // 足場システム
        drawScaffolding();
        
        // 有機的成長ベクターの描画
        drawGrowthVectors();
        
        // 建設波形エフェクト
        drawConstructionWaves();
        
        // 主要構造物
        drawUrbanStructures();
        
        // 建設機械（クレーンなど）
        drawConstructionMachinery();
        
        // 建設エフェクト
        if (constructionIntensity > 0.3f) {
            drawConstructionEffects();
        }
        
        // 高成長時の詳細効果
        if (globalGrowthLevel > 0.7f) {
            drawArchitecturalDetails();
        }
        
        endMasterBuffer();
        
        // 全画面エフェクトの描画
        drawFullscreenEffects();
        
        // 情報表示
        drawConstructionInfo();
    }
    
    void onMidiMessage(ofxMidiMessage& msg) override {
        if (msg.status == MIDI_NOTE_ON && msg.velocity > 0) {
            currentNote = msg.pitch;
            currentVelocity = msg.velocity;
            
            triggerImpact(msg.pitch, msg.velocity);
            
            constructionIntensity = impactIntensity;
            
            // ドラムタイプに応じた建設活動
            switch(msg.pitch) {
                case KICK:
                    // 基礎工事・重機作業
                    currentBuildingType = INFRASTRUCTURE;
                    triggerFoundationWork(impactIntensity * 2.0f);
                    triggerConstructionWave(ofVec2f(ofGetWidth() * 0.5f, ofGetHeight() * 0.8f), impactIntensity);
                    break;
                    
                case SNARE:
                    // 骨組み・鉄骨工事
                    currentBuildingType = COMMERCIAL;
                    triggerFramework(impactIntensity * 1.5f);
                    addGrowthVector(ofVec2f(ofRandom(ofGetWidth()), ofRandom(ofGetHeight())), impactIntensity);
                    break;
                    
                case HIHAT_CLOSED:
                    // 細部工事・仕上げ
                    currentBuildingType = RESIDENTIAL;
                    triggerDetailWork(impactIntensity);
                    break;
                    
                case CRASH:
                    // 大規模建設・モニュメント
                    currentBuildingType = MONUMENT;
                    triggerMassiveConstruction();
                    break;
                    
                default:
                    // 音程に基づく構造配置
                    ofVec2f buildingPos(
                        ofMap(msg.pitch % 12, 0, 12, 100, ofGetWidth() - 100),
                        ofMap(msg.pitch / 12, 0, 10, ofGetHeight() * 0.8f, ofGetHeight() * 0.3f)
                    );
                    createStructuralElement(buildingPos, impactIntensity);
            }
            
            // 建設進度の促進
            constructionProgress += impactIntensity * 0.1f;
            architecturalComplexity += impactIntensity * 0.15f;
            
        } else if (msg.status == MIDI_CONTROL_CHANGE) {
            if (msg.control == 1) { // Mod wheel
                modulation = mapCC(msg.value);
                // モジュレーションで建築スタイルを変更
                urbanPlanning = modulation;
            }
        }
    }
    
private:
    void drawConstructionBackground() {
        // 繊維束地面に置き換え
        drawFiberGround();
        
        // 都市計画グリッド
        if (urbanPlanning > 0.3f) {
            ofEnableBlendMode(OF_BLENDMODE_ADD);
            
            ofColor gridColor = urbanColor(currentNote, urbanPlanning * 0.5f);
            gridColor.a = 80 * urbanPlanning;
            ofSetColor(gridColor);
            
            float gridSize = 100 - globalGrowthLevel * 15;  // グリッドサイズを大幅拡大
            ofSetLineWidth(0.5f + globalGrowthLevel);
            
            // 建設現場風不規則配置
            ofNoFill();
            
            // 建設ゾーンとして不規則配置
            int numSites = (ofGetWidth() * ofGetHeight()) / (gridSize * gridSize * 2); // 建設現場密度
            
            for (int i = 0; i < numSites; i++) {
                // クラスター化された建設現場配置
                float clusterX = ofRandom(0, ofGetWidth());
                float clusterY = ofRandom(0, ofGetHeight());
                
                // 建設現場風ランダム多角形
                ofBeginShape();
                int numVertices = 3 + (int)(ofRandom(5)); // 3-7角形（建設現場の不規則性）
                float radius = ofRandom(gridSize * 0.1, gridSize * 0.4);
                
                // 建設ノイズで位置変形
                float noiseOffset = sin(clusterX * 0.01f + constructionNoise) * urbanPlanning * 15;
                clusterX += noiseOffset;
                clusterY += cos(clusterY * 0.01f + constructionNoise) * urbanPlanning * 10;
                
                for (int j = 0; j < numVertices; j++) {
                    float angle = (j * TWO_PI / numVertices) + ofRandom(-0.6, 0.6);
                    float r = radius + ofRandom(-radius * 0.5, radius * 0.5);
                    float vx = clusterX + cos(angle) * r;
                    float vy = clusterY + sin(angle) * r;
                    ofVertex(vx, vy);
                }
                ofEndShape(true);
            }
            ofFill();
            
            ofDisableBlendMode();
        }
    }
    
    void drawUrbanStructures() {
        ofEnableBlendMode(OF_BLENDMODE_ADD);
        
        for (auto& structure : structures) {
            float alpha = structure.stability * (0.7f + globalGrowthLevel * 0.3f);
            ofColor structColor = structure.color;
            structColor.a = 200 * alpha;
            ofSetColor(structColor);
            
            float lineWidth = structure.size * (0.6f + globalGrowthLevel * 0.3f);
            ofSetLineWidth(lineWidth);
            
            // 構造タイプに応じた描画
            if (structure.type == "foundation") {
                drawFoundationElement(structure);
            } else if (structure.type == "pillar") {
                drawPillarElement(structure);
            } else if (structure.type == "beam") {
                drawBeamElement(structure);
            } else if (structure.type == "detail") {
                drawDetailElement(structure);
            } else if (structure.type == "pendulum") {
                drawPendulumElement(structure);
            } else if (structure.type == "lever") {
                drawLeverElement(structure);
            }
            
            // 有機的な接続線で置き換え
            if (structure.isConnected && structure.stability > 0.5f) {
                ofColor connectionColor = urbanColor(currentNote + 15, structure.stability);
                connectionColor.a = 120;
                ofSetColor(connectionColor);
                ofSetLineWidth(lineWidth * 0.3f);
                
                // 有機的な放射状線分
                int numLines = 3 + structure.stability * 4;
                for (int i = 0; i < numLines; i++) {
                    float angle = (i * TWO_PI / numLines) + ofNoise(structure.position.x * 0.01f, structure.position.y * 0.01f, systemTime * 0.1f) * TWO_PI;
                    float length = lineWidth * (2 + ofRandom(3));
                    ofVec2f endPos = structure.position + ofVec2f(cos(angle), sin(angle)) * length;
                    ofDrawLine(structure.position, endPos);
                }
            }
        }
        
        ofDisableBlendMode();
    }
    
    void drawScaffolding() {
        if (constructionIntensity > 0.2f || globalGrowthLevel > 0.3f) {
            ofEnableBlendMode(OF_BLENDMODE_ADD);
            
            ofColor scaffoldColor = urbanColor(currentNote + 30, 0.6f);
            scaffoldColor.a = 120 * (0.5f + globalGrowthLevel * 0.5f);
            ofSetColor(scaffoldColor);
            
            ofSetLineWidth(0.5 + globalGrowthLevel * 0.8);
            
            // 足場ラインの描画
            for (auto& line : scaffoldLines) {
                if (line.size() > 1) {
                    line.draw();
                }
            }
            
            // 足場ノードの描画
            for (auto& site : constructionSites) {
                float radius = 30 + globalGrowthLevel * 20;
                int segments = 8 + globalGrowthLevel * 8;
                
                for (int i = 0; i < segments; i++) {
                    float angle = (i / float(segments)) * TWO_PI;
                    ofVec2f scaffoldPoint = site + ofVec2f(cos(angle), sin(angle)) * radius;
                    
                    ofDrawLine(site, scaffoldPoint);
                    
                    // 横梁
                    if (i > 0) {
                        float prevAngle = ((i-1) / float(segments)) * TWO_PI;
                        ofVec2f prevPoint = site + ofVec2f(cos(prevAngle), sin(prevAngle)) * radius;
                        ofDrawLine(scaffoldPoint, prevPoint);
                    }
                }
            }
            
            ofDisableBlendMode();
        }
    }
    
    void drawConstructionMachinery() {
        if (globalGrowthLevel > 0.5f) {
            ofEnableBlendMode(OF_BLENDMODE_ADD);
            
            // クレーンの描画
            for (int i = 0; i < cranePositions.size(); i++) {
                auto& cranePos = cranePositions[i];
                
                ofColor craneColor = accentColor(0.7f);
                craneColor.a = 180;
                ofSetColor(craneColor);
                
                float craneHeight = 150 + globalGrowthLevel * 100;
                float armLength = 100 + globalGrowthLevel * 50;
                
                // クレーンタワー
                ofSetLineWidth(1.5 + globalGrowthLevel * 0.8);
                ofDrawLine(cranePos, cranePos + ofVec2f(0, -craneHeight));
                
                // クレーンアーム
                float armAngle = systemTime * (0.5f + i * 0.3f) + i * PI;
                ofVec2f armEnd = cranePos + ofVec2f(-craneHeight * 0.3f, -craneHeight) + 
                                ofVec2f(cos(armAngle), sin(armAngle)) * armLength;
                
                ofDrawLine(cranePos + ofVec2f(0, -craneHeight), armEnd);
                
                // ケーブル
                ofSetLineWidth(1);
                ofVec2f hookPos = armEnd + ofVec2f(0, 50 + sin(systemTime * 2 + i) * 20);
                ofDrawLine(armEnd, hookPos);
                
                // フック（十字マーク）
                ofSetLineWidth(2);
                ofDrawLine(hookPos - ofVec2f(3, 0), hookPos + ofVec2f(3, 0));
                ofDrawLine(hookPos - ofVec2f(0, 3), hookPos + ofVec2f(0, 3));
                
                // 作業エリアの照明（放射状線分）
                if (constructionIntensity > 0.4f) {
                    ofColor lightColor = ofColor::yellow;
                    lightColor.a = 80 * constructionIntensity;
                    ofSetColor(lightColor);
                    ofSetLineWidth(1);
                    
                    // 照明を放射状線分で表現
                    for (int j = 0; j < 8; j++) {
                        float angle = j * TWO_PI / 8;
                        float length = 15 * constructionIntensity;
                        ofVec2f endPos = hookPos + ofVec2f(cos(angle), sin(angle)) * length;
                        ofDrawLine(hookPos, endPos);
                    }
                }
            }
            
            ofDisableBlendMode();
        }
    }
    
    void drawConstructionEffects() {
        ofEnableBlendMode(OF_BLENDMODE_ADD);
        
        // 溶接火花（線分ベース）
        if (constructionIntensity > 0.6f) {
            for (int i = 0; i < 6; i++) {
                ofVec2f sparkPos(ofRandom(ofGetWidth()), ofRandom(ofGetHeight() * 0.7f, ofGetHeight()));
                
                ofColor sparkColor = ofColor::fromHsb(ofRandom(20, 60), 200, 180);
                sparkColor.a = ofRandom(80, 200) * constructionIntensity;
                ofSetColor(sparkColor);
                ofSetLineWidth(ofRandom(0.5, 1.5));
                
                // 線分状の火花
                for (int j = 0; j < 3; j++) {
                    float angle = ofRandom(TWO_PI);
                    float length = ofRandom(3, 8);
                    ofVec2f endPos = sparkPos + ofVec2f(cos(angle), sin(angle)) * length;
                    ofDrawLine(sparkPos, endPos);
                }
            }
        }
        
        // 建設煙塵（有機的な線分群）
        for (int i = 0; i < 4; i++) {
            ofVec2f dustPos(ofRandom(ofGetWidth()), ofRandom(ofGetHeight() * 0.8f, ofGetHeight()));
            
            ofColor dustColor = urbanColor(currentNote, 0.3f);
            dustColor.a = 30 * constructionIntensity;
            ofSetColor(dustColor);
            ofSetLineWidth(0.3f + globalGrowthLevel * 0.2f);
            
            // 煙のような有機的線分
            int numStrokes = 4 + ofRandom(4);
            for (int j = 0; j < numStrokes; j++) {
                float angle = ofRandom(-PI/4, PI/4);  // 上向きの煙
                float length = ofRandom(8, 20) * (1.0f + globalGrowthLevel * 0.5f);
                ofVec2f endPos = dustPos + ofVec2f(sin(angle), -cos(angle)) * length;
                endPos += ofVec2f(ofRandom(-5, 5), ofRandom(-3, 3));  // ランダム性追加
                ofDrawLine(dustPos, endPos);
            }
        }
        
        ofDisableBlendMode();
    }
    
    void drawArchitecturalDetails() {
        ofEnableBlendMode(OF_BLENDMODE_ADD);
        
        // 建築詳細要素
        for (auto& structure : structures) {
            if (structure.type == "detail" && structure.age > 1.0f) {
                ofColor detailColor = accentColor(architecturalComplexity);
                detailColor.a = 150 * globalGrowthLevel;
                ofSetColor(detailColor);
                
                // 装飾的要素
                float detailSize = structure.size * 0.3f;
                ofVec2f pos = structure.position;
                
                // 幾何学的装飾（密度削減）
                for (int i = 0; i < 3; i++) {
                    float angle = (i / 6.0f) * TWO_PI;
                    ofVec2f detailPoint = pos + ofVec2f(cos(angle), sin(angle)) * detailSize;
                    ofDrawLine(pos, detailPoint);
                }
                
                // 窓や開口部（白い矩形を線分フレームに変更）
                if (structure.generation > 2) {
                    ofColor windowColor = urbanColor(currentNote, 0.3f);
                    windowColor.a = 80;
                    ofSetColor(windowColor);
                    ofSetLineWidth(0.5f);
                    ofNoFill();
                    ofDrawRectangle(pos.x - detailSize * 0.3f, pos.y - detailSize * 0.3f, 
                                   detailSize * 0.6f, detailSize * 0.6f);
                    ofFill();
                }
            }
        }
        
        ofDisableBlendMode();
    }
    
    void drawBlueprints() {
        if (blueprints.empty()) return;
        
        ofEnableBlendMode(OF_BLENDMODE_ADD);
        
        auto& blueprint = blueprints[activeBlueprintIndex % blueprints.size()];
        
        ofColor blueprintColor = urbanColor(currentNote + 60, 0.4f);
        blueprintColor.a = 60 * globalGrowthLevel;
        ofSetColor(blueprintColor);
        
        ofSetLineWidth(0.5f + globalGrowthLevel);
        
        // 青写真の線画
        for (auto& connection : blueprint.connections) {
            if (connection.first < blueprint.nodes.size() && connection.second < blueprint.nodes.size()) {
                ofVec2f start = blueprint.origin + blueprint.nodes[connection.first] * blueprint.scale;
                ofVec2f end = blueprint.origin + blueprint.nodes[connection.second] * blueprint.scale;
                ofDrawLine(start, end);
            }
        }
        
        // ノードの表示（十字マーク）
        ofColor nodeColor = blueprintColor;
        nodeColor.a = 80 * globalGrowthLevel;
        ofSetColor(nodeColor);
        ofSetLineWidth(1.0f);
        
        for (auto& node : blueprint.nodes) {
            ofVec2f nodePos = blueprint.origin + node * blueprint.scale;
            // 十字マークでノードを表現
            float crossSize = 3;
            ofDrawLine(nodePos - ofVec2f(crossSize, 0), nodePos + ofVec2f(crossSize, 0));
            ofDrawLine(nodePos - ofVec2f(0, crossSize), nodePos + ofVec2f(0, crossSize));
        }
        
        ofDisableBlendMode();
    }
    
    void drawConstructionInfo() {
        if (getTimeSinceLastMidi() < 5.0f) {
            ofColor infoColor = urbanColor(currentNote, 0.5f);
            infoColor.a = 150;
            ofSetColor(infoColor);
            ofDrawBitmapString("L-System Construction - Urban Development", 20, ofGetHeight() - 100);
            ofDrawBitmapString("Construction Progress: " + ofToString(constructionProgress * 100, 1) + "%", 20, ofGetHeight() - 80);
            ofDrawBitmapString("Architectural Complexity: " + ofToString(architecturalComplexity * 100, 1) + "%", 20, ofGetHeight() - 60);
            ofDrawBitmapString("Structures: " + ofToString(structures.size()), 20, ofGetHeight() - 40);
            if (isCollapsing) {
                ofSetColor(255, 100, 100);
                ofDrawBitmapString("STRUCTURAL FAILURE", 20, ofGetHeight() - 20);
            }
        }
    }
    
    // 構造要素の描画メソッド
    void drawFoundationElement(const UrbanStructure& structure) {
        float width = structure.size * 2;
        float height = structure.size * 0.5f;
        ofDrawRectangle(structure.position.x - width/2, structure.position.y - height/2, width, height);
    }
    
    void drawPillarElement(const UrbanStructure& structure) {
        ofVec2f top = structure.position + structure.direction * structure.size;
        ofDrawLine(structure.position, top);
        
        // 柱の装飾（白い円を十字マークに変更）
        int segments = 3 + structure.generation;
        for (int i = 0; i < segments; i++) {
            float t = i / float(segments);
            ofVec2f segmentPos = structure.position + structure.direction * structure.size * t;
            float crossSize = structure.size * 0.08f;
            ofDrawLine(segmentPos - ofVec2f(crossSize, 0), segmentPos + ofVec2f(crossSize, 0));
            ofDrawLine(segmentPos - ofVec2f(0, crossSize), segmentPos + ofVec2f(0, crossSize));
        }
    }
    
    void drawBeamElement(const UrbanStructure& structure) {
        ofVec2f end = structure.position + structure.direction * structure.size;
        
        // メインビーム
        ofDrawLine(structure.position, end);
        
        // 支持構造
        ofVec2f perpendicular(-structure.direction.y, structure.direction.x);
        perpendicular.normalize();
        perpendicular *= structure.size * 0.1f;
        
        ofDrawLine(structure.position + perpendicular, structure.position - perpendicular);
        ofDrawLine(end + perpendicular, end - perpendicular);
    }
    
    void drawDetailElement(const UrbanStructure& structure) {
        // 装飾的詳細（白い円を除去）
        float detailSize = structure.size * 0.5f;
        
        // 十字マークで装飾要素を表現
        ofSetLineWidth(1.0f + structure.stability);
        ofDrawLine(structure.position - ofVec2f(detailSize, 0), structure.position + ofVec2f(detailSize, 0));
        ofDrawLine(structure.position - ofVec2f(0, detailSize), structure.position + ofVec2f(0, detailSize));
        
        if (structure.stability > 0.7f) {
            // 高品質詳細（放射状線分）
            for (int i = 0; i < 8; i++) {
                float angle = (i / 8.0f) * TWO_PI;
                ofVec2f detailPoint = structure.position + ofVec2f(cos(angle), sin(angle)) * detailSize;
                ofDrawLine(structure.position, detailPoint);
            }
        }
    }
    
    // 建設活動メソッド
    void updateStructures(float deltaTime) {
        for (auto& structure : structures) {
            structure.update(deltaTime, globalGrowthLevel);
            
            // 色の更新
            if (structure.stability > 0.8f) {
                structure.color = urbanColor(currentNote + structure.generation * 10, architecturalComplexity);
            } else if (structure.stability > 0.5f) {
                structure.color = urbanColor(currentNote, 0.5f);
            } else {
                structure.color = urbanColor(currentNote - 20, 0.3f); // 劣化色
            }
        }
        
        // 不安定な構造の除去
        structures.erase(
            std::remove_if(structures.begin(), structures.end(),
                [](const UrbanStructure& s) { return s.stability < 0.1f; }),
            structures.end()
        );
    }
    
    void createFoundation(ofVec2f position) {
        UrbanStructure foundation(position, ofVec2f(0, -1), 20, "foundation", 0);
        foundation.color = urbanColor(currentNote, 0.8f);
        foundation.isConnected = true;
        structures.push_back(foundation);
    }
    
    void createStructuralElement(ofVec2f position, float intensity) {
        // より多様な大きめの線分・振り子パーツ
        string elementType;
        float random = ofRandom(1.0f);
        if (random < 0.3f) {
            elementType = "pendulum";
        } else if (random < 0.6f) {
            elementType = "lever";
        } else if (intensity > 0.7f) {
            elementType = "pillar";
        } else if (intensity > 0.4f) {
            elementType = "beam";
        } else {
            elementType = "detail";
        }
        
        ofVec2f direction = ofVec2f(cos(ofRandom(TWO_PI)), sin(ofRandom(TWO_PI)));
        float size = 30 + intensity * 80; // より大きなサイズ
        int generation = ofRandom(4);
        
        UrbanStructure element(position, direction, size, elementType, generation);
        element.color = urbanColor(currentNote + generation * 15, intensity);
        
        // 振り子・線分用の特別な設定
        if (elementType == "pendulum" || elementType == "lever") {
            element.anchor = position;
            element.length = size * (2.0f + ofRandom(1.0f));
            element.position = element.anchor + ofVec2f(sin(element.pendulumAngle), cos(element.pendulumAngle)) * element.length;
        }
        
        structures.push_back(element);
    }
    
    void proceduralConstruction() {
        if (ofRandom(1.0f) < 0.02f * globalGrowthLevel) {
            // 新しい構造要素の自動生成
            ofVec2f newPos(ofRandom(100, ofGetWidth()-100), ofRandom(100, ofGetHeight()-100));
            createStructuralElement(newPos, globalGrowthLevel);
        }
    }
    
    void triggerFoundationWork(float intensity) {
        for (auto& site : constructionSites) {
            createFoundation(site + ofVec2f(ofRandom(-30, 30), ofRandom(-30, 30)));
        }
    }
    
    void triggerFramework(float intensity) {
        // 骨組み作業
        for (int i = 0; i < intensity * 5; i++) {
            ofVec2f pos(ofRandom(ofGetWidth()), ofRandom(ofGetHeight()));
            createStructuralElement(pos, intensity);
        }
    }
    
    void triggerDetailWork(float intensity) {
        // 既存構造への詳細追加
        for (auto& structure : structures) {
            if (structure.type == "pillar" && ofRandom(1.0f) < intensity) {
                ofVec2f detailPos = structure.position + ofVec2f(ofRandom(-10, 10), ofRandom(-10, 10));
                createStructuralElement(detailPos, intensity * 0.5f);
            }
        }
    }
    
    void triggerMassiveConstruction() {
        // 大規模建設
        ofVec2f center(ofGetWidth() * 0.5f, ofGetHeight() * 0.5f);
        
        for (int i = 0; i < 20; i++) {
            float angle = (i / 20.0f) * TWO_PI;
            float radius = 50 + i * 10;
            ofVec2f pos = center + ofVec2f(cos(angle), sin(angle)) * radius;
            createStructuralElement(pos, 1.0f);
        }
        
        // 新しい建設サイトの追加
        constructionSites.push_back(center + ofVec2f(ofRandom(-100, 100), ofRandom(-100, 100)));
        if (constructionSites.size() > 6) {
            constructionSites.erase(constructionSites.begin());
        }
    }
    
    void applyStructuralFailure() {
        // 崩壊時の構造破綻
        for (auto& structure : structures) {
            if (ofRandom(1.0f) < 0.05f) {
                structure.stability *= 0.9f;
                structure.color = urbanColor(currentNote - 30, 0.2f);
            }
        }
    }
    
    void updateScaffolding(float deltaTime) {
        // 足場ラインの動的更新
        for (int i = 0; i < scaffoldLines.size(); i++) {
            scaffoldLines[i].clear();
            
            if (i < constructionSites.size()) {
                auto& site = constructionSites[i];
                
                int numPoints = 10 + globalGrowthLevel * 15;
                for (int j = 0; j < numPoints; j++) {
                    float angle = (j / float(numPoints)) * TWO_PI + systemTime * 0.5f;
                    float radius = 20 + j * 3 + sin(systemTime + i) * 5;
                    
                    ofVec2f scaffoldPoint = site + ofVec2f(cos(angle), sin(angle)) * radius;
                    scaffoldLines[i].addVertex(scaffoldPoint.x, scaffoldPoint.y);
                }
            }
        }
        
        // クレーン位置の更新
        for (int i = 0; i < cranePositions.size(); i++) {
            cranePositions[i] += ofVec2f(sin(systemTime * 0.1f + i), 0) * 0.5f;
            
            // 境界チェック
            if (cranePositions[i].x < 50) cranePositions[i].x = 50;
            if (cranePositions[i].x > ofGetWidth() - 50) cranePositions[i].x = ofGetWidth() - 50;
        }
    }
    
    void generateInitialBlueprints() {
        // 建築青写真の生成
        for (int type = 0; type < 3; type++) {
            BluePrint bp;
            bp.origin = ofVec2f(ofRandom(200, ofGetWidth()-200), ofRandom(200, ofGetHeight()-200));
            bp.scale = ofRandom(0.5f, 2.0f);
            bp.type = static_cast<BuildingType>(type);
            
            // ノードの生成
            int numNodes = 8 + type * 4;
            for (int i = 0; i < numNodes; i++) {
                bp.nodes.push_back(ofVec2f(ofRandom(-50, 50), ofRandom(-50, 50)));
            }
            
            // 接続の生成
            for (int i = 0; i < numNodes - 1; i++) {
                bp.connections.push_back(std::make_pair(i, i + 1));
                if (i > 0 && ofRandom(1.0f) < 0.3f) {
                    bp.connections.push_back(std::make_pair(i, ofRandom(i)));
                }
            }
            
            blueprints.push_back(bp);
        }
    }
    
    // 新しい描画メソッド
    void drawPendulumElement(const UrbanStructure& structure) {
        // 振り子の支持点
        ofColor anchorColor = structure.color;
        anchorColor.setBrightness(ofClamp(anchorColor.getBrightness() * 0.8f, 30, 120));
        ofSetColor(anchorColor);
        ofDrawCircle(structure.anchor.x, structure.anchor.y, 3 + structure.size * 0.05f);
        
        // 振り子の線
        ofSetLineWidth(2 + structure.size * 0.03f);
        ofDrawLine(structure.anchor, structure.position);
        
        // 振り子の重り（大きめの線分十字）
        ofColor weightColor = structure.color;
        weightColor.setBrightness(ofClamp(weightColor.getBrightness() * 0.9f, 40, 150));
        ofSetColor(weightColor);
        ofSetLineWidth(3 + structure.size * 0.04f);
        
        float crossSize = 8 + structure.size * 0.1f;
        ofDrawLine(structure.position - ofVec2f(crossSize, 0), structure.position + ofVec2f(crossSize, 0));
        ofDrawLine(structure.position - ofVec2f(0, crossSize), structure.position + ofVec2f(0, crossSize));
    }
    
    void drawLeverElement(const UrbanStructure& structure) {
        // レバーの支点
        ofColor fulcrumColor = structure.color;
        fulcrumColor.setBrightness(ofClamp(fulcrumColor.getBrightness() * 0.7f, 20, 100));
        ofSetColor(fulcrumColor);
        
        float fulcrumSize = 5 + structure.size * 0.06f;
        // 三角形の支点
        ofDrawTriangle(structure.anchor.x, structure.anchor.y - fulcrumSize,
                      structure.anchor.x - fulcrumSize * 0.8f, structure.anchor.y + fulcrumSize * 0.5f,
                      structure.anchor.x + fulcrumSize * 0.8f, structure.anchor.y + fulcrumSize * 0.5f);
        
        // レバーアーム（両側に伸びる線分）
        ofSetLineWidth(3 + structure.size * 0.04f);
        ofColor leverColor = structure.color;
        leverColor.setBrightness(ofClamp(leverColor.getBrightness() * 0.8f, 30, 130));
        ofSetColor(leverColor);
        
        ofVec2f leverDirection = ofVec2f(cos(structure.pendulumAngle), sin(structure.pendulumAngle));
        ofVec2f end1 = structure.anchor + leverDirection * structure.length;
        ofVec2f end2 = structure.anchor - leverDirection * structure.length * 0.7f;
        
        ofDrawLine(end1, end2);
        
        // 端点の重り
        ofSetLineWidth(2);
        float endSize = 6 + structure.size * 0.08f;
        ofDrawCircle(end1.x, end1.y, endSize);
        ofDrawCircle(end2.x, end2.y, endSize * 0.7f);
    }
    
    void drawFiberGround() {
        ofEnableBlendMode(OF_BLENDMODE_ADD);
        
        for (auto& fiber : fibers) {
            ofColor fiberColor = fiber.color;
            fiberColor.a = 120 * (0.5f + globalGrowthLevel * 0.4f);
            fiberColor.setBrightness(ofClamp(fiberColor.getBrightness() * 0.6f, 20, 100));
            ofSetColor(fiberColor);
            
            ofSetLineWidth(fiber.thickness);
            
            // 波状の繊維線分
            ofPolyline fiberLine;
            int segments = 20;
            for (int i = 0; i <= segments; i++) {
                float t = i / float(segments);
                ofVec2f pos = fiber.startPos.getInterpolated(fiber.endPos, t);
                
                // 繊維の波状変形
                float waveOffset = sin(t * PI * 3 + fiber.phase + systemTime * 0.5f) * fiber.thickness * 2;
                pos.y += waveOffset;
                
                fiberLine.addVertex(pos.x, pos.y);
            }
            fiberLine.draw();
        }
        
        ofDisableBlendMode();
    }
    
    void initializeFibers() {
        fibers.clear();
        
        // 画面下部の繊維束
        for (int i = 0; i < 25; i++) {
            Fiber fiber;
            fiber.startPos = ofVec2f(ofRandom(-50, ofGetWidth() + 50), ofGetHeight() * (0.75f + ofRandom(0.2f)));
            fiber.endPos = ofVec2f(ofRandom(-50, ofGetWidth() + 50), ofGetHeight() * (0.85f + ofRandom(0.15f)));
            fiber.velocity = ofVec2f(ofRandom(-0.5f, 0.5f), ofRandom(-0.2f, 0.2f));
            fiber.thickness = ofRandom(0.5f, 2.5f);
            fiber.age = 0.0f;
            fiber.color = urbanColor(i * 12, 0.6f);
            fiber.phase = ofRandom(TWO_PI);
            fibers.push_back(fiber);
        }
    }
    
    void updateFibers(float deltaTime) {
        for (auto& fiber : fibers) {
            fiber.age += deltaTime;
            fiber.phase += deltaTime * (0.5f + globalGrowthLevel * 0.3f);
            
            // ランダム移動
            fiber.startPos += fiber.velocity * deltaTime * 10.0f;
            fiber.endPos += fiber.velocity * deltaTime * 10.0f;
            
            // 速度のランダム変化
            fiber.velocity += ofVec2f(ofRandom(-0.1f, 0.1f), ofRandom(-0.05f, 0.05f)) * deltaTime;
            fiber.velocity *= 0.98f; // 抵抗
            
            // 境界での循環
            if (fiber.startPos.x < -100) {
                fiber.startPos.x = ofGetWidth() + 100;
                fiber.endPos.x = ofGetWidth() + 100;
            } else if (fiber.startPos.x > ofGetWidth() + 100) {
                fiber.startPos.x = -100;
                fiber.endPos.x = -100;
            }
            
            // 垂直方向の境界
            if (fiber.startPos.y < ofGetHeight() * 0.7f) {
                fiber.velocity.y = abs(fiber.velocity.y);
            } else if (fiber.startPos.y > ofGetHeight()) {
                fiber.velocity.y = -abs(fiber.velocity.y);
            }
            
            // 色の更新
            fiber.color = urbanColor(fiber.startPos.x * 0.1f, 0.4f + globalGrowthLevel * 0.3f);
        }
    }
    
    void updateGrowthVectors(float deltaTime) {
        for (auto& gv : growthVectors) {
            gv.age += deltaTime;
            gv.position += gv.direction * gv.intensity * deltaTime * 20.0f;
            
            // 境界での反射
            if (gv.position.x < 0 || gv.position.x > ofGetWidth()) {
                gv.direction.x *= -1;
                gv.position.x = ofClamp(gv.position.x, 0, ofGetWidth());
            }
            if (gv.position.y < 0 || gv.position.y > ofGetHeight()) {
                gv.direction.y *= -1;
                gv.position.y = ofClamp(gv.position.y, 0, ofGetHeight());
            }
            
            // 方向のランダム変化
            gv.direction += ofVec2f(ofRandom(-0.1f, 0.1f), ofRandom(-0.1f, 0.1f)) * deltaTime;
            gv.direction.normalize();
            
            // 色の更新
            gv.color = urbanColor(gv.position.x * 0.05f + gv.age * 10, gv.intensity);
        }
    }
    
    void updateConstructionWaves(float deltaTime) {
        for (auto& wave : constructionWaves) {
            wave.age += deltaTime;
            wave.radius += deltaTime * 50.0f * wave.intensity;
            wave.intensity *= 0.95f;
        }
        
        // 古い波を削除
        constructionWaves.erase(
            std::remove_if(constructionWaves.begin(), constructionWaves.end(),
                [](const ConstructionWave& w) { return w.intensity < 0.1f || w.radius > 300; }),
            constructionWaves.end()
        );
    }
    
    void drawGrowthVectors() {
        ofEnableBlendMode(OF_BLENDMODE_ADD);
        
        for (auto& gv : growthVectors) {
            ofColor growthColor = gv.color;
            growthColor.a = 150 * gv.intensity * (0.5f + globalGrowthLevel * 0.4f);
            growthColor.setBrightness(ofClamp(growthColor.getBrightness() * 0.7f, 30, 120));
            ofSetColor(growthColor);
            
            ofSetLineWidth(1 + gv.intensity * 2);
            ofVec2f endPos = gv.position + gv.direction * gv.intensity * 30;
            ofDrawLine(gv.position, endPos);
            
            // 矢印の先端
            ofVec2f perpendicular(-gv.direction.y, gv.direction.x);
            perpendicular *= 5;
            ofDrawLine(endPos, endPos - gv.direction * 8 + perpendicular);
            ofDrawLine(endPos, endPos - gv.direction * 8 - perpendicular);
        }
        
        ofDisableBlendMode();
    }
    
    void drawConstructionWaves() {
        ofEnableBlendMode(OF_BLENDMODE_ADD);
        
        for (auto& wave : constructionWaves) {
            ofColor waveColor = accentColor(wave.intensity);
            waveColor.a = 100 * wave.intensity * (0.4f + globalGrowthLevel * 0.3f);
            waveColor.setBrightness(ofClamp(waveColor.getBrightness() * 0.6f, 20, 100));
            ofSetColor(waveColor);
            
            ofSetLineWidth(1 + wave.intensity * 2);
            ofNoFill();
            ofDrawCircle(wave.center.x, wave.center.y, wave.radius);
            ofFill();
        }
        
        ofDisableBlendMode();
    }
    
    void triggerConstructionWave(ofVec2f center, float intensity) {
        ConstructionWave wave;
        wave.center = center;
        wave.radius = 10;
        wave.intensity = intensity;
        wave.age = 0;
        constructionWaves.push_back(wave);
    }
    
    void addGrowthVector(ofVec2f position, float intensity) {
        if (growthVectors.size() < 12) {
            GrowthVector gv;
            gv.position = position;
            gv.direction = ofVec2f(cos(ofRandom(TWO_PI)), sin(ofRandom(TWO_PI)));
            gv.intensity = intensity;
            gv.age = 0.0f;
            gv.color = urbanColor(position.x * 0.1f, intensity);
            growthVectors.push_back(gv);
        }
    }
};