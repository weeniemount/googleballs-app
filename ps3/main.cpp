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

// Resolved at runtime
static int SCREEN_WIDTH  = 848;
static int SCREEN_HEIGHT = 512;

// Detects the current video resolution via sysutil and updates SCREEN_WIDTH/HEIGHT.
// Falls back to 848x512 (tiny3d default) if detection fails.
static void detectResolution() {
    videoState state;
    if (videoGetState(0, 0, &state) != 0) return;

    videoResolution res;
    if (videoGetResolution(state.displayMode.resolution, &res) != 0) return;

    if (res.width > 0 && res.height > 0) {
        SCREEN_WIDTH  = (int)res.width;
        SCREEN_HEIGHT = (int)res.height;
    }

    printf("Detected resolution: %dx%d\n", SCREEN_WIDTH, SCREEN_HEIGHT);
}

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
        double dx = targetPos.x - curPos.x;
        velocity.x += dx * springStrength;
        velocity.x *= friction;
        if (fabs(dx) < 0.1 && fabs(velocity.x) < 0.01) { curPos.x = targetPos.x; velocity.x = 0; }
        else curPos.x += velocity.x;

        double dy = targetPos.y - curPos.y;
        velocity.y += dy * springStrength;
        velocity.y *= friction;
        if (fabs(dy) < 0.1 && fabs(velocity.y) < 0.01) { curPos.y = targetPos.y; velocity.y = 0; }
        else curPos.y += velocity.y;

        double dox = originalPos.x - curPos.x;
        double doy = originalPos.y - curPos.y;
        double d = sqrt(dox * dox + doy * doy);
        targetPos.z = d / 100.0 + 1.0;
        double dz = targetPos.z - curPos.z;
        velocity.z += dz * springStrength;
        velocity.z *= friction;
        if (fabs(dz) < 0.01 && fabs(velocity.z) < 0.001) { curPos.z = targetPos.z; velocity.z = 0; }
        else curPos.z += velocity.z;

        radius = size * curPos.z;
        if (radius < 1) radius = 1;
    }

    void draw() {
        float x0 = (float)curPos.x;
        float y0 = (float)curPos.y;
        float r  = (float)radius;
        u32 colorValue = color.toU32();

        if (x0 < -r*2 || x0 > SCREEN_WIDTH + r*2 ||
            y0 < -r*2 || y0 > SCREEN_HEIGHT + r*2) return;

        const int segments = 20;
        const float angleStep = (2.0f * 3.14159265f) / segments;

        tiny3d_SetPolygon(TINY3D_TRIANGLES);
        for (int i = 0; i < segments; i++) {
            float a1 = i * angleStep;
            float a2 = (i + 1) * angleStep;
            tiny3d_VertexPos(x0, y0, 1);                              tiny3d_VertexColor(colorValue);
            tiny3d_VertexPos(x0 + r * cosf(a1), y0 + r * sinf(a1), 1); tiny3d_VertexColor(colorValue);
            tiny3d_VertexPos(x0 + r * cosf(a2), y0 + r * sinf(a2), 1); tiny3d_VertexColor(colorValue);
        }
        tiny3d_End();
    }
};

struct PointData {
    int x, y, size;
    const char* color;
};

