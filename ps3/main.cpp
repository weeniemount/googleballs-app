#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <time.h>
#include <sys/time.h>
#include <sysutil/video.h>
#include <sysutil/sysutil.h>
#include <io/pad.h>
#include <tiny3d.h>

#define SCREEN_WIDTH 848
#define SCREEN_HEIGHT 512

struct Vector3 {
    double x, y, z;
    
    Vector3(double x = 0, double y = 0, double z = 0) : x(x), y(y), z(z) {}
    
    void set(double newX, double newY, double newZ = 0) {
        x = newX; y = newY; z = newZ;
    }
};

struct Color {
    u8 r, g, b, a;
    
    Color(u8 r = 255, u8 g = 255, u8 b = 255, u8 a = 255) 
        : r(r), g(g), b(b), a(a) {}
    
    static Color fromHex(const char* hex) {
        if (hex[0] == '#') {
            unsigned int value;
            sscanf(hex + 1, "%x", &value);
            return Color((value >> 16) & 0xFF, (value >> 8) & 0xFF, value & 0xFF, 255);
        }
        return Color();
    }
    
    // PS3 big-endian: try ABGR format (0xAABBGGRR)
    u32 toU32() const {
        return (r << 24) | (g << 16) | (b << 8) | a;
    }
};

class Point {
public:
    Vector3 curPos, originalPos, targetPos, velocity;
    Color color;
    double radius, size;
    double friction;
    double springStrength;
    
    Point(double x, double y, double z, double size, const char* colorHex)
        : curPos(x, y, z), originalPos(x, y, z), targetPos(x, y, z),
          velocity(0, 0, 0), radius(size), size(size), friction(0.8), springStrength(0.1) {
        color = Color::fromHex(colorHex);
    }
    
