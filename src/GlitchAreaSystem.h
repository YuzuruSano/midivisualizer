#pragma once

#include "ofMain.h"
#include "ofxPostGlitch.h"
#include <vector>

enum GlitchAreaShape {
    CIRCLE = 0,
    ELLIPSE,
    RECTANGLE,
    DIAMOND,
    TRIANGLE,
    NUM_SHAPES
};

enum MovementPattern {
    STATIC = 0,
    LINEAR_SWEEP,
    CIRCULAR_ORBIT,
    ZIGZAG,
    RANDOM_WALK,
    SPOTLIGHT_SCAN,
    NUM_MOVEMENT_PATTERNS
};

struct TrailPoint {
    ofVec2f position;
    float age;
    float maxAge;
    float intensity;
    
    TrailPoint(ofVec2f pos, float maxAge) : position(pos), age(0), maxAge(maxAge), intensity(1.0) {}
    
    void update(float dt) {
        age += dt;
        if (age > maxAge) age = maxAge;
        intensity = 1.0 - (age / maxAge); // フェードアウト
    }
    
    bool isDead() const {
        return age >= maxAge;
    }
};

class GlitchArea {
public:
    ofVec2f position;
    ofVec2f targetPosition;
    ofVec2f startPosition;
    float width, height;  // 楕円・矩形用
    float rotation;
    float lifetime;
    float maxLifetime;
    int glitchType;
    float intensity;
    GlitchAreaShape shape;
    MovementPattern movementPattern;
    
    // 移動関連
    float movementSpeed;
    float baseMovementSpeed;
    float targetMovementSpeed;
    float speedTransitionTime;
    float speedTransitionDuration;
    float easingProgress;
    ofVec2f movementDirection;
    float orbitRadius;
    float orbitAngle;
    float nextTargetTime;
    
    // 緩急システム
    float accelerationPhase;
    float pauseTimer;
    float pauseDuration;
    bool isPaused;
    float intensityMultiplier;
    
    // トレイル関連
    std::vector<TrailPoint> trail;
    float trailInterval;
    float lastTrailTime;
    float trailMaxAge;
    int maxTrailPoints;
    
    GlitchArea(float x, float y, float w, float h, float life, int type, GlitchAreaShape s = CIRCLE, MovementPattern mp = STATIC) 
        : position(x, y), targetPosition(x, y), startPosition(x, y), 
          width(w), height(h), rotation(0), lifetime(life), maxLifetime(life), 
          glitchType(type), intensity(1.0), shape(s), movementPattern(mp),
          movementSpeed(ofRandom(40, 120)), baseMovementSpeed(ofRandom(40, 120)),
          targetMovementSpeed(ofRandom(120, 300)), speedTransitionTime(0), speedTransitionDuration(ofRandom(2.0, 5.0)),
          easingProgress(0), orbitRadius(ofRandom(50, 150)), orbitAngle(0), nextTargetTime(ofRandom(0.5, 2.0)),
          accelerationPhase(0), pauseTimer(0), pauseDuration(ofRandom(0.3, 1.5)), isPaused(false), intensityMultiplier(1.0),
          trailInterval(0.05f), lastTrailTime(0), trailMaxAge(3.0f), maxTrailPoints(60) {
        
        // 初期移動方向
        movementDirection = ofVec2f(ofRandom(-1, 1), ofRandom(-1, 1)).getNormalized();
        
        // 形状別のサイズ調整
        if (shape == ELLIPSE) {
            height = h * ofRandom(0.3, 0.8); // 楕円の縦横比
        } else if (shape == RECTANGLE || shape == DIAMOND) {
            width = w * ofRandom(0.7, 1.3);
            height = h * ofRandom(0.7, 1.3);
        }
        
        rotation = ofRandom(0, 360);
        
        // 初期トレイルポイントを追加
        trail.push_back(TrailPoint(position, trailMaxAge));
        lastTrailTime = ofGetElapsedTimef();
    }
    