static const PointData pointData[] = {
    {202, 78, 9, "#ed9d33"}, {348, 83, 9, "#d44d61"}, {256, 69, 9, "#4f7af2"},
    {214, 59, 9, "#ef9a1e"}, {265, 36, 9, "#4976f3"}, {300, 78, 9, "#269230"},
    {294, 59, 9, "#1f9e2c"}, {45,  88, 9, "#1c48dd"}, {268, 52, 9, "#2a56ea"},
    {73,  83, 9, "#3355d8"}, {294,  6, 9, "#36b641"}, {235, 62, 9, "#2e5def"},
    {353, 42, 8, "#d53747"}, {336, 52, 8, "#eb676f"}, {208, 41, 8, "#f9b125"},
    {321, 70, 8, "#de3646"}, {8,   60, 8, "#2a59f0"}, {180, 81, 8, "#eb9c31"},
    {146, 65, 8, "#c41731"}, {145, 49, 8, "#d82038"}, {246, 34, 8, "#5f8af8"},
    {169, 69, 8, "#efa11e"}, {273, 99, 8, "#2e55e2"}, {248,120, 8, "#4167e4"},
    {294, 41, 8, "#0b991a"}, {267,114, 8, "#4869e3"}, {78,  67, 8, "#3059e3"},
    {294, 23, 8, "#10a11d"}, {117, 83, 8, "#cf4055"}, {137, 80, 8, "#cd4359"},
    {14,  71, 8, "#2855ea"}, {331, 80, 8, "#ca273c"}, {25,  82, 8, "#2650e1"},
    {233, 46, 8, "#4a7bf9"}, {73,  13, 8, "#3d65e7"}, {327, 35, 6, "#f47875"},
    {319, 46, 6, "#f36764"}, {256, 81, 6, "#1d4eeb"}, {244, 88, 6, "#698bf1"},
    {194, 32, 6, "#fac652"}, {97,  56, 6, "#ee5257"}, {105, 75, 6, "#cf2a3f"},
    {42,   4, 6, "#5681f5"}, {10,  27, 6, "#4577f6"}, {166, 55, 6, "#f7b326"},
    {266, 88, 6, "#2b58e8"}, {178, 34, 6, "#facb5e"}, {100, 65, 6, "#e02e3d"},
    {343, 32, 6, "#f16d6f"}, {59,   5, 6, "#507bf2"}, {27,   9, 6, "#5683f7"},
    {233,116, 6, "#3158e2"}, {123, 32, 6, "#f0696c"}, {6,   38, 6, "#3769f6"},
    {63,  62, 6, "#6084ef"}, {6,   49, 6, "#2a5cf4"}, {108, 36, 6, "#f4716e"},
    {169, 43, 6, "#f8c247"}, {137, 37, 6, "#e74653"}, {318, 58, 6, "#ec4147"},
    {226,100, 5, "#4876f1"}, {101, 46, 5, "#ef5c5c"}, {226,108, 5, "#2552ea"},
    {17,  17, 5, "#4779f7"}, {232, 93, 5, "#4b78f1"}
};
static const int POINT_COUNT = sizeof(pointData) / sizeof(PointData);

// Original logo canvas dimensions (the point coords above were designed for this)
static const double LOGO_CANVAS_W = 360.0;
static const double LOGO_CANVAS_H = 130.0;

class PointCollection {
public:
    Vector3 mousePos;
    Point** points;
    int pointCount;

    PointCollection()
        : mousePos(0, 0, 0), points(NULL), pointCount(0) {}

    ~PointCollection() {
        if (points) {
            for (int i = 0; i < pointCount; i++) delete points[i];
            delete[] points;
        }
    }

    // Scale and center the logo to fit the actual screen, preserving aspect ratio.
    void init(const PointData* data, int count) {
        pointCount = count;
        points = new Point*[count];

        // Find actual min/max bounds of the point data (it doesn't start at 0,0)
        int minX = 99999, maxX = -99999;
        int minY = 99999, maxY = -99999;
        for (int i = 0; i < count; i++) {
            if (data[i].x < minX) minX = data[i].x;
            if (data[i].x > maxX) maxX = data[i].x;
            if (data[i].y < minY) minY = data[i].y;
            if (data[i].y > maxY) maxY = data[i].y;
        }
        double dataW = (double)(maxX - minX);
        double dataH = (double)(maxY - minY);

        // Scale so logo is ~50% of screen width, no taller than 35% of screen height
        double scaleX = (SCREEN_WIDTH  * 0.50) / dataW;
        double scaleY = (SCREEN_HEIGHT * 0.35) / dataH;
        double scale  = (scaleX < scaleY) ? scaleX : scaleY;

        // Centre based on actual data bounds, not assumed 0,0 origin
        double scaledW = dataW * scale;
        double scaledH = dataH * scale;
        double offsetX = (SCREEN_WIDTH  - scaledW) / 2.0 - minX * scale;
        double offsetY = (SCREEN_HEIGHT - scaledH) / 2.0 - minY * scale;

        for (int i = 0; i < count; i++) {
            double x = offsetX + data[i].x * scale;
            double y = offsetY + data[i].y * scale;
            double r = data[i].size * scale;
            points[i] = new Point(x, y, 0.0, r, data[i].color);
        }

        mousePos.x = SCREEN_WIDTH  / 2.0;
        mousePos.y = SCREEN_HEIGHT / 2.0;
    }

