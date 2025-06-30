#pragma once

#include "VisualSystem.h"
#include <vector>
#include <map>

// 都市セル（Chemical Reaction Diffusionを都市現象にメタファー）
struct UrbanCell {
    ofVec2f position;
    float density;          // 人口密度 (chemical A)
    float activity;         // 経済活動 (chemical B)
    float infrastructure;   // インフラ発達度
    float pollution;        // 汚染度
    float age;             // セルの年齢
    ofColor color;
    bool isActive;
    
    // 近隣セルへの影響度
    float diffusionRate;
    float reactionRate;
    
    UrbanCell() {
        position = ofVec2f(0, 0);
        density = ofRandom(0.1f, 0.3f);
        activity = ofRandom(0.0f, 0.1f);
        infrastructure = 0.0f;
        pollution = 0.0f;
        age = 0.0f;
        color = ofColor::white;
        isActive = true;
        diffusionRate = ofRandom(0.8f, 1.2f);
        reactionRate = ofRandom(0.9f, 1.1f);
    }
    
    void update(float deltaTime, float globalGrowth) {
        age += deltaTime;
        
        // 都市発達の自然進化
        if (density > 0.5f && activity > 0.3f) {
            infrastructure += deltaTime * 0.1f * globalGrowth;
        }
        
        // 汚染の蓄積
        pollution += activity * deltaTime * 0.05f;
        pollution *= 0.995f; // 自然減衰
        
        // 色の更新
        updateColor(globalGrowth);
    }
    
    void updateColor(float globalGrowth) {
        // モノトーンベース + アクセントカラー方式
        if (infrastructure > 0.7f) {
            // 高度発達地区: アクセント1 (ブルー)
            color = ofColor(60, 90 + activity * 30, 130 + activity * 25);
        } else if (density > 0.6f) {
            // 住宅密集地: ライトグレー（モノトーン）
            color = ofColor(100 + density * 40, 100 + density * 40, 100 + density * 40);
        } else if (activity > 0.4f) {
            // 商業地区: アクセント2 (シアン)
            color = ofColor(50, 110 + activity * 30, 120 + activity * 25);
        } else {
            // 未発達地区: ダークグレー（モノトーン）
            color = ofColor(60 + density * 30, 60 + density * 30, 60 + density * 30);
        }
        
        // 汚染による変化（控えめ）
        if (pollution > 0.5f) {
            color.r = ofClamp(color.r * (1.0f + pollution * 0.1f), 0, 200);
            color.g *= (1.0f - pollution * 0.1f);
        }
    }
};

// 都市ゾーンの定義
enum UrbanZoneType {
    RESIDENTIAL,
    COMMERCIAL,
    INDUSTRIAL,
    RECREATIONAL,
    TRANSPORTATION
};

struct UrbanZone {
    ofVec2f center;
    float radius;
    UrbanZoneType type;
    float development;
    float influence;
    ofColor zoneColor;
    
    UrbanZone(ofVec2f c, float r, UrbanZoneType t) {
        center = c;
        radius = r;
        type = t;
        development = 0.0f;
        influence = 1.0f;
        
        switch(type) {
            case RESIDENTIAL: zoneColor = ofColor(100, 150, 100); break;
            case COMMERCIAL: zoneColor = ofColor(150, 150, 100); break;
            case INDUSTRIAL: zoneColor = ofColor(150, 100, 100); break;
            case RECREATIONAL: zoneColor = ofColor(100, 100, 150); break;
            case TRANSPORTATION: zoneColor = ofColor(120, 120, 120); break;
        }
    }
};

class ReactionDiffusionSystem : public VisualSystem {
private:
    // 不規則配置ベースの都市シミュレーション
    std::vector<UrbanCell> urbanCells;  // 1D vector for scattered cells
    std::vector<ofVec2f> cellPositions; // Random positions for cells
    int numCells;
    float cellSize;
    
    // 都市ゾーン
    std::vector<UrbanZone> urbanZones;
    
    // パラメータ
    float populationGrowthRate = 0.02f;
    float economicGrowthRate = 0.015f;
    float diffusionSpeed = 0.8f;
    float reactionIntensity = 1.0f;
    
    // 視覚エフェクト
    std::vector<ofVec2f> activityCenters;
    std::vector<ofPolyline> transportationLines;
    std::vector<ofVec2f> constructionSites;
    
    // 都市現象
    float trafficFlow = 0.0f;
    float urbanPulse = 0.0f;
    float economicCycle = 0.0f;
    
    // 累積エフェクト
    float totalUrbanization = 0.0f;
    float infrastructureDensity = 0.0f;
    bool isMegaCity = false;
    
    // 革新的視覚要素
    std::vector<ofVec2f> lightPoints;      // 都市の光
    std::vector<ofVec2f> smokeSources;     // 工業煙
    std::vector<ofPolyline> energyFlows;   // エネルギーフロー
    
