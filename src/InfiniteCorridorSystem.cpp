#include "InfiniteCorridorSystem.h"

InfiniteCorridorSystem::InfiniteCorridorSystem() {
    corridorDepth = 0.0f;
    walkingSpeed = 30.0f;
    perspectiveShift = 0.0f;
    vanishingPoint.set(ofGetWidth() * 0.5f, ofGetHeight() * 0.4f);
    ambientIntensity = 60.0f;
    corridorWidth = 800.0f;
    corridorHeight = 600.0f;
    
    // モノトーンカラーパレット
    darkGray = ofColor(30, 30, 30);
    mediumGray = ofColor(80, 80, 80);
    lightGray = ofColor(120, 120, 120);
    shadowColor = ofColor(15, 15, 15);
    
    walkCycleTime = 0.0f;
    perspectiveOscillation = 0.0f;
    corridorSway = 0.0f;
    
    kickIntensity = 0.0f;
    snareIntensity = 0.0f;
    hihatIntensity = 0.0f;
    crashIntensity = 0.0f;
}

void InfiniteCorridorSystem::setup() {
    // 回廊セグメントの初期化
    for (int i = 0; i < 8; i++) {
        CorridorSegment segment;
        segment.depth = i * 100.0f;
        segment.vanishingPoint = vanishingPoint;
        corridorSegments.push_back(segment);
    }
    
    // 歩行人物の初期化
    for (int i = 0; i < 3; i++) {
        WalkingFigure figure;
        figure.position.set(
            vanishingPoint.x + ofRandom(-50, 50),
            vanishingPoint.y + ofRandom(100, 200)
        );
        figure.walkSpeed = ofRandom(20.0f, 40.0f);
        figure.walkCycle = ofRandom(0, TWO_PI);
        figure.scale = ofRandom(0.8f, 1.2f);
        figures.push_back(figure);
    }
}

void InfiniteCorridorSystem::update(float deltaTime) {
    float currentTime = ofGetElapsedTimef();
    
    // 全体的なアニメーション
    walkCycleTime += deltaTime * 2.0f;
    perspectiveOscillation += deltaTime * 0.5f;
    corridorSway += deltaTime * 0.3f;
    
    // 回廊の深度更新
    corridorDepth += walkingSpeed * deltaTime;
    if (corridorDepth > 800.0f) {
        corridorDepth = 0.0f;
    }
    
    // 消失点の微妙な揺れ
    vanishingPoint.x = ofGetWidth() * 0.5f + sin(perspectiveOscillation) * 20.0f * globalGrowthLevel;
    vanishingPoint.y = ofGetHeight() * 0.4f + cos(perspectiveOscillation * 0.7f) * 10.0f * globalGrowthLevel;
    
    // 回廊セグメントの更新
    for (auto& segment : corridorSegments) {
        segment.vanishingPoint = vanishingPoint;
        segment.depth -= walkingSpeed * deltaTime;
        
        if (segment.depth < -100.0f) {
            segment.depth = 700.0f;
        }
        
        // 成長レベルに基づく壁の明度調整
        segment.wallIntensity = 80.0f + globalGrowthLevel * 40.0f;
        segment.floorIntensity = 40.0f + globalGrowthLevel * 20.0f;
    }
    
    // 歩行人物の更新
    for (auto& figure : figures) {
        updateFigureWalk(figure, deltaTime);
    }
    
    // MIDI連動強度の減衰
    kickIntensity *= 0.95f;
    snareIntensity *= 0.92f;
    hihatIntensity *= 0.9f;
    crashIntensity *= 0.88f;
    
    // 新しい人物の生成（低確率）
    if (ofRandom(1.0f) < 0.005f * globalGrowthLevel) {
        createNewFigure();
    }
    
    updateCorridorPerspective(deltaTime);
}

void InfiniteCorridorSystem::draw() {
    ofPushMatrix();
    
    // 背景
    ofSetColor(darkGray.r * 0.8f, darkGray.g * 0.8f, darkGray.b * 0.8f);
    ofDrawRectangle(0, 0, ofGetWidth(), ofGetHeight());
    
    // 回廊の描画
    drawCorridor();
    
    // 床パターンの描画
    drawFloorPattern();
    
    // 壁の詳細描画
    drawWallDetails();
    
    // 歩行人物の描画
    drawWalkingFigures();
    
    // 遠近線の描画
    drawPerspectiveLines();
    
    ofPopMatrix();
}

