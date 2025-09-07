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
#define MAX_SPRITES 32

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
        if (hex[0] == '#' && hex.length() == 7) {
            unsigned int value = 0;
            // Simple hex parsing without std::stoul
            for (int i = 1; i < 7; i++) {
                char c = hex[i];
                value *= 16;
                if (c >= '0' && c <= '9') value += c - '0';
                else if (c >= 'a' && c <= 'f') value += c - 'a' + 10;
                else if (c >= 'A' && c <= 'F') value += c - 'A' + 10;
            }
            return Color((value >> 16) & 0xFF, (value >> 8) & 0xFF, value & 0xFF);
        }
        return Color(255, 255, 255);
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
    u16* spriteGfx;
    bool visible;
    
    Point(double x, double y, double z, double size, const std::string& colorHex, int id)
        : curPos(x, y, z), originalPos(x, y, z), targetPos(x, y, z),
          velocity(0, 0, 0), radius(size), size(size), spriteId(id), 
          spriteGfx(nullptr), visible(false) {
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
        if (abs(dx) < 0.1 && abs(velocity.x) < 0.01) {
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
        if (abs(dy) < 0.1 && abs(velocity.y) < 0.01) {
            curPos.y = targetPos.y;
            velocity.y = 0;
        } else {
            curPos.y += velocity.y;
        }
        
        // Z axis (depth) based on distance from original position
        double dox = originalPos.x - curPos.x;
        double doy = originalPos.y - curPos.y;
        double dd = (dox * dox) + (doy * doy);
        double d = sqrt(dd);
        
        targetPos.z = d / 100.0 + 1.0;
        double dz = targetPos.z - curPos.z;
        double az = dz * springStrength;
        velocity.z += az;
        velocity.z *= friction;
        
        // Stop small Z oscillations
        if (abs(dz) < 0.01 && abs(velocity.z) < 0.001) {
            curPos.z = targetPos.z;
            velocity.z = 0;
        } else {
            curPos.z += velocity.z;
        }
        
        // Update radius based on depth
        radius = size * curPos.z;
        if (radius < 2) radius = 2;
    }
    
    void updateSprite() {
        if (spriteId >= MAX_SPRITES || !spriteGfx || !visible) return;
        
        // Update sprite position
        int x = static_cast<int>(curPos.x) - 4; // Center the 8x8 sprite
        int y = static_cast<int>(curPos.y) - 4;
        
        // Clamp to screen bounds
        if (x < -8) x = -8;
        if (x > 256) x = 256;
        if (y < -8) y = -8;
        if (y > 192) y = 192;
        
        // Set sprite attributes using correct OAM function
        oamSet(&oamMain, spriteId, x, y, 0, 0, SpriteSize_8x8, SpriteColorFormat_16Color, 
               spriteGfx, -1, false, false, false, false, false);
    }
    
    void allocateSprite() {
        // Allocate graphics memory for this sprite
        spriteGfx = oamAllocateGfx(&oamMain, SpriteSize_8x8, SpriteColorFormat_16Color);
        
        if (spriteGfx) {
            // Create simple filled circle sprite (8x8 pixels, 4bpp format)
            u8 dotSprite[32] = {
                0x00, 0x11, 0x11, 0x00,  // .XX..... .XX.....
                0x01, 0x11, 0x11, 0x10,  // XXXX.... XXXX....
                0x01, 0x11, 0x11, 0x10,  // XXXX.... XXXX....
                0x01, 0x11, 0x11, 0x10,  // XXXX.... XXXX....
                0x01, 0x11, 0x11, 0x10,  // XXXX.... XXXX....
                0x01, 0x11, 0x11, 0x10,  // XXXX.... XXXX....
                0x01, 0x11, 0x11, 0x10,  // XXXX.... XXXX....
                0x00, 0x11, 0x11, 0x00   // .XX..... .XX.....
            };
            
            // Copy sprite data to VRAM
            dmaCopy(dotSprite, spriteGfx, 32);
            visible = true;
        }
    }
};

// Point data scaled for NDS top screen (256x192)
struct PointData {
    int x, y;
    int size;
    const char* color;
};

static void computeBounds(const PointData* data, int count, double& w, double& h) {
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
    
    PointCollection() : mousePos(128, 96, 0) {} // Start in center
    
    void addPoint(double x, double y, double z, double size, const std::string& color, int id) {
        points.emplace_back(x, y, z, size, color, id);
    }
    
    void update() {
        for (auto& point : points) {
            double dx = mousePos.x - point.curPos.x;
            double dy = mousePos.y - point.curPos.y;
            double dd = (dx * dx) + (dy * dy);
            double d = sqrt(dd);
            
            if (d < 60) {  // Interaction distance for NDS screen
                point.targetPos.x = point.curPos.x - dx;
                point.targetPos.y = point.curPos.y - dy;
            } else {
                point.targetPos.x = point.originalPos.x;
                point.targetPos.y = point.originalPos.y;
            }
            
            point.update();
        }
    }
    
    void updateSprites() {
        for (auto& point : points) {
            point.updateSprite();
        }
    }
    
    void allocateSprites() {
        for (auto& point : points) {
            point.allocateSprite();
        }
    }
};

class App {
private:
    PointCollection pointCollection;
    bool running;
    touchPosition touch;
    bool touching;
    
public:
    App() : running(false), touching(false) {}
    