    void update(float dt, int screenWidth, int screenHeight) {
        lifetime -= dt;
        if (lifetime < 0) lifetime = 0;
        
        // フェードイン・フェードアウト効果
        float lifeRatio = lifetime / maxLifetime;
        if (lifeRatio > 0.8) {
            intensity = (1.0 - lifeRatio) / 0.2; // フェードイン
        } else if (lifeRatio < 0.3) {
            intensity = lifeRatio / 0.3; // フェードアウト
        } else {
            intensity = 1.0;
        }
        
        // 緩急システムの更新
        updateSpeedDynamics(dt);
        
        // 移動パターンに応じた位置更新
        ofVec2f oldPosition = position;
        updateMovement(dt, screenWidth, screenHeight);
        
        // トレイルポイントの更新
        updateTrail(dt, oldPosition);
        
        // 回転更新（速度に応じて回転速度も変化）
        rotation += dt * (20 + movementSpeed * 0.5); // 速度に応じた回転
    }
    
    void updateMovement(float dt, int screenWidth, int screenHeight) {
        switch (movementPattern) {
            case STATIC:
                // 静止
                break;
                
            case LINEAR_SWEEP:
                // 直線移動（サーチライト的）
                position += movementDirection * movementSpeed * dt;
                
                // 画面外に出たら反対側から
                if (position.x < -width || position.x > screenWidth + width) {
                    movementDirection.x *= -1;
                    position.x = ofClamp(position.x, -width, screenWidth + width);
                }
                if (position.y < -height || position.y > screenHeight + height) {
                    movementDirection.y *= -1;
                    position.y = ofClamp(position.y, -height, screenHeight + height);
                }
                break;
                
            case CIRCULAR_ORBIT:
                // 円軌道移動
                orbitAngle += dt * 60; // 60度/秒
                position.x = startPosition.x + cos(ofDegToRad(orbitAngle)) * orbitRadius;
                position.y = startPosition.y + sin(ofDegToRad(orbitAngle)) * orbitRadius;
                break;
                
            case ZIGZAG:
                // ジグザグ移動
                position.x += movementDirection.x * movementSpeed * dt;
                position.y += sin(ofGetElapsedTimef() * 3) * 50 * dt;
                
                if (position.x < 0 || position.x > screenWidth) {
                    movementDirection.x *= -1;
                }
                break;
                
            case RANDOM_WALK: {
                // ランダムウォーク（新しいターゲットに向かってイージング移動）
                nextTargetTime -= dt;
                if (nextTargetTime <= 0) {
                    // 新しいターゲット設定
                    startPosition = position;
                    targetPosition.x = ofRandom(width, screenWidth - width);
                    targetPosition.y = ofRandom(height, screenHeight - height);
                    easingProgress = 0;
                    nextTargetTime = ofRandom(1.0, 3.0);
                }
                
                // イージング移動（easeInOutCubic）
                easingProgress += dt / 2.0; // 2秒で移動完了
                if (easingProgress > 1.0) easingProgress = 1.0;
                
                float t = easingProgress;
                t = t < 0.5 ? 4 * t * t * t : (t - 1) * (2 * t - 2) * (2 * t - 2) + 1;
                
                position = startPosition.getInterpolated(targetPosition, t);
                break;
            }
                
            case SPOTLIGHT_SCAN: {
                // サーチライト風スキャン（緩急効果付き）
                if (!isPaused) {
                    float currentSpeed = movementSpeed * intensityMultiplier;
                    position.x += movementDirection.x * currentSpeed * dt;
                    position.y += movementDirection.y * (currentSpeed * 0.4) * dt;
                }
                
                // 左右反転（画面端でバウンス）
                if (position.x < width/2 || position.x > screenWidth - width/2) {
                    movementDirection.x *= -1;
                    position.x = ofClamp(position.x, width/2, screenWidth - width/2);
                    
                    // 反転時にドラマチックな効果
                    triggerSpeedBurst(); // 急加速
                    
                    // 反転時に縦方向の動きも微調整
                    if (ofRandom(1.0) < 0.4) { // 40%の確率で縦方向も反転
                        movementDirection.y *= -1;
                    }
                    
                    // 50%の確率で一時停止
                    if (ofRandom(1.0) < 0.5) {
                        triggerPause();
                    }
                }
                
                // 上下反転
                if (position.y < height/2 || position.y > screenHeight - height/2) {
                    movementDirection.y *= -1;
                    position.y = ofClamp(position.y, height/2, screenHeight - height/2);
                    
                    // 縦反転時も効果的な変化
                    if (ofRandom(1.0) < 0.3) {
                        triggerSpeedBurst();
                    }
                }
                break;
            }
                
            case NUM_MOVEMENT_PATTERNS:
                // デフォルトケース（何もしない）
                break;
        }
    }
    
