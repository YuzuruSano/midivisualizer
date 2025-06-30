#pragma once

#include "VisualSystem.h"

class FractalSystem : public VisualSystem {
private:
    ofFbo fractalBuffer;
    ofFbo complexityBuffer; // 複雑性の蓄積用
    
    float zoom = 1.0f;
    ofVec2f center;
    int iterations = 128;
    float colorOffset = 0.0f;
    
    // ジュリア集合のパラメータ
    float juliaReal = -0.7f;
    float juliaImag = 0.27015f;
    
    // 都市的フラクタル
    std::vector<ofVec2f> fractalSeeds; // フラクタル成長の種子
    float urbanComplexity = 0.0f;      // 都市の複雑性
    float structuralDensity = 0.0f;    // 構造密度
    
    // マルチスケールフラクタル
    std::vector<float> scaleFactors;
    std::vector<float> scaleIntensities;
    
    // シェーダー
    ofShader fractalShader;
    ofShader urbanFractalShader;
    
    // CPU版フラクタル
    struct FractalSegment {
        ofVec2f start;
        ofVec2f end;
        float generation;
        float intensity;
        ofColor color;
        bool isUrbanStructure;
    };
    std::vector<FractalSegment> fractalSegments;
    static const int MAX_SEGMENTS = 300;  // セグメント数の上限
    
public:
    void setup() override {
        int width = ofGetWidth();
        int height = ofGetHeight();
        
        fractalBuffer.allocate(width, height, GL_RGBA32F);
        complexityBuffer.allocate(width, height, GL_RGBA32F);
        
        center = ofVec2f(0, 0);
        
        // フラクタル種子の初期化（画面全体に配置）
        for (int i = 0; i < 12; i++) {
            fractalSeeds.push_back(ofVec2f(
                ofMap(i % 4, 0, 3, -0.8f, 0.8f),
                ofMap(i / 4, 0, 2, -0.6f, 0.6f)
            ));
        }
        
        // マルチスケール設定
        for (int i = 0; i < 6; i++) {
            scaleFactors.push_back(pow(2.0f, i));
            scaleIntensities.push_back(1.0f / pow(2.0f, i));
        }
        
        // シェーダーの作成
        if (ofIsGLProgrammableRenderer()) {
            createFractalShaders();
        }
        
        // 初期フラクタルセグメント
        generateInitialSegments();
    }
    
    void update(float deltaTime) override {
        // 統一エフェクトシステムの更新
        updateGlobalEffects(deltaTime);
        
        // 都市複雑性の進化
        urbanComplexity += deltaTime * 0.02f * (1.0f + globalGrowthLevel);
        structuralDensity = urbanComplexity * globalGrowthLevel;
        
        // ズームレベルの更新（成長に応じて拡大）
        float targetZoom = 0.5f + globalGrowthLevel * 2.0f + impactIntensity * 3.0f;
        zoom = ofLerp(zoom, targetZoom, deltaTime * 3.0f);
        
        // 色のオフセットを成長とインパクトで変化
        colorOffset += deltaTime * 30.0f * (1.0f + modulation * 2.0f + globalGrowthLevel);
        
        // ジュリア集合のパラメータを動的変化
        float time = systemTime;
        juliaReal = -0.7f + sin(time * 0.1f + globalGrowthLevel * PI) * 0.4f * (1.0f + modulation);
        juliaImag = 0.27015f + cos(time * 0.07f + globalGrowthLevel * PI) * 0.4f * (1.0f + modulation);
        
        // 反復数を成長レベルで増加（軽量化）
        iterations = 32 + globalGrowthLevel * 64 + impactIntensity * 32;
        iterations = ofClamp(iterations, 16, 128);
        
        // セグメントクリーンアップ（メモリ制御）
        if (fractalSegments.size() > MAX_SEGMENTS * 0.8) {
            // 古いセグメントを削除
            fractalSegments.erase(fractalSegments.begin(), fractalSegments.begin() + fractalSegments.size() / 4);
        }
        
        // フラクタルセグメントの更新
        updateFractalSegments(deltaTime);
        
        // 成長に応じた中心移動
        if (globalGrowthLevel > 0.7f) {
            center += ofVec2f(
                sin(time * 0.05f) * globalGrowthLevel * 0.1f,
                cos(time * 0.03f) * globalGrowthLevel * 0.1f
            );
        }
        
        // 崩壊時の効果
        if (isCollapsing) {
            // フラクタルの分裂
            if (ofRandom(1.0f) < 0.1f) {
                fragmentFractal();
            }
        }
    }
    
