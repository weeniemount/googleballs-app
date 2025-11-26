#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <unistd.h>
#include <sysutil/video.h>
#include <rsx/reality.h>
#include <io/pad.h>
#include <math.h>
#include <time.h>
#include <sys/time.h>
#include <sysutil/sysutil.h>

#define SCREEN_WIDTH 1920
#define SCREEN_HEIGHT 1080
#define BUFFER_SIZE (4 * 1024 * 1024)
#define MAX_PADS 7

// RSX Context
gcmContextData *context = NULL;
void *host_addr = NULL;
u32 *depth_buffer;
u32 *color_buffer[2];
u32 color_pitch;
u32 depth_pitch;
int current_buffer = 0;

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
          velocity(0, 0, 0), size(size), radius(size), friction(0.8), springStrength(0.1) {
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
    
    void draw(u32* buffer, int pitch) {
        int x0 = (int)curPos.x;
        int y0 = (int)curPos.y;
        double r = radius;
        
        // Draw filled circle with anti-aliasing
        for (int x = (int)(-r - 1); x <= (int)(r + 1); x++) {
            for (int y = (int)(-r - 1); y <= (int)(r + 1); y++) {
                double coverage = 0.0;
                const int samples = 4;
                
                for (int sx = 0; sx < samples; sx++) {
                    for (int sy = 0; sy < samples; sy++) {
                        double px = x + (sx + 0.5) / samples - 0.5;
                        double py = y + (sy + 0.5) / samples - 0.5;
                        double dist = sqrt(px * px + py * py);
                        
                        if (dist <= r) {
                            coverage += 1.0 / (samples * samples);
                        }
                    }
                }
                
                if (coverage > 0.0) {
                    int px = x0 + x;
                    int py = y0 + y;
                    
                    if (px >= 0 && px < SCREEN_WIDTH && py >= 0 && py < SCREEN_HEIGHT) {
                        u8 alpha = (u8)(coverage * color.a);
                        u32 pixel_offset = py * (pitch / 4) + px;
                        
                        // Alpha blending
                        u32 dst = buffer[pixel_offset];
                        u8 dst_r = (dst >> 16) & 0xFF;
                        u8 dst_g = (dst >> 8) & 0xFF;
                        u8 dst_b = dst & 0xFF;
                        
                        float a = alpha / 255.0f;
                        u8 out_r = (u8)(color.r * a + dst_r * (1.0f - a));
                        u8 out_g = (u8)(color.g * a + dst_g * (1.0f - a));
                        u8 out_b = (u8)(color.b * a + dst_b * (1.0f - a));
                        
                        buffer[pixel_offset] = (out_r << 16) | (out_g << 8) | out_b;
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
    
    void draw(u32* buffer, int pitch) {
        for (int i = 0; i < pointCount; i++) {
            points[i]->draw(buffer, pitch);
        }
    }
};

void initScreen() {
    void *host_addr = memalign(1024 * 1024, BUFFER_SIZE);
    context = initScreen(host_addr, BUFFER_SIZE);
    
    videoState state;
    videoConfiguration vconfig;
    
    videoGetState(0, 0, &state);
    videoGetResolution(state.displayMode.resolution, &vconfig);
    
    // Setup buffers
    color_pitch = 4 * SCREEN_WIDTH;
    depth_pitch = 4 * SCREEN_WIDTH;
    
    color_buffer[0] = (u32*)rsxMemalign(64, color_pitch * SCREEN_HEIGHT);
    color_buffer[1] = (u32*)rsxMemalign(64, color_pitch * SCREEN_HEIGHT);
    depth_buffer = (u32*)rsxMemalign(64, depth_pitch * SCREEN_HEIGHT);
    
    gcmSetDisplayBuffer(0, rsxMemoryAddress(color_buffer[0]), color_pitch, SCREEN_WIDTH, SCREEN_HEIGHT);
    gcmSetDisplayBuffer(1, rsxMemoryAddress(color_buffer[1]), color_pitch, SCREEN_WIDTH, SCREEN_HEIGHT);
}

void flipBuffer() {
    gcmSetFlip(context, current_buffer);
    rsxFlushBuffer(context);
    gcmSetWaitFlip(context);
    current_buffer = !current_buffer;
}

void clearBuffer(u32* buffer) {
    for (int i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; i++) {
        buffer[i] = 0xFFFFFF; // White background
    }
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
    
    // Initialize screen
    initScreen();
    
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
                
                // Analog stick control
                if (paddata.len >= 24) {
                    s16 lx = paddata.ANA_L_H - 128;
                    s16 ly = paddata.ANA_L_V - 128;
                    s16 rx = paddata.ANA_R_H - 128;
                    s16 ry = paddata.ANA_R_V - 128;
                    
                    // Use right analog for cursor (more precise)
                    if (abs(rx) > 10 || abs(ry) > 10) {
                        collection.mousePos.x += rx / 20.0;
                        collection.mousePos.y += ry / 20.0;
                    }
                    
                    // Use left analog as alternative
                    if (abs(lx) > 10 || abs(ly) > 10) {
                        collection.mousePos.x += lx / 20.0;
                        collection.mousePos.y += ly / 20.0;
                    }
                }
                
                // D-pad control
                if (paddata.BTN_LEFT) collection.mousePos.x -= 5;
                if (paddata.BTN_RIGHT) collection.mousePos.x += 5;
                if (paddata.BTN_UP) collection.mousePos.y -= 5;
                if (paddata.BTN_DOWN) collection.mousePos.y += 5;
                
                // Gyro/Sixaxis control (tilt sensor)
                if (paddata.len >= 24) {
                    // Sensor data starts at offset 41 in padData
                    s16 accelX = ((s16*)&paddata)[20]; // X-axis tilt
                    s16 accelZ = ((s16*)&paddata)[22]; // Z-axis tilt
                    
                    // Use tilt for cursor movement
                    if (abs(accelX) > 50) {
                        collection.mousePos.x += accelX / 100.0;
                    }
                    if (abs(accelZ) > 50) {
                        collection.mousePos.y -= accelZ / 100.0;
                    }
                }
                
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
        
        // Update and draw
        collection.update();
        
        u32* buffer = color_buffer[current_buffer];
        clearBuffer(buffer);
        collection.draw(buffer, color_pitch);
        
        flipBuffer();
        usleep(30000); // 30ms delay (match original)
    }
    
cleanup:
    ioPadEnd();
    return 0;
}