    void update() {
        // X axis spring physics
        double dx = targetPos.x - curPos.x;
        double ax = dx * springStrength;
        velocity.x += ax;
        velocity.x *= friction;
        
        if (fabs(dx) < 0.1 && fabs(velocity.x) < 0.01) {
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
        
        if (fabs(dy) < 0.1 && fabs(velocity.y) < 0.01) {
            curPos.y = targetPos.y;
            velocity.y = 0;
        } else {
            curPos.y += velocity.y;
        }
        
        // Z axis (depth)
        double dox = originalPos.x - curPos.x;
        double doy = originalPos.y - curPos.y;
        double dd = (dox * dox) + (doy * doy);
        double d = sqrt(dd);
        
        targetPos.z = d / 100.0 + 1.0;
        double dz = targetPos.z - curPos.z;
        double az = dz * springStrength;
        velocity.z += az;
        velocity.z *= friction;
        
        if (fabs(dz) < 0.01 && fabs(velocity.z) < 0.001) {
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
        float x0 = (float)curPos.x;
        float y0 = (float)curPos.y;
        float r = (float)radius;
        u32 colorValue = color.toU32();
        
        // Skip if off screen
        if (x0 < -r*2 || x0 > SCREEN_WIDTH + r*2 || 
            y0 < -r*2 || y0 > SCREEN_HEIGHT + r*2) {
            return;
        }
        
        // Draw circle using triangle fan approximation
        const int segments = 20;
        const float angleStep = (2.0f * 3.14159265f) / segments;
        
        tiny3d_SetPolygon(TINY3D_TRIANGLES);
        
        for (int i = 0; i < segments; i++) {
            float angle1 = i * angleStep;
            float angle2 = (i + 1) * angleStep;
            
            // Center point
            tiny3d_VertexPos(x0, y0, 1);
            tiny3d_VertexColor(colorValue);
            
            // First edge point
            tiny3d_VertexPos(x0 + r * cosf(angle1), y0 + r * sinf(angle1), 1);
            tiny3d_VertexColor(colorValue);
            
            // Second edge point
            tiny3d_VertexPos(x0 + r * cosf(angle2), y0 + r * sinf(angle2), 1);
            tiny3d_VertexColor(colorValue);
        }
        
        tiny3d_End();
    }
};

struct PointData {
    int x, y;
    int size;
    const char* color;
};

void computeBounds(const PointData* data, int count, double& w, double& h) {
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
    Point** points;
    int pointCount;
    
    PointCollection() : mousePos(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, 0), points(NULL), pointCount(0) {}
    
    ~PointCollection() {
        if (points) {
            for (int i = 0; i < pointCount; i++) {
                delete points[i];
            }
            delete[] points;
        }
    }
    
    void init(const PointData* data, int count) {
        pointCount = count;
        points = new Point*[count];
        
        double logoW, logoH;
        computeBounds(data, count, logoW, logoH);
        
        double offsetX = (SCREEN_WIDTH / 2.0) - (logoW / 2.0);
        double offsetY = (SCREEN_HEIGHT / 2.0) - (logoH / 2.0);
        
        for (int i = 0; i < count; i++) {
            double x = offsetX + data[i].x;
            double y = offsetY + data[i].y;
            points[i] = new Point(x, y, 0.0, (double)data[i].size, data[i].color);
        }
    }
    
    void update() {
        for (int i = 0; i < pointCount; i++) {
            double dx = mousePos.x - points[i]->curPos.x;
            double dy = mousePos.y - points[i]->curPos.y;
            double dd = (dx * dx) + (dy * dy);
            double d = sqrt(dd);
            
            if (d < 150) {
                points[i]->targetPos.x = points[i]->curPos.x - dx;
                points[i]->targetPos.y = points[i]->curPos.y - dy;
            } else {
                points[i]->targetPos.x = points[i]->originalPos.x;
                points[i]->targetPos.y = points[i]->originalPos.y;
            }
            
            points[i]->update();
        }
    }
    
    void draw() {
        for (int i = 0; i < pointCount; i++) {
            points[i]->draw();
        }
    }
};

// Draw cursor
void drawCursor(float x, float y) {
    const float cursorSize = 8.0f;
    u32 cursorColor = 0x000000ff; // Black cursor
    
    // Draw crosshair cursor
    tiny3d_SetPolygon(TINY3D_LINES);
    
    // Horizontal line
    tiny3d_VertexPos(x - cursorSize, y, 0);
    tiny3d_VertexColor(cursorColor);
    tiny3d_VertexPos(x + cursorSize, y, 0);
    tiny3d_VertexColor(cursorColor);
    
    // Vertical line
    tiny3d_VertexPos(x, y - cursorSize, 0);
    tiny3d_VertexColor(cursorColor);
    tiny3d_VertexPos(x, y + cursorSize, 0);
    tiny3d_VertexColor(cursorColor);
    
    tiny3d_End();
    
    // Draw center dot
    tiny3d_SetPolygon(TINY3D_TRIANGLES);
    const int segments = 8;
    const float dotRadius = 2.0f;
    const float angleStep = (2.0f * 3.14159265f) / segments;
    
    for (int i = 0; i < segments; i++) {
        float angle1 = i * angleStep;
        float angle2 = (i + 1) * angleStep;
        
        tiny3d_VertexPos(x, y, 0);
        tiny3d_VertexColor(cursorColor);
        
        tiny3d_VertexPos(x + dotRadius * cosf(angle1), y + dotRadius * sinf(angle1), 0);
        tiny3d_VertexColor(cursorColor);
        
        tiny3d_VertexPos(x + dotRadius * cosf(angle2), y + dotRadius * sinf(angle2), 0);
        tiny3d_VertexColor(cursorColor);
    }
    
    tiny3d_End();
}

static void eventHandler(u64 status, u64 param, void *userdata) {
    (void)param;
    (void)userdata;
    if (status == SYSUTIL_EXIT_GAME) {
        exit(0);
    }
}

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;
    
    // Initialize pad
    ioPadInit(MAX_PADS);
    
    // Register exit handler
    sysUtilRegisterCallback(SYSUTIL_EVENT_SLOT0, eventHandler, NULL);
    
    // Initialize tiny3d
    tiny3d_Init(1024*1024);
    
    // Point data
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
    
    PointCollection collection;
    collection.init(pointData, sizeof(pointData) / sizeof(PointData));
    
    padInfo padinfo;
    padData paddata;
    
    // Main loop
    while (1) {
        sysUtilCheckCallback();
        
        // Handle input
        ioPadGetInfo(&padinfo);
        for (int i = 0; i < MAX_PADS; i++) {
            if (padinfo.status[i]) {
                ioPadGetData(i, &paddata);
                
                // Analog stick control (faster movement)
                if (paddata.len > 0) {
                    // Right analog stick
                    int rx = (int)paddata.ANA_R_H - 128;
                    int ry = (int)paddata.ANA_R_V - 128;
                    
                    if (abs(rx) > 15) {
                        collection.mousePos.x += rx / 8.0;  // Changed from /15.0 to /8.0 for faster movement
                    }
                    if (abs(ry) > 15) {
                        collection.mousePos.y += ry / 8.0;  // Changed from /15.0 to /8.0 for faster movement
                    }
                    
                    // Left analog stick
                    int lx = (int)paddata.ANA_L_H - 128;
                    int ly = (int)paddata.ANA_L_V - 128;
                    
                    if (abs(lx) > 15) {
                        collection.mousePos.x += lx / 8.0;  // Changed from /15.0 to /8.0 for faster movement
                    }
                    if (abs(ly) > 15) {
                        collection.mousePos.y += ly / 8.0;  // Changed from /15.0 to /8.0 for faster movement
                    }
                }
                
                // D-pad control (faster movement)
                if (paddata.BTN_LEFT) collection.mousePos.x -= 8;   // Changed from 5 to 8
                if (paddata.BTN_RIGHT) collection.mousePos.x += 8;  // Changed from 5 to 8
                if (paddata.BTN_UP) collection.mousePos.y -= 8;     // Changed from 5 to 8
                if (paddata.BTN_DOWN) collection.mousePos.y += 8;   // Changed from 5 to 8
                
                // Clamp cursor position
                if (collection.mousePos.x < 0) collection.mousePos.x = 0;
                if (collection.mousePos.x >= SCREEN_WIDTH) collection.mousePos.x = SCREEN_WIDTH - 1;
                if (collection.mousePos.y < 0) collection.mousePos.y = 0;
                if (collection.mousePos.y >= SCREEN_HEIGHT) collection.mousePos.y = SCREEN_HEIGHT - 1;
                
                // Exit on START button
                if (paddata.BTN_START) {
                    goto cleanup;
                }
            }
        }
        
        // Update
        collection.update();
        
        // Draw
        tiny3d_Clear(0xffffffff, TINY3D_CLEAR_ALL);
        tiny3d_AlphaTest(1, 0x10, TINY3D_ALPHA_FUNC_GEQUAL);
        tiny3d_BlendFunc(1, 
                        (blend_src_func)TINY3D_BLEND_FUNC_SRC_RGB_SRC_ALPHA, 
                        (blend_dst_func)TINY3D_BLEND_FUNC_SRC_RGB_ONE_MINUS_SRC_ALPHA,
                        (blend_func)TINY3D_BLEND_RGB_FUNC_ADD);
        
        tiny3d_Project2D();
        collection.draw();
        drawCursor((float)collection.mousePos.x, (float)collection.mousePos.y);
        
        tiny3d_Flip();
        usleep(30303); // 33 fps (1000000 / 33 = 30303)
    }
    
cleanup:
    tiny3d_End();
    ioPadEnd();
    return 0;
}