    void draw() override {
        // マスターバッファに描画開始
        beginMasterBuffer();
        
        // フラクタルの描画
        if (fractalShader.isLoaded() && urbanFractalShader.isLoaded()) {
            drawGPUFractals();
        } else {
            drawCPUFractals();
        }
        
        // 複雑性の蓄積
        updateComplexityBuffer();
        
        // 都市的フラクタル構造
        drawUrbanFractalStructures();
        
        endMasterBuffer();
        
        // 全画面エフェクトの描画
        drawFullscreenEffects();
        
        // フラクタル情報の表示
        drawFractalInfo();
    }
    
    void onMidiMessage(ofxMidiMessage& msg) override {
        if (msg.status == MIDI_NOTE_ON && msg.velocity > 0) {
            currentNote = msg.pitch;
            currentVelocity = msg.velocity;
            
            triggerImpact(msg.pitch, msg.velocity);
            
            // ドラムタイプに応じたフラクタル効果
            switch(msg.pitch) {
                case KICK:
                    // 中心から放射状フラクタル
                    triggerRadialFractal(ofVec2f(0, 0), impactIntensity * 2.0f);
                    // ズームイン効果
                    zoom += impactIntensity * 1.5f;
                    break;
                    
                case SNARE:
                    // 複数点からの複合フラクタル
                    for (int i = 0; i < 4; i++) {
                        float angle = i * PI / 2;
                        ofVec2f pos(cos(angle) * 0.5f, sin(angle) * 0.5f);
                        triggerRadialFractal(pos, impactIntensity * 1.2f);
                    }
                    break;
                    
                case HIHAT_CLOSED:
                    // 細かいフラクタル詳細の追加
                    addFractalDetail(impactIntensity);
                    break;
                    
                case CRASH:
                    // 大規模フラクタル爆発
                    triggerFractalExplosion();
                    // パラメータの急激な変化
                    juliaReal += ofRandom(-0.3f, 0.3f);
                    juliaImag += ofRandom(-0.3f, 0.3f);
                    break;
                    
                default:
                    // 音程に基づくフラクタル生成
                    ofVec2f notePos(
                        ofMap(msg.pitch % 12, 0, 12, -1.0f, 1.0f),
                        ofMap(msg.pitch / 12, 0, 10, -0.8f, 0.8f)
                    );
                    triggerRadialFractal(notePos, impactIntensity);
            }
            
            // 都市複雑性の促進
            urbanComplexity += impactIntensity * 0.2f;
            
        } else if (msg.status == MIDI_CONTROL_CHANGE) {
            if (msg.control == 1) { // Mod wheel
                modulation = mapCC(msg.value);
                // モジュレーションでフラクタルパラメータ変化
                float modEffect = modulation * 0.5f;
                juliaReal = ofClamp(juliaReal + ofRandom(-modEffect, modEffect), -1.5f, 0.5f);
                juliaImag = ofClamp(juliaImag + ofRandom(-modEffect, modEffect), -1.0f, 1.0f);
            }
        }
    }
    
private:
    void createFractalShaders() {
        // 基本フラクタルシェーダー
        string vertexShader = R"(
            #version 150
            uniform mat4 modelViewProjectionMatrix;
            in vec4 position;
            in vec2 texcoord;
            out vec2 texCoordVarying;
            
            void main() {
                texCoordVarying = texcoord;
                gl_Position = modelViewProjectionMatrix * position;
            }
        )";
        
        string fractalFragmentShader = R"(
            #version 150
            uniform vec2 resolution;
            uniform vec2 center;
            uniform float zoom;
            uniform int iterations;
            uniform float colorOffset;
            uniform vec2 julia;
            uniform float intensity;
            uniform int noteValue;
            uniform float globalGrowth;
            uniform float urbanComplexity;
            
            in vec2 texCoordVarying;
            out vec4 outputColor;
            
            vec3 hsv2rgb(vec3 c) {
                vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
                vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
                return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
            }
            
