#include <time.h>
extern "C" int nanosleep(const struct timespec *, struct timespec *);

#include <orbis/libkernel.h>
#include <orbis/VideoOut.h>
#include <orbis/Pad.h>
#include <orbis/UserService.h>

#if __has_include(<orbis/SystemService.h>)
#include <orbis/SystemService.h>
#else
extern "C" int sceSystemServiceHideSplashScreen(void);
#endif

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

static const int    SCREEN_W   = 1920;
static const int    SCREEN_H   = 1080;
static const int    NUM_BUFS   = 2;
static const double SCALE      = 4.0;
static const double PUSH_RADIUS = 150.0 * SCALE;

static const uint32_t PIXEL_FMT_BGRA_SRGB = 0x80000000;
static const int      TILING_LINEAR       = 1;
static const int      ASPECT_16_9         = 0;

static const double GYRO_SENS   = 2000.0;
static const float  GYRO_DEAD   = 0.03f;
static const float  STICK_DEAD  = 0.15f;

struct Vector3 {
    double x, y, z;
    Vector3(double x = 0, double y = 0, double z = 0) : x(x), y(y), z(z) {}
};

class Point {
public:
    Vector3 curPos, originalPos, targetPos, velocity;
    uint8_t r, g, b;
    double radius, size;
    double friction = 0.8833;
    double springStrength = 0.0309;

    Point(double x, double y, double z, double size, uint32_t rgb)
        : curPos(x, y, z), originalPos(x, y, z), targetPos(x, y, z),
          velocity(0, 0, 0), r((rgb >> 16) & 0xFF), g((rgb >> 8) & 0xFF), b(rgb & 0xFF),
          radius(size), size(size) {}

    void update() {
        double dx = targetPos.x - curPos.x;
        velocity.x += dx * springStrength;
        velocity.x *= friction;
        if (std::fabs(dx) < 0.1 * SCALE && std::fabs(velocity.x) < 0.01 * SCALE) {
            curPos.x = targetPos.x; velocity.x = 0;
        } else curPos.x += velocity.x;

        double dy = targetPos.y - curPos.y;
        velocity.y += dy * springStrength;
        velocity.y *= friction;
        if (std::fabs(dy) < 0.1 * SCALE && std::fabs(velocity.y) < 0.01 * SCALE) {
            curPos.y = targetPos.y; velocity.y = 0;
        } else curPos.y += velocity.y;

        double dox = originalPos.x - curPos.x;
        double doy = originalPos.y - curPos.y;
        double d = std::sqrt(dox * dox + doy * doy);

        targetPos.z = d / (100.0 * SCALE) + 1.0;
        double dz = targetPos.z - curPos.z;
        velocity.z += dz * springStrength;
        velocity.z *= friction;
        if (std::fabs(dz) < 0.01 && std::fabs(velocity.z) < 0.001) {
            curPos.z = targetPos.z; velocity.z = 0;
        } else curPos.z += velocity.z;

        radius = size * SCALE * curPos.z;
        if (radius < 1) radius = 1;
    }
};

struct PointData { int x, y, size; uint32_t rgb; };

class Canvas {
public:
    std::vector<uint32_t> px;
    Canvas() : px(SCREEN_W * SCREEN_H, 0xFFFFFFFF) {}

    void clear(uint32_t argb) { std::fill(px.begin(), px.end(), argb); }

    void blend(int x, int y, uint8_t r, uint8_t g, uint8_t b, float a) {
        if ((unsigned)x >= (unsigned)SCREEN_W || (unsigned)y >= (unsigned)SCREEN_H) return;
        uint32_t &d = px[y * SCREEN_W + x];
        float dr = (d >> 16) & 0xFF, dg = (d >> 8) & 0xFF, db = d & 0xFF;
        uint32_t nr = (uint32_t)(dr + (r - dr) * a + 0.5f);
        uint32_t ng = (uint32_t)(dg + (g - dg) * a + 0.5f);
        uint32_t nb = (uint32_t)(db + (b - db) * a + 0.5f);
        d = 0xFF000000u | (nr << 16) | (ng << 8) | nb;
    }