    void updateTrail(float dt, ofVec2f oldPosition) {
        float currentTime = ofGetElapsedTimef();
        
        // 位置が変わった場合のみ新しいトレイルポイントを追加
        float distance = position.distance(oldPosition);
        if (distance > 5.0f && currentTime - lastTrailTime > trailInterval) {
            trail.push_back(TrailPoint(oldPosition, trailMaxAge));
            lastTrailTime = currentTime;
            
            // 最大数を超えた場合は古いものを削除
            if (trail.size() > maxTrailPoints) {
                trail.erase(trail.begin());
            }
        }
        
        // 既存のトレイルポイントを更新
        trail.erase(
            std::remove_if(trail.begin(), trail.end(),
                [dt](TrailPoint& point) {
                    point.update(dt);
                    return point.isDead();
                }),
            trail.end()
        );
    }
    
    void updateSpeedDynamics(float dt) {
        // 一時停止システム
        if (isPaused) {
            pauseTimer -= dt;
            if (pauseTimer <= 0) {
                isPaused = false;
                // 停止明けは急加速
                triggerSpeedBurst();
            }
            return;
        }
        
        // 速度遷移システム
        if (speedTransitionTime < speedTransitionDuration) {
            speedTransitionTime += dt;
            float progress = speedTransitionTime / speedTransitionDuration;
            
            // easeInOutQuart（より急激な変化）
            float t;
            if (progress < 0.5) {
                t = 8 * progress * progress * progress * progress;
            } else {
                float p = progress - 1;
                t = 1 - 8 * p * p * p * p;
            }
            
            movementSpeed = ofLerp(baseMovementSpeed, targetMovementSpeed, t);
            
            if (speedTransitionTime >= speedTransitionDuration) {
                // 次の遷移を準備
                prepareNextSpeedTransition();
            }
        }
        
        // 加速フェーズの更新
        if (accelerationPhase > 0) {
            accelerationPhase -= dt;
            intensityMultiplier = 1.0 + (accelerationPhase / 1.0) * 2.0; // 最大3倍速
        } else {
            intensityMultiplier = 1.0;
        }
        
        // ランダムな緩急変化
        if (ofRandom(1.0) < 0.005 * dt * 60) { // 60FPSで約0.3%/秒の確率
            if (ofRandom(1.0) < 0.6) {
                triggerSpeedBurst();
            } else {
                triggerPause();
            }
        }
    }
    
    void prepareNextSpeedTransition() {
        baseMovementSpeed = movementSpeed;
        
        // より極端な速度変化
        if (ofRandom(1.0) < 0.4) {
            // 高速モード
            targetMovementSpeed = ofRandom(120, 300);
        } else if (ofRandom(1.0) < 0.3) {
            // 低速モード
            targetMovementSpeed = ofRandom(10, 40);
        } else {
            // 中速モード
            targetMovementSpeed = ofRandom(50, 100);
        }
        
        speedTransitionTime = 0;
        speedTransitionDuration = ofRandom(2.0, 5.0); // より長い遷移時間
    }
    
    void triggerSpeedBurst() {
        accelerationPhase = ofRandom(0.5, 1.5); // 0.5-1.5秒の急加速
        intensityMultiplier = ofRandom(2.0, 4.0); // 2-4倍速
    }
    
    void triggerPause() {
        isPaused = true;
        pauseTimer = ofRandom(0.3, 1.5); // 0.3-1.5秒の停止
    }
    
    bool isDead() const {
        return lifetime <= 0;
    }
    
    float getIntensity() const {
        return intensity;
    }
};

class GlitchAreaSystem {
private:
    std::vector<GlitchArea> areas;
    ofxPostGlitch postGlitch;
    ofFbo glitchFbo;
    ofFbo areaMaskFbo;
    int width, height;
    bool isInitialized;
    bool lightweightMode = false;  // 軽量モードを無効化（グリッチ効果優先）
    
