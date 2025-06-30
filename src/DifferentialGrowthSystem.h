#pragma once

#include "VisualSystem.h"
#include <vector>
#include <deque>

struct UrbanNode {
    ofVec2f position;
    ofVec2f velocity;
    ofVec2f previousPosition;
    float mass = 1.0f;
    float maxSpeed = 3.0f;
    ofColor color;
    bool active = true;
    float age = 0.0f;
    float size = 1.0f;
    
    // 都市的特性
    float urbanDensity = 0.0f;         // 都市密度（建物の密集度）
    float infrastructureLevel = 0.0f;   // インフラ発達度
    float economicActivity = 0.0f;      // 経済活動
    float connectivity = 0.0f;          // 接続性（交通網）
    
    // ノードタイプ
    enum NodeType {
        RESIDENTIAL,
        COMMERCIAL,
        INDUSTRIAL,
        TRANSPORT_HUB,
        LANDMARK
    } type = RESIDENTIAL;
    
    UrbanNode(ofVec2f pos = ofVec2f(0, 0)) {
        position = pos;
        previousPosition = pos;
        velocity = ofVec2f(0, 0);
        color = ofColor::white;
        type = static_cast<NodeType>(ofRandom(5));
        
        // タイプに応じた初期特性
        switch(type) {
            case RESIDENTIAL:
                urbanDensity = ofRandom(0.3f, 0.7f);
                economicActivity = ofRandom(0.1f, 0.4f);
                break;
            case COMMERCIAL:
                urbanDensity = ofRandom(0.5f, 0.8f);
                economicActivity = ofRandom(0.6f, 0.9f);
                break;
            case INDUSTRIAL:
                urbanDensity = ofRandom(0.4f, 0.6f);
                economicActivity = ofRandom(0.7f, 1.0f);
                break;
            case TRANSPORT_HUB:
                connectivity = ofRandom(0.7f, 1.0f);
                infrastructureLevel = ofRandom(0.6f, 0.9f);
                break;
            case LANDMARK:
                urbanDensity = ofRandom(0.2f, 0.5f);
                economicActivity = ofRandom(0.3f, 0.6f);
                size = ofRandom(3.0f, 6.0f);
                break;
        }
    }
    
    void update(float deltaTime, float globalGrowth) {
        age += deltaTime;
        previousPosition = position;
        
        // 都市の発達による変化
        urbanDensity += deltaTime * 0.02f * globalGrowth;
        infrastructureLevel += deltaTime * 0.015f * globalGrowth;
        economicActivity += deltaTime * 0.01f * globalGrowth;
        connectivity += deltaTime * 0.008f * globalGrowth;
        
        // 値の制限
        urbanDensity = ofClamp(urbanDensity, 0.0f, 1.0f);
        infrastructureLevel = ofClamp(infrastructureLevel, 0.0f, 1.0f);
        economicActivity = ofClamp(economicActivity, 0.0f, 1.0f);
        connectivity = ofClamp(connectivity, 0.0f, 1.0f);
        
        // 速度の適用
        position += velocity * deltaTime;
        velocity *= 0.95f; // 抵抗
        
        // サイズの成長（都市発達に応じて）
        float targetSize = 1.0f + urbanDensity * 2.0f + economicActivity * 1.5f;
        size = ofLerp(size, targetSize, deltaTime * 2.0f);
        
        // 色の更新
        updateColor(globalGrowth);
    }
    
    void updateColor(float globalGrowth) {
        // モノトーンベース + アクセントカラー方式
        switch(type) {
            case RESIDENTIAL:
                // グレー系（モノトーン）
                color = ofColor(80 + urbanDensity * 60, 80 + urbanDensity * 60, 80 + urbanDensity * 60);
                break;
            case COMMERCIAL:
                // アクセント1: ソフトブルー
                color = ofColor(60, 90 + economicActivity * 40, 120 + economicActivity * 30);
                break;
            case INDUSTRIAL:
                // ダークグレー（モノトーン）
                color = ofColor(50 + economicActivity * 30, 50 + economicActivity * 30, 50 + economicActivity * 30);
                break;
            case TRANSPORT_HUB:
                // アクセント2: シアン
                color = ofColor(40, 100 + connectivity * 40, 110 + connectivity * 30);
                break;
            case LANDMARK:
                // アクセント3: ウォームグレー
                color = ofColor(100 + urbanDensity * 30, 90 + urbanDensity * 25, 80 + urbanDensity * 20);
                break;
        }
    }
};

// 都市接続（道路、線路など）
struct UrbanConnection {
    int nodeA, nodeB;
    float strength;
    float traffic;
    ofColor connectionColor;
    
    enum ConnectionType {
        ROAD,
        RAILWAY,
        PIPELINE,
        DATA_LINE,
        WALKWAY
    } type = ROAD;
    
    UrbanConnection(int a, int b, ConnectionType t = ROAD) {
        nodeA = a;
        nodeB = b;
        type = t;
        strength = ofRandom(0.5f, 1.0f);
        traffic = 0.0f;
        
        switch(type) {
            case ROAD: connectionColor = ofColor(120, 120, 120); break;
            case RAILWAY: connectionColor = ofColor(80, 80, 150); break;
            case PIPELINE: connectionColor = ofColor(150, 100, 80); break;
            case DATA_LINE: connectionColor = ofColor(100, 150, 200); break;
            case WALKWAY: connectionColor = ofColor(100, 150, 100); break;
        }
    }
};

class DifferentialGrowthSystem : public VisualSystem {
private:
    std::deque<UrbanNode> nodes;
    std::vector<UrbanConnection> connections;
    
    // Growth parameters
    float minDistance = 20.0f;     // 最小ノード間距離（スペーシング拡大）
    float maxDistance = 60.0f;     // 最大ノード間距離（スペーシング拡大）
    float cohesionRadius = 80.0f;  // 結合半径（拡大）
    float separationRadius = 35.0f; // 分離半径（拡大）
    
