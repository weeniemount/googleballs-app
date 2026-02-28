#include <SDL2/SDL.h>
#include <vector>
#include <cmath>
#include <string>
#include <algorithm>

// NXDK entry point is XMain, not main()
// Window is fixed at 640x480 on OG Xbox

#define WINDOW_WIDTH  640
#define WINDOW_HEIGHT 480

// ---------------------------------------------------------------------------
// Math / data structures
// ---------------------------------------------------------------------------

struct Vector3 {
    double x, y, z;
    Vector3(double x = 0, double y = 0, double z = 0) : x(x), y(y), z(z) {}
    void set(double nx, double ny, double nz = 0) { x = nx; y = ny; z = nz; }
};

struct Color {
    Uint8 r, g, b, a;
    Color(Uint8 r = 255, Uint8 g = 255, Uint8 b = 255, Uint8 a = 255)
        : r(r), g(g), b(b), a(a) {}

    static Color fromHex(const char* hex) {
        // Skip leading '#'
        const char* s = (hex[0] == '#') ? hex + 1 : hex;
        unsigned long v = 0;
        while (*s) {
            char c = *s++;
            v <<= 4;
            if (c >= '0' && c <= '9') v |= c - '0';
            else if (c >= 'a' && c <= 'f') v |= c - 'a' + 10;
            else if (c >= 'A' && c <= 'F') v |= c - 'A' + 10;
        }
        return Color((v >> 16) & 0xFF, (v >> 8) & 0xFF, v & 0xFF, 255);
    }
};

// ---------------------------------------------------------------------------
// Point
// ---------------------------------------------------------------------------

class Point {
public:
    Vector3 curPos, originalPos, targetPos, velocity;
    Color color;
    double radius, size;
    double friction      = 0.8;
    double springStrength = 0.1;

    Point(double x, double y, double z, double size, const char* colorHex)
        : curPos(x,y,z), originalPos(x,y,z), targetPos(x,y,z),
          velocity(0,0,0), size(size), radius(size) {
        color = Color::fromHex(colorHex);
    }

    void update() {
        auto spring1D = [&](double target, double& cur, double& vel) {
            double d = target - cur;
            vel += d * springStrength;
            vel *= friction;
            if (fabs(d) < 0.1 && fabs(vel) < 0.01) { cur = target; vel = 0; }
            else cur += vel;
        };

        spring1D(targetPos.x, curPos.x, velocity.x);
        spring1D(targetPos.y, curPos.y, velocity.y);

        double dox = originalPos.x - curPos.x;
        double doy = originalPos.y - curPos.y;
        double d   = sqrt(dox*dox + doy*doy);
        targetPos.z = d / 100.0 + 1.0;

        double dz = targetPos.z - curPos.z;
        velocity.z += dz * springStrength;
        velocity.z *= friction;
        if (fabs(dz) < 0.01 && fabs(velocity.z) < 0.001) { curPos.z = targetPos.z; velocity.z = 0; }
        else curPos.z += velocity.z;

        radius = size * curPos.z;
        if (radius < 1) radius = 1;
    }

    // Simplified circle draw for OG Xbox performance (no 4x4 MSAA — too slow on 733 MHz)
    // Uses midpoint circle algorithm for filled circle instead.
    void draw(SDL_Renderer* renderer) {
        SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
        int cx = (int)curPos.x;
        int cy = (int)curPos.y;
        int r  = (int)(radius + 0.5);
        if (r < 1) r = 1;

        for (int dy = -r; dy <= r; dy++) {
            int dx = (int)sqrt((double)(r*r - dy*dy));
            SDL_RenderDrawLine(renderer, cx - dx, cy + dy, cx + dx, cy + dy);
        }
    }
};

// ---------------------------------------------------------------------------
// Point data (same as original)
// ---------------------------------------------------------------------------

struct PointData { int x, y, size; const char* color; };

