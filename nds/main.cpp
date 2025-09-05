#include <nds.h>
#include <stdio.h>
#include <vector>
#include <cmath>
#include <string>
#include <algorithm>

// NDS screen dimensions
#define TOP_SCREEN_WIDTH  256
#define TOP_SCREEN_HEIGHT 192
#define BOTTOM_SCREEN_WIDTH  256
#define BOTTOM_SCREEN_HEIGHT 192

// Target FPS
#define TARGET_FPS 30

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
    u8 r, g, b;
    
    Color(u8 r = 255, u8 g = 255, u8 b = 255) : r(r), g(g), b(b) {}
    
    // Convert hex string to color
    static Color fromHex(const std::string& hex) {
        if (hex[0] == '#') {
            unsigned int value = std::stoul(hex.substr(1), nullptr, 16);
            return Color((value >> 16) & 0xFF, (value >> 8) & 0xFF, value & 0xFF);
        }
        return Color();
    }
    
    // Convert to NDS 15-bit RGB color
    u16 toNDS() const {
        return RGB15(r >> 3, g >> 3, b >> 3);
    }
};

class Point {
public:
    Vector3 curPos, originalPos, targetPos, velocity;
    Color color;
    double radius, size;
    double friction = 0.8;
    double springStrength = 0.1;
    u16 ndsColor;
    
    Point(double x, double y, double z, double size, const std::string& colorHex)
        : curPos(x, y, z), originalPos(x, y, z), targetPos(x, y, z),
          velocity(0, 0, 0), radius(size), size(size) {
        color = Color::fromHex(colorHex);
        ndsColor = color.toNDS();
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
    
    void draw(u16* framebuffer) {
        int centerX = static_cast<int>(curPos.x);
        int centerY = static_cast<int>(curPos.y);
        int r = static_cast<int>(radius);
        
        // Simple filled circle using Bresenham-style algorithm
        for (int y = -r; y <= r; y++) {
            for (int x = -r; x <= r; x++) {
                if (x * x + y * y <= r * r) {
                    int pixelX = centerX + x;
                    int pixelY = centerY + y;
                    
                    // Bounds check
                    if (pixelX >= 0 && pixelX < TOP_SCREEN_WIDTH && 
                        pixelY >= 0 && pixelY < TOP_SCREEN_HEIGHT) {
                        int index = pixelY * TOP_SCREEN_WIDTH + pixelX;
                        framebuffer[index] = ndsColor;
                    }
                }
            }
        }
    }
};

// Point data scaled for NDS top screen (256x192)
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
            
            if (d < 60) {  // Reduced interaction distance for NDS screen
                // Match the original interaction logic
                point.targetPos.x = point.curPos.x - dx;
                point.targetPos.y = point.curPos.y - dy;
            } else {
                point.targetPos.x = point.originalPos.x;
                point.targetPos.y = point.originalPos.y;
            }
            
            point.update();
        }
    }
    
    void draw(u16* framebuffer) {
        for (auto& point : points) {
            point.draw(framebuffer);
        }
    }
};

class App {
private:
    PointCollection pointCollection;
    bool running;
    touchPosition touch;
    bool touching;
    u16* topFramebuffer;
    u16* bottomFramebuffer;
    
public:
    App() : running(false), touching(false), topFramebuffer(nullptr), bottomFramebuffer(nullptr) {}
    
    bool init() {
        // Initialize video modes
        videoSetMode(MODE_FB0);
        videoSetModeSub(MODE_0_2D);
        
        // Map VRAM banks
        vramSetBankA(VRAM_A_LCD);
        vramSetBankC(VRAM_C_SUB_BG);
        
        // Get framebuffer pointers
        topFramebuffer = (u16*)VRAM_A;
        
        // Initialize console on bottom screen for text
        consoleInit(NULL, 1, BgType_Text4bpp, BgSize_T_256x256, 31, 0, false, true);
        
        initPoints();
        running = true;
        return true;
    }
    