    void update() {
        for (int i = 0; i < pointCount; i++) {
            double dx = mousePos.x - points[i]->curPos.x;
            double dy = mousePos.y - points[i]->curPos.y;
            double d  = sqrt(dx * dx + dy * dy);

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
        for (int i = 0; i < pointCount; i++) points[i]->draw();
    }
};

void drawCursor(float x, float y) {
    const float s = 8.0f;
    u32 col = 0x000000ff;

    tiny3d_SetPolygon(TINY3D_LINES);
    tiny3d_VertexPos(x - s, y, 0); tiny3d_VertexColor(col);
    tiny3d_VertexPos(x + s, y, 0); tiny3d_VertexColor(col);
    tiny3d_VertexPos(x, y - s, 0); tiny3d_VertexColor(col);
    tiny3d_VertexPos(x, y + s, 0); tiny3d_VertexColor(col);
    tiny3d_End();

    tiny3d_SetPolygon(TINY3D_TRIANGLES);
    const int segs = 8;
    const float dr = 2.0f;
    const float as = (2.0f * 3.14159265f) / segs;
    for (int i = 0; i < segs; i++) {
        float a1 = i * as, a2 = (i + 1) * as;
        tiny3d_VertexPos(x, y, 0); tiny3d_VertexColor(col);
        tiny3d_VertexPos(x + dr * cosf(a1), y + dr * sinf(a1), 0); tiny3d_VertexColor(col);
        tiny3d_VertexPos(x + dr * cosf(a2), y + dr * sinf(a2), 0); tiny3d_VertexColor(col);
    }
    tiny3d_End();
}

static void eventHandler(u64 status, u64 param, void *userdata) {
    (void)param; (void)userdata;
    if (status == SYSUTIL_EXIT_GAME) exit(0);
}

int main(int argc, char *argv[]) {
    (void)argc; (void)argv;

    ioPadInit(MAX_PADS);
    sysUtilRegisterCallback(SYSUTIL_EVENT_SLOT0, eventHandler, NULL);

    // Detect resolution BEFORE tiny3d_Init so we know the actual dimensions
    detectResolution();

    tiny3d_Init(1024 * 1024);

    PointCollection collection;
    collection.init(pointData, POINT_COUNT);

    padInfo padinfo;
    padData paddata;

    while (1) {
        sysUtilCheckCallback();

        ioPadGetInfo(&padinfo);
        for (int i = 0; i < MAX_PADS; i++) {
            if (!padinfo.status[i]) continue;
            ioPadGetData(i, &paddata);

            if (paddata.len > 0) {
                int rx = (int)paddata.ANA_R_H - 128;
                int ry = (int)paddata.ANA_R_V - 128;
                int lx = (int)paddata.ANA_L_H - 128;
                int ly = (int)paddata.ANA_L_V - 128;

                if (abs(rx) > 15) collection.mousePos.x += rx / 8.0;
                if (abs(ry) > 15) collection.mousePos.y += ry / 8.0;
                if (abs(lx) > 15) collection.mousePos.x += lx / 8.0;
                if (abs(ly) > 15) collection.mousePos.y += ly / 8.0;
            }

            if (paddata.BTN_LEFT)  collection.mousePos.x -= 8;
            if (paddata.BTN_RIGHT) collection.mousePos.x += 8;
            if (paddata.BTN_UP)    collection.mousePos.y -= 8;
            if (paddata.BTN_DOWN)  collection.mousePos.y += 8;

            if (collection.mousePos.x < 0) collection.mousePos.x = 0;
            if (collection.mousePos.x >= SCREEN_WIDTH)  collection.mousePos.x = SCREEN_WIDTH - 1;
            if (collection.mousePos.y < 0) collection.mousePos.y = 0;
            if (collection.mousePos.y >= SCREEN_HEIGHT) collection.mousePos.y = SCREEN_HEIGHT - 1;

            if (paddata.BTN_START) goto cleanup;
        }

        collection.update();

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

        usleep(30303);
    }

cleanup:
    tiny3d_End();
    ioPadEnd();
    return 0;
}