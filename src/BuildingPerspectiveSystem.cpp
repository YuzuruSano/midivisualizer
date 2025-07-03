#include "BuildingPerspectiveSystem.h"

BuildingPerspectiveSystem::BuildingPerspectiveSystem() {
    cameraPosition.set(0, 0, 0);
    cameraTarget.set(0, 0, 100);
    cameraSpeed = 80.0f;
    cameraRotation = 0.0f;
    perspectiveAngle = 60.0f;
    
    walkBobbing = 0.0f;
    walkSpeed = 1.5f;
    headSway = 0.0f;
    breathingOffset = 0.0f;
    
    // モノトーンカラーパレット
    buildingDark = ofColor(40, 40, 40);
    buildingMedium = ofColor(85, 85, 85);
    buildingLight = ofColor(130, 130, 130);
    edgeColor = ofColor(180, 180, 180);
    floorColor = ofColor(25, 25, 25);
    windowColor = ofColor(200, 200, 200);
    
    buildingSpawnRate = 0.1f;
    buildingDensity = 0.8f;
    streetWidth = 200.0f;
    buildingHeight = 150.0f;
    generationDistance = 500.0f;
    
    fogDensity = 0.02f;
    ambientLight = 0.7f;
    shadowIntensity = 0.4f;
    
    kickIntensity = 0.0f;
    snareIntensity = 0.0f;
    hihatIntensity = 0.0f;
    crashIntensity = 0.0f;
}

void BuildingPerspectiveSystem::setup() {
    // 初期建物の生成
    for (int i = 0; i < 20; i++) {
        float depth = i * 50.0f + 100.0f;
        generateBuilding(depth);
    }
}

void BuildingPerspectiveSystem::update(float deltaTime) {
    
    // カメラ移動の更新
    updateCameraMovement(deltaTime);
    
    // 一人称視点の自然な揺れ
    walkBobbing += deltaTime * walkSpeed * 3.0f;
    breathingOffset += deltaTime * 0.8f;
    headSway += deltaTime * 0.6f;
    
    // カメラの前進
    cameraPosition.z += cameraSpeed * deltaTime;
    cameraTarget.z = cameraPosition.z + 100.0f;
    
    // 新しい建物の生成
    if (ofRandom(1.0f) < buildingSpawnRate * deltaTime) {
        float newDepth = cameraPosition.z + generationDistance;
        generateBuilding(newDepth);
    }
    
    // 建物の更新
    for (auto& building : buildings) {
        building.depth = building.position.z - cameraPosition.z;
        
        // 建物の可視性判定
        building.isActive = (building.depth > -100.0f && building.depth < generationDistance);
    }
    
    // 遠くの建物を削除
    cleanupDistantBuildings();
    
    // MIDI連動強度の減衰
    kickIntensity *= 0.92f;
    snareIntensity *= 0.88f;
    hihatIntensity *= 0.85f;
    crashIntensity *= 0.80f;
    
    // 成長レベルに基づく環境調整
    buildingSpawnRate = 0.1f + globalGrowthLevel * 0.15f;
    cameraSpeed = 80.0f + globalGrowthLevel * 40.0f;
    buildingHeight = 150.0f + globalGrowthLevel * 100.0f;
}

void BuildingPerspectiveSystem::draw() {
    // 背景（空）
    ofBackground(buildingDark.r * 0.6f, buildingDark.g * 0.6f, buildingDark.b * 0.6f);
    
    ofPushMatrix();
    
    // 画面中央に座標系を移動
    ofTranslate(ofGetWidth() * 0.5f, ofGetHeight() * 0.6f);
    
    // 一人称視点の揺れを適用
    float bobbingY = sin(walkBobbing) * 3.0f;
    float swayX = sin(headSway) * 2.0f;
    float breathingZ = sin(breathingOffset) * 1.0f;
    
    ofTranslate(swayX, bobbingY + breathingZ);
    
    // カメラ回転の適用
    if (abs(cameraRotation) > 0.1f) {
        ofRotateDeg(cameraRotation);
        cameraRotation *= 0.95f; // 回転の減衰
    }
    
    // 建物の描画（深度順）
    std::vector<Building*> sortedBuildings;
    for (auto& building : buildings) {
        if (building.isActive) {
            sortedBuildings.push_back(&building);
        }
    }
    
    // 深度ソート（遠い順）
    std::sort(sortedBuildings.begin(), sortedBuildings.end(), 
              [](const Building* a, const Building* b) {
                  return a->depth > b->depth;
              });
    
    // 街路要素の描画（先に描画）
    drawStreetElements();
    
    // 遠近グリッドの描画
    drawPerspectiveGrid();
    
    // 建物の描画
    for (auto* building : sortedBuildings) {
        drawBuilding(*building);
    }
    
    ofPopMatrix();
}