    // ポリゴンベースの都市要素
    std::vector<std::vector<ofVec2f>> cellPolygons;  // セル用不規則ポリゴン
    std::vector<std::vector<ofVec2f>> buildingPolygons; // 建物用不規則ポリゴン
    std::vector<std::vector<ofVec2f>> windowPolygons;   // 窓用不規則ポリゴン
    
public:
    void setup() override {
        // 不規則セル配置の初期化
        cellSize = 12.0f; // ベースセルサイズ
        numCells = 800; // 固定セル数でランダム配置
        
        urbanCells.resize(numCells);
        cellPositions.resize(numCells);
        
        // ランダムスキャッター配置
        for (int i = 0; i < numCells; i++) {
            // クラスター配置とランダム配置のミックス
            if (ofRandom(1.0f) < 0.6f) {
                // クラスター配置（60%）
                ofVec2f clusterCenter = ofVec2f(ofRandom(100, ofGetWidth()-100), ofRandom(100, ofGetHeight()-100));
                float clusterRadius = ofRandom(30, 80);
                float angle = ofRandom(TWO_PI);
                float distance = ofRandom(clusterRadius);
                cellPositions[i] = clusterCenter + ofVec2f(cos(angle) * distance, sin(angle) * distance);
            } else {
                // 完全ランダム配置（40%）
                cellPositions[i] = ofVec2f(ofRandom(20, ofGetWidth()-20), ofRandom(20, ofGetHeight()-20));
            }
            
            urbanCells[i].position = cellPositions[i];
        }
        
        // 初期都市核の配置
        createInitialUrbanSeeds();
        
        // 都市ゾーンの初期設定
        initializeUrbanZones();
        
        // エフェクト要素の初期化（密度削減）
        lightPoints.resize(8);
        smokeSources.resize(3);
        energyFlows.resize(3);
        
        // ポリゴン要素の初期化
        initializeCellPolygons();
        initializeBuildingPolygons();
        initializeWindowPolygons();
        
        // 交通網の初期化
        initializeTransportation();
    }
    
    void update(float deltaTime) override {
        // 統一エフェクトシステムの更新
        updateGlobalEffects(deltaTime);
        
        urbanPulse += deltaTime * 1.5f;
        economicCycle += deltaTime * 0.3f;
        
        // 都市シミュレーションの更新
        updateUrbanCells(deltaTime);
        
        // ゾーンの発達
        updateUrbanZones(deltaTime);
        
        // 拡散・反応の計算
        calculateUrbanDiffusion(deltaTime);
        calculateUrbanReactions(deltaTime);
        
        // 交通フローの更新
        updateTrafficFlow(deltaTime);
        
        // 都市化レベルの計算
        calculateUrbanizationLevel();
        
        // 革新的エフェクトの更新
        updateAdvancedUrbanEffects(deltaTime);
        
        // 崩壊時の効果
        if (isCollapsing) {
            applyUrbanDecay();
        }
    }
    
    void draw() override {
        // マスターバッファに描画開始
        beginMasterBuffer();
        
        // 都市背景
        drawUrbanBackground();
        
        // セルラーオートマトンの都市グリッド
        drawUrbanCells();
        
        // 都市ゾーンの描画
        drawUrbanZones();
        
        // 交通網
        drawTransportationNetwork();
        
        // 活動センター
        drawActivityCenters();
        
        // 高成長時の革新的エフェクト
        if (globalGrowthLevel > 0.4f) {
            drawAdvancedUrbanVisualization();
        }
        
        // メガシティエフェクト
        if (isMegaCity) {
            drawMegaCityEffects();
        }
        
        endMasterBuffer();
        
        // 全画面エフェクトの描画
        drawFullscreenEffects();
        
        // 情報表示
        drawUrbanStatistics();
    }
    
    void onMidiMessage(ofxMidiMessage& msg) override {
        if (msg.status == MIDI_NOTE_ON && msg.velocity > 0) {
            currentNote = msg.pitch;
            currentVelocity = msg.velocity;
            
            triggerImpact(msg.pitch, msg.velocity);
            
            // ドラムに応じた都市現象
            switch(msg.pitch) {
                case KICK:
                    // 人口爆発・密集化
                    triggerPopulationBoom(impactIntensity * 2.0f);
                    break;
                    
                case SNARE:
                    // 経済発展・商業活動
                    triggerEconomicDevelopment(impactIntensity * 1.5f);
                    break;
                    
                case HIHAT_CLOSED:
                    // インフラ建設・細かい発達
                    triggerInfrastructureDevelopment(impactIntensity);
                    break;
                    
                case CRASH:
                    // 都市大変革・メガプロジェクト
                    triggerUrbanTransformation();
                    break;
                    
                default:
                    // 音程に基づく局所的発展
                    ofVec2f developmentCenter(
                        ofMap(msg.pitch % 12, 0, 12, 50, ofGetWidth() - 50),
                        ofMap(msg.pitch / 12, 0, 10, 50, ofGetHeight() - 50)
                    );
                    triggerLocalDevelopment(developmentCenter, impactIntensity);
            }
            
            // 都市化の促進
            totalUrbanization += impactIntensity * 0.1f;
            infrastructureDensity += impactIntensity * 0.05f;
            
        } else if (msg.status == MIDI_CONTROL_CHANGE) {
            if (msg.control == 1) { // Mod wheel
                modulation = mapCC(msg.value);
                // モジュレーションで都市計画を変更
                reactionIntensity = 0.5f + modulation * 1.5f;
                diffusionSpeed = 0.5f + modulation * 1.0f;
            }
        }
    }
    
private:
    void drawWaveCircle(ofVec2f center, float baseRadius, float intensity) {
        // MIDI連動ウェーブ変形円を描画
        ofBeginShape();
        int numPoints = 20;  // 円の分割数
        
        // ベースウェーブ強度（常時動作）
        float baseWaveIntensity = 0.4f + globalGrowthLevel * 0.3f;
        
        for (int i = 0; i <= numPoints; i++) {
            float angle = (i / float(numPoints)) * TWO_PI;
            
            // 特殊な細胞オートマトン風ウェーブ効果
            float waveOffset = sin(angle * 4 + systemTime * 2.5f) * baseRadius * 0.3f * baseWaveIntensity;
            waveOffset += sin(angle * 6 + systemTime * 3.5f) * baseRadius * 0.2f * baseWaveIntensity;
            waveOffset += sin(angle * 8 + systemTime * 1.8f) * baseRadius * 0.1f * baseWaveIntensity;
            
            // 細胞間相互作用風の細かい振動
            waveOffset += sin(angle * 12 + systemTime * 4.5f) * baseRadius * 0.06f * baseWaveIntensity;
            
            // MIDI連動：反応拡散インパクト
            float midiInfluence = (impactIntensity + intensity) * sin(angle * 5 + systemTime * 5) * baseRadius * 0.4f;
            
            float radius = baseRadius + waveOffset + midiInfluence;
            radius = max(radius, baseRadius * 0.25f);  // 最小サイズ制限
            
            float x = center.x + cos(angle) * radius;
            float y = center.y + sin(angle) * radius;
            ofVertex(x, y);
        }
        ofEndShape(true);
    }
    
