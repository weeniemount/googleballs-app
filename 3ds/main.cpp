#include <3ds.h>
#include <citro2d.h>
#include <citro3d.h>
#include <vector>
#include <cmath>
#include <string>
#include <algorithm>

// 3DS screen dimensions
#define TOP_SCREEN_WIDTH  400
#define TOP_SCREEN_HEIGHT 240
#define BOTTOM_SCREEN_WIDTH  320
#define BOTTOM_SCREEN_HEIGHT 240

// Target FPS and frame timing
#define TARGET_FPS 33
#define FRAME_TIME_NS (1000000000LL / TARGET_FPS)

struct Vector3 {
    double x, y, z;
    
    Vector3(double x = 0, double y = 0, double z = 0) : x(x), y(y), z(z) {}
    
    void set(double newX, double newY, double newZ = 0) {
        x = newX; y = newY; z = newZ;
    }
    
    void addX(double dx) { x += dx; }
    void addY(double dy) { y += dy; }
    void addZ(double dz) { z += dz; }
};

struct Color {
    u8 r, g, b, a;
    
    Color(u8 r = 255, u8 g = 255, u8 b = 255, u8 a = 255) 
        : r(r), g(g), b(b), a(a) {}
    
    // Convert hex string to color
    static Color fromHex(const std::string& hex) {
        if (hex[0] == '#') {
            unsigned int value = std::stoul(hex.substr(1), nullptr, 16);
            return Color((value >> 16) & 0xFF, (value >> 8) & 0xFF, value & 0xFF, 255);
        }
        return Color();
    }
    
    u32 to3DS() const {
        return C2D_Color32(r, g, b, a);
    }
};

class Point {
public:
    Vector3 curPos, originalPos, targetPos, velocity;
    Color color;
    double radius, size;
    double friction = 0.8;
    double springStrength = 0.1;
    
    Point(double x, double y, double z, double size, const std::string& colorHex)
        : curPos(x, y, z), originalPos(x, y, z), targetPos(x, y, z),
          velocity(0, 0, 0), size(size), radius(size) {
        color = Color::fromHex(colorHex);
    }
    
    void update() {
        // X axis spring physics
        double dx = targetPos.x - curPos.x;
        double ax = dx * springStrength;
        velocity.x += ax;
        velocity.x *= friction;
        
        // Stop small oscillations
        if (std::abs(dx) < 0.1 && std::abs(velocity.x) < 0.01) {
            curPos.x = targetPos.x;
            velocity.x = 0;
        } else {
            curPos.x += velocity.x;
        }
        
        // Y axis spring physics
        double dy = targetPos.y - curPos.y;
        double ay = dy * springStrength;
        velocity.y += ay;
        velocity.y *= friction;
        
        // Stop small oscillations
        if (std::abs(dy) < 0.1 && std::abs(velocity.y) < 0.01) {
            curPos.y = targetPos.y;
            velocity.y = 0;
        } else {
            curPos.y += velocity.y;
        }
        
        // Z axis (depth) based on distance from original position
        double dox = originalPos.x - curPos.x;
        double doy = originalPos.y - curPos.y;
        double dd = (dox * dox) + (doy * doy);
        double d = std::sqrt(dd);
        
        targetPos.z = d / 100.0 + 1.0;
        double dz = targetPos.z - curPos.z;
        double az = dz * springStrength;
        velocity.z += az;
        velocity.z *= friction;
        
        // Stop small Z oscillations
        if (std::abs(dz) < 0.01 && std::abs(velocity.z) < 0.001) {
            curPos.z = targetPos.z;
            velocity.z = 0;
        } else {
            curPos.z += velocity.z;
        }
        
        // Update radius based on depth
        radius = size * curPos.z;
        if (radius < 1) radius = 1;
    }
    
    void draw() {
        // Draw filled circle using citro2d
        C2D_DrawCircleSolid(curPos.x, curPos.y, 0.0f, radius, color.to3DS());
    }
};