    // Urban growth parameters
    std::vector<ofVec2f> developmentCenters;
    std::vector<ofVec2f> metropolisSeeds;    // メトロポリスの種子
    float urbanPressure = 0.0f;
    float metropolitanExpansion = 0.0f;
    
    // 革新的要素
    std::vector<ofPolyline> transitLines;    // 交通路線
    std::vector<ofVec2f> megaProjects;       // 大型プロジェクト
    std::vector<ofVec2f> greenSpaces;        // 緑地・公園
    
    // Growth state
    bool isMetropolis = false;
    float metropolisLevel = 0.0f;
    float urbanComplexity = 0.0f;
    
    // MIDI-driven expansion
    std::vector<ofVec2f> impactCenters;
    float constructionActivity = 0.0f;
    
    // 視覚効果
    float trafficAnimation = 0.0f;
    float economicPulse = 0.0f;
    float developmentGlow = 0.0f;
    
    // ランダム眩しい演出
    float flashEffect = 0.0f;
    float flashTimer = 0.0f;
    
public:
    void setup() override {
        // 初期ノードの配置（都市の核）
        ofVec2f center(ofGetWidth() * 0.5f, ofGetHeight() * 0.5f);
        
        // 中心都市核（密度削減）
        for (int i = 0; i < 6; i++) {
            float angle = (i / 6.0f) * TWO_PI;
            float radius = 50 + ofRandom(-15, 15);  // 初期半径を拡大
            ofVec2f pos = center + ofVec2f(cos(angle), sin(angle)) * radius;
            nodes.push_back(UrbanNode(pos));
        }
        
        // 初期接続の作成
        createInitialConnections();
        
        // 開発センターの設定
        developmentCenters.push_back(center);
        
        // 革新的要素の初期化
        initializeUrbanInfrastructure();
    }
    
    void update(float deltaTime) override {
        // 統一エフェクトシステムの更新
        updateGlobalEffects(deltaTime);
        
        trafficAnimation += deltaTime * 2.0f;
        economicPulse += deltaTime * 1.0f;
        
        // 都市圧力の更新
        urbanPressure += deltaTime * 0.01f * (1.0f + globalGrowthLevel);
        metropolitanExpansion = globalGrowthLevel * (1.0f + sin(systemTime * 0.2f) * 0.3f);
        
        // ノードの更新と成長ロジック
        updateNodes(deltaTime);
        applyUrbanGrowthForces();
        handleNodeEvolution();
        handleConnectionEvolution();
        
        // 都市インフラの更新
        updateUrbanInfrastructure(deltaTime);
        
        // メトロポリス判定
        calculateMetropolisLevel();
        
        // 建設活動の減衰
        constructionActivity *= 0.98f;
        
        // 崩壊時の効果
        if (isCollapsing) {
            applyUrbanDecline();
        }
        
        // 革新的都市現象の更新
        updateAdvancedUrbanPhenomena(deltaTime);
    }
    
    void draw() override {
        // マスターバッファに描画開始
        beginMasterBuffer();
        
        // 都市背景
        drawMetropolitanBackground();
        
        // 都市接続ネットワーク
        drawUrbanConnections();
        
        // 成長ノードネットワーク
        drawUrbanNodes();
        
        // 交通システム
        drawTransitSystem();
        
        // 開発プロジェクト
        drawDevelopmentProjects();
        
        // 高成長時の革新的エフェクト
        if (globalGrowthLevel > 0.5f) {
            drawAdvancedMetropolitanEffects();
        }
        
        // メトロポリス効果
        if (isMetropolis) {
            drawMetropolisEffects();
        }
        
        endMasterBuffer();
        
        // 全画面エフェクトの描画
        drawFullscreenEffects();
        
        // 統計情報の表示
        drawUrbanStatistics();
    }
    
    void onMidiMessage(ofxMidiMessage& msg) override {
        if (msg.status == MIDI_NOTE_ON && msg.velocity > 0) {
            currentNote = msg.pitch;
            currentVelocity = msg.velocity;
            
            triggerImpact(msg.pitch, msg.velocity);
            
            constructionActivity = impactIntensity;
            
            // ドラムに応じた都市開発
            switch(msg.pitch) {
                case KICK:
                    // 大規模都市拡張
                    triggerMajorUrbanExpansion(impactIntensity * 2.0f);
                    break;
                    
                case SNARE:
                    // 交通網拡張
                    triggerTransportExpansion(impactIntensity * 1.5f);
                    break;
                    
                case HIHAT_CLOSED:
                    // 地域開発・細分化
                    triggerLocalDevelopment(impactIntensity);
                    break;
                    
                case CRASH:
                    // メトロポリス変革 + フラッシュ効果
                    triggerMetropolitanTransformation();
                    flashEffect = 1.0f; // 眩しい演出トリガー
                    break;
                    
                default:
                    // 音程に基づく開発
                    ofVec2f developmentSite(
                        ofMap(msg.pitch % 12, 0, 12, 100, ofGetWidth() - 100),
                        ofMap(msg.pitch / 12, 0, 10, 100, ofGetHeight() - 100)
                    );
                    triggerTargetedDevelopment(developmentSite, impactIntensity);
            }
            
            // 都市圧力の増加
            urbanPressure += impactIntensity * 0.2f;
            urbanComplexity += impactIntensity * 0.1f;
            
        } else if (msg.status == MIDI_CONTROL_CHANGE) {
            if (msg.control == 1) { // Mod wheel
                modulation = mapCC(msg.value);
                // モジュレーションで都市計画を調整
                cohesionRadius = 30 + modulation * 50;
                maxDistance = 25 + modulation * 30;
            }
        }
    }
    
private:
    void drawWaveCircle(ofVec2f center, float baseRadius, float intensity) {
        // MIDI連動ウェーブ変形円を描画
        ofBeginShape();
        int numPoints = 24;  // 円の分割数を増加してより滑らかに
        
        // ベースウェーブ強度（常時動作）
        float baseWaveIntensity = 0.3f + globalGrowthLevel * 0.4f;
        
        for (int i = 0; i <= numPoints; i++) {
            float angle = (i / float(numPoints)) * TWO_PI;
            
            // ベース半径にウェーブ効果を適用（振幅大幅増加）
            float waveOffset = sin(angle * 3 + systemTime * 2) * baseRadius * 0.4f * baseWaveIntensity;  // 3つの波
            waveOffset += sin(angle * 5 + systemTime * 3) * baseRadius * 0.25f * baseWaveIntensity;  // 5つの波
            waveOffset += sin(angle * 7 + systemTime * 1.5f) * baseRadius * 0.15f * baseWaveIntensity;  // 7つの波
            
            // 細かい波紋を追加してより有機的に
            waveOffset += sin(angle * 11 + systemTime * 4) * baseRadius * 0.08f * baseWaveIntensity;
            
            // MIDI連動：インパクト時に強い変形
            float midiInfluence = (impactIntensity + intensity) * sin(angle * 4 + systemTime * 4) * baseRadius * 0.5f;
            
            float radius = baseRadius + waveOffset + midiInfluence;
            radius = max(radius, baseRadius * 0.2f);  // 最小サイズ制限を緩和
            
            float x = center.x + cos(angle) * radius;
            float y = center.y + sin(angle) * radius;
            ofVertex(x, y);
        }
        ofEndShape(true);
    }
    