    bool init() {
        // Initialize video modes - CRITICAL: Must enable sprites on main screen
        videoSetMode(MODE_0_2D | DISPLAY_SPR_ACTIVE | DISPLAY_BG0_ACTIVE);
        videoSetModeSub(MODE_0_2D | DISPLAY_BG0_ACTIVE);
        
        // VRAM setup - Based on common DS patterns found in forums
        vramSetPrimaryBanks(VRAM_A_MAIN_SPRITE,      // Main sprites
                           VRAM_B_MAIN_BG_0x06000000, // Main background  
                           VRAM_C_SUB_BG,             // Sub background
                           VRAM_D_LCD);               // Not used
        
        // Initialize OAM for sprites
        oamInit(&oamMain, SpriteMapping_1D_32, false);
        
        // Clear all sprites initially
        for (int i = 0; i < 128; i++) {
            oamClearSprite(&oamMain, i);
        }
        
        // Initialize main background
        int mainBg = bgInit(0, BgType_Text4bpp, BgSize_T_256x256, 0, 1);
        
        // Initialize console on sub screen  
        consoleInit(NULL, 0, BgType_Text4bpp, BgSize_T_256x256, 31, 0, false, true);
        
        // Set up background palette
        BG_PALETTE[0] = RGB15(0, 0, 8);     // Dark blue background
        BG_PALETTE[1] = RGB15(31, 31, 31);  // White text
        
        // Clear main background
        u16* bgMap = bgGetMapPtr(mainBg);
        dmaFillHalfWords(0, bgMap, 32*24*2);
        
        // Initialize sprite palette - simpler approach
        SPRITE_PALETTE[0] = RGB15(0, 0, 0);    // Transparent
        SPRITE_PALETTE[1] = RGB15(31, 31, 31); // White default
        
        printf("Initializing points...\n");
        initPoints();
        
        printf("Setup complete!\n");
        running = true;
        return true;
    }
    
    void initPoints() {
        // Smaller set of points for testing
        static const PointData pointData[] = {
            {87, 32, 4, "#ed9d33"}, {141, 34, 4, "#d44d61"}, {104, 28, 4, "#4f7af2"},
            {87, 24, 4, "#ef9a1e"}, {107, 15, 4, "#4976f3"}, {122, 32, 4, "#269230"},
            {119, 24, 4, "#1f9e2c"}, {18, 36, 4, "#1c48dd"}, {109, 21, 4, "#2a56ea"},
            {30, 34, 4, "#3355d8"}, {119, 3, 4, "#36b641"}, {95, 25, 4, "#2e5def"},
            {143, 17, 3, "#d53747"}, {136, 21, 3, "#eb676f"}, {84, 17, 3, "#f9b125"},
            {130, 28, 3, "#de3646"}
        };
        
        const int pointCount = sizeof(pointData) / sizeof(pointData[0]);
        
        double logoW, logoH;
        computeBounds(pointData, pointCount, logoW, logoH);
        
        double offsetX = (TOP_SCREEN_WIDTH / 2.0) - (logoW / 2.0);
        double offsetY = (TOP_SCREEN_HEIGHT / 2.0) - (logoH / 2.0);
        
        // Create points
        for (int i = 0; i < pointCount && i < MAX_SPRITES; i++) {
            double x = offsetX + pointData[i].x;
            double y = offsetY + pointData[i].y;
            pointCollection.addPoint(x, y, 0.0, static_cast<double>(pointData[i].size), 
                                   std::string(pointData[i].color), i);
            
            // Set up individual sprite colors
            Color c = Color::fromHex(std::string(pointData[i].color));
            SPRITE_PALETTE[i * 16 + 1] = c.toNDS(); // Each sprite gets 16 color slot
        }
        
        printf("Allocating sprites...\n");
        pointCollection.allocateSprites();
        printf("Points created: %d\n", pointCount);
    }
    
    void handleInput() {
        scanKeys();
        
        u32 keys = keysHeld();
        
        // Handle touch input
        if (keys & KEY_TOUCH) {
            touchRead(&touch);
            if (touch.px >= 0 && touch.py >= 0) {
                touching = true;
                // Map touch coordinates to top screen
                pointCollection.mousePos.set(touch.px, touch.py);
            }
        } else {
            touching = false;
        }
        
        // Handle D-pad input as alternative to touch
        static double padX = TOP_SCREEN_WIDTH / 2.0;
        static double padY = TOP_SCREEN_HEIGHT / 2.0;
        
        if (keys & KEY_UP) padY -= 3.0;
        if (keys & KEY_DOWN) padY += 3.0;
        if (keys & KEY_LEFT) padX -= 3.0;
        if (keys & KEY_RIGHT) padX += 3.0;
        
        // Clamp to screen bounds
        if (padX < 0) padX = 0;
        if (padX > TOP_SCREEN_WIDTH) padX = TOP_SCREEN_WIDTH;
        if (padY < 0) padY = 0;
        if (padY > TOP_SCREEN_HEIGHT) padY = TOP_SCREEN_HEIGHT;
        
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
        pointCollection.updateSprites();
        
        // Critical: Update OAM to apply changes
        oamUpdate(&oamMain);
        
        // Update bottom screen text
        consoleClear();
        printf("Google Balls - Nintendo DS\n");
        printf("========================\n\n");
        printf("Touch screen or use D-pad\n");
        printf("to interact with the dots!\n\n");
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
        printf("Sprites allocated\n");
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
    
    printf("Starting DS Google Balls...\n");
    
    if (!app.init()) {
        printf("Failed to initialize!\n");
        while(1) swiWaitForVBlank();
        return -1;
    }
    
    app.run();
    app.cleanup();
    
    return 0;
}