    // グリッチタイプ定義（指定の10種類）
    enum GlitchType {
        CONVERGENCE = 0,
        GLOW,
        SHAKE,
        CUT_SLIDER,
        TWIST,
        OUTLINE,
        NOISE,
        SLITSCAN,
        SWELL,
        INVERT,
        NUM_GLITCH_TYPES
    };
    
public:
    GlitchAreaSystem() : isInitialized(false) {}
    
    void setup(int w, int h) {
        width = w;
        height = h;
        
        // FBO初期化（高品質：32ビットFloatRGBA）
        ofFbo::Settings settings;
        settings.width = width;
        settings.height = height;
        settings.internalformat = GL_RGBA32F_ARB;  // 高品質な32F_ARBを復活
        settings.useDepth = true;  // 深度バッファを有効化
        settings.useStencil = true;  // ステンシルバッファも有効化
        
        glitchFbo.allocate(settings);
        areaMaskFbo.allocate(width, height, GL_RGBA32F_ARB);  // 高品質化
        
        // ofxPostGlitch設定
        postGlitch.setup(&glitchFbo);
        
        isInitialized = true;
    }
    
    void triggerGlitch(int numAreas = 1) {
        if (!isInitialized) return;
        
        // 軽量モード：非常に厳格な制限
        int maxTotalAreas = lightweightMode ? 1 : 2; // 軽量モードでは1個のみ
        int currentAreas = areas.size();
        
        if (currentAreas >= maxTotalAreas) {
            // 既に最大数に達している場合は何もしない
            cout << "GlitchArea limit reached: " << currentAreas << "/" << maxTotalAreas << endl;
            return;
        }
        
        // 追加可能な数を計算（軽量モードでは常に1個）
        int actualNumAreas = lightweightMode ? 1 : ofClamp(numAreas, 1, maxTotalAreas - currentAreas);
        
        for (int i = 0; i < actualNumAreas; i++) {
            float x = ofRandom(width * 0.2, width * 0.8);
            float y = ofRandom(height * 0.2, height * 0.8);
            
            // 軽量モード：小さめのエリア、短い持続時間
            float w, h, lifetime;
            int glitchType;
            GlitchAreaShape shape;
            MovementPattern movement;
            
            if (lightweightMode) {
                w = ofRandom(150, 350); // 元のサイズを維持
                h = ofRandom(150, 350); // 元のサイズを維持
                lifetime = ofRandom(8.0, 15.0); // 元の持続時間を維持
                glitchType = (int)ofRandom(NUM_GLITCH_TYPES); // 全10種類を使用
                shape = CIRCLE; // 単純な円のみ（要望通り）
                
                // サーチライト風の動きを重み付けして選択（移動を復活）
                if (ofRandom(1.0) < 0.7) {
                    movement = SPOTLIGHT_SCAN;
                } else if (ofRandom(1.0) < 0.2) {
                    movement = RANDOM_WALK;
                } else if (ofRandom(1.0) < 0.1) {
                    movement = LINEAR_SWEEP;
                } else {
                    movement = STATIC;
                }
            } else {
                w = ofRandom(150, 350);
                h = ofRandom(150, 350);
                lifetime = ofRandom(8.0, 15.0);
                glitchType = (int)ofRandom(NUM_GLITCH_TYPES);
                shape = CIRCLE; // 全モードで円形に統一
                
                // サーチライト風の動きを重み付けして選択
                if (ofRandom(1.0) < 0.7) {
                    movement = SPOTLIGHT_SCAN;
                } else if (ofRandom(1.0) < 0.2) {
                    movement = RANDOM_WALK;
                } else if (ofRandom(1.0) < 0.1) {
                    movement = LINEAR_SWEEP;
                } else {
                    movement = STATIC;
                }
            }
            
            areas.emplace_back(x, y, w, h, lifetime, glitchType, shape, movement);
        }
    }
    
    void update(float dt) {
        if (!isInitialized) return;
        
        // エリアの更新
        areas.erase(
            std::remove_if(areas.begin(), areas.end(), 
                [](const GlitchArea& area) { return area.isDead(); }),
            areas.end()
        );
        
        for (auto& area : areas) {
            area.update(dt, width, height);
        }
    }
    