// Original point data (scaled for 3DS top screen)
struct PointData {
    int x, y;
    int size;
    std::string color;
};

static void computeBounds(const std::vector<PointData>& data, double& w, double& h) {
    int minX = 99999, maxX = -99999;
    int minY = 99999, maxY = -99999;
    
    for (const auto& p : data) {
        if (p.x < minX) minX = p.x;
        if (p.x > maxX) maxX = p.x;
        if (p.y < minY) minY = p.y;
        if (p.y > maxY) maxY = p.y;
    }
    
    w = maxX - minX;
    h = maxY - minY;
}

class PointCollection {
public:
    Vector3 mousePos;
    std::vector<Point> points;
    
    PointCollection() : mousePos(0, 0, 0) {}
    
    void addPoint(double x, double y, double z, double size, const std::string& color) {
        points.emplace_back(x, y, z, size, color);
    }
    
    void update() {
        for (auto& point : points) {
            double dx = mousePos.x - point.curPos.x;
            double dy = mousePos.y - point.curPos.y;
            double dd = (dx * dx) + (dy * dy);
            double d = std::sqrt(dd);
            
            if (d < 75) {  // Reduced interaction distance for 3DS screen
                // Fixed: Match JavaScript logic exactly
                point.targetPos.x = point.curPos.x - dx;
                point.targetPos.y = point.curPos.y - dy;
            } else {
                point.targetPos.x = point.originalPos.x;
                point.targetPos.y = point.originalPos.y;
            }
            
            point.update();
        }
    }
    
    void draw() {
        for (auto& point : points) {
            point.draw();
        }
    }
};

class App {
private:
    C3D_RenderTarget* topTarget;
    C3D_RenderTarget* bottomTarget;
    PointCollection pointCollection;
    bool running;
    touchPosition touch;
    bool touching;
    C2D_TextBuf textBuf;
    C2D_Text instructionText;
    C2D_Font font;
    u64 lastFrameTime;
    
public:
    App() : topTarget(nullptr), bottomTarget(nullptr), running(false), touching(false), lastFrameTime(0) {}
    
    bool init() {
        // Initialize services
        romfsInit();
        gfxInitDefault();
        C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
        C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
        C2D_Prepare();
        
        // Create render targets
        topTarget = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
        bottomTarget = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);
        
        if (!topTarget || !bottomTarget) {
            return false;
        }
        
        // Initialize text rendering
        textBuf = C2D_TextBufNew(4096);
        font = C2D_FontLoadSystem(CFG_REGION_USA);
        
        // Create instruction text
        C2D_TextFontParse(&instructionText, font, textBuf, 
                         "Touch the screen or use the Circle Pad\nto interact with the points!\n\nPress START to exit");
        C2D_TextOptimize(&instructionText);
        