void InfiniteCorridorSystem::drawCorridor() {
    ofSetLineWidth(2.0f);
    
    for (const auto& segment : corridorSegments) {
        float perspectiveFactor = 1.0f - (segment.depth / 800.0f);
        if (perspectiveFactor <= 0.0f) continue;
        
        float segmentWidth = corridorWidth * perspectiveFactor;
        float segmentHeight = corridorHeight * perspectiveFactor;
        
        float leftX = segment.vanishingPoint.x - segmentWidth * 0.5f;
        float rightX = segment.vanishingPoint.x + segmentWidth * 0.5f;
        float topY = segment.vanishingPoint.y - segmentHeight * 0.3f;
        float bottomY = segment.vanishingPoint.y + segmentHeight * 0.7f;
        
        // 壁面の描画
        ofSetColor(segment.wallIntensity, segment.wallIntensity, segment.wallIntensity, 
                   200.0f * perspectiveFactor);
        
        // 左壁
        ofDrawLine(leftX, topY, leftX, bottomY);
        
        // 右壁
        ofDrawLine(rightX, topY, rightX, bottomY);
        
        // 天井
        ofDrawLine(leftX, topY, rightX, topY);
        
        // 床面
        ofSetColor(segment.floorIntensity, segment.floorIntensity, segment.floorIntensity, 
                   150.0f * perspectiveFactor);
        ofDrawLine(leftX, bottomY, rightX, bottomY);
    }
}

void InfiniteCorridorSystem::drawWalkingFigures() {
    for (const auto& figure : figures) {
        if (!figure.isActive) continue;
        
        ofPushMatrix();
        ofTranslate(figure.position.x, figure.position.y);
        ofScale(figure.scale, figure.scale);
        
        // 影の描画
        ofSetColor(shadowColor.r, shadowColor.g, shadowColor.b, figure.fadeAlpha * 0.6f);
        ofPushMatrix();
        ofTranslate(figure.shadowOffset, 20.0f);
        ofDrawEllipse(0, 0, 30, 10);
        ofPopMatrix();
        
        // 人物の描画（シンプルなシルエット）
        ofSetColor(mediumGray.r, mediumGray.g, mediumGray.b, figure.fadeAlpha);
        
        // 体
        ofDrawRectangle(-8, -30, 16, 40);
        
        // 頭
        ofDrawEllipse(0, -40, 12, 12);
        
        // 歩行アニメーション用の脚
        float legOffset = sin(figure.walkCycle) * 8.0f;
        ofDrawRectangle(-6 + legOffset, 10, 4, 20);
        ofDrawRectangle(2 - legOffset, 10, 4, 20);
        
        // 腕の振り
        float armOffset = sin(figure.walkCycle + PI) * 5.0f;
        ofDrawRectangle(-12 + armOffset, -20, 3, 15);
        ofDrawRectangle(9 - armOffset, -20, 3, 15);
        
        ofPopMatrix();
    }
}

void InfiniteCorridorSystem::drawPerspectiveLines() {
    ofSetLineWidth(1.0f);
    ofSetColor(lightGray.r * 0.6f, lightGray.g * 0.6f, lightGray.b * 0.6f, 100);
    
    // 消失点への収束線
    int numLines = 8;
    for (int i = 0; i < numLines; i++) {
        float angle = (float)i / numLines * TWO_PI;
        float startRadius = 200.0f;
        float endRadius = 800.0f;
        
        float startX = vanishingPoint.x + cos(angle) * startRadius;
        float startY = vanishingPoint.y + sin(angle) * startRadius;
        float endX = vanishingPoint.x + cos(angle) * endRadius;
        float endY = vanishingPoint.y + sin(angle) * endRadius;
        
        ofDrawLine(startX, startY, endX, endY);
    }
}

void InfiniteCorridorSystem::drawFloorPattern() {
    ofSetLineWidth(1.0f);
    
    // 床のタイルパターン
    float tileSpacing = 50.0f + corridorDepth;
    int numTiles = 15;
    
    for (int i = 0; i < numTiles; i++) {
        float tileDepth = i * tileSpacing - fmod(corridorDepth, tileSpacing);
        float perspectiveFactor = 1.0f - (tileDepth / 800.0f);
        
        if (perspectiveFactor <= 0.0f) continue;
        
        float tileWidth = corridorWidth * perspectiveFactor;
        float tileY = vanishingPoint.y + 200.0f * perspectiveFactor;
        
        ofSetColor(mediumGray.r * 0.7f, mediumGray.g * 0.7f, mediumGray.b * 0.7f, 
                   100.0f * perspectiveFactor);
        
        ofDrawLine(vanishingPoint.x - tileWidth * 0.5f, tileY, 
                   vanishingPoint.x + tileWidth * 0.5f, tileY);
    }
}

void InfiniteCorridorSystem::drawWallDetails() {
    // 壁の装飾要素（ドア、窓、パネル等）
    float detailSpacing = 120.0f + corridorDepth;
    int numDetails = 8;
    
    for (int i = 0; i < numDetails; i++) {
        float detailDepth = i * detailSpacing - fmod(corridorDepth, detailSpacing);
        float perspectiveFactor = 1.0f - (detailDepth / 800.0f);
        
        if (perspectiveFactor <= 0.0f) continue;
        
        float detailWidth = corridorWidth * perspectiveFactor;
        float detailHeight = corridorHeight * perspectiveFactor;
        
        float leftX = vanishingPoint.x - detailWidth * 0.5f;
        float rightX = vanishingPoint.x + detailWidth * 0.5f;
        float midY = vanishingPoint.y;
        
        // 左壁のドア/窓
        ofSetColor(darkGray.r * 1.2f, darkGray.g * 1.2f, darkGray.b * 1.2f, 
                   150.0f * perspectiveFactor);
        ofDrawRectangle(leftX - 5, midY - 30 * perspectiveFactor, 
                       10, 60 * perspectiveFactor);
        
        // 右壁のドア/窓
        ofDrawRectangle(rightX - 5, midY - 30 * perspectiveFactor, 
                       10, 60 * perspectiveFactor);
    }
}

