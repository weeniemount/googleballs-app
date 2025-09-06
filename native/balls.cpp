#include <nds.h>
#include <stdio.h>
#include <math.h>
#include <vector>
#include <algorithm>

// Color structure for DS
struct Color {
    u16 rgb15;  // 15-bit RGB color for DS
    
    Color(u8 r = 255, u8 g = 255, u8 b = 255) {
        // Convert 8-bit RGB to 15-bit RGB (5 bits per component)
        u8 r5 = r >> 3;
        u8 g5 = g >> 3;
        u8 b5 = b >> 3;
        rgb15 = RGB15(r5, g5, b5);
    }
    
    // Convert hex string to color (simplified for DS)
    static Color fromHex(const char* hex) {
        u32 value = 0;
        if (hex[0] == '#') {
            // Simple hex parsing
            for (int i = 1; hex[i] != '\0' && i <= 6; i++) {
                value = value * 16;
                if (hex[i] >= '0' && hex[i] <= '9') {
                    value += hex[i] - '0';
                } else if (hex[i] >= 'a' && hex[i] <= 'f') {
                    value += hex[i] - 'a' + 10;
                } else if (hex[i] >= 'A' && hex[i] <= 'F') {
                    value += hex[i] - 'A' + 10;
                }
            }
        }
        u8 r = (value >> 16) & 0xFF;
        u8 g = (value >> 8) & 0xFF;
        u8 b = value & 0xFF;
        return Color(r, g, b);
    }
};

// Vector structure
struct Vector3 {
    float x, y, z;
    
    Vector3(float x = 0, float y = 0, float z = 0) : x(x), y(y), z(z) {}
    
    void set(float newX, float newY, float newZ = 0) {
        x = newX; y = newY; z = newZ;
    }
};

// Point data structure
struct PointData {
    int x, y;
    int size;
    const char* color;
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
          velocity(0, 0, 0), size(size), radius(size), color(Color::fromHex(colorHex)) {
    }
    
    void update() {
        // X axis spring physics
        float dx = targetPos.x - curPos.x;
        float ax = dx * springStrength;
        velocity.x += ax;
        velocity.x *= friction;
        
        // Stop small oscillations
        if (fabsf(dx) < 0.1f && fabsf(velocity.x) < 0.01f) {
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
        
        // Stop small oscillations
        if (fabsf(dy) < 0.1f && fabsf(velocity.y) < 0.01f) {
            curPos.y = targetPos.y;
            velocity.y = 0;
        } else {
            curPos.y += velocity.y;
        }
        
        // Z axis (depth) based on distance from original position
        float dox = originalPos.x - curPos.x;
        float doy = originalPos.y - curPos.y;
        float dd = (dox * dox) + (doy * doy);
        float d = sqrtf(dd);
        
        targetPos.z = d / 100.0f + 1.0f;
        float dz = targetPos.z - curPos.z;
        float az = dz * springStrength;
        velocity.z += az;
        velocity.z *= friction;
        
        // Stop small Z oscillations
        if (fabsf(dz) < 0.01f && fabsf(velocity.z) < 0.001f) {
            curPos.z = targetPos.z;
            velocity.z = 0;
        } else {
            curPos.z += velocity.z;
        }
        
        // Update radius based on depth
        radius = size * curPos.z;
        if (radius < 1) radius = 1;
    }
    
    void drawPixel(u16* framebuffer, int screenWidth, int screenHeight, int px, int py) {
        if (px >= 0 && px < screenWidth && py >= 0 && py < screenHeight) {
            framebuffer[py * screenWidth + px] = color.rgb15;
        }
    }
    
    void draw(u16* framebuffer, int screenWidth, int screenHeight) {
        int x0 = (int)curPos.x;
        int y0 = (int)curPos.y;
        int r = (int)radius;
        
        // Simple filled circle using midpoint circle algorithm
        for (int x = -r; x <= r; x++) {
            for (int y = -r; y <= r; y++) {
                float dist = sqrtf((float)(x * x + y * y));
                if (dist <= radius) {
                    drawPixel(framebuffer, screenWidth, screenHeight, x0 + x, y0 + y);
                }
            }
        }
    }
};

class PointCollection {
public:
    Vector3 touchPos;
    std::vector<Point> points;
    