        initPoints();
        running = true;
        lastFrameTime = osGetTime();
        return true;
    }
    
    void initPoints() {
        // Scaled down point data for 3DS top screen (400x240)
        std::vector<PointData> pointData = {
            // Scaled coordinates (roughly 0.6x scale factor to fit 3DS screen)
            {121, 47, 5, "#ed9d33"}, {209, 50, 5, "#d44d61"}, {154, 41, 5, "#4f7af2"},
            {128, 35, 5, "#ef9a1e"}, {159, 22, 5, "#4976f3"}, {180, 47, 5, "#269230"},
            {176, 35, 5, "#1f9e2c"}, {27, 53, 5, "#1c48dd"}, {161, 31, 5, "#2a56ea"},
            {44, 50, 5, "#3355d8"}, {176, 4, 5, "#36b641"}, {141, 37, 5, "#2e5def"},
            {212, 25, 4, "#d53747"}, {202, 31, 4, "#eb676f"}, {125, 25, 4, "#f9b125"},
            {193, 42, 4, "#de3646"}, {5, 36, 4, "#2a59f0"}, {108, 49, 4, "#eb9c31"},
            {88, 39, 4, "#c41731"}, {87, 29, 4, "#d82038"}, {148, 20, 4, "#5f8af8"},
            {101, 41, 4, "#efa11e"}, {164, 59, 4, "#2e55e2"}, {149, 72, 4, "#4167e4"},
            {176, 25, 4, "#0b991a"}, {160, 68, 4, "#4869e3"}, {47, 40, 4, "#3059e3"},
            {176, 14, 4, "#10a11d"}, {70, 50, 4, "#cf4055"}, {82, 48, 4, "#cd4359"},
            {8, 43, 4, "#2855ea"}, {199, 48, 4, "#ca273c"}, {15, 49, 4, "#2650e1"},
            {140, 28, 4, "#4a7bf9"}, {44, 8, 4, "#3d65e7"}, {196, 21, 3, "#f47875"},
            {191, 28, 3, "#f36764"}, {154, 49, 3, "#1d4eeb"}, {146, 53, 3, "#698bf1"},
            {116, 19, 3, "#fac652"}, {58, 34, 3, "#ee5257"}, {63, 45, 3, "#cf2a3f"},
            {25, 2, 3, "#5681f5"}, {6, 16, 3, "#4577f6"}, {100, 33, 3, "#f7b326"},
            {160, 53, 3, "#2b58e8"}, {107, 20, 3, "#facb5e"}, {60, 39, 3, "#e02e3d"},
            {206, 19, 3, "#f16d6f"}, {35, 3, 3, "#507bf2"}, {16, 5, 3, "#5683f7"},
            {140, 70, 3, "#3158e2"}, {74, 19, 3, "#f0696c"}, {4, 23, 3, "#3769f6"},
            {38, 37, 3, "#6084ef"}, {4, 29, 3, "#2a5cf4"}, {65, 22, 3, "#f4716e"},
            {101, 26, 3, "#f8c247"}, {82, 22, 3, "#e74653"}, {191, 35, 3, "#ec4147"},
            {136, 60, 3, "#4876f1"}, {61, 28, 3, "#ef5c5c"}, {136, 65, 3, "#2552ea"},
            {10, 10, 3, "#4779f7"}, {139, 56, 3, "#4b78f1"}
        };
        
        double logoW, logoH;
        computeBounds(pointData, logoW, logoH);
        
        double offsetX = (TOP_SCREEN_WIDTH / 2.0) - (logoW / 2.0);
        double offsetY = (TOP_SCREEN_HEIGHT / 2.0) - (logoH / 2.0);
        
        // Center the points on the top screen
        for (const auto& data : pointData) {
            double x = offsetX + data.x;
            double y = offsetY + data.y;
            pointCollection.addPoint(x, y, 0.0, static_cast<double>(data.size), data.color);
        }
    }
    
    void handleInput() {
        hidScanInput();
        
        u32 kDown = hidKeysDown();
        if (kDown & KEY_START) {
            running = false;
        }
        
        // Handle touch input
        if (hidKeysHeld() & KEY_TOUCH) {
            hidTouchRead(&touch);
            touching = true;
            pointCollection.mousePos.set(touch.px, touch.py);
        } else {
            touching = false;
        }
        
        // Handle circle pad input (alternative to touch)
        circlePosition circlePos;
        hidCircleRead(&circlePos);
        if (abs(circlePos.dx) > 20 || abs(circlePos.dy) > 20) {
            static double padX = TOP_SCREEN_WIDTH / 2.0;
            static double padY = TOP_SCREEN_HEIGHT / 2.0;
            
            padX += circlePos.dx / 256.0;
            padY -= circlePos.dy / 256.0;  // Invert Y axis
            
            // Clamp to screen bounds
            padX = std::max(0.0, std::min((double)TOP_SCREEN_WIDTH, padX));
            padY = std::max(0.0, std::min((double)TOP_SCREEN_HEIGHT, padY));
            
            pointCollection.mousePos.set(padX, padY);
        }
    }
    
    void update() {
        pointCollection.update();
    }
    
    void limitFrameRate() {
        u64 currentTime = osGetTime();
        u64 deltaTime = currentTime - lastFrameTime;
        u64 targetTime = FRAME_TIME_NS / 1000000; // Convert to milliseconds
        
        if (deltaTime < targetTime) {
            u64 sleepTime = targetTime - deltaTime;
            svcSleepThread(sleepTime * 1000000); // Convert to nanoseconds
        }
        
        lastFrameTime = osGetTime();
    }
    
    void render() {
        // Render top screen
        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        C2D_TargetClear(topTarget, C2D_Color32f(1.0f, 1.0f, 1.0f, 1.0f));  // White background
        C2D_SceneBegin(topTarget);
        
        pointCollection.draw();
        
        // Always draw cursor indicator when interacting (touch or circle pad)
        bool isInteracting = touching || (abs(hidKeysHeld() & KEY_CPAD_UP) || abs(hidKeysHeld() & KEY_CPAD_DOWN) || 
                                         abs(hidKeysHeld() & KEY_CPAD_LEFT) || abs(hidKeysHeld() & KEY_CPAD_RIGHT));
        
        // Draw pointer with pulsing effect when dragging
        if (touching) {
            // Pulsing red pointer when dragging with touch
            static u32 pulseCounter = 0;
            pulseCounter++;
            float pulseIntensity = 0.5f + 0.5f * sinf(pulseCounter * 0.3f);
            u8 red = 255;
            u8 green = (u8)(100 * pulseIntensity);
            u8 blue = (u8)(100 * pulseIntensity);
            
            C2D_DrawCircleSolid(pointCollection.mousePos.x, pointCollection.mousePos.y, 0.0f, 4.0f, 
                               C2D_Color32(red, green, blue, 200));
            // Outer ring
            C2D_DrawCircleSolid(pointCollection.mousePos.x, pointCollection.mousePos.y, 0.0f, 6.0f, 
                               C2D_Color32(red, green, blue, 100));
        } else {
            // Gray pointer when using circle pad or idle
            C2D_DrawCircleSolid(pointCollection.mousePos.x, pointCollection.mousePos.y, 0.0f, 3.0f, 
                               C2D_Color32(100, 100, 100, 128));
        }
        
        // Render bottom screen (instructions)
        C2D_TargetClear(bottomTarget, C2D_Color32f(0.9f, 0.9f, 0.9f, 1.0f));  // Light gray background
        C2D_SceneBegin(bottomTarget);
        
        // Draw instruction text centered on bottom screen
        float textWidth, textHeight;
        C2D_TextGetDimensions(&instructionText, 0.6f, 0.6f, &textWidth, &textHeight);
        
        float textX = (BOTTOM_SCREEN_WIDTH - textWidth) / 2.0f;
        float textY = (BOTTOM_SCREEN_HEIGHT - textHeight) / 2.0f;
        
        C2D_DrawText(&instructionText, C2D_WithColor, textX, textY, 0.0f, 0.6f, 0.6f, 
                     C2D_Color32(50, 50, 50, 255));
        
        C3D_FrameEnd(0);
    }
    
    void run() {
        while (running && aptMainLoop()) {
            handleInput();
            update();
            render();
            limitFrameRate(); // Lock to 33 FPS
        }
    }
    
    void cleanup() {
        C2D_TextBufDelete(textBuf);
        C2D_Fini();
        C3D_Fini();
        gfxExit();
        romfsExit();
    }
};

int main(int argc, char* argv[]) {
    App app;
    
    if (!app.init()) {
        return -1;
    }
    
    app.run();
    app.cleanup();
    
    return 0;
}