void InfiniteCorridorSystem::updateFigureWalk(WalkingFigure& figure, float deltaTime) {
    figure.walkCycle += deltaTime * figure.walkSpeed * 0.1f;
    
    // 歩行に伴う位置の微調整
    figure.position.x += sin(figure.walkCycle * 0.1f) * 0.5f;
    
    // 影のオフセット
    figure.shadowOffset = sin(figure.walkCycle) * 3.0f;
    
    // 深度に基づくスケール調整
    float depthFactor = (figure.position.y - vanishingPoint.y) / 200.0f;
    figure.scale = 1.0f + depthFactor * 0.3f;
    
    // 画面端での消失・再生成
    if (figure.position.y > ofGetHeight() + 50) {
        figure.position.y = vanishingPoint.y - 50;
        figure.position.x = vanishingPoint.x + ofRandom(-30, 30);
        figure.walkSpeed = ofRandom(20.0f, 40.0f);
    }
    
    // 前進
    figure.position.y += figure.walkSpeed * deltaTime * 0.5f;
}

void InfiniteCorridorSystem::createNewFigure() {
    if (figures.size() >= 5) return;
    
    WalkingFigure newFigure;
    newFigure.position.set(
        vanishingPoint.x + ofRandom(-40, 40),
        vanishingPoint.y + ofRandom(-30, 0)
    );
    newFigure.walkSpeed = ofRandom(15.0f, 35.0f);
    newFigure.walkCycle = ofRandom(0, TWO_PI);
    newFigure.scale = ofRandom(0.6f, 1.0f);
    newFigure.fadeAlpha = 255.0f;
    
    figures.push_back(newFigure);
}

void InfiniteCorridorSystem::updateCorridorPerspective(float deltaTime) {
    // 成長レベルに基づく遠近感の調整
    float targetWidth = 800.0f + globalGrowthLevel * 200.0f;
    float targetHeight = 600.0f + globalGrowthLevel * 150.0f;
    
    corridorWidth = ofLerp(corridorWidth, targetWidth, deltaTime * 2.0f);
    corridorHeight = ofLerp(corridorHeight, targetHeight, deltaTime * 2.0f);
    
    // 歩行速度の調整
    float targetSpeed = 30.0f + globalGrowthLevel * 20.0f;
    walkingSpeed = ofLerp(walkingSpeed, targetSpeed, deltaTime * 3.0f);
}

void InfiniteCorridorSystem::onMidiMessage(ofxMidiMessage& msg) {
    if (msg.status == MIDI_NOTE_ON) {
        int note = msg.pitch;
        float velocity = msg.velocity / 127.0f;
        
        // ドラムマッピング
        if (note == 36) {  // KICK
            kickIntensity = velocity;
            // キックで新しい人物を生成
            if (ofRandom(1.0f) < 0.7f) {
                createNewFigure();
            }
        }
        else if (note == 38) {  // SNARE
            snareIntensity = velocity;
            // スネアで遠近感を一時的に強調
            perspectiveShift = velocity * 50.0f;
        }
        else if (note == 42) {  // HIHAT_CLOSED
            hihatIntensity = velocity;
            // ハイハットで歩行速度を一時的に上げる
            walkingSpeed += velocity * 10.0f;
        }
        else if (note == 49) {  // CRASH
            crashIntensity = velocity;
            // クラッシュで劇的な遠近感の変化
            vanishingPoint.x += ofRandom(-100, 100) * velocity;
            vanishingPoint.y += ofRandom(-50, 50) * velocity;
        }
    }
}

void InfiniteCorridorSystem::onBeatDetected(float velocity) {
    // ビート検出で歩行リズムを同期
    for (auto& figure : figures) {
        figure.walkCycle += velocity * 0.5f;
    }
    
    // 回廊の振動
    corridorSway += velocity * 0.3f;
}

void InfiniteCorridorSystem::reset() {
    figures.clear();
    corridorSegments.clear();
    setup();
}

void InfiniteCorridorSystem::setGlobalGrowthLevel(float level) {
    globalGrowthLevel = level;
    
    // 成長レベルに基づく環境の変化
    ambientIntensity = 60.0f + level * 40.0f;
    
    // 色彩の調整
    darkGray = ofColor(30 + level * 20, 30 + level * 20, 30 + level * 20);
    mediumGray = ofColor(80 + level * 30, 80 + level * 30, 80 + level * 30);
    lightGray = ofColor(120 + level * 20, 120 + level * 20, 120 + level * 20);
}