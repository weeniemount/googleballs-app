#include <gccore.h>
#include <wiiuse/wpad.h>
#include <fat.h>
#include <vector>
#include <cmath>
#include <string>
#include <cstdio>
#include <cstdlib>

static void *xfb = NULL;
static GXRModeObj *rmode = NULL;

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
    
    static Color fromHex(const std::string& hex) {
        if (hex[0] == '#') {
            unsigned int value = strtoul(hex.c_str() + 1, NULL, 16);
            return Color((value >> 16) & 0xFF, (value >> 8) & 0xFF, value & 0xFF, 255);
        }
        return Color();
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
    
    void draw(u32* framebuffer, int fbWidth, int fbHeight) {
        int x0 = static_cast<int>(curPos.x);
        int y0 = static_cast<int>(curPos.y);
        double r = radius;
        
        // Simple circle drawing with anti-aliasing
        for (int x = static_cast<int>(-r - 1); x <= static_cast<int>(r + 1); x++) {
            for (int y = static_cast<int>(-r - 1); y <= static_cast<int>(r + 1); y++) {
                double coverage = 0.0;
                const int samples = 2; // Reduce samples for performance
                
                for (int sx = 0; sx < samples; sx++) {
                    for (int sy = 0; sy < samples; sy++) {
                        double px = x + (sx + 0.5) / samples - 0.5;
                        double py = y + (sy + 0.5) / samples - 0.5;
                        double dist = std::sqrt(px * px + py * py);
                        
                        if (dist <= r) {
                            coverage += 1.0 / (samples * samples);
                        }
                    }
                }
                
                if (coverage > 0.0) {
                    int px = x0 + x;
                    int py = y0 + y;
                    
                    if (px >= 0 && px < fbWidth && py >= 0 && py < fbHeight) {
                        u8 alpha = static_cast<u8>(coverage * color.a);
                        u32 pixel = (color.r << 24) | (color.g << 16) | (color.b << 8) | alpha;
                        
                        // Simple alpha blending
                        if (alpha == 255) {
                            framebuffer[py * fbWidth + px] = pixel;
                        } else if (alpha > 0) {
                            u32 bg = framebuffer[py * fbWidth + px];
                            u8 bgR = (bg >> 24) & 0xFF;
                            u8 bgG = (bg >> 16) & 0xFF;
                            u8 bgB = (bg >> 8) & 0xFF;
                            
                            float a = alpha / 255.0f;
                            u8 outR = color.r * a + bgR * (1 - a);
                            u8 outG = color.g * a + bgG * (1 - a);
                            u8 outB = color.b * a + bgB * (1 - a);
                            
                            framebuffer[py * fbWidth + px] = (outR << 24) | (outG << 16) | (outB << 8) | 0xFF;
                        }
                    }
                }
            }
        }
    }
};

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
    
    PointCollection() : mousePos(320, 240, 0) {}
    
    void addPoint(double x, double y, double z, double size, const std::string& color) {
        points.emplace_back(x, y, z, size, color);
    }
    
    void update() {
        for (auto& point : points) {
            double dx = mousePos.x - point.curPos.x;
            double dy = mousePos.y - point.curPos.y;
            double dd = (dx * dx) + (dy * dy);
            double d = std::sqrt(dd);
            
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
    
    void draw(u32* framebuffer, int fbWidth, int fbHeight) {
        for (auto& point : points) {
            point.draw(framebuffer, fbWidth, fbHeight);
        }
    }
};

void initPoints(PointCollection& pointCollection, int windowWidth, int windowHeight) {
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

    double offsetX = (windowWidth / 2.0) - (logoW / 2.0);
    double offsetY = (windowHeight / 2.0) - (logoH / 2.0);

    for (const auto& data : pointData) {
        double x = offsetX + data.x;
        double y = offsetY + data.y;
        pointCollection.addPoint(x, y, 0.0, static_cast<double>(data.size), data.color);
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
    u32* framebuffer = (u32*)malloc(fbWidth * fbHeight * sizeof(u32));
    
    // Initialize points
    PointCollection pointCollection;
    initPoints(pointCollection, fbWidth, fbHeight);
    
    // Initialize IR for pointer
    WPAD_SetVRes(WPAD_CHAN_0, fbWidth, fbHeight);
    WPAD_SetDataFormat(WPAD_CHAN_0, WPAD_FMT_BTNS_ACC_IR);
    
    bool running = true;
    
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
            pointCollection.mousePos.set(ir.x, ir.y);
        }
        
        // Clear framebuffer (white background)
        for(int i = 0; i < fbWidth * fbHeight; i++) {
            framebuffer[i] = 0xFFFFFFFF;
        }
        
        // Update and draw
        pointCollection.update();
        pointCollection.draw(framebuffer, fbWidth, fbHeight);
        
        // Convert and copy to xfb (YUV422 format)
        // Simple RGB to YUV conversion
        u32* dst = (u32*)xfb;
        for(int y = 0; y < fbHeight; y++) {
            for(int x = 0; x < fbWidth / 2; x++) {
                u32 p1 = framebuffer[y * fbWidth + x * 2];
                u32 p2 = framebuffer[y * fbWidth + x * 2 + 1];
                
                u8 r1 = (p1 >> 24) & 0xFF;
                u8 g1 = (p1 >> 16) & 0xFF;
                u8 b1 = (p1 >> 8) & 0xFF;
                
                u8 r2 = (p2 >> 24) & 0xFF;
                u8 g2 = (p2 >> 16) & 0xFF;
                u8 b2 = (p2 >> 8) & 0xFF;
                
                // RGB to YUV
                u8 y1 = (u8)(0.299 * r1 + 0.587 * g1 + 0.114 * b1);
                u8 y2 = (u8)(0.299 * r2 + 0.587 * g2 + 0.114 * b2);
                u8 u = (u8)(128 - 0.168736 * ((r1+r2)/2) - 0.331264 * ((g1+g2)/2) + 0.5 * ((b1+b2)/2));
                u8 v = (u8)(128 + 0.5 * ((r1+r2)/2) - 0.418688 * ((g1+g2)/2) - 0.081312 * ((b1+b2)/2));
                
                dst[y * fbWidth / 2 + x] = (y1 << 24) | (u << 16) | (y2 << 8) | v;
            }
        }
        
        VIDEO_SetNextFramebuffer(xfb);
        VIDEO_Flush();
        VIDEO_WaitVSync();
        
        // Match original frame timing (30ms)
        usleep(30000);
    }
    
    free(framebuffer);
    return 0;
}