    void createInitialConnections() {
        // 初期ノード間の接続を作成
        for (int i = 0; i < nodes.size(); i++) {
            int next = (i + 1) % nodes.size();
            connections.push_back(UrbanConnection(i, next, UrbanConnection::ROAD));
            
            // 一部にランダム接続を追加
            if (ofRandom(1.0f) < 0.3f) {
                int randomTarget = ofRandom(nodes.size());
                if (randomTarget != i) {
                    connections.push_back(UrbanConnection(i, randomTarget, UrbanConnection::WALKWAY));
                }
            }
        }
    }
    
    void initializeUrbanInfrastructure() {
        // 初期交通路線（密度削減）
        transitLines.resize(2);
        
        // 主要路線の設定
        for (int i = 0; i < 2; i++) {
            ofVec2f start(ofRandom(100, ofGetWidth()-100), ofRandom(100, ofGetHeight()-100));
            ofVec2f end(ofRandom(100, ofGetWidth()-100), ofRandom(100, ofGetHeight()-100));
            
            transitLines[i].addVertex(start.x, start.y);
            
            // 中間点を追加して自然な曲線に
            int numSegments = 5 + ofRandom(3);
            for (int j = 1; j < numSegments; j++) {
                float t = j / float(numSegments);
                ofVec2f intermediate = start.getInterpolated(end, t);
                intermediate += ofVec2f(ofRandom(-50, 50), ofRandom(-50, 50));
                transitLines[i].addVertex(intermediate.x, intermediate.y);
            }
            
            transitLines[i].addVertex(end.x, end.y);
        }
        
        // 緑地の初期配置（密度削減）
        for (int i = 0; i < 3; i++) {
            greenSpaces.push_back(ofVec2f(ofRandom(ofGetWidth()), ofRandom(ofGetHeight())));
        }
    }
    
    void updateNodes(float deltaTime) {
        for (auto& node : nodes) {
            node.update(deltaTime, globalGrowthLevel);
        }
    }
    
    void applyUrbanGrowthForces() {
        for (int i = 0; i < nodes.size(); i++) {
            ofVec2f cohesion(0, 0);
            ofVec2f separation(0, 0);
            ofVec2f alignment(0, 0);
            ofVec2f urbanAttraction(0, 0);
            
            int neighborCount = 0;
            
            for (int j = 0; j < nodes.size(); j++) {
                if (i == j) continue;
                
                float distance = nodes[i].position.distance(nodes[j].position);
                
                if (distance < cohesionRadius && distance > 0.1f) {
                    // 結合力（都市の集積効果）
                    cohesion += nodes[j].position;
                    alignment += nodes[j].velocity;
                    neighborCount++;
                    
                    if (distance < separationRadius) {
                        // 分離力（都市密度の適正化）
                        ofVec2f diff = nodes[i].position - nodes[j].position;
                        diff.normalize();
                        diff /= distance;
                        separation += diff;
                    }
                }
            }
            
            if (neighborCount > 0) {
                cohesion /= neighborCount;
                cohesion -= nodes[i].position;
                cohesion.normalize();
                cohesion *= 0.4f * (1.0f + nodes[i].economicActivity);
                
                alignment /= neighborCount;
                alignment.normalize();
                alignment *= 0.15f;
                
                separation.normalize();
                separation *= 0.6f;
            }
            
            // 開発センターへの引力
            for (auto& center : developmentCenters) {
                ofVec2f force = center - nodes[i].position;
                float distance = force.length();
                if (distance > 0.1f) {
                    force.normalize();
                    force *= 0.2f / (1.0f + distance * 0.005f);
                    urbanAttraction += force;
                }
            }
            
            // メトロポリス効果
            if (isMetropolis) {
                ofVec2f metroForce(0, 0);
                for (auto& seed : metropolisSeeds) {
                    ofVec2f force = seed - nodes[i].position;
                    float distance = force.length();
                    if (distance > 0.1f && distance < 200) {
                        force.normalize();
                        force *= metropolisLevel * 0.3f / (1.0f + distance * 0.01f);
                        metroForce += force;
                    }
                }
                urbanAttraction += metroForce;
            }
            
            // 全ての力を適用
            ofVec2f totalForce = cohesion + separation + alignment + urbanAttraction;
            totalForce *= (1.0f + globalGrowthLevel * 0.5f);
            
            nodes[i].velocity += totalForce;
            
            // 速度制限（ノードタイプに応じて）
            float maxSpeed = nodes[i].maxSpeed * (1.0f + nodes[i].connectivity * 0.5f);
            if (nodes[i].velocity.length() > maxSpeed) {
                nodes[i].velocity.normalize();
                nodes[i].velocity *= maxSpeed;
            }
        }
    }
    
