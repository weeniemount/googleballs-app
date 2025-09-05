#include <3ds.h>
#include <citro2d.h>
#include <vector>
#include <cmath>
#include <algorithm>

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
    static Color fromHex(const char* hex) {
        if (hex[0] == '#') {
            unsigned int value = strtoul(hex + 1, nullptr, 16);
            return Color((value >> 16) & 0xFF, (value >> 8) & 0xFF, value & 0xFF, 255);
        }
        return Color();
    }
    
    // Convert to C2D_Color32 format
    u32 toC2D() const {
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
    
    Point(double x, double y, double z, double size, const char* colorHex)
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
        // Use citro2d to draw filled circle
        C2D_DrawCircleSolid((float)curPos.x, (float)curPos.y, 0.0f, 
                           (float)radius, color.toC2D());
    }
};

// Original point data from JavaScript (adjusted for 3DS bottom screen)
struct PointData {
    int x, y;
    int size;
    const char* color;
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
    Vector3 touchPos;
    std::vector<Point> points;
    
    PointCollection() : touchPos(0, 0, 0) {}
    
    void addPoint(double x, double y, double z, double size, const char* color) {
        points.emplace_back(x, y, z, size, color);
    }
    
    void update() {
        for (auto& point : points) {
            double dx = touchPos.x - point.curPos.x;
            double dy = touchPos.y - point.curPos.y;
            double dd = (dx * dx) + (dy * dy);
            double d = std::sqrt(dd);
            
            if (d < 75) {  // Reduced interaction distance for 3DS screen
                // Match JavaScript logic exactly
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
    PointCollection pointCollection;
    bool running;
    C3D_RenderTarget* bottom;
    
public:
    App() : running(false), bottom(nullptr) {}
    
    bool init() {
        // Initialize services
        gfxInitDefault();
        C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
        C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
        C2D_Prepare();
        
        // Create render target for bottom screen
        bottom = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);
        
        initPoints();
        running = true;
        return true;
    }
    
    void initPoints() {
        std::vector<PointData> pointData = {
            {202, 78, 9, "#ed9d33"}, {348, 83, 9, "#d44d61"}, {256, 69, 9, "#4f7af2"},
            {214, 59, 9, "#ef9a1e"}, {265, 36, 9, "#4976f3"}, {300, 78, 9, "#269230"},
            {294, 59, 9, "#1f9e2c"}, {45, 88, 9, "#1c48dd"}, {268, 52, 9, "#2a56ea"},
            {73, 83, 9, "#3355d8"}, {294, 6, 9, "#36b641"}, {235, 62, 9, "#2e5def"},
            {353, 42, 8, "#d53747"}, {336, 52, 8, "#eb676f"}, {208, 41, 8, "#f9b125"},
            {321, 70, 8, "#de3646"}, {8, 60, 8, "#2a59f0"}, {180, 81, 8, "#eb9c31"},
            {146, 65, 8, "#c41731"}, {145, 49, 8, "#d82038"}, {246, 34, 8, "#5f8af8"},
            {169, 69, 8, "#efa11e"}, {273, 99, 8, "#2e55e2"}, {248, 120, 8, "#4167e4"},
            {294, 41, 8, "#0b991a"}, {267, 114, 8, "#4869e3"}, {78, 67, 8, "#3059e3"},
            {294, 23, 8, "#10a11d"}, {117, 83, 8, "#cf4055"}, {137, 80, 8, "#cd4359"},
            {14, 71, 8, "#2855ea"}, {331, 80, 8, "#ca273c"}, {25, 82, 8, "#2650e1"},
            {233, 46, 8, "#4a7bf9"}, {73, 13, 8, "#3d65e7"}, {327, 35, 6, "#f47875"},
            {319, 46, 6, "#f36764"}, {256, 81, 6, "#1d4eeb"}, {244, 88, 6, "#698bf1"},
            {194, 32, 6, "#fac652"}, {97, 56, 6, "#ee5257"}, {105, 75, 6, "#cf2a3f"},
            {42, 4, 6, "#5681f5"}, {10, 27, 6, "#4577f6"}, {166, 55, 6, "#f7b326"},
            {266, 88, 6, "#2b58e8"}, {178, 34, 6, "#facb5e"}, {100, 65, 6, "#e02e3d"},
            {343, 32, 6, "#f16d6f"}, {59, 5, 6, "#507bf2"}, {27, 9, 6, "#5683f7"},
            {233, 116, 6, "#3158e2"}, {123, 32, 6, "#f0696c"}, {6, 38, 6, "#3769f6"},
            {63, 62, 6, "#6084ef"}, {6, 49, 6, "#2a5cf4"}, {108, 36, 6, "#f4716e"},
            {169, 43, 6, "#f8c247"}, {137, 37, 6, "#e74653"}, {318, 58, 6, "#ec4147"},
            {226, 100, 5, "#4876f1"}, {101, 46, 5, "#ef5c5c"}, {226, 108, 5, "#2552ea"},
            {17, 17, 5, "#4779f7"}, {232, 93, 5, "#4b78f1"}
        };
        
        double logoW, logoH;
        computeBounds(pointData, logoW, logoH);
        
        // Center on 3DS bottom screen (320x240)
        double offsetX = (320 / 2.0) - (logoW / 2.0);
        double offsetY = (240 / 2.0) - (logoH / 2.0);
        
        // Scale down to fit better on smaller screen
        double scale = 0.8;
        
        // Center the points
        for (const auto& data : pointData) {
            double x = offsetX + (data.x * scale);
            double y = offsetY + (data.y * scale);
            pointCollection.addPoint(x, y, 0.0, static_cast<double>(data.size) * scale, data.color);
        }
    }
    
    void handleInput() {
        hidScanInput();
        
        // Check for exit
        u32 kDown = hidKeysDown();
        if (kDown & KEY_START) {
            running = false;
        }
        
        // Handle touchscreen input
        touchPosition touch;
        if (hidTouchRead(&touch)) {
            pointCollection.touchPos.set(touch.px, touch.py);
        }
    }
    
    void update() {
        pointCollection.update();
    }
    
    void render() {
        C3D_FrameBegin(C3D_FRAME_SYNCED);
        
        // Render to bottom screen
        C2D_TargetClear(bottom, C2D_Color32(255, 255, 255, 255));  // White background
        C2D_SceneBegin(bottom);
        
        pointCollection.draw();
        
        C3D_FrameEnd(0);
    }
    
    void run() {
        while (running && aptMainLoop()) {
            handleInput();
            update();
            render();
        }
    }
    
    void cleanup() {
        C2D_Fini();
        C3D_Fini();
        gfxExit();
    }
};

int main() {
    App app;
    
    if (!app.init()) {
        return -1;
    }
    
    app.run();
    app.cleanup();
    
    return 0;
}