    void createInitialUrbanSeeds() {
        // 初期都市核の不規則配置
        int numSeeds = 3;
        std::vector<ofVec2f> seedCenters;
        
        for (int i = 0; i < numSeeds; i++) {
            seedCenters.push_back(ofVec2f(
                ofRandom(ofGetWidth() * 0.2f, ofGetWidth() * 0.8f),
                ofRandom(ofGetHeight() * 0.2f, ofGetHeight() * 0.8f)
            ));
        }
        
        // 各セルを最寄りの核と距離で影響度を計算
        for (int i = 0; i < numCells; i++) {
            float minDistance = ofGetWidth();
            for (auto& seedCenter : seedCenters) {
                float distance = cellPositions[i].distance(seedCenter);
                if (distance < minDistance) {
                    minDistance = distance;
                }
            }
            
            // 距離に基づく影響度（不規則分散）
            float influence = ofClamp(1.0f - (minDistance / 120.0f), 0.0f, 1.0f);
            if (influence > 0.2f) {
                urbanCells[i].density = ofRandom(0.6f, 0.9f) * influence;
                urbanCells[i].activity = ofRandom(0.4f, 0.7f) * influence;
                urbanCells[i].infrastructure = ofRandom(0.2f, 0.5f) * influence;
            }
        }
    }
    
    void initializeUrbanZones() {
        // 都市ゾーンの配置（スペーシング拡大）
        urbanZones.push_back(UrbanZone(ofVec2f(ofGetWidth() * 0.25f, ofGetHeight() * 0.25f), 100, RESIDENTIAL));
        urbanZones.push_back(UrbanZone(ofVec2f(ofGetWidth() * 0.75f, ofGetHeight() * 0.25f), 80, COMMERCIAL));
        urbanZones.push_back(UrbanZone(ofVec2f(ofGetWidth() * 0.5f, ofGetHeight() * 0.75f), 90, INDUSTRIAL));
    }
    
    void initializeTransportation() {
        // 主要交通路の設定（密度削減）
        transportationLines.resize(4);
        
        // 横断道路
        transportationLines[0].addVertex(0, ofGetHeight() * 0.3f);
        transportationLines[0].addVertex(ofGetWidth(), ofGetHeight() * 0.3f);
        
        // 縦断道路
        transportationLines[1].addVertex(ofGetWidth() * 0.5f, 0);
        transportationLines[1].addVertex(ofGetWidth() * 0.5f, ofGetHeight());
        
        // 対角線道路
        transportationLines[2].addVertex(0, 0);
        transportationLines[2].addVertex(ofGetWidth(), ofGetHeight());
        
        transportationLines[3].addVertex(ofGetWidth(), 0);
        transportationLines[3].addVertex(0, ofGetHeight());
    }
    
    void updateUrbanCells(float deltaTime) {
        for (int i = 0; i < numCells; i++) {
            if (urbanCells[i].isActive) {
                urbanCells[i].update(deltaTime, globalGrowthLevel);
            }
        }
    }
    
    void updateUrbanZones(float deltaTime) {
        for (auto& zone : urbanZones) {
            zone.development += deltaTime * 0.02f * (1.0f + globalGrowthLevel);
            zone.influence = 0.8f + sin(economicCycle + zone.center.x * 0.01f) * 0.2f;
            
            // ゾーンの周囲のセルに不規則影響
            for (int i = 0; i < numCells; i++) {
                float distance = cellPositions[i].distance(zone.center);
                
                if (distance <= zone.radius) {
                    float influence = (1.0f - distance / zone.radius) * zone.influence;
                    
                    switch(zone.type) {
                        case RESIDENTIAL:
                            urbanCells[i].density += deltaTime * influence * 0.1f;
                            break;
                        case COMMERCIAL:
                            urbanCells[i].activity += deltaTime * influence * 0.15f;
                            break;
                        case INDUSTRIAL:
                            urbanCells[i].activity += deltaTime * influence * 0.1f;
                            urbanCells[i].pollution += deltaTime * influence * 0.05f;
                            break;
                        case RECREATIONAL:
                            urbanCells[i].pollution -= deltaTime * influence * 0.02f;
                            break;
                        case TRANSPORTATION:
                            urbanCells[i].infrastructure += deltaTime * influence * 0.08f;
                            break;
                    }
                }
            }
        }
    }
    
    void calculateUrbanDiffusion(float deltaTime) {
        // 不規則配置での拡散計算（近隣セル検索）
        std::vector<UrbanCell> newCells = urbanCells;
        
        for (int i = 0; i < numCells; i++) {
            float totalDensity = 0;
            float totalActivity = 0;
            int neighbors = 0;
            float neighborRadius = cellSize * 2.5f; // 近隣検索半径
            
            // 近隣セルの不規則検索
            for (int j = 0; j < numCells; j++) {
                if (i == j) continue;
                
                float distance = cellPositions[i].distance(cellPositions[j]);
                if (distance <= neighborRadius) {
                    float weight = 1.0f - (distance / neighborRadius); // 距離による重み
                    totalDensity += urbanCells[j].density * weight;
                    totalActivity += urbanCells[j].activity * weight;
                    neighbors++;
                }
            }
            
            if (neighbors > 0) {
                float avgDensity = totalDensity / neighbors;
                float avgActivity = totalActivity / neighbors;
                
                // 拡散適用
                float diffusionFactor = diffusionSpeed * deltaTime * urbanCells[i].diffusionRate;
                newCells[i].density += (avgDensity - urbanCells[i].density) * diffusionFactor * 0.1f;
                newCells[i].activity += (avgActivity - urbanCells[i].activity) * diffusionFactor * 0.15f;
            }
        }
        
        urbanCells = newCells;
    }
    