    void applyGlitch(ofFbo& inputFbo, ofFbo& outputFbo) {
        if (!isInitialized || areas.empty()) {
            // グリッチエリアがない場合は入力をそのまま出力にコピー
            outputFbo.begin();
            ofClear(0, 0, 0, 0);
            inputFbo.draw(0, 0);
            outputFbo.end();
            return;
        }
        
        // 安全性チェック: FBOの有効性を確認
        if (!inputFbo.isAllocated() || !outputFbo.isAllocated() || !glitchFbo.isAllocated()) {
            cout << "Warning: FBO not properly allocated" << endl;
            return;
        }
        
        // 最終結果を出力FBOに描画
        outputFbo.begin();
        ofClear(0, 0, 0, 0);
        
        // オリジナル画像を描画
        inputFbo.draw(0, 0);
        
        // 各エリアにグリッチエフェクトを適用
        for (const auto& area : areas) {
            try {
                // 入力をグリッチFBOにコピー
                glitchFbo.begin();
                ofClear(0, 0, 0, 0);
                inputFbo.draw(0, 0);
                glitchFbo.end();
                
                // このエリア用のグリッチエフェクトを設定
                applyGlitchToArea(area);
                
                // グリッチを生成
                postGlitch.generateFx();
                
                // トレイルを描画（現在位置より前に）
                drawTrail(area);
                
                // グリッチエリアを円形マスクで描画
                drawGlitchArea(area);
            } catch (const std::exception& e) {
                cout << "Error in glitch processing: " << e.what() << endl;
                // エラーが発生した場合はそのエリアをスキップ
                continue;
            } catch (...) {
                cout << "Unknown error in glitch processing" << endl;
                continue;
            }
        }
        
        outputFbo.end();
    }
    
    bool hasActiveGlitch() const {
        return !areas.empty();
    }
    
    int getActiveAreaCount() const {
        return areas.size();
    }
    
private:
    void applyGlitchToArea(const GlitchArea& area) {
        // グリッチエフェクトの設定をリセット
        postGlitch.setFx(OFXPOSTGLITCH_CONVERGENCE, false);
        postGlitch.setFx(OFXPOSTGLITCH_GLOW, false);
        postGlitch.setFx(OFXPOSTGLITCH_SHAKER, false);
        postGlitch.setFx(OFXPOSTGLITCH_CUTSLIDER, false);
        postGlitch.setFx(OFXPOSTGLITCH_TWIST, false);
        postGlitch.setFx(OFXPOSTGLITCH_OUTLINE, false);
        postGlitch.setFx(OFXPOSTGLITCH_NOISE, false);
        postGlitch.setFx(OFXPOSTGLITCH_SLITSCAN, false);
        postGlitch.setFx(OFXPOSTGLITCH_SWELL, false);
        postGlitch.setFx(OFXPOSTGLITCH_INVERT, false);
        
        // 選択されたグリッチタイプを適用
        switch (area.glitchType) {
            case CONVERGENCE:
                postGlitch.setFx(OFXPOSTGLITCH_CONVERGENCE, true);
                break;
            case GLOW:
                postGlitch.setFx(OFXPOSTGLITCH_GLOW, true);
                break;
            case SHAKE:
                postGlitch.setFx(OFXPOSTGLITCH_SHAKER, true);
                break;
            case CUT_SLIDER:
                postGlitch.setFx(OFXPOSTGLITCH_CUTSLIDER, true);
                break;
            case TWIST:
                postGlitch.setFx(OFXPOSTGLITCH_TWIST, true);
                break;
            case OUTLINE:
                postGlitch.setFx(OFXPOSTGLITCH_OUTLINE, true);
                break;
            case NOISE:
                postGlitch.setFx(OFXPOSTGLITCH_NOISE, true);
                break;
            case SLITSCAN:
                postGlitch.setFx(OFXPOSTGLITCH_SLITSCAN, true);
                break;
            case SWELL:
                postGlitch.setFx(OFXPOSTGLITCH_SWELL, true);
                break;
            case INVERT:
                postGlitch.setFx(OFXPOSTGLITCH_INVERT, true);
                break;
        }
        
    }
    