    void handleNodeEvolution() {
        // ノードの動的進化
        std::vector<UrbanNode> newNodes;
        
        for (int i = 0; i < nodes.size(); i++) {
            int next = (i + 1) % nodes.size();
            float distance = nodes[i].position.distance(nodes[next].position);
            
            if (distance > maxDistance && nodes.size() < 120) {  // 最大ノード数を大幅削減
                // 新しいノードを挿入（都市拡張）
                ofVec2f newPos = (nodes[i].position + nodes[next].position) * 0.5f;
                newPos += ofVec2f(ofRandom(-8, 8), ofRandom(-8, 8));
                
                UrbanNode newNode(newPos);
                
                // 特性の継承と変異
                newNode.urbanDensity = (nodes[i].urbanDensity + nodes[next].urbanDensity) * 0.5f + ofRandom(-0.1f, 0.1f);
                newNode.economicActivity = (nodes[i].economicActivity + nodes[next].economicActivity) * 0.5f + ofRandom(-0.1f, 0.1f);
                newNode.infrastructureLevel = (nodes[i].infrastructureLevel + nodes[next].infrastructureLevel) * 0.5f;
                
                // 成長に応じてタイプを決定
                if (globalGrowthLevel > 0.7f && ofRandom(1.0f) < 0.2f) {
                    newNode.type = UrbanNode::LANDMARK;
                } else if (urbanPressure > 0.6f && ofRandom(1.0f) < 0.3f) {
                    newNode.type = UrbanNode::TRANSPORT_HUB;
                }
                
                newNodes.push_back(newNode);
                
                // 接続の追加
                connections.push_back(UrbanConnection(i, nodes.size() + newNodes.size() - 1));
                connections.push_back(UrbanConnection(nodes.size() + newNodes.size() - 1, next));
            }
        }
        
        // 新しいノードを追加
        for (auto& newNode : newNodes) {
            nodes.push_back(newNode);
        }
        
        // 過密ノードの除去
        for (int i = nodes.size() - 1; i >= 0; i--) {
            bool shouldRemove = false;
            
            for (int j = 0; j < nodes.size(); j++) {
                if (i == j) continue;
                
                float distance = nodes[i].position.distance(nodes[j].position);
                if (distance < minDistance) {
                    shouldRemove = true;
                    break;
                }
            }
            
            if (shouldRemove) {
                // 接続の更新
                for (int k = connections.size() - 1; k >= 0; k--) {
                    if (connections[k].nodeA == i || connections[k].nodeB == i) {
                        connections.erase(connections.begin() + k);
                    } else {
                        if (connections[k].nodeA > i) connections[k].nodeA--;
                        if (connections[k].nodeB > i) connections[k].nodeB--;
                    }
                }
                
                nodes.erase(nodes.begin() + i);
            }
        }
    }
    
    void handleConnectionEvolution() {
        // 接続の動的進化
        for (auto& conn : connections) {
            if (conn.nodeA < nodes.size() && conn.nodeB < nodes.size()) {
                // 交通量の計算
                float nodeActivity = (nodes[conn.nodeA].economicActivity + nodes[conn.nodeB].economicActivity) * 0.5f;
                conn.traffic = nodeActivity * (1.0f + globalGrowthLevel) * sin(trafficAnimation + conn.nodeA * 0.1f) * 0.5f + 0.5f;
                
                // 接続強度の更新
                conn.strength += (nodeActivity - 0.5f) * 0.01f;
                conn.strength = ofClamp(conn.strength, 0.1f, 2.0f);
            }
        }
        
        // 新しい接続の生成（頻度と密度を削減）
        if (ofRandom(1.0f) < 0.008f * globalGrowthLevel && connections.size() < nodes.size() * 2) {
            int nodeA = ofRandom(nodes.size());
            int nodeB = ofRandom(nodes.size());
            
            if (nodeA != nodeB) {
                float distance = nodes[nodeA].position.distance(nodes[nodeB].position);
                if (distance < cohesionRadius * 1.5f) {
                    // 接続タイプの決定
                    UrbanConnection::ConnectionType type = UrbanConnection::ROAD;
                    if (nodes[nodeA].type == UrbanNode::TRANSPORT_HUB || nodes[nodeB].type == UrbanNode::TRANSPORT_HUB) {
                        type = UrbanConnection::RAILWAY;
                    } else if (globalGrowthLevel > 0.8f && ofRandom(1.0f) < 0.2f) {
                        type = UrbanConnection::DATA_LINE;
                    }
                    
                    connections.push_back(UrbanConnection(nodeA, nodeB, type));
                }
            }
        }
    }
    
    void updateUrbanInfrastructure(float deltaTime) {
        // 交通路線の動的更新
        for (auto& line : transitLines) {
            if (ofRandom(1.0f) < 0.05f * globalGrowthLevel) {
                // 路線の延長
                if (line.size() > 0) {
                    ofVec2f lastPoint = line.getVertices().back();
                    ofVec2f newPoint = lastPoint + ofVec2f(ofRandom(-30, 30), ofRandom(-30, 30));
                    
                    // 境界チェック
                    if (newPoint.x > 50 && newPoint.x < ofGetWidth() - 50 &&
                        newPoint.y > 50 && newPoint.y < ofGetHeight() - 50) {
                        line.addVertex(newPoint.x, newPoint.y);
                    }
                }
            }
        }
        
        // メガプロジェクトの管理（数を削減）
        if (constructionActivity > 0.7f && megaProjects.size() < 2) {
            ofVec2f projectSite(ofRandom(100, ofGetWidth()-100), ofRandom(100, ofGetHeight()-100));
            megaProjects.push_back(projectSite);
        }
        
        // 緑地の管理（数を削減）
        if (globalGrowthLevel > 0.5f && ofRandom(1.0f) < 0.005f && greenSpaces.size() < 5) {
            greenSpaces.push_back(ofVec2f(ofRandom(ofGetWidth()), ofRandom(ofGetHeight())));
        }
    }
    