void BuildingPerspectiveSystem::generateBuilding(float depth) {
    Building building;
    
    // より広範囲の建物配置
    float side = ofRandom(1.0f);
    if (side < 0.5f) {
        // 左側の建物
        building.position.set(
            ofRandom(-streetWidth * 2.0f, -streetWidth * 0.5f),
            ofRandom(-20, 0),
            depth
        );
    } else {
        // 右側の建物
        building.position.set(
            ofRandom(streetWidth * 0.5f, streetWidth * 2.0f),
            ofRandom(-20, 0),
            depth
        );
    }
    
    building.size.set(
        ofRandom(40, 120),
        ofRandom(buildingHeight * 0.3f, buildingHeight * 2.0f),
        ofRandom(60, 150)
    );
    
    building.rotationY = ofRandom(-15, 15);
    building.depth = depth - cameraPosition.z;
    building.spawnTime = ofGetElapsedTimef();
    
    createBuildingGeometry(building);
    buildings.push_back(building);
}

void BuildingPerspectiveSystem::updateCameraMovement(float deltaTime) {
    // MIDI連動によるカメラ効果
    if (crashIntensity > 0.5f) {
        cameraRotation += ofRandom(-5, 5) * crashIntensity;
        cameraSpeed *= (1.0f + crashIntensity * 0.5f);
    }
    
    if (kickIntensity > 0.3f) {
        walkSpeed = 1.5f + kickIntensity * 1.0f;
    }
    
    // カメラの自然な左右移動
    float lateralMovement = sin(ofGetElapsedTimef() * 0.3f) * 5.0f;
    cameraPosition.x = ofLerp(cameraPosition.x, lateralMovement, deltaTime * 2.0f);
    
    // 高さの調整
    float targetHeight = -10.0f + sin(ofGetElapsedTimef() * 0.5f) * 3.0f;
    cameraPosition.y = ofLerp(cameraPosition.y, targetHeight, deltaTime * 3.0f);
}

void BuildingPerspectiveSystem::drawBuilding(const Building& building) {
    if (!building.isActive) return;
    
    // 距離に基づく透明度計算
    float distanceFactor = 1.0f - (building.depth / generationDistance);
    distanceFactor = ofClamp(distanceFactor, 0.0f, 1.0f);
    
    if (distanceFactor < 0.01f) return; // 見えないものは描画しない
    
    // 建物の基本情報
    float halfWidth = building.size.x * 0.5f;
    float halfDepth = building.size.z * 0.5f;
    float height = building.size.y;
    
    // 建物の角の3D座標を計算（回転を適用）
    float cosRot = cos(building.rotationY * PI / 180.0f);
    float sinRot = sin(building.rotationY * PI / 180.0f);
    
    std::vector<ofVec3f> corners(8);
    
    // 下面の4角
    corners[0] = ofVec3f(building.position.x + (-halfWidth * cosRot - -halfDepth * sinRot),
                        building.position.y,
                        building.position.z + (-halfWidth * sinRot + -halfDepth * cosRot));
    corners[1] = ofVec3f(building.position.x + (halfWidth * cosRot - -halfDepth * sinRot),
                        building.position.y,
                        building.position.z + (halfWidth * sinRot + -halfDepth * cosRot));
    corners[2] = ofVec3f(building.position.x + (halfWidth * cosRot - halfDepth * sinRot),
                        building.position.y,
                        building.position.z + (halfWidth * sinRot + halfDepth * cosRot));
    corners[3] = ofVec3f(building.position.x + (-halfWidth * cosRot - halfDepth * sinRot),
                        building.position.y,
                        building.position.z + (-halfWidth * sinRot + halfDepth * cosRot));
    
    // 上面の4角
    for (int i = 0; i < 4; i++) {
        corners[i + 4] = corners[i];
        corners[i + 4].y -= height;
    }
    
    // 2D投影座標を計算
    std::vector<ofVec2f> projectedCorners(8);
    for (int i = 0; i < 8; i++) {
        projectPoint(corners[i], projectedCorners[i]);
    }
    
    // 面の描画（奥から手前へ）
    
    // 左面（可視性チェック）
    if (building.position.x < cameraPosition.x) {
        ofSetColor(buildingDark.r, buildingDark.g, buildingDark.b, 200 * distanceFactor);
        ofBeginShape();
        ofVertex(projectedCorners[0].x, projectedCorners[0].y);
        ofVertex(projectedCorners[3].x, projectedCorners[3].y);
        ofVertex(projectedCorners[7].x, projectedCorners[7].y);
        ofVertex(projectedCorners[4].x, projectedCorners[4].y);
        ofEndShape(true);
    }
    
    // 右面
    if (building.position.x > cameraPosition.x) {
        ofSetColor(buildingMedium.r, buildingMedium.g, buildingMedium.b, 200 * distanceFactor);
        ofBeginShape();
        ofVertex(projectedCorners[1].x, projectedCorners[1].y);
        ofVertex(projectedCorners[5].x, projectedCorners[5].y);
        ofVertex(projectedCorners[6].x, projectedCorners[6].y);
        ofVertex(projectedCorners[2].x, projectedCorners[2].y);
        ofEndShape(true);
    }
    
    // 正面
    ofSetColor(buildingLight.r, buildingLight.g, buildingLight.b, 220 * distanceFactor);
    ofBeginShape();
    ofVertex(projectedCorners[0].x, projectedCorners[0].y);
    ofVertex(projectedCorners[1].x, projectedCorners[1].y);
    ofVertex(projectedCorners[5].x, projectedCorners[5].y);
    ofVertex(projectedCorners[4].x, projectedCorners[4].y);
    ofEndShape(true);
    
    // 建物のエッジを描画
    drawBuildingEdges(building);
    
    // 窓の描画
    drawWindows(building);
}