    void disc(double cx, double cy, double rad, uint8_t r, uint8_t g, uint8_t b) {
        const uint32_t solid = 0xFF000000u | ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
        const double ro = rad + 0.5, ri = rad - 0.5;
        int y0 = std::max(0, (int)std::floor(cy - rad - 1));
        int y1 = std::min(SCREEN_H - 1, (int)std::ceil(cy + rad + 1));
        for (int y = y0; y <= y1; y++) {
            double dy = y + 0.5 - cy;
            double o2 = ro * ro - dy * dy;
            if (o2 <= 0) continue;
            double ho = std::sqrt(o2);
            int xo0 = std::max(0, (int)std::floor(cx - ho - 0.5));
            int xo1 = std::min(SCREEN_W - 1, (int)std::ceil(cx + ho - 0.5));

            int xi0 = 1, xi1 = 0;
            double i2 = (ri > 0) ? ri * ri - dy * dy : -1.0;
            if (i2 > 0) {
                double hi = std::sqrt(i2);
                xi0 = std::max(xo0, (int)std::ceil(cx - hi - 0.5));
                xi1 = std::min(xo1, (int)std::floor(cx + hi - 0.5));
            }

            auto edge = [&](int x) {
                double dx = x + 0.5 - cx;
                double cov = rad - std::sqrt(dx * dx + dy * dy) + 0.5;
                if (cov <= 0) return;
                if (cov > 1) cov = 1;
                blend(x, y, r, g, b, (float)cov);
            };

            if (xi0 > xi1) {
                for (int x = xo0; x <= xo1; x++) edge(x);
            } else {
                for (int x = xo0; x < xi0; x++) edge(x);
                std::fill(px.begin() + (size_t)y * SCREEN_W + xi0,
                          px.begin() + (size_t)y * SCREEN_W + xi1 + 1, solid);
                for (int x = xi1 + 1; x <= xo1; x++) edge(x);
            }
        }
    }

    void ring(double cx, double cy, double rad, double thick, uint8_t r, uint8_t g, uint8_t b) {
        int x0 = (int)(cx - rad - thick - 1), x1 = (int)(cx + rad + thick + 1);
        int y0 = (int)(cy - rad - thick - 1), y1 = (int)(cy + rad + thick + 1);
        for (int y = y0; y <= y1; y++)
            for (int x = x0; x <= x1; x++) {
                double dx = x + 0.5 - cx, dy = y + 0.5 - cy;
                double d = std::fabs(std::sqrt(dx * dx + dy * dy) - rad);
                double cov = thick * 0.5 - d + 0.5;
                if (cov <= 0) continue;
                if (cov > 1) cov = 1;
                blend(x, y, r, g, b, (float)cov);
            }
    }

    void rect(int x0, int y0, int w, int h, uint8_t r, uint8_t g, uint8_t b) {
        for (int y = y0; y < y0 + h; y++)
            for (int x = x0; x < x0 + w; x++) blend(x, y, r, g, b, 1.f);
    }
};

class Video {
    int handle = -1;
    OrbisKernelEqueue eq = 0;
    void *buf[NUM_BUFS] = {};
    int cur = 0;

public:
    bool init() {
        handle = sceVideoOutOpen(ORBIS_USER_SERVICE_USER_ID_SYSTEM, ORBIS_VIDEO_OUT_BUS_MAIN, 0, nullptr);
        if (handle < 0) return false;

        const size_t align = 0x200000;
        size_t frameBytes = ((size_t)SCREEN_W * SCREEN_H * 4 + align - 1) & ~(align - 1);
        size_t total = frameBytes * NUM_BUFS;

        off_t phys = 0;
        if (sceKernelAllocateDirectMemory(0, sceKernelGetDirectMemorySize(), total, align, 3, &phys) != 0)
            return false;

        void *base = nullptr;
        if (sceKernelMapDirectMemory(&base, total, 0x33, 0, phys, align) != 0)
            return false;

        for (int i = 0; i < NUM_BUFS; i++) buf[i] = (uint8_t *)base + frameBytes * i;

        OrbisVideoOutBufferAttribute attr;
        sceVideoOutSetBufferAttribute(&attr, PIXEL_FMT_BGRA_SRGB, TILING_LINEAR, ASPECT_16_9, SCREEN_W, SCREEN_H, SCREEN_W);
        if (sceVideoOutRegisterBuffers(handle, 0, buf, NUM_BUFS, &attr) != 0) return false;

        sceVideoOutSetFlipRate(handle, 0);
        sceKernelCreateEqueue(&eq, "balls_flip");
        sceVideoOutAddFlipEvent(eq, handle, nullptr);
        return true;
    }

    void present(const Canvas &c) {
        std::memcpy(buf[cur], c.px.data(), (size_t)SCREEN_W * SCREEN_H * 4);
        sceVideoOutSubmitFlip(handle, cur, ORBIS_VIDEO_OUT_FLIP_VSYNC, 0);

        OrbisKernelEvent ev;
        int n = 0;
        sceKernelWaitEqueue(eq, &ev, 1, &n, nullptr);
        cur = (cur + 1) % NUM_BUFS;
    }
};

struct Input {
    int pad = -1;
    uint32_t prevButtons = 0;

    double cx = SCREEN_W / 2.0, cy = SCREEN_H / 2.0;
    bool motionMode = false;
    bool quit = false;