    void calculateMetropolisLevel() {
        float totalDensity = 0;
        float totalActivity = 0;
        float totalInfra = 0;
        
        for (auto& node : nodes) {
            totalDensity += node.urbanDensity;
            totalActivity += node.economicActivity;
            totalInfra += node.infrastructureLevel;
        }
        
        if (nodes.size() > 0) {
            float avgDensity = totalDensity / nodes.size();
            float avgActivity = totalActivity / nodes.size();
            float avgInfra = totalInfra / nodes.size();
            
            metropolisLevel = (avgDensity + avgActivity + avgInfra) / 3.0f;
            urbanComplexity = metropolisLevel * (1.0f + globalGrowthLevel);
            
            // メトロポリス判定（闾値を調整）
            isMetropolis = (metropolisLevel > 0.75f && nodes.size() > 50);
            
            if (isMetropolis && metropolisSeeds.size() < 2) {
                metropolisSeeds.push_back(ofVec2f(ofRandom(ofGetWidth()), ofRandom(ofGetHeight())));
            }
        }
    }
    
    void updateAdvancedUrbanPhenomena(float deltaTime) {
        developmentGlow = sin(economicPulse) * 0.3f + 0.7f;
        
        // フラッシュ効果の更新
        flashTimer += deltaTime;
        flashEffect *= 0.92f; // 減衰
        
        // ランダムフラッシュの発生（稀）
        if (flashTimer > 3.0f && ofRandom(1.0f) < 0.02f) {
            flashEffect = 1.0f;
            flashTimer = 0.0f;
        }
        
        // 影響センターの減衰
        for (int i = impactCenters.size() - 1; i >= 0; i--) {
            if (ofRandom(1.0f) < 0.05f) {
                impactCenters.erase(impactCenters.begin() + i);
            }
        }
    }
    
    void drawMetropolitanBackground() {
        // クールな都市背景グラデーション
        ofColor bgTop = ofColor::fromHsb(200, 40, 25 + globalGrowthLevel * 15);  // ダークグレー
        ofColor bgBottom = ofColor::fromHsb(220, 60, 45 + globalGrowthLevel * 25);  // ダークブルー
        
        // グラデーション描画
        ofMesh gradientMesh;
        gradientMesh.setMode(OF_PRIMITIVE_TRIANGLE_STRIP);
        
        gradientMesh.addVertex(ofVec3f(0, 0));
        gradientMesh.addColor(bgTop);
        gradientMesh.addVertex(ofVec3f(ofGetWidth(), 0));
        gradientMesh.addColor(bgTop);
        gradientMesh.addVertex(ofVec3f(0, ofGetHeight()));
        gradientMesh.addColor(bgBottom);
        gradientMesh.addVertex(ofVec3f(ofGetWidth(), ofGetHeight()));
        gradientMesh.addColor(bgBottom);
        
        gradientMesh.draw();
    }
    
    void drawUrbanConnections() {
        // ホワイトアウト防止: ADDモードを避けてALPHAモードに変更
        ofEnableBlendMode(OF_BLENDMODE_ALPHA);
        
        for (auto& conn : connections) {
            if (conn.nodeA < nodes.size() && conn.nodeB < nodes.size()) {
                ofVec2f posA = nodes[conn.nodeA].position;
                ofVec2f posB = nodes[conn.nodeB].position;
                
                // クールな接続色（ホワイトアウト防止）
                ofColor connColor;
                switch(conn.type) {
                    case UrbanConnection::ROAD:
                        connColor = ofColor(80, 80, 80);  // グレー（モノトーン）
                        break;
                    case UrbanConnection::RAILWAY:
                        connColor = ofColor(50, 80, 120);  // アクセント1: ソフトブルー
                        break;
                    case UrbanConnection::DATA_LINE:
                        connColor = ofColor(40, 100, 110);  // アクセント2: シアン
                        break;
                    default:
                        connColor = ofColor(60, 60, 60);  // ダークグレー（モノトーン）
                }
                connColor.a = 80 + conn.strength * 60; // より見えやすいアルファ値
                ofSetColor(connColor);
                
                float lineWidth = 0.5f + conn.strength * 0.5 + globalGrowthLevel * 0.5;
                if (conn.type == UrbanConnection::RAILWAY) {
                    lineWidth *= 1.5f;
                } else if (conn.type == UrbanConnection::DATA_LINE) {
                    lineWidth *= 0.7f;
                }
                
                ofSetLineWidth(lineWidth);
                ofDrawLine(posA, posB);
                
                // 交通流の可視化（MIDI連動ウェーブ変形）
                if (conn.traffic > 0.6f) {
                    ofVec2f mid = (posA + posB) * 0.5f;
                    // 交通流: アクセントカラー
                    ofColor trafficColor = ofColor(80, 140, 160);  // アクセント: ライトシアン
                    trafficColor.a = 100 + conn.traffic * 50; // より見えやすいアルファ値
                    ofSetColor(trafficColor);
                    
                    float trafficSize = 2 + conn.traffic * 4;
                    drawWaveCircle(mid, trafficSize, conn.traffic);
                }
            }
        }
        
        ofDisableBlendMode();
    }
    