    void drawGlitchArea(const GlitchArea& area) {
        ofPushMatrix();
        ofPushStyle();
        
        // 軽量モード：シンプルな円形クリップ描画
        if (lightweightMode) {
            try {
                ofEnableBlendMode(OF_BLENDMODE_ALPHA);
                
                // シンプルな方法：円形の部分だけグリッチFBOを表示
                ofPushMatrix();
                ofTranslate(area.position.x, area.position.y);
                ofRotateDeg(area.rotation);
                
                // クリッピングマスクを設定
                ofPushStyle();
                ofSetCircleResolution(32);
                
                // 円形にクリップしてグリッチを描画
                float radius = area.width / 2;
                
                // グリッチFBOから円形部分を切り取って描画
                ofSetColor(255, 255 * area.getIntensity());
                
                // テクスチャ座標を計算
                float texX = area.position.x - radius;
                float texY = area.position.y - radius;
                float texW = area.width;
                float texH = area.height;
                
                // 円形マスクで描画（簡易版）
                glitchFbo.getTexture().drawSubsection(
                    -radius, -radius, texW, texH,
                    texX, texY, texW, texH
                );
                
                ofPopStyle();
                ofPopMatrix();
                ofDisableBlendMode();
            } catch (...) {
                cout << "Error in lightweight glitch draw" << endl;
            }
            
            ofPopStyle();
            ofPopMatrix();
            return;
        }
        
        // 従来のステンシルバッファモード（重い）
        try {
            // ステンシルバッファを使用して形状マスクを作成
            glEnable(GL_STENCIL_TEST);
            glClearStencil(0);
            glClear(GL_STENCIL_BUFFER_BIT);
        
        // ステンシルバッファに形状を描画
        glStencilFunc(GL_ALWAYS, 1, 0xFF);
        glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
        glStencilMask(0xFF);
        glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
        
        ofFill();
        
        // 回転とポジション設定
        ofPushMatrix();
        ofTranslate(area.position.x, area.position.y);
        ofRotateDeg(area.rotation);
        
        // 形状に応じた描画
        switch (area.shape) {
            case CIRCLE:
                ofDrawCircle(0, 0, area.width / 2);
                break;
                
            case ELLIPSE:
                ofDrawEllipse(0, 0, area.width, area.height);
                break;
                
            case RECTANGLE:
                ofDrawRectangle(-area.width/2, -area.height/2, area.width, area.height);
                break;
                
            case DIAMOND: {
                // ダイヤモンド形状
                ofPath diamond;
                diamond.moveTo(0, -area.height/2);
                diamond.lineTo(area.width/2, 0);
                diamond.lineTo(0, area.height/2);
                diamond.lineTo(-area.width/2, 0);
                diamond.close();
                diamond.draw();
                break;
            }
            
            case TRIANGLE: {
                // 三角形
                ofPath triangle;
                triangle.moveTo(0, -area.height/2);
                triangle.lineTo(area.width/2, area.height/2);
                triangle.lineTo(-area.width/2, area.height/2);
                triangle.close();
                triangle.draw();
                break;
            }
            
            case NUM_SHAPES:
                // デフォルトケース（円を描画）
                ofDrawCircle(0, 0, area.width / 2);
                break;
        }
        
        ofPopMatrix();
        
        // ステンシルテストを設定してグリッチFBOを描画
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glStencilFunc(GL_EQUAL, 1, 0xFF);
        glStencilMask(0x00);
        
        // グリッチFBOを描画（エリア内のみ）
        ofEnableBlendMode(OF_BLENDMODE_ALPHA);
        ofSetColor(255, 255 * area.getIntensity());
        glitchFbo.draw(0, 0);
        
            // ステンシルテストを無効化
            glDisable(GL_STENCIL_TEST);
            
        } catch (const std::exception& e) {
            cout << "Error in drawGlitchArea: " << e.what() << endl;
            // エラー時は必ずステンシルテストを無効化
            glDisable(GL_STENCIL_TEST);
        } catch (...) {
            cout << "Unknown error in drawGlitchArea" << endl;
            glDisable(GL_STENCIL_TEST);
        }
        
        ofDisableBlendMode();
        ofPopStyle();
        ofPopMatrix();
    }
    