    void calculateUrbanReactions(float deltaTime) {
        // 反応計算（人口と経済活動の相互作用）
        for (int i = 0; i < numCells; i++) {
            UrbanCell& cell = urbanCells[i];
            
            // Gray-Scott方程式の都市版
            float density = cell.density;
            float activity = cell.activity;
            
            // 都市反応: 人口と経済活動の相互促進
            float populationBoost = density * activity * activity * reactionIntensity;
            float economicBoost = density * activity * activity * reactionIntensity;
            
            // フィードバック
            float populationSupply = populationGrowthRate * (1.0f - density) * (1.0f + globalGrowthLevel);
            float economicSupply = economicGrowthRate * (1.0f - activity) * (1.0f + globalGrowthLevel);
            
            // 減衰
            float populationLoss = (0.01f + populationGrowthRate) * activity;
            float economicLoss = (0.015f + economicGrowthRate) * density;
            
            // 更新
            cell.density += deltaTime * (populationSupply - populationBoost + populationLoss);
            cell.activity += deltaTime * (economicSupply + economicBoost - economicLoss);
            
            // 範囲制限
            cell.density = ofClamp(cell.density, 0.0f, 1.0f);
            cell.activity = ofClamp(cell.activity, 0.0f, 1.0f);
        }
    }
    
    void updateTrafficFlow(float deltaTime) {
        trafficFlow = sin(urbanPulse) * 0.5f + 0.5f;
        
        // 交通渋滞の動的更新
        for (int i = 0; i < transportationLines.size(); i++) {
            auto& line = transportationLines[i];
            
            // 交通密度に基づく道路の変化
            float trafficDensity = 0.3f + trafficFlow * 0.7f + globalGrowthLevel * 0.2f;
            
            // 道路沿いの発達促進
            for (int j = 0; j < line.size(); j++) {
                ofVec2f point = line[j];
                int gridX = point.x / cellSize;
                int gridY = point.y / cellSize;
                
                // 道路沿いの不規則セルに影響
                for (int k = 0; k < numCells; k++) {
                    float distance = cellPositions[k].distance(point);
                    if (distance <= cellSize * 1.5f) {
                        float influence = 1.0f - (distance / (cellSize * 1.5f));
                        urbanCells[k].infrastructure += deltaTime * trafficDensity * 0.05f * influence;
                        urbanCells[k].activity += deltaTime * trafficDensity * 0.03f * influence;
                    }
                }
            }
        }
    }
    
    void calculateUrbanizationLevel() {
        float totalDensity = 0;
        float totalActivity = 0;
        float totalInfra = 0;
        int activeCells = 0;
        
        for (int i = 0; i < numCells; i++) {
            if (urbanCells[i].isActive) {
                totalDensity += urbanCells[i].density;
                totalActivity += urbanCells[i].activity;
                totalInfra += urbanCells[i].infrastructure;
                activeCells++;
            }
        }
        
        if (activeCells > 0) {
            float avgDensity = totalDensity / activeCells;
            float avgActivity = totalActivity / activeCells;
            float avgInfra = totalInfra / activeCells;
            
            totalUrbanization = (avgDensity + avgActivity + avgInfra) / 3.0f;
            infrastructureDensity = avgInfra;
            
            // メガシティ判定
            isMegaCity = (totalUrbanization > 0.8f && infrastructureDensity > 0.7f);
        }
    }
    
    void updateAdvancedUrbanEffects(float deltaTime) {
        // 都市の光の更新
        for (int i = 0; i < lightPoints.size(); i++) {
            if (ofRandom(1.0f) < 0.1f) {
                lightPoints[i] = ofVec2f(ofRandom(ofGetWidth()), ofRandom(ofGetHeight()));
            }
        }
        
        // 煙源の更新
        for (int i = 0; i < smokeSources.size(); i++) {
            // 工業地域の近くに配置
            for (auto& zone : urbanZones) {
                if (zone.type == INDUSTRIAL && ofRandom(1.0f) < 0.2f) {
                    smokeSources[i] = zone.center + ofVec2f(ofRandom(-30, 30), ofRandom(-30, 30));
                }
            }
        }
        
        // エネルギーフローの更新
        for (int i = 0; i < energyFlows.size(); i++) {
            energyFlows[i].clear();
            
            int numPoints = 8 + globalGrowthLevel * 12;
            for (int j = 0; j < numPoints; j++) {
                float t = j / float(numPoints - 1);
                float x = ofLerp(ofRandom(ofGetWidth() * 0.1f), ofRandom(ofGetWidth() * 0.9f), t);
                float y = ofGetHeight() * 0.5f + sin(t * PI * 2 + systemTime + i) * 50;
                
                energyFlows[i].addVertex(x, y);
            }
        }
    }
    
    void drawUrbanBackground() {
        // クールな都市背景
        ofColor bgColor = ofColor::fromHsb(210, 50, 20 + globalGrowthLevel * 20);
        ofSetColor(bgColor);
        ofDrawRectangle(0, 0, ofGetWidth(), ofGetHeight());
    }
    