    void drawUrbanNodes() {
        // ホワイトアウト防止: ADDモードをALPHAモードに変更
        ofEnableBlendMode(OF_BLENDMODE_ALPHA);
        
        for (auto& node : nodes) {
            if (node.active) {
                ofColor nodeColor = node.color;
                // モノトーン+アクセント方式: 色をそのまま使用、適度なアルファ値
                // ランダムフラッシュ演出
                if (flashEffect > 0.5f) {
                    nodeColor = ofColor(255, 255, 255); // 眩しい白
                    nodeColor.a = 200 * flashEffect;
                } else {
                    nodeColor.a = 60 + globalGrowthLevel * 40; // より見えやすいアルファ値
                }
                ofSetColor(nodeColor);
                
                float nodeSize = node.size * (1.0f + globalGrowthLevel * 0.5f);
                
                // ノードタイプに応じた描画
                switch(node.type) {
                    case UrbanNode::RESIDENTIAL:
                        // MIDI連動ウェーブ変形円
                        drawWaveCircle(node.position, nodeSize, node.economicActivity);
                        break;
                    case UrbanNode::COMMERCIAL:
                        ofDrawRectangle(node.position.x - nodeSize/2, node.position.y - nodeSize/2, nodeSize, nodeSize);
                        break;
                    case UrbanNode::INDUSTRIAL:
                        // 工業地帯（三角形）
                        ofDrawTriangle(node.position + ofVec2f(0, -nodeSize), 
                                      node.position + ofVec2f(-nodeSize, nodeSize), 
                                      node.position + ofVec2f(nodeSize, nodeSize));
                        break;
                    case UrbanNode::TRANSPORT_HUB:
                        // 交通ハブ（六角形風・MIDI連動ウェーブ変形）
                        for (int i = 0; i < 6; i++) {
                            float angle = (i / 6.0f) * TWO_PI;
                            ofVec2f point = node.position + ofVec2f(cos(angle), sin(angle)) * nodeSize;
                            drawWaveCircle(point, nodeSize * 0.3f, node.connectivity);
                        }
                        break;
                    case UrbanNode::LANDMARK:
                        // ランドマーク（星形・MIDI連動ウェーブ変形）
                        for (int i = 0; i < 8; i++) {
                            float angle = (i / 8.0f) * TWO_PI;
                            ofVec2f rayEnd = node.position + ofVec2f(cos(angle), sin(angle)) * nodeSize;
                            ofDrawLine(node.position, rayEnd);
                        }
                        drawWaveCircle(node.position, nodeSize * 0.5f, node.economicActivity);
                        break;
                }
                
                // 活動レベルの表示
                if (node.economicActivity > 0.7f) {
                    ofColor activityColor = accentColor(node.economicActivity);
                    activityColor.setBrightness(ofClamp(activityColor.getBrightness() * 0.3f, 10, 50));
                    activityColor.setSaturation(ofClamp(activityColor.getSaturation() * 1.5f, 120, 255));
                    activityColor.a = 80 * node.economicActivity; // 透明度削減
                    ofSetColor(activityColor);
                    
                    float activityRadius = nodeSize * (1.0f + node.economicActivity);
                    ofNoFill();
                    ofSetLineWidth(1 + node.economicActivity * 2);
                    drawWaveCircle(node.position, activityRadius, node.economicActivity);
                    ofFill();
                }
            }
        }
        
        ofDisableBlendMode();
    }
    
    void drawTransitSystem() {
        if (globalGrowthLevel > 0.3f) {
            // ホワイトアウト防止: ALPHAモード使用
            ofEnableBlendMode(OF_BLENDMODE_ALPHA);
            
            for (int i = 0; i < transitLines.size(); i++) {
                auto& line = transitLines[i];
                
                // クールな路線色
                ofColor lineColor = ofColor::fromHsb(200 + i * 30, 80, 130 + globalGrowthLevel * 50);
                lineColor.a = 150 + globalGrowthLevel * 80;
                ofSetColor(lineColor);
                
                ofSetLineWidth(1.5 + globalGrowthLevel * 0.8);
                line.draw();
                
                // 駅の表示（MIDI連動ウェーブ変形）
                for (int j = 0; j < line.size(); j += 3) {
                    ofVec2f station = line[j];
                    
                    // クールな駅色
                    ofSetColor(ofColor::fromHsb(190, 70, 180));
                    drawWaveCircle(station, 4 + globalGrowthLevel * 2, globalGrowthLevel);
                }
                
                // 運行車両の表示
                if (globalGrowthLevel > 0.6f) {
                    float trainProgress = fmod(trafficAnimation * 0.3f + i * 0.5f, 1.0f);
                    if (line.size() > 1) {
                        ofVec2f trainPos = line.getPointAtPercent(trainProgress);
                        
                        // クールな電車色
                        ofColor trainColor = ofColor::fromHsb(210, 100, 200);
                        trainColor.a = 200;
                        ofSetColor(trainColor);
                        drawWaveCircle(trainPos, 3 + globalGrowthLevel, 0.9f);
                    }
                }
            }
            
            ofDisableBlendMode();
        }
    }
    
    void drawDevelopmentProjects() {
        if (constructionActivity > 0.3f) {
            // ホワイトアウト防止: ALPHAモード使用
            ofEnableBlendMode(OF_BLENDMODE_ALPHA);
            
            // メガプロジェクト
            for (auto& project : megaProjects) {
                ofColor projectColor = accentColor(constructionActivity);
                projectColor.a = 180 * constructionActivity;
                ofSetColor(projectColor);
                
                float projectSize = 15 + constructionActivity * 20;
                
                // 建設現場の表現
                ofNoFill();
                ofSetLineWidth(2 + constructionActivity * 3);
                ofDrawRectangle(project.x - projectSize/2, project.y - projectSize/2, projectSize, projectSize);
                ofFill();
                
                // 建設活動の表示
                for (int i = 0; i < 4; i++) {
                    float angle = (i / 4.0f) * TWO_PI + systemTime;
                    ofVec2f activityPoint = project + ofVec2f(cos(angle), sin(angle)) * projectSize * 0.7f;
                    
                    ofSetColor(255, 150);
                    float dynamicSize = 2 + sin(systemTime * 3 + i) * 2;
                    drawWaveCircle(activityPoint, dynamicSize, constructionActivity);
                }
            }
            
            // 緑地・公園
            for (auto& green : greenSpaces) {
                ofColor greenColor = ofColor::fromHsb(120, 180, 150);
                greenColor.a = 100 + globalGrowthLevel * 80;
                ofSetColor(greenColor);
                
                float greenSize = 8 + globalGrowthLevel * 12;
                drawWaveCircle(green, greenSize, globalGrowthLevel * 0.5f);
                
                // 緑地の詳細
                if (globalGrowthLevel > 0.5f) {
                    for (int i = 0; i < 6; i++) {
                        float angle = (i / 6.0f) * TWO_PI;
                        ofVec2f treePos = green + ofVec2f(cos(angle), sin(angle)) * greenSize * 0.6f;
                        drawWaveCircle(treePos, 2, globalGrowthLevel * 0.3f);
                    }
                }
            }
            
            ofDisableBlendMode();
        }
    }
    