void BuildingPerspectiveSystem::drawBuildingEdges(const Building& building) {
    float distanceFactor = 1.0f - (building.depth / generationDistance);
    distanceFactor = ofClamp(distanceFactor, 0.0f, 1.0f);
    
    if (distanceFactor < 0.1f) return;
    
    ofSetColor(edgeColor.r, edgeColor.g, edgeColor.b, 150 * distanceFactor);
    ofSetLineWidth(1.0f + distanceFactor * 1.5f);
    
    // 建物の基本情報
    float halfWidth = building.size.x * 0.5f;
    float halfDepth = building.size.z * 0.5f;
    float height = building.size.y;
    
    // 建物の角の3D座標を計算（回転を適用）
    float cosRot = cos(building.rotationY * PI / 180.0f);
    float sinRot = sin(building.rotationY * PI / 180.0f);
    
    std::vector<ofVec3f> corners(8);
    
    // 下面の4角
    corners[0] = ofVec3f(building.position.x + (-halfWidth * cosRot - -halfDepth * sinRot),
                        building.position.y,
                        building.position.z + (-halfWidth * sinRot + -halfDepth * cosRot));
    corners[1] = ofVec3f(building.position.x + (halfWidth * cosRot - -halfDepth * sinRot),
                        building.position.y,
                        building.position.z + (halfWidth * sinRot + -halfDepth * cosRot));
    corners[2] = ofVec3f(building.position.x + (halfWidth * cosRot - halfDepth * sinRot),
                        building.position.y,
                        building.position.z + (halfWidth * sinRot + halfDepth * cosRot));
    corners[3] = ofVec3f(building.position.x + (-halfWidth * cosRot - halfDepth * sinRot),
                        building.position.y,
                        building.position.z + (-halfWidth * sinRot + halfDepth * cosRot));
    
    // 上面の4角
    for (int i = 0; i < 4; i++) {
        corners[i + 4] = corners[i];
        corners[i + 4].y -= height;
    }
    
    // 2D投影座標を計算
    std::vector<ofVec2f> projectedCorners(8);
    for (int i = 0; i < 8; i++) {
        projectPoint(corners[i], projectedCorners[i]);
    }
    
    // エッジラインの描画
    // 垂直エッジ
    for (int i = 0; i < 4; i++) {
        ofDrawLine(projectedCorners[i].x, projectedCorners[i].y,
                   projectedCorners[i + 4].x, projectedCorners[i + 4].y);
    }
    
    // 下面エッジ
    for (int i = 0; i < 4; i++) {
        int next = (i + 1) % 4;
        ofDrawLine(projectedCorners[i].x, projectedCorners[i].y,
                   projectedCorners[next].x, projectedCorners[next].y);
    }
    
    // 上面エッジ
    for (int i = 4; i < 8; i++) {
        int next = 4 + ((i - 4 + 1) % 4);
        ofDrawLine(projectedCorners[i].x, projectedCorners[i].y,
                   projectedCorners[next].x, projectedCorners[next].y);
    }
}