static const PointData kPoints[] = {
    {202,78,9,"#ed9d33"},{348,83,9,"#d44d61"},{256,69,9,"#4f7af2"},
    {214,59,9,"#ef9a1e"},{265,36,9,"#4976f3"},{300,78,9,"#269230"},
    {294,59,9,"#1f9e2c"},{45,88,9,"#1c48dd"},{268,52,9,"#2a56ea"},
    {73,83,9,"#3355d8"},{294,6,9,"#36b641"},{235,62,9,"#2e5def"},
    {353,42,8,"#d53747"},{336,52,8,"#eb676f"},{208,41,8,"#f9b125"},
    {321,70,8,"#de3646"},{8,60,8,"#2a59f0"},{180,81,8,"#eb9c31"},
    {146,65,8,"#c41731"},{145,49,8,"#d82038"},{246,34,8,"#5f8af8"},
    {169,69,8,"#efa11e"},{273,99,8,"#2e55e2"},{248,120,8,"#4167e4"},
    {294,41,8,"#0b991a"},{267,114,8,"#4869e3"},{78,67,8,"#3059e3"},
    {294,23,8,"#10a11d"},{117,83,8,"#cf4055"},{137,80,8,"#cd4359"},
    {14,71,8,"#2855ea"},{331,80,8,"#ca273c"},{25,82,8,"#2650e1"},
    {233,46,8,"#4a7bf9"},{73,13,8,"#3d65e7"},{327,35,6,"#f47875"},
    {319,46,6,"#f36764"},{256,81,6,"#1d4eeb"},{244,88,6,"#698bf1"},
    {194,32,6,"#fac652"},{97,56,6,"#ee5257"},{105,75,6,"#cf2a3f"},
    {42,4,6,"#5681f5"},{10,27,6,"#4577f6"},{166,55,6,"#f7b326"},
    {266,88,6,"#2b58e8"},{178,34,6,"#facb5e"},{100,65,6,"#e02e3d"},
    {343,32,6,"#f16d6f"},{59,5,6,"#507bf2"},{27,9,6,"#5683f7"},
    {233,116,6,"#3158e2"},{123,32,6,"#f0696c"},{6,38,6,"#3769f6"},
    {63,62,6,"#6084ef"},{6,49,6,"#2a5cf4"},{108,36,6,"#f4716e"},
    {169,43,6,"#f8c247"},{137,37,6,"#e74653"},{318,58,6,"#ec4147"},
    {226,100,5,"#4876f1"},{101,46,5,"#ef5c5c"},{226,108,5,"#2552ea"},
    {17,17,5,"#4779f7"},{232,93,5,"#4b78f1"}
};
static const int kPointCount = sizeof(kPoints) / sizeof(kPoints[0]);

// ---------------------------------------------------------------------------
// PointCollection
// ---------------------------------------------------------------------------

class PointCollection {
public:
    Vector3 mousePos;
    std::vector<Point> points;

    void init(int winW, int winH) {
        // Compute bounds
        int minX = 99999, maxX = -99999, minY = 99999, maxY = -99999;
        for (int i = 0; i < kPointCount; i++) {
            if (kPoints[i].x < minX) minX = kPoints[i].x;
            if (kPoints[i].x > maxX) maxX = kPoints[i].x;
            if (kPoints[i].y < minY) minY = kPoints[i].y;
            if (kPoints[i].y > maxY) maxY = kPoints[i].y;
        }
        double offsetX = (winW / 2.0) - ((maxX - minX) / 2.0);
        double offsetY = (winH / 2.0) - ((maxY - minY) / 2.0);

        points.reserve(kPointCount);
        for (int i = 0; i < kPointCount; i++) {
            points.emplace_back(
                offsetX + kPoints[i].x,
                offsetY + kPoints[i].y,
                0.0,
                (double)kPoints[i].size,
                kPoints[i].color
            );
        }
    }

    void update() {
        for (auto& p : points) {
            double dx = mousePos.x - p.curPos.x;
            double dy = mousePos.y - p.curPos.y;
            double d  = sqrt(dx*dx + dy*dy);
            if (d < 150) {
                p.targetPos.x = p.curPos.x - dx;
                p.targetPos.y = p.curPos.y - dy;
            } else {
                p.targetPos.x = p.originalPos.x;
                p.targetPos.y = p.originalPos.y;
            }
            p.update();
        }
    }