    void drawAdvancedMetropolitanEffects() {
        // ホワイトアウト防止: ALPHAモード使用
        ofEnableBlendMode(OF_BLENDMODE_ALPHA);
        
        // 経済パルス効果
        if (economicPulse > 0) {
            float pulse = developmentGlow;
            
            for (auto& center : developmentCenters) {
                ofColor pulseColor = accentColor(pulse);
                pulseColor.a = 100 * pulse * globalGrowthLevel;
                ofSetColor(pulseColor);
                
                float pulseRadius = 30 + pulse * 50;
                
                ofNoFill();
                ofSetLineWidth(1 + pulse * 3);
                drawWaveCircle(center, pulseRadius, pulse);
                ofFill();
                
                // 経済波の拡散
                for (int i = 0; i < 8; i++) {
                    float angle = (i / 8.0f) * TWO_PI + economicPulse;
                    ofVec2f waveEnd = center + ofVec2f(cos(angle), sin(angle)) * pulseRadius * 1.5f;
                    
                    ofSetLineWidth(0.5f + pulse);
                    ofDrawLine(center, waveEnd);
                }
            }
        }
        
        // データフロー表示
        if (globalGrowthLevel > 0.8f) {
            for (auto& conn : connections) {
                if (conn.type == UrbanConnection::DATA_LINE && conn.nodeA < nodes.size() && conn.nodeB < nodes.size()) {
                    ofVec2f posA = nodes[conn.nodeA].position;
                    ofVec2f posB = nodes[conn.nodeB].position;
                    
                    // データパケットの移動
                    float dataProgress = fmod(systemTime * 2.0f + conn.nodeA * 0.3f, 1.0f);
                    ofVec2f dataPos = posA.getInterpolated(posB, dataProgress);
                    
                    ofColor dataColor = ofColor::fromHsb(200, 255, 255);
                    dataColor.a = 180;
                    ofSetColor(dataColor);
                    
                    float dataSize = 2 + sin(systemTime * 4) * 1;
                    drawWaveCircle(dataPos, dataSize, globalGrowthLevel);
                }
            }
        }
        
        ofDisableBlendMode();
    }
    
    void drawMetropolisEffects() {
        // ホワイトアウト防止: ALPHAモード使用
        ofEnableBlendMode(OF_BLENDMODE_ALPHA);
        
        // メトロポリスの光のドーム
        ofColor domeColor = accentColor(metropolisLevel);
        domeColor.a = 30 * metropolisLevel;
        ofSetColor(domeColor);
        
        ofVec2f center(ofGetWidth() * 0.5f, ofGetHeight() * 0.5f);
        float domeRadius = ofGetWidth() * 0.4f * metropolisLevel;
        
        ofNoFill();
        ofSetLineWidth(1.5 + metropolisLevel * 1.5);
        drawWaveCircle(center, domeRadius, metropolisLevel);
        ofFill();
        
        // メトロポリスのスカイライン（密度削減）
        for (int i = 0; i < 12; i++) {
            float angle = (i / 30.0f) * TWO_PI;
            float radius = domeRadius * 0.8f;
            ofVec2f buildingBase = center + ofVec2f(cos(angle), sin(angle)) * radius;
            
            float buildingHeight = 20 + ofNoise(i * 0.1f, systemTime * 0.1f) * 80 * metropolisLevel;
            
            ofColor buildingColor = urbanColor(currentNote + i * 5, metropolisLevel);
            buildingColor.a = 150;
            ofSetColor(buildingColor);
            
            ofDrawRectangle(buildingBase.x - 3, buildingBase.y - buildingHeight, 6, buildingHeight);
            
            // 建物の窓
            if (metropolisLevel > 0.8f) {
                for (int w = 0; w < buildingHeight / 10; w++) {
                    if (ofRandom(1.0f) < 0.8f) {
                        ofSetColor(255, 255, 150, 200);
                        ofDrawRectangle(buildingBase.x - 1, buildingBase.y - w * 10 - 8, 2, 4);
                    }
                }
            }
        }
        
        ofDisableBlendMode();
    }
    
    void drawUrbanStatistics() {
        if (getTimeSinceLastMidi() < 5.0f) {
            ofSetColor(200);
            ofDrawBitmapString("Differential Growth - Metropolitan Development", 20, ofGetHeight() - 120);
            ofDrawBitmapString("Urban Nodes: " + ofToString(nodes.size()), 20, ofGetHeight() - 100);
            ofDrawBitmapString("Connections: " + ofToString(connections.size()), 20, ofGetHeight() - 80);
            ofDrawBitmapString("Metropolis Level: " + ofToString(metropolisLevel * 100, 1) + "%", 20, ofGetHeight() - 60);
            ofDrawBitmapString("Urban Complexity: " + ofToString(urbanComplexity * 100, 1) + "%", 20, ofGetHeight() - 40);
            if (isMetropolis) {
                ofSetColor(255, 200, 100);
                ofDrawBitmapString("METROPOLIS STATUS", 20, ofGetHeight() - 20);
            }
        }
    }
    
    // MIDI反応メソッド
    void triggerMajorUrbanExpansion(float intensity) {
        // 大規模都市拡張（密度削減）
        ofVec2f expansionCenter(ofGetWidth() * 0.5f, ofGetHeight() * 0.5f);
        
        for (int i = 0; i < intensity * 3; i++) {
            float angle = ofRandom(TWO_PI);
            float radius = 50 + ofRandom(100);
            ofVec2f newPos = expansionCenter + ofVec2f(cos(angle), sin(angle)) * radius;
            
            UrbanNode newNode(newPos);
            newNode.urbanDensity = intensity * 0.7f;
            newNode.economicActivity = intensity * 0.6f;
            newNode.type = (intensity > 0.8f) ? UrbanNode::LANDMARK : UrbanNode::COMMERCIAL;
            
            nodes.push_back(newNode);
        }
        
        developmentCenters.push_back(expansionCenter);
        if (developmentCenters.size() > 5) {
            developmentCenters.erase(developmentCenters.begin());
        }
    }
    