void BuildingPerspectiveSystem::drawWindows(const Building& building) {
    float distanceFactor = 1.0f - (building.depth / generationDistance);
    if (distanceFactor < 0.2f) return;
    
    ofSetColor(windowColor.r, windowColor.g, windowColor.b, 120 * distanceFactor);
    
    // 建物の正面にのみ窓を描画
    float halfWidth = building.size.x * 0.5f;
    float height = building.size.y;
    
    // 窓の行と列を計算
    int windowRows = ofClamp((int)(height / 25.0f), 2, 8);
    int windowCols = ofClamp((int)(building.size.x / 20.0f), 2, 6);
    
    float cosRot = cos(building.rotationY * PI / 180.0f);
    float sinRot = sin(building.rotationY * PI / 180.0f);
    
    for (int row = 1; row < windowRows; row++) {
        for (int col = 1; col < windowCols; col++) {
            if (ofRandom(1.0f) < 0.8f) {  // 80%の確率で窓を描画
                
                // 窓の3D位置を計算
                float localX = -halfWidth + (col * building.size.x / windowCols);
                float localY = building.position.y - (row * height / windowRows);
                float localZ = building.size.z * 0.5f; // 正面
                
                // 回転を適用
                ofVec3f windowPos(
                    building.position.x + (localX * cosRot - localZ * sinRot),
                    localY,
                    building.position.z + (localX * sinRot + localZ * cosRot)
                );
                
                ofVec2f windowScreen;
                projectPoint(windowPos, windowScreen);
                
                // 距離に基づくサイズ調整
                float windowSize = ofClamp(4.0f * distanceFactor, 1.0f, 6.0f);
                
                // 時々点滅する窓
                float brightness = 1.0f;
                if (ofRandom(1.0f) < 0.1f) {
                    brightness = 0.3f + 0.7f * sin(ofGetElapsedTimef() * 5.0f + row * col);
                }
                
                ofSetColor(windowColor.r * brightness, windowColor.g * brightness, 
                          windowColor.b * brightness, 120 * distanceFactor);
                
                ofDrawRectangle(windowScreen.x - windowSize * 0.5f, 
                               windowScreen.y - windowSize * 0.5f,
                               windowSize, windowSize);
            }
        }
    }
}

void BuildingPerspectiveSystem::drawStreetElements() {
    ofSetLineWidth(2.0f);
    
    // 道路のセンターライン
    for (int i = 0; i < 60; i++) {
        float lineZ = cameraPosition.z + i * 15.0f;
        float distanceFactor = 1.0f - (i * 15.0f / generationDistance);
        
        if (distanceFactor <= 0.0f) continue;
        
        ofVec2f lineStart, lineEnd;
        projectPoint(ofVec3f(-3, 2, lineZ), lineStart);
        projectPoint(ofVec3f(3, 2, lineZ), lineEnd);
        
        ofSetColor(floorColor.r + 20, floorColor.g + 20, floorColor.b + 20, 100 * distanceFactor);
        ofDrawLine(lineStart.x, lineStart.y, lineEnd.x, lineEnd.y);
    }
    
    // 左右の歩道境界
    for (int i = 0; i < 40; i++) {
        float lineZ = cameraPosition.z + i * 25.0f;
        float distanceFactor = 1.0f - (i * 25.0f / generationDistance);
        
        if (distanceFactor <= 0.0f) continue;
        
        // 左歩道
        ofVec2f leftStart, leftEnd;
        projectPoint(ofVec3f(-streetWidth * 0.7f, 5, lineZ), leftStart);
        projectPoint(ofVec3f(-streetWidth * 0.7f, -5, lineZ), leftEnd);
        
        ofSetColor(buildingMedium.r, buildingMedium.g, buildingMedium.b, 80 * distanceFactor);
        ofDrawLine(leftStart.x, leftStart.y, leftEnd.x, leftEnd.y);
        
        // 右歩道
        ofVec2f rightStart, rightEnd;
        projectPoint(ofVec3f(streetWidth * 0.7f, 5, lineZ), rightStart);
        projectPoint(ofVec3f(streetWidth * 0.7f, -5, lineZ), rightEnd);
        
        ofDrawLine(rightStart.x, rightStart.y, rightEnd.x, rightEnd.y);
    }
    
    // 地平線の描画
    ofSetColor(buildingDark.r, buildingDark.g, buildingDark.b, 40);
    ofSetLineWidth(1.0f);
    
    ofVec2f horizonLeft, horizonRight;
    projectPoint(ofVec3f(-streetWidth * 3.0f, 0, cameraPosition.z + generationDistance), horizonLeft);
    projectPoint(ofVec3f(streetWidth * 3.0f, 0, cameraPosition.z + generationDistance), horizonRight);
    
    ofDrawLine(horizonLeft.x, horizonLeft.y, horizonRight.x, horizonRight.y);
}