    bool init() {
        sceUserServiceInitialize(nullptr);
        int32_t userId = 0;
        if (sceUserServiceGetInitialUser(&userId) != 0) return false;
        if (scePadInit() != 0) return false;
        pad = scePadOpen(userId, 0, 0, nullptr);
        if (pad < 0) return false;
        typedef int (*MotionStateFn)(int32_t, bool);
        reinterpret_cast<MotionStateFn>(&scePadSetMotionSensorState)(pad, true);
        return true;
    }

    static bool readTouch(const OrbisPadData &d, double &tx, double &ty) {
        const uint8_t *t = reinterpret_cast<const uint8_t *>(&d) + offsetof(OrbisPadData, touch);
        if (t[0] == 0 || t[0] > 2) return false;
        uint16_t x, y;
        std::memcpy(&x, t + 8, 2);
        std::memcpy(&y, t + 10, 2);
        tx = std::min<int>(x, 1919);
        ty = std::min<int>(y, 941);
        return true;
    }

    static void readGyro(const OrbisPadData &d, float &pitch, float &yaw) {
        float g[3];
        std::memcpy(g, reinterpret_cast<const uint8_t *>(&d) + offsetof(OrbisPadData, touch) - sizeof(g), sizeof(g));
        pitch = g[0];
        yaw   = g[1];
    }

    static float axis(uint8_t v) {
        float f = ((int)v - 128) / 128.0f;
        if (std::fabs(f) < STICK_DEAD) return 0.f;
        return f;
    }

    void update(double dt) {
        OrbisPadData d;
        std::memset(&d, 0, sizeof(d));
        if (scePadReadState(pad, &d) != 0) return;

        uint32_t pressed = d.buttons & ~prevButtons;

        if (pressed & ORBIS_PAD_BUTTON_TOUCH_PAD) motionMode = !motionMode;
        if (pressed & ORBIS_PAD_BUTTON_OPTIONS)   quit = true;
        if (pressed & (ORBIS_PAD_BUTTON_L3 | ORBIS_PAD_BUTTON_R3)) { cx = SCREEN_W / 2.0; cy = SCREEN_H / 2.0; }

        const double dpadSpeed = 900.0;
        if (d.buttons & ORBIS_PAD_BUTTON_LEFT)  cx -= dpadSpeed * dt;
        if (d.buttons & ORBIS_PAD_BUTTON_RIGHT) cx += dpadSpeed * dt;
        if (d.buttons & ORBIS_PAD_BUTTON_UP)    cy -= dpadSpeed * dt;
        if (d.buttons & ORBIS_PAD_BUTTON_DOWN)  cy += dpadSpeed * dt;

        cx += axis(d.leftStick.x)  * 1600.0 * dt;
        cy += axis(d.leftStick.y)  * 1600.0 * dt;
        cx += axis(d.rightStick.x) * 450.0  * dt;
        cy += axis(d.rightStick.y) * 450.0  * dt;

        {
            double tx, ty;
            if (readTouch(d, tx, ty)) {
                cx = tx * (double)SCREEN_W / 1920.0;
                cy = ty * (double)SCREEN_H / 942.0;
            }
        }

        if (motionMode) {
            float pitch, yaw;
            readGyro(d, pitch, yaw);
            if (std::fabs(yaw)   < GYRO_DEAD) yaw = 0;
            if (std::fabs(pitch) < GYRO_DEAD) pitch = 0;
            cx -= yaw   * GYRO_SENS * dt;
            cy -= pitch * GYRO_SENS * dt;
        }

        cx = std::clamp(cx, 0.0, (double)SCREEN_W - 1);
        cy = std::clamp(cy, 0.0, (double)SCREEN_H - 1);
        prevButtons = d.buttons;
    }
};

class PointCollection {
public:
    Vector3 mousePos;
    std::vector<Point> points;

    void addPoint(double x, double y, double size, uint32_t rgb) { points.emplace_back(x, y, 0.0, size, rgb); }

    void update() {
        for (auto &p : points) {
            double dx = mousePos.x - p.curPos.x;
            double dy = mousePos.y - p.curPos.y;
            double d = std::sqrt(dx * dx + dy * dy);

            if (d < PUSH_RADIUS) {
                p.targetPos.x = p.curPos.x - dx;
                p.targetPos.y = p.curPos.y - dy;
            } else {
                p.targetPos.x = p.originalPos.x;
                p.targetPos.y = p.originalPos.y;
            }
            p.update();
        }
    }

    void draw(Canvas &c) {
        for (auto &p : points) c.disc(p.curPos.x, p.curPos.y, p.radius, p.r, p.g, p.b);
    }
};

