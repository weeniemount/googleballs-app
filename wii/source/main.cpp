#include <gccore.h>
#include <wiiuse/wpad.h>
#include <fat.h>
#include <unistd.h>
#include <vector>
#include <cmath>
#include <string>
#include <malloc.h>
#include <cstdio>
#include <cstdlib>

static void *xfb = NULL;
static GXRModeObj *rmode = NULL;

struct Vector3 {
    float x, y, z;  // Changed from double to float for better performance
    
    Vector3(float x = 0, float y = 0, float z = 0) : x(x), y(y), z(z) {}
    
    inline void set(float newX, float newY, float newZ = 0) {
        x = newX; y = newY; z = newZ;
    }
};

struct Color {
    u8 r, g, b, a;
    
    Color(u8 r = 255, u8 g = 255, u8 b = 255, u8 a = 255) 
        : r(r), g(g), b(b), a(a) {}
    
    static Color fromHex(const char* hex) {  // Changed from std::string& to const char*
        if (hex[0] == '#') {
            unsigned int value = strtoul(hex + 1, NULL, 16);
            return Color((value >> 16) & 0xFF, (value >> 8) & 0xFF, value & 0xFF, 255);
        }
        return Color();
    }
};

class Point {
public:
    Vector3 curPos, originalPos, targetPos, velocity;
    Color color;
    float radius, size;
    float friction;
    float springStrength;
    
    Point(float x, float y, float z, float size, const char* colorHex)
        : curPos(x, y, z), originalPos(x, y, z), targetPos(x, y, z),
          velocity(0, 0, 0), radius(size), size(size), 
          friction(0.8f), springStrength(0.1f) {
        color = Color::fromHex(colorHex);
    }
    
    void update() {
        // X axis spring physics
        float dx = targetPos.x - curPos.x;
        float ax = dx * springStrength;
        velocity.x += ax;
        velocity.x *= friction;
        
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
        
        targetPos.z = d * 0.01f + 1.0f;  // Optimized division
        float dz = targetPos.z - curPos.z;
        float az = dz * springStrength;
        velocity.z += az;
        velocity.z *= friction;
        
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
    
    inline void drawPixel(u32* framebuffer, int px, int py, int fbWidth, int fbHeight, u8 alpha) {
        if (px >= 0 && px < fbWidth && py >= 0 && py < fbHeight) {
            int idx = py * fbWidth + px;
            
            if (alpha == 255) {
                framebuffer[idx] = (color.r << 24) | (color.g << 16) | (color.b << 8) | 0xFF;
            } else if (alpha > 0) {
                u32 bg = framebuffer[idx];
                u8 bgR = (bg >> 24) & 0xFF;
                u8 bgG = (bg >> 16) & 0xFF;
                u8 bgB = (bg >> 8) & 0xFF;
                
                // Fast integer alpha blending
                u32 invAlpha = 255 - alpha;
                u8 outR = (color.r * alpha + bgR * invAlpha) >> 8;
                u8 outG = (color.g * alpha + bgG * invAlpha) >> 8;
                u8 outB = (color.b * alpha + bgB * invAlpha) >> 8;
                
                framebuffer[idx] = (outR << 24) | (outG << 16) | (outB << 8) | 0xFF;
            }
        }
    }
    
    void draw(u32* framebuffer, int fbWidth, int fbHeight) {
        int x0 = (int)curPos.x;
        int y0 = (int)curPos.y;
        int r = (int)(radius + 0.5f);
        int rSq = r * r;
        
        // Optimized circle drawing with minimal anti-aliasing
        int minY = -r - 1;
        int maxY = r + 1;
        int minX = -r - 1;
        int maxX = r + 1;
        
        for (int y = minY; y <= maxY; y++) {
            int ySq = y * y;
            for (int x = minX; x <= maxX; x++) {
                int distSq = x * x + ySq;
                
                if (distSq <= rSq) {
                    // Fully inside
                    drawPixel(framebuffer, x0 + x, y0 + y, fbWidth, fbHeight, 255);
                } else if (distSq <= (r + 1) * (r + 1)) {
                    // Edge pixels - simple anti-aliasing
                    float dist = sqrtf((float)distSq);
                    float coverage = r + 1.0f - dist;
                    if (coverage > 0) {
                        u8 alpha = (u8)(coverage * 255);
                        drawPixel(framebuffer, x0 + x, y0 + y, fbWidth, fbHeight, alpha);
                    }
                }
            }
        }
    }
};

struct PointData {
    int x, y;
    int size;
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
    
