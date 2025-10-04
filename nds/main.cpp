#include <nds.h>
#include <stdio.h>
#include <math.h>
#include <vector>
#include <string>
#include <algorithm>

struct Vector3 {
    float x, y, z;
    
    Vector3(float x = 0, float y = 0, float z = 0) : x(x), y(y), z(z) {}
    
    void set(float newX, float newY, float newZ = 0) {
        x = newX; y = newY; z = newZ;
    }
};

struct Color {
    u8 r, g, b;
    
    Color(u8 r = 255, u8 g = 255, u8 b = 255) : r(r), g(g), b(b) {}
    
    static Color fromHex(const char* hex) {
        unsigned int value = 0;
        if (hex[0] == '#') hex++;
        sscanf(hex, "%x", &value);
        return Color((value >> 16) & 0xFF, (value >> 8) & 0xFF, value & 0xFF);
    }
    
    u16 toRGB15() const {
        return RGB15(r >> 3, g >> 3, b >> 3);
    }
};

class Point {
public:
    Vector3 curPos, originalPos, targetPos, velocity;
    Color color;
    float radius, size;
    float friction = 0.8f;
    float springStrength = 0.1f;
    
    Point(float x, float y, float z, float size, const char* colorHex)
        : curPos(x, y, z), originalPos(x, y, z), targetPos(x, y, z),
          velocity(0, 0, 0), size(size), radius(size) {
        color = Color::fromHex(colorHex);
    }
    
    void update() {
        // X axis spring physics
        float dx = targetPos.x - curPos.x;
        float ax = dx * springStrength;
        velocity.x += ax;
        velocity.x *= friction;
        
        if (fabs(dx) < 0.1f && fabs(velocity.x) < 0.01f) {
            curPos.x = targetPos.x;
            velocity.x = 0;
        } else {
            curPos.x += velocity.x;
        }
        
        // Y axis spring physics
        float dy = targetPos.y - curPos.y;
        float ay = dy * springStrength;
        velocity.y += ay;
        velocity.y *= friction;
        
        if (fabs(dy) < 0.1f && fabs(velocity.y) < 0.01f) {
            curPos.y = targetPos.y;
            velocity.y = 0;
        } else {
            curPos.y += velocity.y;
        }
        
        // Z axis (depth)
        float dox = originalPos.x - curPos.x;
        float doy = originalPos.y - curPos.y;
        float dd = (dox * dox) + (doy * doy);
        float d = sqrtf(dd);
        
        targetPos.z = d / 100.0f + 1.0f;
        float dz = targetPos.z - curPos.z;
        float az = dz * springStrength;
        velocity.z += az;
        velocity.z *= friction;
        
        if (fabs(dz) < 0.01f && fabs(velocity.z) < 0.001f) {
            curPos.z = targetPos.z;
            velocity.z = 0;
        } else {
            curPos.z += velocity.z;
        }
        
        // Update radius based on depth
        radius = size * curPos.z;
        if (radius < 1) radius = 1;
    }
    
    void draw(u16* vram) {
        int x0 = (int)curPos.x;
        int y0 = (int)curPos.y;
        int r = (int)radius;
        u16 pixelColor = color.toRGB15() | BIT(15);
        
        // Simple filled circle for DS performance
        for (int y = -r; y <= r; y++) {
            for (int x = -r; x <= r; x++) {
                if (x*x + y*y <= r*r) {
                    int px = x0 + x;
                    int py = y0 + y;
                    if (px >= 0 && px < 256 && py >= 0 && py < 192) {
                        vram[py * 256 + px] = pixelColor;
                    }
                }
            }
        }
    }
};

struct PointData {
    int x, y, size;
    const char* color;
};

static void computeBounds(const PointData* data, int count, float& w, float& h) {
    int minX = 99999, maxX = -99999;
    int minY = 99999, maxY = -99999;
    
    for (int i = 0; i < count; i++) {
        if (data[i].x < minX) minX = data[i].x;
        if (data[i].x > maxX) maxX = data[i].x;
        if (data[i].y < minY) minY = data[i].y;
        if (data[i].y > maxY) maxY = data[i].y;
    }
    
    w = maxX - minX;
    h = maxY - minY;
}

class PointCollection {
public:
    Vector3 mousePos;
    std::vector<Point> points;
    
    PointCollection() : mousePos(0, 0, 0) {}
    
    void addPoint(float x, float y, float z, float size, const char* color) {
        points.push_back(Point(x, y, z, size, color));
    }
    
    void update() {
        for (size_t i = 0; i < points.size(); i++) {
            Point& point = points[i];
            float dx = mousePos.x - point.curPos.x;
            float dy = mousePos.y - point.curPos.y;
            float dd = (dx * dx) + (dy * dy);
            float d = sqrtf(dd);
            
            if (d < 150) {
                point.targetPos.x = point.curPos.x - dx;
                point.targetPos.y = point.curPos.y - dy;
            } else {
                point.targetPos.x = point.originalPos.x;
                point.targetPos.y = point.originalPos.y;
            }
            
            point.update();
        }
    }
    
    void draw(u16* vram) {
        for (size_t i = 0; i < points.size(); i++) {
            points[i].draw(vram);
        }
    }
};

extern "C" int main(void) {
    // Initialize video mode for main screen
    videoSetMode(MODE_FB0);
    vramSetBankA(VRAM_A_LCD);
    
    // Initialize touchscreen
    touchPosition touch;
    
    // Get framebuffer
    u16* vram = (u16*)VRAM_A;
    
    PointCollection pointCollection;
    
    // Point data (scaled for DS screen - 256x192)
    static const PointData pointData[] = {
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
    
    int pointCount = sizeof(pointData) / sizeof(PointData);
    
    // Compute bounds and center
    float logoW, logoH;
    computeBounds(pointData, pointCount, logoW, logoH);
    
    float scale = 0.5f; // Scale down for DS screen
    float offsetX = (256 / 2.0f) - (logoW * scale / 2.0f);
    float offsetY = (192 / 2.0f) - (logoH * scale / 2.0f);
    
    // Initialize points
    for (int i = 0; i < pointCount; i++) {
        float x = offsetX + pointData[i].x * scale;
        float y = offsetY + pointData[i].y * scale;
        pointCollection.addPoint(x, y, 0.0f, pointData[i].size * scale * 0.6f, pointData[i].color);
    }
    
    while(1) {
        swiWaitForVBlank();
        
        // Read touchscreen
        scanKeys();
        int keys = keysHeld();
        
        if (keys & KEY_TOUCH) {
            touchRead(&touch);
            pointCollection.mousePos.set(touch.px, touch.py);
        }
        
        // Clear screen (white background)
        for (int i = 0; i < 256 * 192; i++) {
            vram[i] = RGB15(31, 31, 31) | BIT(15);
        }
        
        // Update and draw
        pointCollection.update();
        pointCollection.draw(vram);
    }
    
    return 0;
}