void BuildingPerspectiveSystem::drawPerspectiveGrid() {
    ofSetColor(buildingMedium.r * 0.4f, buildingMedium.g * 0.4f, buildingMedium.b * 0.4f, 25);
    ofSetLineWidth(0.5f);
    
    // 消失点に向かう収束線
    for (int i = -8; i <= 8; i++) {
        if (i == 0) continue; // 中央線は除く
        
        float x = i * 60.0f;
        ofVec2f gridStart, gridEnd;
        projectPoint(ofVec3f(x, 10, cameraPosition.z + 20), gridStart);
        projectPoint(ofVec3f(x * 0.1f, 0, cameraPosition.z + generationDistance * 0.8f), gridEnd);
        
        float alpha = 25.0f / (1.0f + abs(i) * 0.5f);
        ofSetColor(buildingMedium.r * 0.4f, buildingMedium.g * 0.4f, buildingMedium.b * 0.4f, alpha);
        ofDrawLine(gridStart.x, gridStart.y, gridEnd.x, gridEnd.y);
    }
    
    // 深度グリッドライン（道路の横線）
    for (int i = 1; i < 25; i++) {
        float z = cameraPosition.z + i * 20.0f;
        float distanceFactor = 1.0f - (i * 20.0f / generationDistance);
        
        if (distanceFactor <= 0.0f) continue;
        
        float width = streetWidth * 1.5f * distanceFactor;
        
        ofVec2f gridStart, gridEnd;
        projectPoint(ofVec3f(-width, 8, z), gridStart);
        projectPoint(ofVec3f(width, 8, z), gridEnd);
        
        ofSetColor(buildingMedium.r * 0.3f, buildingMedium.g * 0.3f, buildingMedium.b * 0.3f, 
                   20 * distanceFactor);
        ofDrawLine(gridStart.x, gridStart.y, gridEnd.x, gridEnd.y);
    }
}

void BuildingPerspectiveSystem::projectPoint(const ofVec3f& point3D, ofVec2f& point2D) {
    // 改良された透視投影
    float relativeX = point3D.x - cameraPosition.x;
    float relativeY = point3D.y - cameraPosition.y;
    float relativeZ = point3D.z - cameraPosition.z;
    
    // 最小距離制限
    if (relativeZ <= 1.0f) {
        relativeZ = 1.0f;
    }
    
    // 視野角とアスペクト比を考慮した透視投影
    float fov = perspectiveAngle * PI / 180.0f;
    float aspectRatio = (float)ofGetWidth() / ofGetHeight();
    
    // 正規化デバイス座標への変換
    float ndcX = relativeX / (relativeZ * tan(fov * 0.5f) * aspectRatio);
    float ndcY = -relativeY / (relativeZ * tan(fov * 0.5f));
    
    // スクリーン座標への変換（座標系は画面中央基準に変更済み）
    point2D.x = ndcX * ofGetWidth() * 0.5f;
    point2D.y = ndcY * ofGetHeight() * 0.5f;
}