    void triggerTransportExpansion(float intensity) {
        // 交通網拡張（密度削減）
        for (int i = 0; i < intensity * 1; i++) {
            ofPolyline newLine;
            
            ofVec2f start(ofRandom(100, ofGetWidth()-100), ofRandom(100, ofGetHeight()-100));
            ofVec2f end(ofRandom(100, ofGetWidth()-100), ofRandom(100, ofGetHeight()-100));
            
            newLine.addVertex(start.x, start.y);
            
            int segments = 3 + intensity * 3;
            for (int j = 1; j < segments; j++) {
                float t = j / float(segments);
                ofVec2f intermediate = start.getInterpolated(end, t);
                intermediate += ofVec2f(ofRandom(-30, 30), ofRandom(-30, 30));
                newLine.addVertex(intermediate.x, intermediate.y);
            }
            
            newLine.addVertex(end.x, end.y);
            transitLines.push_back(newLine);
        }
        
        // 新しい交通ハブの追加
        for (int i = 0; i < nodes.size(); i++) {
            if (ofRandom(1.0f) < intensity * 0.3f && nodes[i].type != UrbanNode::TRANSPORT_HUB) {
                nodes[i].type = UrbanNode::TRANSPORT_HUB;
                nodes[i].connectivity = intensity;
                break;
            }
        }
    }
    
    void triggerLocalDevelopment(float intensity) {
        // 地域開発
        for (auto& node : nodes) {
            if (ofRandom(1.0f) < intensity * 0.4f) {
                node.urbanDensity += intensity * 0.2f;
                node.economicActivity += intensity * 0.3f;
                node.infrastructureLevel += intensity * 0.1f;
                
                node.urbanDensity = ofClamp(node.urbanDensity, 0.0f, 1.0f);
                node.economicActivity = ofClamp(node.economicActivity, 0.0f, 1.0f);
                node.infrastructureLevel = ofClamp(node.infrastructureLevel, 0.0f, 1.0f);
            }
        }
        
        // 細かい接続の追加（密度削減）
        if (connections.size() < nodes.size() * 1.5) {
            for (int i = 0; i < intensity * 2; i++) {
                int nodeA = ofRandom(nodes.size());
                int nodeB = ofRandom(nodes.size());
                
                if (nodeA != nodeB) {
                    connections.push_back(UrbanConnection(nodeA, nodeB, UrbanConnection::WALKWAY));
                }
            }
        }
    }
    
    void triggerMetropolitanTransformation() {
        // メトロポリス変革
        isMetropolis = true;
        metropolisLevel = 0.9f;
        
        // 全ノードの大幅強化
        for (auto& node : nodes) {
            node.urbanDensity += 0.3f;
            node.economicActivity += 0.4f;
            node.infrastructureLevel += 0.2f;
            
            node.urbanDensity = ofClamp(node.urbanDensity, 0.0f, 1.0f);
            node.economicActivity = ofClamp(node.economicActivity, 0.0f, 1.0f);
            node.infrastructureLevel = ofClamp(node.infrastructureLevel, 0.0f, 1.0f);
        }
        
        // メトロポリス種子の追加（数を削減）
        for (int i = 0; i < 2; i++) {
            metropolisSeeds.push_back(ofVec2f(
                ofRandom(200, ofGetWidth() - 200),
                ofRandom(200, ofGetHeight() - 200)
            ));
        }
        
        // データライン接続の追加（数を削減）
        for (int i = 0; i < 4; i++) {
            int nodeA = ofRandom(nodes.size());
            int nodeB = ofRandom(nodes.size());
            
            if (nodeA != nodeB) {
                connections.push_back(UrbanConnection(nodeA, nodeB, UrbanConnection::DATA_LINE));
            }
        }
    }
    
    void triggerTargetedDevelopment(ofVec2f target, float intensity) {
        // 指定地点での開発
        impactCenters.push_back(target);
        
        // 近くのノードに影響
        for (auto& node : nodes) {
            float distance = node.position.distance(target);
            if (distance < 100) {
                float influence = (1.0f - distance / 100.0f) * intensity;
                
                node.urbanDensity += influence * 0.3f;
                node.economicActivity += influence * 0.4f;
                node.infrastructureLevel += influence * 0.2f;
                
                node.urbanDensity = ofClamp(node.urbanDensity, 0.0f, 1.0f);
                node.economicActivity = ofClamp(node.economicActivity, 0.0f, 1.0f);
                node.infrastructureLevel = ofClamp(node.infrastructureLevel, 0.0f, 1.0f);
            }
        }
        
        // 新しい開発ノードの追加
        if (intensity > 0.6f) {
            UrbanNode targetNode(target);
            targetNode.urbanDensity = intensity;
            targetNode.economicActivity = intensity * 0.8f;
            targetNode.type = UrbanNode::COMMERCIAL;
            
            nodes.push_back(targetNode);
        }
    }
    
    void applyUrbanDecline() {
        // 都市衰退効果
        for (auto& node : nodes) {
            if (ofRandom(1.0f) < 0.1f) {
                node.urbanDensity *= 0.95f;
                node.economicActivity *= 0.9f;
                node.infrastructureLevel *= 0.97f;
            }
        }
        
        // 接続の劣化
        for (auto& conn : connections) {
            if (ofRandom(1.0f) < 0.05f) {
                conn.strength *= 0.9f;
            }
        }
        
        if (isMetropolis && ofRandom(1.0f) < 0.3f) {
            isMetropolis = false;
            metropolisLevel *= 0.8f;
        }
    }
};