    void drawUrbanCells() {
        // ホワイトアウト防止: ADDモードをALPHAモードに変更
        ofEnableBlendMode(OF_BLENDMODE_ALPHA);
        
        for (int i = 0; i < numCells; i++) {
            UrbanCell& cell = urbanCells[i];
            
            if (cell.isActive && (cell.density > 0.1f || cell.activity > 0.1f)) {
                ofColor cellColor = cell.color;
                // モノトーン+アクセント方式: 色をそのまま使用
                cellColor.a = 80 + (cell.density + cell.activity + cell.infrastructure) * 30; // より見えやすいアルファ値
                ofSetColor(cellColor);
                
                // 変数サイズの不規則ポリゴンでセルを描画
                float sizeMultiplier = 0.5f + (cell.density + cell.activity) * 1.2f; // より大きなサイズ変動
                float variableSize = cellSize * sizeMultiplier * ofRandom(0.7f, 1.4f); // ランダムサイズ変動
                
                drawVariableCellPolygon(i, cellPositions[i], variableSize);
                
                // 高密度セルの特別表示（不規則形状）
                if (cell.density > 0.8f && cell.activity > 0.6f) {
                    // アクセントマーカー: ウォームカラー
                    ofColor markerColor = ofColor(140, 120, 100, 150);
                    ofSetColor(markerColor);
                    // 不規則な高密度マーカー
                    for (int j = 0; j < 6; j++) {
                        float angle = j * PI / 3.0f + ofRandom(-0.3f, 0.3f);
                        float radius = variableSize * 0.2f + ofRandom(-2, 2);
                        ofVec2f markerPos = cellPositions[i] + ofVec2f(cos(angle) * radius, sin(angle) * radius);
                        float markerSize = ofRandom(1, 3);
                        drawWaveCircle(markerPos, markerSize, cell.density);
                    }
                }
            }
        }
        
        ofDisableBlendMode();
    }
    
    void drawUrbanZones() {
        if (globalGrowthLevel > 0.3f) {
            // ホワイトアウト防止: ALPHAモード使用
            ofEnableBlendMode(OF_BLENDMODE_ALPHA);
            
            for (auto& zone : urbanZones) {
                ofColor zoneColor = zone.zoneColor;
                // モノトーン+アクセント: ゾーン色をそのまま使用
                zoneColor.a = 60 + zone.development * 40; // より見えやすいアルファ値
                ofSetColor(zoneColor);
                
                ofSetLineWidth(1 + globalGrowthLevel * 0.8);
                ofNoFill();
                float zoneRadius = zone.radius * (0.8f + zone.influence * 0.2f);
                drawWaveCircle(zone.center, zoneRadius, zone.influence);
                ofFill();
                
                // ゾーン名表示
                if (zone.development > 0.5f) {
                    ofSetColor(120, 120, 120, 120); // グレーテキスト
                    string zoneTypeStr = (zone.type == RESIDENTIAL) ? "R" : 
                                        (zone.type == COMMERCIAL) ? "C" : 
                                        (zone.type == INDUSTRIAL) ? "I" : 
                                        (zone.type == RECREATIONAL) ? "P" : "T";
                    ofDrawBitmapString(zoneTypeStr, zone.center.x - 5, zone.center.y + 5);
                }
            }
            
            ofDisableBlendMode();
        }
    }
    
    void drawTransportationNetwork() {
        if (infrastructureDensity > 0.2f) {
            // ホワイトアウト防止: ALPHAモード使用
            ofEnableBlendMode(OF_BLENDMODE_ALPHA);
            
            // インフラ: グレーライン（モノトーン）
            ofColor roadColor = ofColor(80, 80, 80);
            roadColor.a = 100 + infrastructureDensity * 50; // より見えやすいアルファ値
            ofSetColor(roadColor);
            
            ofSetLineWidth(1 + infrastructureDensity * 2);
            
            for (auto& line : transportationLines) {
                line.draw();
                
                // 交通流の可視化
                if (trafficFlow > 0.6f) {
                    for (int i = 0; i < line.size() - 1; i++) {
                        ofVec2f start = line[i];
                        ofVec2f end = line[i + 1];
                        ofVec2f mid = (start + end) * 0.5f;
                        
                        ofSetColor(accentColor(trafficFlow));
                        float trafficSize = 2 + trafficFlow * 3;
                        drawWaveCircle(mid, trafficSize, trafficFlow);
                    }
                }
            }
            
            ofDisableBlendMode();
        }
    }
    
    void drawActivityCenters() {
        if (activityCenters.empty()) return;
        
        // ホワイトアウト防止: ALPHAモード使用
        ofEnableBlendMode(OF_BLENDMODE_ALPHA);
        
        for (auto& center : activityCenters) {
            ofColor centerColor = accentColor(totalUrbanization);
            // アクティビティセンター: アクセントカラー
            centerColor = ofColor(100, 140, 160); // アクセント: ライトシアン
            centerColor.a = 120; // より見えやすいアルファ値
            ofSetColor(centerColor);
            
            float pulse = sin(urbanPulse * 2 + center.x * 0.01f) * 0.3f + 0.7f;
            float pulseSize = 3 + pulse * 5;
            drawWaveCircle(center, pulseSize, pulse);
            
            // 活動の放射
            if (globalGrowthLevel > 0.6f) {
                for (int i = 0; i < 8; i++) {
                    float angle = (i / 8.0f) * TWO_PI + urbanPulse;
                    ofVec2f rayEnd = center + ofVec2f(cos(angle), sin(angle)) * (20 + pulse * 15);
                    
                    ofSetLineWidth(1 + pulse);
                    ofDrawLine(center, rayEnd);
                }
            }
        }
        
        ofDisableBlendMode();
    }
    