void BuildingPerspectiveSystem::createBuildingGeometry(Building& building) {
    // 建物の基本形状を作成
    float halfWidth = building.size.x * 0.5f;
    float halfDepth = building.size.z * 0.5f;
    float height = building.size.y;
    
    // 前面
    BuildingFace frontFace;
    frontFace.vertices.push_back(ofVec3f(-halfWidth, 0, halfDepth));
    frontFace.vertices.push_back(ofVec3f(halfWidth, 0, halfDepth));
    frontFace.vertices.push_back(ofVec3f(halfWidth, height, halfDepth));
    frontFace.vertices.push_back(ofVec3f(-halfWidth, height, halfDepth));
    frontFace.faceColor = buildingMedium;
    frontFace.windowDensity = 0.8f;
    
    // 側面
    BuildingFace sideFace;
    sideFace.vertices.push_back(ofVec3f(halfWidth, 0, halfDepth));
    sideFace.vertices.push_back(ofVec3f(halfWidth, 0, -halfDepth));
    sideFace.vertices.push_back(ofVec3f(halfWidth, height, -halfDepth));
    sideFace.vertices.push_back(ofVec3f(halfWidth, height, halfDepth));
    sideFace.faceColor = buildingDark;
    sideFace.windowDensity = 0.6f;
    
    building.faces.push_back(frontFace);
    building.faces.push_back(sideFace);
    
    // エッジの作成
    for (int i = 0; i < 12; i++) {
        BuildingEdge edge;
        edge.intensity = 180.0f;
        edge.width = 1.0f;
        edge.isVisible = true;
        
        // 建物の輪郭エッジを定義
        switch (i) {
            case 0: edge.start.set(-halfWidth, 0, halfDepth); edge.end.set(halfWidth, 0, halfDepth); break;
            case 1: edge.start.set(halfWidth, 0, halfDepth); edge.end.set(halfWidth, height, halfDepth); break;
            case 2: edge.start.set(halfWidth, height, halfDepth); edge.end.set(-halfWidth, height, halfDepth); break;
            case 3: edge.start.set(-halfWidth, height, halfDepth); edge.end.set(-halfWidth, 0, halfDepth); break;
            // 追加のエッジ...
        }
        
        building.edges.push_back(edge);
    }
}

void BuildingPerspectiveSystem::cleanupDistantBuildings() {
    buildings.erase(
        std::remove_if(buildings.begin(), buildings.end(),
                      [](const Building& b) {
                          return b.depth < -200.0f;
                      }),
        buildings.end()
    );
}

void BuildingPerspectiveSystem::onMidiMessage(ofxMidiMessage& msg) {
    if (msg.status == MIDI_NOTE_ON) {
        int note = msg.pitch;
        float velocity = msg.velocity / 127.0f;
        
        if (note == 36) {  // KICK
            kickIntensity = velocity;
            walkSpeed = 1.5f + velocity * 2.0f;
        }
        else if (note == 38) {  // SNARE
            snareIntensity = velocity;
            cameraSpeed *= (1.0f + velocity * 0.5f);
        }
        else if (note == 42) {  // HIHAT_CLOSED
            hihatIntensity = velocity;
            buildingSpawnRate = 0.1f + velocity * 0.2f;
        }
        else if (note == 49) {  // CRASH
            crashIntensity = velocity;
            cameraRotation += ofRandom(-20, 20) * velocity;
            
            // 新しい建物を大量生成
            for (int i = 0; i < 5; i++) {
                float newDepth = cameraPosition.z + ofRandom(100, 400);
                generateBuilding(newDepth);
            }
        }
    }
}

void BuildingPerspectiveSystem::onBeatDetected(float velocity) {
    // ビート検出で歩行リズムを同期
    walkBobbing += velocity * 0.5f;
    
    // 建物の生成頻度を一時的に上げる
    buildingSpawnRate = 0.1f + velocity * 0.3f;
}

void BuildingPerspectiveSystem::reset() {
    buildings.clear();
    cameraPosition.set(0, 0, 0);
    cameraTarget.set(0, 0, 100);
    cameraRotation = 0.0f;
    setup();
}

void BuildingPerspectiveSystem::setGlobalGrowthLevel(float level) {
    globalGrowthLevel = level;
    
    // 成長レベルに基づく色彩の調整
    buildingDark = ofColor(40 + level * 25, 40 + level * 25, 40 + level * 25);
    buildingMedium = ofColor(85 + level * 35, 85 + level * 35, 85 + level * 35);
    buildingLight = ofColor(130 + level * 25, 130 + level * 25, 130 + level * 25);
    
    // 建物の複雑度調整
    buildingHeight = 150.0f + level * 100.0f;
    buildingDensity = 0.8f + level * 0.4f;
    
    // 移動速度の調整
    cameraSpeed = 80.0f + level * 40.0f;
    walkSpeed = 1.5f + level * 0.8f;
}