static void initPoints(PointCollection &pc) {
    static const PointData data[] = {
        {202, 78, 9, 0xed9d33}, {348, 83, 9, 0xd44d61}, {256, 69, 9, 0x4f7af2},
        {214, 59, 9, 0xef9a1e}, {265, 36, 9, 0x4976f3}, {300, 78, 9, 0x269230},
        {294, 59, 9, 0x1f9e2c}, {45, 88, 9, 0x1c48dd},  {268, 52, 9, 0x2a56ea},
        {73, 83, 9, 0x3355d8},  {294, 6, 9, 0x36b641},  {235, 62, 9, 0x2e5def},
        {353, 42, 8, 0xd53747}, {336, 52, 8, 0xeb676f}, {208, 41, 8, 0xf9b125},
        {321, 70, 8, 0xde3646}, {8, 60, 8, 0x2a59f0},   {180, 81, 8, 0xeb9c31},
        {146, 65, 8, 0xc41731}, {145, 49, 8, 0xd82038}, {246, 34, 8, 0x5f8af8},
        {169, 69, 8, 0xefa11e}, {273, 99, 8, 0x2e55e2}, {248, 120, 8, 0x4167e4},
        {294, 41, 8, 0x0b991a}, {267, 114, 8, 0x4869e3},{78, 67, 8, 0x3059e3},
        {294, 23, 8, 0x10a11d}, {117, 83, 8, 0xcf4055}, {137, 80, 8, 0xcd4359},
        {14, 71, 8, 0x2855ea},  {331, 80, 8, 0xca273c}, {25, 82, 8, 0x2650e1},
        {233, 46, 8, 0x4a7bf9}, {73, 13, 8, 0x3d65e7},  {327, 35, 6, 0xf47875},
        {319, 46, 6, 0xf36764}, {256, 81, 6, 0x1d4eeb}, {244, 88, 6, 0x698bf1},
        {194, 32, 6, 0xfac652}, {97, 56, 6, 0xee5257},  {105, 75, 6, 0xcf2a3f},
        {42, 4, 6, 0x5681f5},   {10, 27, 6, 0x4577f6},  {166, 55, 6, 0xf7b326},
        {266, 88, 6, 0x2b58e8}, {178, 34, 6, 0xfacb5e}, {100, 65, 6, 0xe02e3d},
        {343, 32, 6, 0xf16d6f}, {59, 5, 6, 0x507bf2},   {27, 9, 6, 0x5683f7},
        {233, 116, 6, 0x3158e2},{123, 32, 6, 0xf0696c}, {6, 38, 6, 0x3769f6},
        {63, 62, 6, 0x6084ef},  {6, 49, 6, 0x2a5cf4},   {108, 36, 6, 0xf4716e},
        {169, 43, 6, 0xf8c247}, {137, 37, 6, 0xe74653}, {318, 58, 6, 0xec4147},
        {226, 100, 5, 0x4876f1},{101, 46, 5, 0xef5c5c}, {226, 108, 5, 0x2552ea},
        {17, 17, 5, 0x4779f7},  {232, 93, 5, 0x4b78f1}
    };
    const int n = sizeof(data) / sizeof(data[0]);

    int minX = 99999, maxX = -99999, minY = 99999, maxY = -99999;
    for (int i = 0; i < n; i++) {
        minX = std::min(minX, data[i].x); maxX = std::max(maxX, data[i].x);
        minY = std::min(minY, data[i].y); maxY = std::max(maxY, data[i].y);
    }
    double logoW = (maxX - minX) * SCALE, logoH = (maxY - minY) * SCALE;
    double offX = SCREEN_W / 2.0 - logoW / 2.0;
    double offY = SCREEN_H / 2.0 - logoH / 2.0;

    for (int i = 0; i < n; i++) {
        pc.addPoint(offX + (data[i].x - minX) * SCALE, offY + (data[i].y - minY) * SCALE, data[i].size, data[i].rgb);
    
    }
}

int main() {
    Video video;
    Input input;
    Canvas canvas;
    PointCollection points;

    if (!video.init()) return -1;
    if (!input.init()) return -2;
    initPoints(points);

    const double dt = 1.0 / 60.0;
    unsigned frame = 0;

    while (!input.quit) {
        input.update(dt);
        points.mousePos.x = input.cx;
        points.mousePos.y = input.cy;

        frame++;
        points.update();

        canvas.clear(0xFFFFFFFF);
        points.draw(canvas);

        canvas.ring(input.cx, input.cy, 14, 3, 60, 60, 60);
        canvas.disc(input.cx, input.cy, 3, 60, 60, 60);
        if (input.motionMode) canvas.rect(24, 24, 32, 32, 30, 170, 60);
        else                  canvas.rect(24, 24, 32, 32, 190, 190, 190);

        video.present(canvas);

        if (frame == 1) sceSystemServiceHideSplashScreen();
    }
    return 0;
}