    void drawAdvancedUrbanVisualization() {
        // ホワイトアウト防止: ALPHAモード使用
        ofEnableBlendMode(OF_BLENDMODE_ALPHA);
        
        // 都市の光（夜景）
        for (auto& light : lightPoints) {
            // 都市の光: アクセントカラー
            ofColor lightColor = ofColor(120, 150, 180); // アクセント: ライトブルー
            lightColor.a = 100 * globalGrowthLevel; // より見えやすいアルファ値
            ofSetColor(lightColor);
            
            float brightness = 0.5f + sin(systemTime * 3 + light.x * 0.01f) * 0.5f;
            float lightSize = 2 + brightness * 4;
            drawWaveCircle(light, lightSize, brightness);
        }
        
        // 工業煙
        for (auto& smoke : smokeSources) {
            ofColor smokeColor = urbanColor(currentNote - 20, 0.4f);
            smokeColor.a = 60;
            ofSetColor(smokeColor);
            
            for (int i = 0; i < 8; i++) {
                float offset = i * 10;
                ofVec2f smokePos = smoke + ofVec2f(ofRandom(-5, 5), -offset + sin(systemTime + i) * 3);
                float smokeSize = 3 + i;
                drawWaveCircle(smokePos, smokeSize, globalGrowthLevel);
            }
        }
        
        // エネルギーフロー
        for (auto& flow : energyFlows) {
            if (flow.size() > 1) {
                ofColor flowColor = accentColor(0.8f);
                flowColor.a = 120;
                ofSetColor(flowColor);
                
                ofSetLineWidth(2 + globalGrowthLevel * 2);
                flow.draw();
                
                // フローの方向表示
                for (int i = 0; i < flow.size() - 1; i += 3) {
                    ofVec2f pos = flow[i];
                    float particleSize = 1 + sin(systemTime * 4 + i) * 2;
                    drawWaveCircle(pos, particleSize, globalGrowthLevel);
                }
            }
        }
        
        ofDisableBlendMode();
    }
    
    void drawMegaCityEffects() {
        // ホワイトアウト防止: ALPHAモード使用
        ofEnableBlendMode(OF_BLENDMODE_ALPHA);
        
        // スカイライン: ダークグレー（モノトーン）
        ofColor skylineColor = ofColor(90, 90, 90);
        skylineColor.a = 100; // より見えやすいアルファ値
        ofSetColor(skylineColor);
        
        drawIrregularSkyline();
        
        // 大気エフェクト（不規則ポリゴン）
        ofColor atmosphereColor = urbanColor(currentNote, 0.3f);
        atmosphereColor.a = 40;
        ofSetColor(atmosphereColor);
        
        drawAtmospherePolygons();
        
        ofDisableBlendMode();
    }
    
    void drawUrbanStatistics() {
        if (getTimeSinceLastMidi() < 5.0f) {
            ofSetColor(200);
            ofDrawBitmapString("Reaction-Diffusion Urban Simulation", 20, ofGetHeight() - 120);
            ofDrawBitmapString("Urbanization Level: " + ofToString(totalUrbanization * 100, 1) + "%", 20, ofGetHeight() - 100);
            ofDrawBitmapString("Infrastructure Density: " + ofToString(infrastructureDensity * 100, 1) + "%", 20, ofGetHeight() - 80);
            ofDrawBitmapString("Traffic Flow: " + ofToString(trafficFlow * 100, 1) + "%", 20, ofGetHeight() - 60);
            ofDrawBitmapString("Urban Zones: " + ofToString(urbanZones.size()), 20, ofGetHeight() - 40);
            if (isMegaCity) {
                ofSetColor(255, 200, 100);
                ofDrawBitmapString("MEGACITY STATUS", 20, ofGetHeight() - 20);
            }
        }
    }
    
    // MIDI反応メソッド
    void triggerPopulationBoom(float intensity) {
        // 人口爆発（不規則分散）
        int numAffectedCells = intensity * 60;
        
        // ランダムスキャッター方式で人口密度を上げる
        for (int i = 0; i < numAffectedCells; i++) {
            int randomIndex = ofRandom(numCells);
            
            // 中央寄りのクラスターエリアを重視
            ofVec2f center = ofVec2f(ofGetWidth() * 0.5f, ofGetHeight() * 0.5f);
            float distanceFromCenter = cellPositions[randomIndex].distance(center);
            float centerBias = 1.0f - ofClamp(distanceFromCenter / (ofGetWidth() * 0.4f), 0.0f, 1.0f);
            
            if (ofRandom(1.0f) < (0.3f + centerBias * 0.7f)) {
                urbanCells[randomIndex].density += intensity * ofRandom(0.2f, 0.4f);
                urbanCells[randomIndex].density = ofClamp(urbanCells[randomIndex].density, 0.0f, 1.0f);
            }
        }
        
        // 新しい住宅ゾーンの追加
        if (ofRandom(1.0f) < intensity && urbanZones.size() < 8) {
            ofVec2f newZoneCenter(ofRandom(100, ofGetWidth()-100), ofRandom(100, ofGetHeight()-100));
            urbanZones.push_back(UrbanZone(newZoneCenter, 40 + intensity * 30, RESIDENTIAL));
        }
    }
    