    w = (float)(maxX - minX);
    h = (float)(maxY - minY);
}

class PointCollection {
public:
    Vector3 mousePos;
    std::vector<Point> points;
    float interactionRadiusSq;  // Pre-squared for faster comparison
    
    PointCollection() : mousePos(320, 240, 0), interactionRadiusSq(150.0f * 150.0f) {
        points.reserve(70);  // Pre-allocate space
    }
    
    void addPoint(float x, float y, float z, float size, const char* color) {
        points.emplace_back(x, y, z, size, color);
    }
    
    void update() {
        for (auto& point : points) {
            float dx = mousePos.x - point.curPos.x;
            float dy = mousePos.y - point.curPos.y;
            float distSq = dx * dx + dy * dy;
            
            if (distSq < interactionRadiusSq) {
                point.targetPos.x = point.curPos.x - dx;
                point.targetPos.y = point.curPos.y - dy;
            } else {
                point.targetPos.x = point.originalPos.x;
                point.targetPos.y = point.originalPos.y;
            }
            
            point.update();
        }
    }
    
    void draw(u32* framebuffer, int fbWidth, int fbHeight) {
        for (auto& point : points) {
            point.draw(framebuffer, fbWidth, fbHeight);
        }
    }
};

// Cursor drawing function
void drawCursor(u32* framebuffer, int fbWidth, int fbHeight, int cursorX, int cursorY) {
    // Draw a crosshair cursor
    const int size = 10;
    const int thickness = 2;
    const u32 cursorColor = 0xFF0000FF;  // Red color (RGBA)
    
    // Horizontal line
    for (int x = cursorX - size; x <= cursorX + size; x++) {
        for (int t = 0; t < thickness; t++) {
            int y = cursorY + t - thickness/2;
            if (x >= 0 && x < fbWidth && y >= 0 && y < fbHeight) {
                framebuffer[y * fbWidth + x] = cursorColor;
            }
        }
    }
    
    // Vertical line
    for (int y = cursorY - size; y <= cursorY + size; y++) {
        for (int t = 0; t < thickness; t++) {
            int x = cursorX + t - thickness/2;
            if (x >= 0 && x < fbWidth && y >= 0 && y < fbHeight) {
                framebuffer[y * fbWidth + x] = cursorColor;
            }
        }
    }
    
    const int centerSize = 3;
    for (int y = -centerSize; y <= centerSize; y++) {
        for (int x = -centerSize; x <= centerSize; x++) {
            if (x*x + y*y <= centerSize*centerSize) {
                int px = cursorX + x;
                int py = cursorY + y;
                if (px >= 0 && px < fbWidth && py >= 0 && py < fbHeight) {
                    framebuffer[py * fbWidth + px] = cursorColor;
                }
            }
        }
    }
}

void initPoints(PointCollection& pointCollection, int windowWidth, int windowHeight) {
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
    
    const int pointCount = sizeof(pointData) / sizeof(pointData[0]);
    
    float logoW, logoH;
    computeBounds(pointData, pointCount, logoW, logoH);

    float offsetX = (windowWidth / 2.0f) - (logoW / 2.0f);
    float offsetY = (windowHeight / 2.0f) - (logoH / 2.0f);

    for (int i = 0; i < pointCount; i++) {
        float x = offsetX + pointData[i].x;
        float y = offsetY + pointData[i].y;
        pointCollection.addPoint(x, y, 0.0f, (float)pointData[i].size, pointData[i].color);
    }
}

extern "C" int main(int argc, char **argv) {
    // Initialize video
    VIDEO_Init();
    WPAD_Init();
    
    rmode = VIDEO_GetPreferredMode(NULL);
    xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));
    
    VIDEO_Configure(rmode);
    VIDEO_SetNextFramebuffer(xfb);
    VIDEO_SetBlack(FALSE);
    VIDEO_Flush();
    VIDEO_WaitVSync();
    
    if(rmode->viTVMode & VI_NON_INTERLACE) VIDEO_WaitVSync();
    
    int fbWidth = rmode->fbWidth;
    int fbHeight = rmode->xfbHeight;
    
    // Allocate framebuffer for drawing
    u32* framebuffer = (u32*)memalign(32, fbWidth * fbHeight * sizeof(u32));
    if (!framebuffer) {
        return -1;
    }
    
    // Initialize points
    PointCollection pointCollection;
    initPoints(pointCollection, fbWidth, fbHeight);
    
    // Initialize IR for pointer
    WPAD_SetVRes(WPAD_CHAN_0, fbWidth, fbHeight);
    WPAD_SetDataFormat(WPAD_CHAN_0, WPAD_FMT_BTNS_ACC_IR);
    
    bool running = true;
    int cursorX = fbWidth / 2;
    int cursorY = fbHeight / 2;
    bool cursorVisible = false;
    
    while(running) {
        WPAD_ScanPads();
        u32 pressed = WPAD_ButtonsDown(0);
        
        if(pressed & WPAD_BUTTON_HOME) {
            running = false;
        }
        
        // Get IR pointer position
        ir_t ir;
        WPAD_IR(WPAD_CHAN_0, &ir);
        
        if(ir.valid) {
            cursorX = ir.x;
            cursorY = ir.y;
            cursorVisible = true;
            pointCollection.mousePos.set((float)ir.x, (float)ir.y);
        } else {
            cursorVisible = false;
        }
        
        // Clear framebuffer (white background) - optimized with memset equivalent
        u32 whiteColor = 0xFFFFFFFF;
        u32* fb = framebuffer;
        int totalPixels = fbWidth * fbHeight;
        for(int i = 0; i < totalPixels; i++) {
            fb[i] = whiteColor;
        }
        
        // Update and draw
        pointCollection.update();
        pointCollection.draw(framebuffer, fbWidth, fbHeight);
        
        // Draw cursor if visible
        if (cursorVisible) {
            drawCursor(framebuffer, fbWidth, fbHeight, cursorX, cursorY);
        }
        
        // Convert and copy to xfb (YUV422 format)
        u32* dst = (u32*)xfb;
        for(int y = 0; y < fbHeight; y++) {
            u32* srcRow = &framebuffer[y * fbWidth];
            u32* dstRow = &dst[y * fbWidth / 2];
            
            for(int x = 0; x < fbWidth / 2; x++) {
                u32 p1 = srcRow[x * 2];
                u32 p2 = srcRow[x * 2 + 1];
                
                u8 r1 = (p1 >> 24) & 0xFF;
                u8 g1 = (p1 >> 16) & 0xFF;
                u8 b1 = (p1 >> 8) & 0xFF;
                
                u8 r2 = (p2 >> 24) & 0xFF;
                u8 g2 = (p2 >> 16) & 0xFF;
                u8 b2 = (p2 >> 8) & 0xFF;
                
                // RGB to YUV (using integer math for speed)
                int rAvg = (r1 + r2) >> 1;
                int gAvg = (g1 + g2) >> 1;
                int bAvg = (b1 + b2) >> 1;
                
                u8 y1 = (u8)((77 * r1 + 150 * g1 + 29 * b1) >> 8);
                u8 y2 = (u8)((77 * r2 + 150 * g2 + 29 * b2) >> 8);
                u8 u = (u8)(128 + ((-43 * rAvg - 85 * gAvg + 128 * bAvg) >> 8));
                u8 v = (u8)(128 + ((128 * rAvg - 107 * gAvg - 21 * bAvg) >> 8));
                
                dstRow[x] = (y1 << 24) | (u << 16) | (y2 << 8) | v;
            }
        }
        
        VIDEO_SetNextFramebuffer(xfb);
        VIDEO_Flush();
        VIDEO_WaitVSync();
        
        // Match original frame timing (30ms = ~33 FPS)
        usleep(30000);
    }
    
    free(framebuffer);
    return 0;
}