    PointCollection() : touchPos(0, 0, 0) {}
    
    void addPoint(float x, float y, float z, float size, const char* color) {
        points.emplace_back(x, y, z, size, color);
    }
    
    void update() {
        for (auto& point : points) {
            float dx = touchPos.x - point.curPos.x;
            float dy = touchPos.y - point.curPos.y;
            float dd = (dx * dx) + (dy * dy);
            float d = sqrtf(dd);
            
            if (d < 75) {  // Reduced interaction radius for DS screen
                // Match original logic
                point.targetPos.x = point.curPos.x - dx;
                point.targetPos.y = point.curPos.y - dy;
            } else {
                point.targetPos.x = point.originalPos.x;
                point.targetPos.y = point.originalPos.y;
            }
            
            point.update();
        }
    }
    
    void draw(u16* framebuffer, int screenWidth, int screenHeight) {
        for (auto& point : points) {
            point.draw(framebuffer, screenWidth, screenHeight);
        }
    }
};

// Global variables
PointCollection pointCollection;

// Original point data (scaled down for DS resolution)
static const PointData pointData[] = {
    {101, 39, 4, "#ed9d33"}, {174, 41, 4, "#d44d61"}, {128, 34, 4, "#4f7af2"},
    {107, 29, 4, "#ef9a1e"}, {132, 18, 4, "#4976f3"}, {150, 39, 4, "#269230"},
    {147, 29, 4, "#1f9e2c"}, {22, 44, 4, "#1c48dd"}, {134, 26, 4, "#2a56ea"},
    {36, 41, 4, "#3355d8"}, {147, 3, 4, "#36b641"}, {117, 31, 4, "#2e5def"},
    {176, 21, 4, "#d53747"}, {168, 26, 4, "#eb676f"}, {104, 20, 4, "#f9b125"},
    {160, 35, 4, "#de3646"}, {4, 30, 4, "#2a59f0"}, {90, 40, 4, "#eb9c31"},
    {73, 32, 4, "#c41731"}, {72, 24, 4, "#d82038"}, {123, 17, 4, "#5f8af8"},
    {84, 34, 4, "#efa11e"}, {136, 49, 4, "#2e55e2"}, {124, 60, 4, "#4167e4"},
    {147, 20, 4, "#0b991a"}, {133, 57, 4, "#4869e3"}, {39, 33, 4, "#3059e3"},
    {147, 11, 4, "#10a11d"}, {58, 41, 4, "#cf4055"}, {68, 40, 4, "#cd4359"},
    {7, 35, 4, "#2855ea"}, {165, 40, 4, "#ca273c"}, {12, 41, 4, "#2650e1"},
    {116, 23, 4, "#4a7bf9"}, {36, 6, 4, "#3d65e7"}, {163, 17, 3, "#f47875"},
    {159, 23, 3, "#f36764"}, {128, 40, 3, "#1d4eeb"}, {122, 44, 3, "#698bf1"},
    {97, 16, 3, "#fac652"}, {48, 28, 3, "#ee5257"}, {52, 37, 3, "#cf2a3f"},
    {21, 2, 3, "#5681f5"}, {5, 13, 3, "#4577f6"}, {83, 27, 3, "#f7b326"},
    {133, 44, 3, "#2b58e8"}, {89, 17, 3, "#facb5e"}, {50, 32, 3, "#e02e3d"},
    {171, 16, 3, "#f16d6f"}, {29, 2, 3, "#507bf2"}, {13, 4, 3, "#5683f7"},
    {116, 58, 3, "#3158e2"}, {61, 16, 3, "#f0696c"}, {3, 19, 3, "#3769f6"},
    {31, 31, 3, "#6084ef"}, {3, 24, 3, "#2a5cf4"}, {54, 18, 3, "#f4716e"},
    {84, 21, 3, "#f8c247"}, {68, 18, 3, "#e74653"}, {159, 29, 3, "#ec4147"},
    {113, 50, 2, "#4876f1"}, {50, 23, 2, "#ef5c5c"}, {113, 54, 2, "#2552ea"},
    {8, 8, 2, "#4779f7"}, {116, 46, 2, "#4b78f1"}
};

static const int NUM_POINTS = sizeof(pointData) / sizeof(pointData[0]);

void initPoints() {
    // Center the logo on the main screen (256x192)
    const int screenWidth = 256;
    const int screenHeight = 192;
    
    // Find bounds of original data
    int minX = 999, maxX = -999, minY = 999, maxY = -999;
    for (int i = 0; i < NUM_POINTS; i++) {
        if (pointData[i].x < minX) minX = pointData[i].x;
        if (pointData[i].x > maxX) maxX = pointData[i].x;
        if (pointData[i].y < minY) minY = pointData[i].y;
        if (pointData[i].y > maxY) maxY = pointData[i].y;
    }
    
    int logoW = maxX - minX;
    int logoH = maxY - minY;
    
    int offsetX = (screenWidth / 2) - (logoW / 2) - minX;
    int offsetY = (screenHeight / 2) - (logoH / 2) - minY;
    
    // Add points with centering offset
    for (int i = 0; i < NUM_POINTS; i++) {
        float x = offsetX + pointData[i].x;
        float y = offsetY + pointData[i].y;
        pointCollection.addPoint(x, y, 0.0f, (float)pointData[i].size, pointData[i].color);
    }
}

void handleInput() {
    scanKeys();
    touchPosition touch;
    
    if (keysHeld() & KEY_TOUCH) {
        touchRead(&touch);
        pointCollection.touchPos.set((float)touch.px, (float)touch.py);
    }
    
    // Handle D-pad for moving interaction point (for testing without stylus)
    if (keysHeld() & KEY_LEFT) pointCollection.touchPos.x -= 2;
    if (keysHeld() & KEY_RIGHT) pointCollection.touchPos.x += 2;
    if (keysHeld() & KEY_UP) pointCollection.touchPos.y -= 2;
    if (keysHeld() & KEY_DOWN) pointCollection.touchPos.y += 2;
}

int main() {
    // Initialize DS systems
    powerOn(POWER_ALL);
    
    // Set up main screen for 16-bit bitmap mode
    videoSetMode(MODE_5_2D);
    videoSetModeSub(MODE_0_2D);
    
    vramSetBankA(VRAM_A_MAIN_BG);
    vramSetBankC(VRAM_C_SUB_BG);
    
    // Initialize main screen background
    int bg = bgInit(3, BgType_Bmp16, BgSize_B16_256x256, 0, 0);
    
    // Initialize sub screen for text
    consoleInit(NULL, 1, BgType_Text4bpp, BgSize_T_256x256, 31, 0, false, true);
    
    // Get framebuffer pointer
    u16* framebuffer = bgGetGfxPtr(bg);
    
    // Initialize points
    initPoints();
    
    // Print instructions on bottom screen
    iprintf("\x1b[0;0HGoogle Balls DS\n");
    iprintf("Touch screen to interact\n");
    iprintf("Use D-pad to move cursor\n");
    iprintf("Press START to exit\n");
    
    // Main game loop
    while (1) {
        // Handle input
        handleInput();
        
        // Exit if START is pressed
        if (keysDown() & KEY_START) break;
        
        // Clear framebuffer (white background)
        for (int i = 0; i < 256 * 256; i++) {
            framebuffer[i] = RGB15(31, 31, 31); // White
        }
        
        // Update and draw points
        pointCollection.update();
        pointCollection.draw(framebuffer, 256, 192);
        
        // Wait for VBlank
        swiWaitForVBlank();
    }
    
    return 0;
}