    void triggerEconomicDevelopment(float intensity) {
        // 経済発展（不規則分散活性化）
        int numAffectedCells = intensity * 40;
        
        // ランダムセレクション + クラスター形成
        std::vector<int> developmentCluster;
        
        for (int i = 0; i < numAffectedCells; i++) {
            int randomIndex = ofRandom(numCells);
            
            // 既存の活動的セルの近くを優先
            bool nearActiveCell = false;
            for (int j = 0; j < numCells; j++) {
                if (urbanCells[j].activity > 0.5f) {
                    float distance = cellPositions[randomIndex].distance(cellPositions[j]);
                    if (distance < cellSize * 3.0f && ofRandom(1.0f) < 0.7f) {
                        nearActiveCell = true;
                        break;
                    }
                }
            }
            
            float developmentBoost = nearActiveCell ? intensity * ofRandom(0.5f, 0.7f) : intensity * ofRandom(0.2f, 0.4f);
            
            urbanCells[randomIndex].activity += developmentBoost;
            urbanCells[randomIndex].infrastructure += developmentBoost * 0.5f;
            urbanCells[randomIndex].activity = ofClamp(urbanCells[randomIndex].activity, 0.0f, 1.0f);
            urbanCells[randomIndex].infrastructure = ofClamp(urbanCells[randomIndex].infrastructure, 0.0f, 1.0f);
        }
        
        // 活動センターの追加
        activityCenters.push_back(ofVec2f(ofRandom(ofGetWidth()), ofRandom(ofGetHeight())));
        if (activityCenters.size() > 10) {
            activityCenters.erase(activityCenters.begin());
        }
    }
    
    void triggerInfrastructureDevelopment(float intensity) {
        // インフラ建設
        for (auto& zone : urbanZones) {
            zone.development += intensity * 0.2f;
            zone.influence += intensity * 0.1f;
        }
        
        // 新しい交通路の追加
        if (ofRandom(1.0f) < intensity * 0.5f && transportationLines.size() < 8) {
            ofPolyline newRoad;
            newRoad.addVertex(ofRandom(ofGetWidth()), ofRandom(ofGetHeight()));
            newRoad.addVertex(ofRandom(ofGetWidth()), ofRandom(ofGetHeight()));
            transportationLines.push_back(newRoad);
        }
    }
    
    void triggerUrbanTransformation() {
        // 都市大変革（不規則波及効果）
        std::vector<int> transformationCells;
        
        // ランダムセルの30%を選択（不規則分散）
        for (int i = 0; i < numCells; i++) {
            if (ofRandom(1.0f) < 0.35f) {
                transformationCells.push_back(i);
            }
        }
        
        // 変革の不規則伝播
        for (int cellIndex : transformationCells) {
            float transformationIntensity = ofRandom(0.8f, 1.2f);
            
            urbanCells[cellIndex].density += 0.2f * transformationIntensity;
            urbanCells[cellIndex].activity += 0.3f * transformationIntensity;
            urbanCells[cellIndex].infrastructure += 0.4f * transformationIntensity;
            
            urbanCells[cellIndex].density = ofClamp(urbanCells[cellIndex].density, 0.0f, 1.0f);
            urbanCells[cellIndex].activity = ofClamp(urbanCells[cellIndex].activity, 0.0f, 1.0f);
            urbanCells[cellIndex].infrastructure = ofClamp(urbanCells[cellIndex].infrastructure, 0.0f, 1.0f);
            
            // 近隣セルへの不規則伝播
            for (int j = 0; j < numCells; j++) {
                if (j != cellIndex) {
                    float distance = cellPositions[cellIndex].distance(cellPositions[j]);
                    if (distance < cellSize * 2.0f && ofRandom(1.0f) < 0.4f) {
                        float rippleEffect = (1.0f - distance / (cellSize * 2.0f)) * 0.5f;
                        urbanCells[j].activity += 0.1f * rippleEffect;
                        urbanCells[j].infrastructure += 0.15f * rippleEffect;
                    }
                }
            }
        }
        
        // メガプロジェクトゾーンの追加
        ofVec2f megaCenter(ofGetWidth() * 0.5f, ofGetHeight() * 0.5f);
        urbanZones.push_back(UrbanZone(megaCenter, 100, COMMERCIAL));
        
        isMegaCity = true;
    }
    
    void triggerLocalDevelopment(ofVec2f center, float intensity) {
        // 局所的発展（不規則影響範囲）
        float baseRadius = 40 + intensity * 60;
        
        // 不規則影響範囲でセルを検索
        for (int i = 0; i < numCells; i++) {
            float distance = cellPositions[i].distance(center);
            
            // 不規則な影響範囲（一様でない）
            float irregularRadius = baseRadius * ofRandom(0.6f, 1.4f);
            
            if (distance <= irregularRadius) {
                float influence = (1.0f - distance / irregularRadius) * intensity;
                
                // ランダムな発展強度
                float developmentVariation = ofRandom(0.7f, 1.3f);
                
                urbanCells[i].density += influence * 0.2f * developmentVariation;
                urbanCells[i].activity += influence * 0.3f * developmentVariation;
                urbanCells[i].infrastructure += influence * 0.1f * developmentVariation;
                
                urbanCells[i].density = ofClamp(urbanCells[i].density, 0.0f, 1.0f);
                urbanCells[i].activity = ofClamp(urbanCells[i].activity, 0.0f, 1.0f);
                urbanCells[i].infrastructure = ofClamp(urbanCells[i].infrastructure, 0.0f, 1.0f);
            }
        }
    }
    
    void applyUrbanDecay() {
        // 都市衰退効果（不規則分散）
        for (int i = 0; i < numCells; i++) {
            if (ofRandom(1.0f) < 0.12f) {
                // 不規則な衰退パターン
                float decayIntensity = ofRandom(0.85f, 0.98f);
                
                urbanCells[i].density *= decayIntensity;
                urbanCells[i].activity *= (decayIntensity - 0.05f);
                urbanCells[i].infrastructure *= (decayIntensity - 0.03f);
                urbanCells[i].pollution += ofRandom(0.02f, 0.08f);
            }
        }
        
        isMegaCity = false;
    }
    