    void initPoints() {
        // Scaled down point data for NDS top screen (256x192)
        std::vector<PointData> pointData = {
            // Coordinates scaled by ~0.7 to fit NDS screen
            {87, 32, 4, "#ed9d33"}, {141, 34, 4, "#d44d61"}, {104, 28, 4, "#4f7af2"},
            {87, 24, 4, "#ef9a1e"}, {107, 15, 4, "#4976f3"}, {122, 32, 4, "#269230"},
            {119, 24, 4, "#1f9e2c"}, {18, 36, 4, "#1c48dd"}, {109, 21, 4, "#2a56ea"},
            {30, 34, 4, "#3355d8"}, {119, 3, 4, "#36b641"}, {95, 25, 4, "#2e5def"},
            {143, 17, 3, "#d53747"}, {136, 21, 3, "#eb676f"}, {84, 17, 3, "#f9b125"},
            {130, 28, 3, "#de3646"}, {3, 24, 3, "#2a59f0"}, {73, 33, 3, "#eb9c31"},
            {59, 26, 3, "#c41731"}, {59, 20, 3, "#d82038"}, {100, 14, 3, "#5f8af8"},
            {68, 28, 3, "#efa11e"}, {111, 40, 3, "#2e55e2"}, {100, 49, 3, "#4167e4"},
            {119, 17, 3, "#0b991a"}, {108, 46, 3, "#4869e3"}, {32, 27, 3, "#3059e3"},
            {119, 9, 3, "#10a11d"}, {47, 34, 3, "#cf4055"}, {55, 32, 3, "#cd4359"},
            {6, 29, 3, "#2855ea"}, {134, 32, 3, "#ca273c"}, {10, 33, 3, "#2650e1"},
            {94, 19, 3, "#4a7bf9"}, {30, 5, 3, "#3d65e7"}, {132, 14, 2, "#f47875"},
            {129, 19, 2, "#f36764"}, {104, 33, 2, "#1d4eeb"}, {99, 36, 2, "#698bf1"},
            {78, 13, 2, "#fac652"}, {39, 23, 2, "#ee5257"}, {42, 30, 2, "#cf2a3f"},
            {17, 1, 2, "#5681f5"}, {4, 11, 2, "#4577f6"}, {67, 22, 2, "#f7b326"},
            {108, 36, 2, "#2b58e8"}, {72, 14, 2, "#facb5e"}, {40, 26, 2, "#e02e3d"},
            {139, 13, 2, "#f16d6f"}, {24, 2, 2, "#507bf2"}, {11, 4, 2, "#5683f7"},
            {94, 47, 2, "#3158e2"}, {50, 13, 2, "#f0696c"}, {2, 15, 2, "#3769f6"},
            {25, 25, 2, "#6084ef"}, {2, 20, 2, "#2a5cf4"}, {44, 15, 2, "#f4716e"},
            {68, 18, 2, "#f8c247"}, {55, 15, 2, "#e74653"}, {129, 23, 2, "#ec4147"},
            {92, 41, 2, "#4876f1"}, {41, 19, 2, "#ef5c5c"}, {92, 44, 2, "#2552ea"},
            {7, 7, 2, "#4779f7"}, {94, 38, 2, "#4b78f1"}
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
        scanKeys();
        
        u32 keys = keysHeld();
        
        // Handle touch input
        if (keys & KEY_TOUCH) {
            touchRead(&touch);
            // Map touch coordinates from bottom screen to top screen
            if (touch.rawx > 0 && touch.rawy > 0) {
                touching = true;
                // Convert touch coordinates to top screen space
                pointCollection.mousePos.set(touch.px, touch.py);
            }
        } else {
            touching = false;
        }
        
        // Handle D-pad input as alternative to touch
        static double padX = TOP_SCREEN_WIDTH / 2.0;
        static double padY = TOP_SCREEN_HEIGHT / 2.0;
        
        if (keys & KEY_UP) padY -= 2.0;
        if (keys & KEY_DOWN) padY += 2.0;
        if (keys & KEY_LEFT) padX -= 2.0;
        if (keys & KEY_RIGHT) padX += 2.0;
        
        // Clamp to screen bounds
        padX = std::max(0.0, std::min((double)TOP_SCREEN_WIDTH, padX));
        padY = std::max(0.0, std::min((double)TOP_SCREEN_HEIGHT, padY));
        
        if (keys & (KEY_UP | KEY_DOWN | KEY_LEFT | KEY_RIGHT)) {
            pointCollection.mousePos.set(padX, padY);
        }
        
        // Exit on start button
        if (keys & KEY_START) {
            running = false;
        }
    }
    
    void update() {
        pointCollection.update();
    }
    
    void render() {
        // Clear top screen with white background
        for (int i = 0; i < TOP_SCREEN_WIDTH * TOP_SCREEN_HEIGHT; i++) {
            topFramebuffer[i] = RGB15(31, 31, 31); // White
        }
        
        // Draw points
        pointCollection.draw(topFramebuffer);
        
        // Draw cursor/pointer
        int cursorX = static_cast<int>(pointCollection.mousePos.x);
        int cursorY = static_cast<int>(pointCollection.mousePos.y);
        
        // Draw a small cross as cursor
        u16 cursorColor = touching ? RGB15(31, 0, 0) : RGB15(15, 15, 15); // Red when touching, gray otherwise
        
        for (int i = -3; i <= 3; i++) {
            // Horizontal line
            int x = cursorX + i;
            if (x >= 0 && x < TOP_SCREEN_WIDTH && cursorY >= 0 && cursorY < TOP_SCREEN_HEIGHT) {
                topFramebuffer[cursorY * TOP_SCREEN_WIDTH + x] = cursorColor;
            }
            // Vertical line
            int y = cursorY + i;
            if (cursorX >= 0 && cursorX < TOP_SCREEN_WIDTH && y >= 0 && y < TOP_SCREEN_HEIGHT) {
                topFramebuffer[y * TOP_SCREEN_WIDTH + cursorX] = cursorColor;
            }
        }
        
        // Update bottom screen text
        consoleClear();
        printf("Google Balls - Nintendo DS\n");
        printf("========================\n\n");
        printf("Touch the bottom screen or\n");
        printf("use the D-pad to interact\n");
        printf("with the colorful dots!\n\n");
        printf("Controls:\n");
        printf("Touch: Interactive cursor\n");
        printf("D-pad: Move cursor\n");
        printf("START: Exit\n\n");
        
        if (touching) {
            printf("Status: TOUCHING!\n");
        } else {
            printf("Status: Idle\n");
        }
        
        printf("\nCursor: (%d, %d)\n", cursorX, cursorY);
    }
    
    void run() {
        while (running) {
            handleInput();
            update();
            render();
            swiWaitForVBlank(); // Wait for vertical blank (60 FPS)
        }
    }
    
    void cleanup() {
        // Nothing special needed for cleanup on NDS
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