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

// Maximum number of sprites we can use
#define MAX_SPRITES 64

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
    int spriteId;
    
    Point(double x, double y, double z, double size, const std::string& colorHex, int id)
        : curPos(x, y, z), originalPos(x, y, z), targetPos(x, y, z),
          velocity(0, 0, 0), radius(size), size(size), spriteId(id) {
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
        if (radius < 2) radius = 2;
    }
    
    void updateSprite(OAMTable* oam) {
        if (spriteId >= MAX_SPRITES) return;
        
        // Update sprite position
        int x = static_cast<int>(curPos.x) - 4; // Center the 8x8 sprite
        int y = static_cast<int>(curPos.y) - 4;
        
        // Clamp to screen bounds
        if (x < -8) x = -8;
        if (x > 256) x = 256;
        if (y < -8) y = -8;
        if (y > 192) y = 192;
        
        // Set sprite attributes
        oam->oamBuffer[spriteId].attribute[0] = ATTR0_COLOR_16 | ATTR0_SQUARE | (y & 0xFF);
        oam->oamBuffer[spriteId].attribute[1] = ATTR1_SIZE_8 | (x & 0x1FF);
        oam->oamBuffer[spriteId].attribute[2] = ATTR2_PALETTE(spriteId % 16) | ATTR2_PRIORITY(0);
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
    
    PointCollection() : mousePos(128, 96, 0) {} // Start in center
    
    void addPoint(double x, double y, double z, double size, const std::string& color, int id) {
        points.emplace_back(x, y, z, size, color, id);
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
    
    void updateSprites(OAMTable* oam) {
        for (auto& point : points) {
            point.updateSprite(oam);
        }
    }
};

class App {
private:
    PointCollection pointCollection;
    bool running;
    touchPosition touch;
    bool touching;
    OAMTable* oam;
    u16* spritePalette;
    u8* spriteGfx;
    
public:
    App() : running(false), touching(false), oam(nullptr) {}
    
    bool init() {
        // Initialize video modes
        videoSetMode(MODE_0_2D | DISPLAY_SPR_ACTIVE | DISPLAY_SPR_1D | DISPLAY_BG0_ACTIVE);
        videoSetModeSub(MODE_0_2D);
        
        // Map VRAM banks
        vramSetBankA(VRAM_A_MAIN_SPRITE);
        vramSetBankC(VRAM_C_SUB_BG);
        vramSetBankF(VRAM_F_SPRITE_EXT_PALETTE);
        
        // Initialize sprites
        oam = &oamMain;
        oamInit(oam, SpriteMapping_1D_32, false);
        
        // Get sprite graphics and palette memory
        spriteGfx = (u8*)SPRITE_GFX;
        spritePalette = SPRITE_PALETTE;
        
        // Create simple dot sprite data (8x8 filled circle)
        u8 dotSprite[64] = {
            0,0,1,1,1,1,0,0,
            0,1,1,1,1,1,1,0,
            1,1,1,1,1,1,1,1,
            1,1,1,1,1,1,1,1,
            1,1,1,1,1,1,1,1,
            1,1,1,1,1,1,1,1,
            0,1,1,1,1,1,1,0,
            0,0,1,1,1,1,0,0
        };
        
        // Copy dot sprite to VRAM
        dmaCopy(dotSprite, spriteGfx, 64);
        
        // Initialize console on bottom screen for text
        consoleInit(NULL, 1, BgType_Text4bpp, BgSize_T_256x256, 31, 0, false, true);
        
        // Set up a simple white background
        int bg0 = bgInit(0, BgType_Text4bpp, BgSize_T_256x256, 0, 1);
        BG_PALETTE[0] = RGB15(31, 31, 31); // White background
        BG_PALETTE[1] = RGB15(0, 0, 0);    // Black text (not used here)
        
        initPoints();
        running = true;
        return true;
    }
    
    void initPoints() {
        // Reduced number of points to fit within sprite limit
        std::vector<PointData> pointData = {
            // First 40 most prominent points, scaled for NDS
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
            {78, 13, 2, "#fac652"}
        };
        
        double logoW, logoH;
        computeBounds(pointData, logoW, logoH);
        
        double offsetX = (TOP_SCREEN_WIDTH / 2.0) - (logoW / 2.0);
        double offsetY = (TOP_SCREEN_HEIGHT / 2.0) - (logoH / 2.0);
        
        // Create points and set up sprite palettes
        int pointId = 0;
        for (const auto& data : pointData) {
            if (pointId >= MAX_SPRITES) break;
            
            double x = offsetX + data.x;
            double y = offsetY + data.y;
            pointCollection.addPoint(x, y, 0.0, static_cast<double>(data.size), data.color, pointId);
            
            // Set up palette for this sprite
            Color c = Color::fromHex(data.color);
            spritePalette[pointId * 16] = RGB15(0, 0, 0); // Transparent
            spritePalette[pointId * 16 + 1] = c.toNDS();  // Point color
            
            pointId++;
        }
    }
    
    void handleInput() {
        scanKeys();
        
        u32 keys = keysHeld();
        
        // Handle touch input
        if (keys & KEY_TOUCH) {
            touchRead(&touch);
            if (touch.rawx > 0 && touch.rawy > 0) {
                touching = true;
                // Map touch coordinates from bottom screen to top screen coordinates
                pointCollection.mousePos.set(touch.px, touch.py * 192 / 192); // 1:1 mapping
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
        // Update sprite positions
        pointCollection.updateSprites(oam);
        
        // Update OAM
        oamUpdate(oam);
        
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
        
        int cursorX = static_cast<int>(pointCollection.mousePos.x);
        int cursorY = static_cast<int>(pointCollection.mousePos.y);
        printf("\nCursor: (%d, %d)\n", cursorX, cursorY);
        printf("Points: %zu\n", pointCollection.points.size());
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