            float mandelbrot(vec2 c) {
                vec2 z = vec2(0.0);
                float iter = 0.0;
                
                for (int i = 0; i < iterations; i++) {
                    if (dot(z, z) > 4.0) break;
                    z = vec2(z.x * z.x - z.y * z.y, 2.0 * z.x * z.y) + c;
                    iter++;
                }
                
                if (iter < float(iterations)) {
                    iter -= log2(log2(dot(z, z))) - 2.0;
                }
                
                return iter / float(iterations);
            }
            
            float julia(vec2 z) {
                float iter = 0.0;
                
                for (int i = 0; i < iterations; i++) {
                    if (dot(z, z) > 4.0) break;
                    z = vec2(z.x * z.x - z.y * z.y, 2.0 * z.x * z.y) + julia;
                    iter++;
                }
                
                if (iter < float(iterations)) {
                    iter -= log2(log2(dot(z, z))) - 2.0;
                }
                
                return iter / float(iterations);
            }
            
            void main() {
                vec2 uv = texCoordVarying;
                vec2 c = (uv - 0.5) * 4.0 / zoom + center;
                
                // マルチスケールフラクタル
                float mandel = mandelbrot(c);
                float jul = julia(c);
                
                // 都市的な複合フラクタル
                float combined = mix(mandel, jul, sin(globalGrowth * 3.14159) * 0.5 + 0.5);
                combined += urbanComplexity * 0.1 * sin(c.x * 10.0) * sin(c.y * 10.0);
                
                // 成長による色彩変化
                float hue = fract(combined * 2.0 + colorOffset / 360.0 + float(noteValue) / 127.0);
                hue += globalGrowth * 0.3; // 成長で色相シフト
                
                float saturation = 0.6 + 0.4 * intensity + globalGrowth * 0.3;
                float value = pow(combined, 0.3 + globalGrowth * 0.4) * (0.4 + intensity * 0.6);
                
                // 都市的な色調補正
                if (combined < 0.1) {
                    // 深い部分は都市の暗闇
                    saturation *= 0.3;
                    value *= 0.2 + globalGrowth * 0.3;
                } else if (combined > 0.8) {
                    // 明るい部分は都市の光
                    saturation += 0.2;
                    value += globalGrowth * 0.4;
                }
                
                vec3 color = hsv2rgb(vec3(hue, saturation, value));
                
                // 成長による明度ブースト
                color *= (0.8 + globalGrowth * 0.6);
                
                outputColor = vec4(color, 1.0);
            }
        )";
        
        fractalShader.setupShaderFromSource(GL_VERTEX_SHADER, vertexShader);
        fractalShader.setupShaderFromSource(GL_FRAGMENT_SHADER, fractalFragmentShader);
        fractalShader.bindDefaults();
        fractalShader.linkProgram();
        
        // 都市フラクタルシェーダーも同様に設定...
        urbanFractalShader = fractalShader; // 簡易版
    }
    
    void generateInitialSegments() {
        // 初期フラクタルセグメントの生成
        ofVec2f center(ofGetWidth() * 0.5f, ofGetHeight() * 0.5f);
        
        for (int i = 0; i < 8; i++) {
            float angle = i * TWO_PI / 8;
            ofVec2f start = center;
            ofVec2f end = center + ofVec2f(cos(angle), sin(angle)) * 100;
            
            FractalSegment segment;
            segment.start = start;
            segment.end = end;
            segment.generation = 0;
            segment.intensity = 1.0f;
            segment.color = urbanColor(i * 15, 0.8f);
            segment.isUrbanStructure = (i % 2 == 0);
            
            fractalSegments.push_back(segment);
        }
    }
    
    void updateFractalSegments(float deltaTime) {
        // フラクタルセグメントの動的成長
        if (globalGrowthLevel > 0.3f && fractalSegments.size() < 2000) {
            generateFractalGeneration();
        }
        
        // セグメントの更新
        for (auto& segment : fractalSegments) {
            // 強度の変化
            segment.intensity *= (0.999f + globalGrowthLevel * 0.001f);
            
            // 色の変化
            if (impactIntensity > 0.5f) {
                segment.color = accentColor(impactIntensity);
            }
        }
    }
    
    void generateFractalGeneration() {
        std::vector<FractalSegment> newSegments;
        
        for (auto& segment : fractalSegments) {
            if (segment.generation < 4 && segment.intensity > 0.1f) {
                // 分岐生成
                ofVec2f direction = segment.end - segment.start;
                ofVec2f perpendicular(-direction.y, direction.x);
                perpendicular.normalize();
                
                // 左分岐
                FractalSegment leftBranch;
                leftBranch.start = segment.end;
                leftBranch.end = segment.end + direction * 0.7f + perpendicular * direction.length() * 0.3f;
                leftBranch.generation = segment.generation + 1;
                leftBranch.intensity = segment.intensity * 0.8f;
                leftBranch.color = segment.color;
                leftBranch.isUrbanStructure = segment.isUrbanStructure;
                
                // 右分岐
                FractalSegment rightBranch = leftBranch;
                rightBranch.end = segment.end + direction * 0.7f - perpendicular * direction.length() * 0.3f;
                
                if (leftBranch.end.x >= 0 && leftBranch.end.x <= ofGetWidth() &&
                    leftBranch.end.y >= 0 && leftBranch.end.y <= ofGetHeight()) {
                    newSegments.push_back(leftBranch);
                }
                
                if (rightBranch.end.x >= 0 && rightBranch.end.x <= ofGetWidth() &&
                    rightBranch.end.y >= 0 && rightBranch.end.y <= ofGetHeight()) {
                    newSegments.push_back(rightBranch);
                }
            }
        }
        
        // 新しいセグメントを追加
        for (auto& newSeg : newSegments) {
            fractalSegments.push_back(newSeg);
        }
    }
    
    void drawGPUFractals() {
        fractalBuffer.begin();
        ofClear(0, 0);
        
        fractalShader.begin();
        fractalShader.setUniform2f("resolution", ofGetWidth(), ofGetHeight());
        fractalShader.setUniform2f("center", center.x, center.y);
        fractalShader.setUniform1f("zoom", zoom);
        fractalShader.setUniform1i("iterations", iterations);
        fractalShader.setUniform1f("colorOffset", colorOffset);
        fractalShader.setUniform2f("julia", juliaReal, juliaImag);
        fractalShader.setUniform1f("intensity", intensity);
        fractalShader.setUniform1i("noteValue", currentNote);
        fractalShader.setUniform1f("globalGrowth", globalGrowthLevel);
        fractalShader.setUniform1f("urbanComplexity", urbanComplexity);
        
        ofDrawRectangle(0, 0, ofGetWidth(), ofGetHeight());
        
        fractalShader.end();
        fractalBuffer.end();
        
        // フラクタルバッファを描画
        ofSetColor(255);
        fractalBuffer.draw(0, 0);
    }
    
    void drawCPUFractals() {
        // CPUベースのフラクタル描画
        drawFractalSegments();
        drawMandelPattern();
        drawUrbanGrid();
    }
    
    void drawFractalSegments() {
        ofEnableBlendMode(OF_BLENDMODE_ADD);
        
        for (auto& segment : fractalSegments) {
            ofColor segColor = segment.color;
            segColor.a = 200 * segment.intensity * (0.7f + globalGrowthLevel * 0.3f);
            ofSetColor(segColor);
            
            float lineWidth = 0.5 + segment.intensity * 1.2 + globalGrowthLevel * 0.8;
            ofSetLineWidth(lineWidth);
            
            if (segment.isUrbanStructure) {
                // 都市構造として描画
                drawUrbanSegment(segment);
            } else {
                ofDrawLine(segment.start, segment.end);
            }
        }
        
        ofDisableBlendMode();
    }
    
    void drawUrbanSegment(const FractalSegment& segment) {
        // 都市的なセグメント描画
        ofVec2f direction = segment.end - segment.start;
        ofVec2f perpendicular(-direction.y, direction.x);
        perpendicular.normalize();
        perpendicular *= 3;
        
        // 建物のような形状
        ofDrawLine(segment.start + perpendicular, segment.end + perpendicular);
        ofDrawLine(segment.start - perpendicular, segment.end - perpendicular);
        ofDrawLine(segment.start + perpendicular, segment.start - perpendicular);
        ofDrawLine(segment.end + perpendicular, segment.end - perpendicular);
        
        // 窓のような詳細
        if (segment.generation < 2 && globalGrowthLevel > 0.4f) {
            int numWindows = 3 + globalGrowthLevel * 5;
            for (int i = 1; i < numWindows; i++) {
                float t = i / float(numWindows);
                ofVec2f windowPos = segment.start + direction * t;
                
                ofSetColor(255, 150);
                ofDrawRectangle(windowPos.x - 1, windowPos.y - 1, 2, 2);
            }
        }
    }
    
    void drawMandelPattern() {
        // CPU版マンデルブロ集合の簡易描画
        ofEnableBlendMode(OF_BLENDMODE_ADD);
        
        int resolution = 8 + globalGrowthLevel * 12; // 成長で解像度向上
        float step = 4.0f / resolution;
        
        for (int x = 0; x < resolution; x++) {
            for (int y = 0; y < resolution; y++) {
                float cx = -2.0f + x * step + center.x;
                float cy = -2.0f + y * step + center.y;
                
                int iter = mandelbrotCPU(cx, cy);
                float normalized = iter / float(iterations);
                
                if (normalized < 1.0f) {
                    ofColor color = urbanColor(currentNote + iter, normalized + globalGrowthLevel);
                    color.a = 150 * normalized * (0.5f + globalGrowthLevel * 0.5f);
                    ofSetColor(color);
                    
                    float screenX = ofMap(x, 0, resolution - 1, 0, ofGetWidth());
                    float screenY = ofMap(y, 0, resolution - 1, 0, ofGetHeight());
                    float size = 2 + globalGrowthLevel * 6 + normalized * 4;
                    
                    ofDrawCircle(screenX, screenY, size);
                }
            }
        }
        
        ofDisableBlendMode();
    }
    
    void drawUrbanGrid() {
        // 都市的なグリッドパターン
        ofEnableBlendMode(OF_BLENDMODE_ADD);
        
        ofColor gridColor = urbanColor(currentNote, urbanComplexity);
        gridColor.a = 100 + globalGrowthLevel * 100;
        ofSetColor(gridColor);
        
        float gridSize = 50 - globalGrowthLevel * 30;
        ofSetLineWidth(0.5 + globalGrowthLevel * 0.8);
        
        // 不規則配置の多角形グリッド
        ofNoFill();
        
        // ポアソンディスク風サンプリングで不規則配置
        int numPolygons = (ofGetWidth() * ofGetHeight()) / (gridSize * gridSize * 1.5); // 密度調整
        
        for (int i = 0; i < numPolygons; i++) {
            float x = ofRandom(0, ofGetWidth());
            float y = ofRandom(0, ofGetHeight());
            
            // ランダムサイズの多角形を生成
            ofBeginShape();
            int numVertices = 3 + (int)(ofRandom(5)); // 3-7角形
            float radius = ofRandom(gridSize * 0.1, gridSize * 0.5);
            
            for (int j = 0; j < numVertices; j++) {
                float angle = (j * TWO_PI / numVertices) + ofRandom(-0.5, 0.5);
                float r = radius + ofRandom(-radius * 0.4, radius * 0.4);
                float vx = x + cos(angle) * r;
                float vy = y + sin(angle) * r;
                ofVertex(vx, vy);
            }
            ofEndShape(true);
        }
        ofFill();
        
        ofDisableBlendMode();
    }
    
    int mandelbrotCPU(float cx, float cy) {
        float zx = 0, zy = 0;
        int iter = 0;
        
        while (zx * zx + zy * zy < 4 && iter < iterations) {
            float temp = zx * zx - zy * zy + cx;
            zy = 2 * zx * zy + cy;
            zx = temp;
            iter++;
        }
        
        return iter;
    }
    
    void updateComplexityBuffer() {
        complexityBuffer.begin();
        
        // 軽い減衰
        ofEnableBlendMode(OF_BLENDMODE_MULTIPLY);
        ofSetColor(253 - globalGrowthLevel * 5);
        ofDrawRectangle(0, 0, ofGetWidth(), ofGetHeight());
        ofDisableBlendMode();
        
        // 新しい複雑性を追加
        ofEnableBlendMode(OF_BLENDMODE_ADD);
        ofSetColor(255, 50 + globalGrowthLevel * 100);
        fractalBuffer.draw(0, 0);
        ofDisableBlendMode();
        
        complexityBuffer.end();
    }
    
    void drawUrbanFractalStructures() {
        // 複雑性バッファの描画
        ofEnableBlendMode(OF_BLENDMODE_ADD);
        ofSetColor(255, 150 + globalGrowthLevel * 80);
        complexityBuffer.draw(0, 0);
        ofDisableBlendMode();
    }
    
    void triggerRadialFractal(ofVec2f pos, float intensity) {
        // 放射状フラクタルの生成
        int numRays = 3 + intensity * 4;  // レイ数を大幅削減
        
        ofVec2f screenPos(
            ofMap(pos.x, -1, 1, 0, ofGetWidth()),
            ofMap(pos.y, -1, 1, 0, ofGetHeight())
        );
        
        for (int i = 0; i < numRays; i++) {
            float angle = i * TWO_PI / numRays;
            float length = 50 + intensity * 100;
            
            FractalSegment segment;
            segment.start = screenPos;
            segment.end = screenPos + ofVec2f(cos(angle), sin(angle)) * length;
            segment.generation = 0;
            segment.intensity = intensity;
            segment.color = accentColor(intensity);
            segment.isUrbanStructure = (i % 3 == 0);
            
            if (fractalSegments.size() < MAX_SEGMENTS) {
                fractalSegments.push_back(segment);
            }
        }
    }
    
    void addFractalDetail(float intensity) {
        // 既存セグメントの細分化
        std::vector<FractalSegment> details;
        
        for (auto& segment : fractalSegments) {
            if (segment.generation < 2 && ofRandom(1.0f) < intensity * 0.2) {  // 生成を大幅抑制
                ofVec2f midpoint = (segment.start + segment.end) * 0.5f;
                ofVec2f direction = segment.end - segment.start;
                ofVec2f perpendicular(-direction.y, direction.x);
                perpendicular.normalize();
                perpendicular *= ofRandom(-20, 20);
                
                FractalSegment detail;
                detail.start = midpoint;
                detail.end = midpoint + perpendicular;
                detail.generation = segment.generation + 1;
                detail.intensity = intensity * 0.7f;
                detail.color = accentColor(intensity);
                detail.isUrbanStructure = false;
                
                details.push_back(detail);
            }
        }
        
        for (auto& detail : details) {
            if (fractalSegments.size() < MAX_SEGMENTS) {
                fractalSegments.push_back(detail);
            }
        }
    }
    
    void triggerFractalExplosion() {
        // 大規模フラクタル爆発
        zoom *= 1.5f;
        urbanComplexity += 0.5f;
        
        // 中心からの大量セグメント生成
        ofVec2f center(ofGetWidth() * 0.5f, ofGetHeight() * 0.5f);
        
        for (int i = 0; i < 24; i++) {
            float angle = i * TWO_PI / 24;
            float length = ofRandom(100, 300);
            
            FractalSegment segment;
            segment.start = center;
            segment.end = center + ofVec2f(cos(angle), sin(angle)) * length;
            segment.generation = 0;
            segment.intensity = 1.0f;
            segment.color = accentColor(1.0f);
            segment.isUrbanStructure = (i % 4 == 0);
            
            fractalSegments.push_back(segment);
        }
    }
    
    void fragmentFractal() {
        // 崩壊時のフラクタル断片化
        for (auto& segment : fractalSegments) {
            if (ofRandom(1.0f) < 0.1f) {
                // セグメントを分割
                ofVec2f midpoint = (segment.start + segment.end) * 0.5f;
                segment.end = midpoint;
                segment.intensity *= 0.5f;
            }
        }
    }
    
    void drawFractalInfo() {
        if (getTimeSinceLastMidi() < 5.0f) {
            ofSetColor(200);
            ofDrawBitmapString("Fractal Segments: " + ofToString(fractalSegments.size()), 20, ofGetHeight() - 100);
            ofDrawBitmapString("Urban Complexity: " + ofToString(urbanComplexity, 2), 20, ofGetHeight() - 80);
            ofDrawBitmapString("Zoom: " + ofToString(zoom, 2), 20, ofGetHeight() - 60);
            ofDrawBitmapString("Iterations: " + ofToString(iterations), 20, ofGetHeight() - 40);
            if (isCollapsing) {
                ofSetColor(255, 100, 100);
                ofDrawBitmapString("FRACTAL FRAGMENTATION", 20, ofGetHeight() - 20);
            }
        }
    }
};