    void draw(SDL_Renderer* renderer) {
        for (auto& p : points) p.draw(renderer);
    }
};

// ---------------------------------------------------------------------------
// Xbox controller input (mapped via SDL GameController / Joystick)
// We move a virtual "cursor" with the left stick.
// ---------------------------------------------------------------------------

struct Cursor {
    double x, y;
    double vx = 0, vy = 0;
    static constexpr double SPEED = 6.0;

    Cursor(int winW, int winH) : x(winW/2.0), y(winH/2.0) {}

    void applyStick(Sint16 axisX, Sint16 axisY) {
        const Sint16 DEAD = 8000;
        vx = (abs(axisX) > DEAD) ? (axisX / 32767.0) * SPEED : 0;
        vy = (abs(axisY) > DEAD) ? (axisY / 32767.0) * SPEED : 0;
    }

    void update(int winW, int winH) {
        x += vx; y += vy;
        if (x < 0) x = 0; if (x > winW) x = winW;
        if (y < 0) y = 0; if (y > winH) y = winH;
    }

    void draw(SDL_Renderer* renderer) {
        // Small crosshair so user can see where the cursor is
        SDL_SetRenderDrawColor(renderer, 80, 80, 80, 180);
        SDL_RenderDrawLine(renderer, (int)x-8, (int)y, (int)x+8, (int)y);
        SDL_RenderDrawLine(renderer, (int)x, (int)y-8, (int)x, (int)y+8);
    }
};

// ---------------------------------------------------------------------------
// XMain — NXDK entry point
// ---------------------------------------------------------------------------

#ifdef _XBOX
extern "C" void XMain() {
#else
int main(int argc, char* argv[]) {
#endif

    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER);

    SDL_Window*   window   = SDL_CreateWindow("Google Balls",
                                 SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                 WINDOW_WIDTH, WINDOW_HEIGHT, 0);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    // Open first available gamepad
    SDL_GameController* pad = nullptr;
    for (int i = 0; i < SDL_NumJoysticks(); i++) {
        if (SDL_IsGameController(i)) { pad = SDL_GameControllerOpen(i); break; }
    }

    PointCollection collection;
    collection.init(WINDOW_WIDTH, WINDOW_HEIGHT);

    Cursor cursor(WINDOW_WIDTH, WINDOW_HEIGHT);

    bool running = true;
    const int FRAME_MS = 30;

    while (running) {
        Uint32 frameStart = SDL_GetTicks();

        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            switch (e.type) {
                case SDL_QUIT: running = false; break;
                case SDL_MOUSEMOTION:
                    cursor.x = e.motion.x;
                    cursor.y = e.motion.y;
                    break;
                // Back / Start to quit (Xbox controller buttons)
                case SDL_CONTROLLERBUTTONDOWN:
                    if (e.cbutton.button == SDL_CONTROLLER_BUTTON_BACK ||
                        e.cbutton.button == SDL_CONTROLLER_BUTTON_START)
                        running = false;
                    break;
            }
        }

        // Controller stick → virtual cursor
        if (pad) {
            Sint16 ax = SDL_GameControllerGetAxis(pad, SDL_CONTROLLER_AXIS_LEFTX);
            Sint16 ay = SDL_GameControllerGetAxis(pad, SDL_CONTROLLER_AXIS_LEFTY);
            cursor.applyStick(ax, ay);
        }
        cursor.update(WINDOW_WIDTH, WINDOW_HEIGHT);
        collection.mousePos.set(cursor.x, cursor.y);

        collection.update();

        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderClear(renderer);
        collection.draw(renderer);
        cursor.draw(renderer);
        SDL_RenderPresent(renderer);

        int elapsed = (int)(SDL_GetTicks() - frameStart);
        if (FRAME_MS > elapsed) SDL_Delay(FRAME_MS - elapsed);
    }

    if (pad) SDL_GameControllerClose(pad);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

#ifndef _XBOX
    return 0;
#endif
}