#include <nds.h>
#include <gl2d.h>
#include <vector>
#include <cmath>
#include <string>

#include "ball.h"

// Target FPS and frame timing
#define TARGET_FPS 33
#define FRAME_TIME_NS (1000000000LL / TARGET_FPS)
#define VBLANK_INTERVAL ((long long)(1000000000LL / 59.8261f))

void drawBall(double x, double y, double radius, glImage *image) {
    glSpriteScaleXY(
        x - radius,
        y - radius,
        floatToFixed(radius*2.0/32.0, 12),
        floatToFixed(radius*2.0/32.0, 12),
        GL_FLIP_NONE,
        image
    );
}

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
    
    u32 toNDS() const {
        return ARGB16(a > 0, r >> 3, g >> 3, b >> 3);
    }
};

class Point {
public:
    Vector3 curPos, originalPos, targetPos, velocity;
    Color color;
    double radius, size;  // Fixed: Declare in correct order
    double friction = 0.8;
    double springStrength = 0.1;
    glImage *image;
    
    // Fixed: Initialize in declaration order
    Point(double x, double y, double z, double size, const std::string& colorHex, glImage *image)
        : curPos(x, y, z), originalPos(x, y, z), targetPos(x, y, z),
          velocity(0, 0, 0), radius(size), size(size), image(image) {
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
        // Draw filled circle using a gl2d image
        glColor(color.toNDS());
        drawBall(curPos.x, curPos.y, radius, image);
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
    
    void addPoint(double x, double y, double z, double size, const std::string& color, glImage *image) {
        points.emplace_back(x, y, z, size, color, image);
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
    PointCollection pointCollection;
    bool running;
    touchPosition touch;
    bool touching;
    u64 timing;
    int textureID;
    glImage ballImage;
    
public:
    App() : running(false), touching(false), timing(0) {}
    
    bool init() {
		lcdMainOnBottom();
		videoSetMode(MODE_0_3D);
        vramSetBankA(VRAM_A_TEXTURE);
        vramSetBankE(VRAM_E_TEX_PALETTE);
		glScreen2D();
		glClearColor(31, 31, 31, 31);

        // load ballin' texture
        textureID = glLoadTileSet(
            &ballImage,
            32, 32,
            32, 32,
            GL_RGB4,
            32, 32,
            TEXGEN_TEXCOORD | GL_TEXTURE_COLOR0_TRANSPARENT,
            ballPalLen,
            ballPal,
            ballBitmap
        );
        
        initPoints();
        running = true;
        return true;
    }
    
    void initPoints() {
        // Scaled down point data for DS screen (256x192)
        std::vector<PointData> pointData = {
            // Scaled coordinates (roughly 0.5x scale factor to fit DS screen)
            {101, 39, 5, "#ed9d33"}, {174, 41, 5, "#d44d61"}, {128, 34, 5, "#4f7af2"},
            {107, 29, 5, "#ef9a1e"},{132, 18, 5, "#4976f3"},{150, 39, 5, "#269230"},
            {147, 29, 5, "#1f9e2c"},{22, 44, 5, "#1c48dd"},{134, 26, 5, "#2a56ea"},
            {36, 41, 5, "#3355d8"},{147, 3, 5, "#36b641"},{117, 31, 5, "#2e5def"},
            {176, 21, 4, "#d53747"},{168, 26, 4, "#eb676f"},{104, 20, 4, "#f9b125"},
            {160, 35, 4, "#de3646"},{4, 30, 4, "#2a59f0"},{90, 40, 4, "#eb9c31"},
            {73, 32, 4, "#c41731"},{72, 24, 4, "#d82038"},{123, 17, 4, "#5f8af8"},
            {84, 34, 4, "#efa11e"},{136, 49, 4, "#2e55e2"},{124, 60, 4, "#4167e4"},
            {147, 20, 4, "#0b991a"},{133, 57, 4, "#4869e3"},{39, 33, 4, "#3059e3"},
            {147, 11, 4, "#10a11d"},{58, 41, 4, "#cf4055"},{68, 40, 4, "#cd4359"},
            {7, 35, 4, "#2855ea"},{165, 40, 4, "#ca273c"},{12, 41, 4, "#2650e1"},
            {116, 23, 4, "#4a7bf9"},{36, 6, 4, "#3d65e7"},{163, 17, 3, "#f47875"},
            {159, 23, 3, "#f36764"},{128, 40, 3, "#1d4eeb"},{122, 44, 3, "#698bf1"},
            {97, 16, 3, "#fac652"},{48, 28, 3, "#ee5257"},{52, 37, 3, "#cf2a3f"},
            {21, 2, 3, "#5681f5"},{5, 13, 3, "#4577f6"},{83, 27, 3, "#f7b326"},
            {133, 44, 3, "#2b58e8"},{89, 17, 3, "#facb5e"},{50, 32, 3, "#e02e3d"},
            {171, 16, 3, "#f16d6f"},{29, 2, 3, "#507bf2"},{13, 4, 3, "#5683f7"},
            {116, 58, 3, "#3158e2"},{61, 16, 3, "#f0696c"},{3, 19, 3, "#3769f6"},
            {31, 31, 3, "#6084ef"},{3, 24, 3, "#2a5cf4"},{54, 18, 3, "#f4716e"},
            {84, 21, 3, "#f8c247"},{68, 18, 3, "#e74653"},{159, 29, 3, "#ec4147"},
            {113, 50, 3, "#4876f1"},{50, 23, 3, "#ef5c5c"},{113, 54, 3, "#2552ea"},
            {8, 8, 3, "#4779f7"},{116, 46, 3, "#4b78f1"}
        };
        
        double logoW, logoH;
        computeBounds(pointData, logoW, logoH);
        
        double offsetX = (SCREEN_WIDTH / 2.0) - (logoW / 2.0) - 4; // -4 because it didn't look quite centered on NDS
        double offsetY = (SCREEN_HEIGHT / 2.0) - (logoH / 2.0);
        
        // Center the points on the top screen
        for (const auto& data : pointData) {
            double x = offsetX + data.x;
            double y = offsetY + data.y;
            pointCollection.addPoint(x, y, 0.0, static_cast<double>(data.size), data.color, &ballImage);
        }
    }
    
    void handleInput() {
		scanKeys();
		
		u32 kDown = keysDown();
		if (kDown & KEY_START) {
			running = false;
		}
		
		// Improved touch input handling for real hardware
		u32 kHeld = keysHeld();
		if (kHeld & KEY_TOUCH) {
			touchRead(&touch);
			// Less restrictive validation - allow edge touches and zero coordinates
			if (touch.px >= 0 && touch.px <= SCREEN_WIDTH && 
				touch.py >= 0 && touch.py <= SCREEN_HEIGHT) {
				touching = true;
				pointCollection.mousePos.set(touch.px, touch.py);
			}
		} else {
			touching = false;
		}
	}
    
    void update() {
        pointCollection.update();
    }
    
    void limitFrameRate() {
        while (timing < FRAME_TIME_NS) {
            swiWaitForVBlank();
            timing += VBLANK_INTERVAL;
        }
        timing -= FRAME_TIME_NS;
    }
    
    void render() {

		glBegin2D();
        
        pointCollection.draw();
        
        // Draw pointer when interacting
        if (touching) {
            // Pulsing red pointer when dragging with touch
            static u32 pulseCounter = 0;
            pulseCounter++;
            float pulseIntensity = 0.5f + 0.5f * sinf(pulseCounter * 0.3f);
            u8 red = 255;
            u8 green = (u8)(100 * pulseIntensity);
            u8 blue = (u8)(100 * pulseIntensity);

            u8 outerRed = red * 100 / 255 + 155;
            u8 outerGreen = green * 100 / 255 + 155;
            u8 outerBlue = blue * 100 / 255 + 155;
            
            glColor3b(outerRed, outerGreen, outerBlue);
            drawBall(pointCollection.mousePos.x, pointCollection.mousePos.y, 6.0, &ballImage);
            glSpriteScaleXY(pointCollection.mousePos.x - 3.0f, pointCollection.mousePos.y - 3.0f, floatToFixed(6.0f/32.0f, 12), floatToFixed(6.0f/32.0f, 12), GL_FLIP_NONE, &ballImage);
            glColor3b(red, green, blue);
            drawBall(pointCollection.mousePos.x, pointCollection.mousePos.y, 4.0, &ballImage);
        } else {
            // Gray pointer when idle
            glColor3b(194, 194, 194);
            drawBall(pointCollection.mousePos.x, pointCollection.mousePos.y, 6.0, &ballImage);
        }
        
		glEnd2D();
		glFlush(0);

    }
    
    void run() {
        while (running) {
            handleInput();
            update();
            render();
            limitFrameRate(); // Lock to 33 FPS
        }
    }
    
    void cleanup() {
        glDeleteTextures(1, &textureID);
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