    // ポリゴン生成・描画メソッド
    void initializeCellPolygons() {
        cellPolygons.clear();
        cellPolygons.resize(numCells);
        
        for (int i = 0; i < numCells; i++) {
            // 変数サイズのポリゴン生成
            int vertices = 3 + ofRandom(6); // 3-8角形
            float baseRadius = cellSize * ofRandom(0.5f, 1.5f); // 大きなサイズ変動
            cellPolygons[i] = generateUrbanPolygon(vertices, baseRadius);
        }
    }
    
    void initializeBuildingPolygons() {
        buildingPolygons.clear();
        int numBuildings = 8;
        buildingPolygons.resize(numBuildings);
        
        for (int i = 0; i < numBuildings; i++) {
            buildingPolygons[i] = generateUrbanPolygon(4 + ofRandom(4), 50 + ofRandom(40));
        }
    }
    
    void initializeWindowPolygons() {
        windowPolygons.clear();
        windowPolygons.resize(40); // 複数建物用の窓ポリゴン
        
        for (int i = 0; i < windowPolygons.size(); i++) {
            windowPolygons[i] = generateUrbanPolygon(3 + ofRandom(2), 3 + ofRandom(5));
        }
    }
    
    std::vector<ofVec2f> generateUrbanPolygon(int vertices, float baseRadius) {
        std::vector<ofVec2f> polygon;
        
        for (int i = 0; i < vertices; i++) {
            float angle = (i / float(vertices)) * TWO_PI;
            float radius = baseRadius * (0.7f + ofRandom(0.6f));
            
            // 都市的な角張った形状を生成
            if (ofRandom(1.0f) < 0.3f) {
                radius *= 1.4f; // 時々突出部分を作る
            }
            
            ofVec2f point(cos(angle) * radius, sin(angle) * radius);
            polygon.push_back(point);
        }
        
        return polygon;
    }
    
    void drawVariableCellPolygon(int index, ofVec2f position, float size) {
        if (index >= cellPolygons.size()) return;
        
        ofPushMatrix();
        ofTranslate(position.x, position.y);
        
        // 動的サイズ変動
        float dynamicScale = size / cellSize;
        ofScale(dynamicScale, dynamicScale);
        
        // 軽微な回転でさらに不規則性を追加
        float rotation = ofNoise(index * 0.1f, systemTime * 0.02f) * 15.0f;
        ofRotateDeg(rotation);
        
        // 都市セルの不規則ポリゴン描画
        ofBeginShape();
        for (auto& point : cellPolygons[index]) {
            ofVertex(point.x, point.y);
        }
        ofEndShape(true);
        
        ofPopMatrix();
    }
    
    void drawIrregularSkyline() {
        int numBuildings = buildingPolygons.size();
        
        for (int i = 0; i < numBuildings; i++) {
            float x = (i / float(numBuildings)) * ofGetWidth();
            float height = 50 + ofNoise(i * 0.1f, systemTime * 0.1f) * 150;
            float width = ofGetWidth() / numBuildings * 0.8f;
            
            ofPushMatrix();
            ofTranslate(x + width * 0.5f, ofGetHeight() - height * 0.5f);
            ofScale(width / 100.0f, height / 100.0f);
            
            // 不規則建物ポリゴン描画
            ofBeginShape();
            for (auto& point : buildingPolygons[i]) {
                ofVertex(point.x, point.y);
            }
            ofEndShape(true);
            
            ofPopMatrix();
            
            // 不規則窓ポリゴンの配置
            drawBuildingWindows(x, ofGetHeight() - height, width, height, i);
        }
    }
    
    void drawBuildingWindows(float buildingX, float buildingY, float buildingWidth, float buildingHeight, int buildingIndex) {
        int windowsPerRow = 4;
        int windowRows = buildingHeight / 20;
        
        // 窓: アクセントカラー（ライトイエロー）
        ofSetColor(160, 150, 120, 120);
        
        for (int w = 0; w < windowsPerRow; w++) {
            for (int h = 0; h < windowRows; h++) {
                if (ofRandom(1.0f) < 0.7f) {
                    int windowIndex = (buildingIndex * windowsPerRow + w + h) % windowPolygons.size();
                    
                    float windowX = buildingX + (w / float(windowsPerRow)) * buildingWidth + ofRandom(-5, 5);
                    float windowY = buildingY + (h / float(windowRows)) * buildingHeight + ofRandom(-3, 3);
                    
                    ofPushMatrix();
                    ofTranslate(windowX, windowY);
                    ofScale(0.8f + ofRandom(0.4f), 0.8f + ofRandom(0.4f));
                    
                    // 不規則窓ポリゴン描画
                    ofBeginShape();
                    for (auto& point : windowPolygons[windowIndex]) {
                        ofVertex(point.x, point.y);
                    }
                    ofEndShape(true);
                    
                    ofPopMatrix();
                }
            }
        }
    }
    
    void drawAtmospherePolygons() {
        // 大気中の不規則形状（汚染物質、雲など）
        for (int i = 0; i < 15; i++) {
            float x = ofRandom(ofGetWidth());
            float y = ofRandom(ofGetHeight() * 0.7f);
            float size = ofRandom(8, 25);
            
            // 動的な不規則ポリゴン生成
            std::vector<ofVec2f> atmosphereShape = generateUrbanPolygon(5 + ofRandom(4), size);
            
            ofPushMatrix();
            ofTranslate(x, y);
            
            // 浮遊効果
            float drift = sin(systemTime + i * 0.5f) * 2;
            ofTranslate(drift, 0);
            
            ofBeginShape();
            for (auto& point : atmosphereShape) {
                ofVertex(point.x, point.y);
            }
            ofEndShape(true);
            
            ofPopMatrix();
        }
    }
};