    void drawTrail(const GlitchArea& area) {
        if (area.trail.empty()) return;
        
        // 軽量モード：簡易トレイル描画（円形のみ、ステンシルなし）
        if (lightweightMode) {
            try {
                // 最新の5ポイントのみ描画（軽量化）
                int startIdx = area.trail.size() > 5 ? area.trail.size() - 5 : 0;
                
                for (int i = startIdx; i < area.trail.size(); i++) {
                    const TrailPoint& point = area.trail[i];
                    if (point.intensity <= 0) continue;
                    
                    float trailAlpha = point.intensity * area.getIntensity() * 0.2f; // より薄く
                    float trailSize = area.width * (0.3f + point.intensity * 0.7f);
                    
                    // 単純な円で描画
                    ofSetColor(255, 255 * trailAlpha);
                    ofDrawCircle(point.position.x, point.position.y, trailSize / 2);
                }
            } catch (...) {
                cout << "Error in lightweight trail" << endl;
            }
            
            ofDisableBlendMode();
            ofPopStyle();
            return;
        }
        
        ofPushStyle();
        ofEnableBlendMode(OF_BLENDMODE_ALPHA);
        
        // トレイルポイントを古いものから新しいものへ描画
        for (int i = 0; i < area.trail.size(); i++) {
            const TrailPoint& point = area.trail[i];
            
            if (point.intensity <= 0) continue;
            
            // トレイルの透明度とサイズを調整
            float trailAlpha = point.intensity * area.getIntensity() * 0.3f; // 薄めに
            float trailSize = area.width * (0.3f + point.intensity * 0.7f); // サイズもフェード
            
            try {
                // ステンシルバッファを使用してトレイル形状を描画
                glEnable(GL_STENCIL_TEST);
                glClearStencil(0);
                glClear(GL_STENCIL_BUFFER_BIT);
            
            // ステンシルバッファにトレイル形状を描画
            glStencilFunc(GL_ALWAYS, 1, 0xFF);
            glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
            glStencilMask(0xFF);
            glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
            
            ofFill();
            
            // 回転とポジション設定
            ofPushMatrix();
            ofTranslate(point.position.x, point.position.y);
            ofRotateDeg(area.rotation * point.intensity); // 回転もフェード
            
            // 形状に応じたトレイル描画（現在のエリアと同じ形状）
            switch (area.shape) {
                case CIRCLE:
                    ofDrawCircle(0, 0, trailSize / 2);
                    break;
                    
                case ELLIPSE:
                    ofDrawEllipse(0, 0, trailSize, area.height * (0.3f + point.intensity * 0.7f));
                    break;
                    
                case RECTANGLE:
                    ofDrawRectangle(-trailSize/2, -(area.height * (0.3f + point.intensity * 0.7f))/2, 
                                  trailSize, area.height * (0.3f + point.intensity * 0.7f));
                    break;
                    
                case DIAMOND: {
                    float trailHeight = area.height * (0.3f + point.intensity * 0.7f);
                    ofPath diamond;
                    diamond.moveTo(0, -trailHeight/2);
                    diamond.lineTo(trailSize/2, 0);
                    diamond.lineTo(0, trailHeight/2);
                    diamond.lineTo(-trailSize/2, 0);
                    diamond.close();
                    diamond.draw();
                    break;
                }
                
                case TRIANGLE: {
                    float trailHeight = area.height * (0.3f + point.intensity * 0.7f);
                    ofPath triangle;
                    triangle.moveTo(0, -trailHeight/2);
                    triangle.lineTo(trailSize/2, trailHeight/2);
                    triangle.lineTo(-trailSize/2, trailHeight/2);
                    triangle.close();
                    triangle.draw();
                    break;
                }
                
                case NUM_SHAPES:
                    ofDrawCircle(0, 0, trailSize / 2);
                    break;
            }
            
            ofPopMatrix();
            
            // ステンシルテストを設定してグリッチFBOを描画
            glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
            glStencilFunc(GL_EQUAL, 1, 0xFF);
            glStencilMask(0x00);
            
            // グリッチFBOを描画（トレイル部分のみ）
            ofSetColor(255, 255 * trailAlpha);
            glitchFbo.draw(0, 0);
            
                // ステンシルテストを無効化
                glDisable(GL_STENCIL_TEST);
                
            } catch (const std::exception& e) {
                cout << "Error in drawTrail: " << e.what() << endl;
                glDisable(GL_STENCIL_TEST);
            } catch (...) {
                cout << "Unknown error in drawTrail" << endl;
                glDisable(GL_STENCIL_TEST);
            }
        }
        
        ofDisableBlendMode();
        ofPopStyle();
    }
};