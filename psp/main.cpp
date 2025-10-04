#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include <pspkernel.h>
#include <pspdebug.h>
#include <pspctrl.h>
#include <math.h>

PSP_MODULE_INFO("GoogleBalls", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);

#define MAX_POINTS 100
#define abs(x) ((x) < 0 ? -(x) : (x))

struct Vector3 {
    double x, y, z;
};

struct Color {
    Uint8 r, g, b, a;
};

class Point {
public:
    Vector3 curPos, originalPos, targetPos, velocity;
    Color color;
    double radius, size;
    double friction;
    double springStrength;
    
    Point() {
        friction = 0.8;
        springStrength = 0.1;
        radius = 0;
        size = 0;
        velocity.x = velocity.y = velocity.z = 0;
        curPos.x = curPos.y = curPos.z = 0;
        originalPos.x = originalPos.y = originalPos.z = 0;
        targetPos.x = targetPos.y = targetPos.z = 0;
        color.r = color.g = color.b = 255;
        color.a = 255;
    }
    
    void init(double x, double y, double z, double sz, const char* colorHex) {
        curPos.x = x; curPos.y = y; curPos.z = z;
        originalPos.x = x; originalPos.y = y; originalPos.z = z;
        targetPos.x = x; targetPos.y = y; targetPos.z = z;
        velocity.x = velocity.y = velocity.z = 0;
        radius = sz;
        size = sz;
        
        // Parse hex color manually
        if (colorHex[0] == '#') {
            unsigned long value = 0;
            const char* p = colorHex + 1;
            
            while (*p) {
                value <<= 4;
                if (*p >= '0' && *p <= '9') {
                    value += *p - '0';
                } else if (*p >= 'a' && *p <= 'f') {
                    value += *p - 'a' + 10;
                } else if (*p >= 'A' && *p <= 'F') {
                    value += *p - 'A' + 10;
                }
                p++;
            }
            
            color.r = (value >> 16) & 0xFF;
            color.g = (value >> 8) & 0xFF;
            color.b = value & 0xFF;
            color.a = 255;
        }
    }
    
    void update() {
        // X axis spring physics
        double dx = targetPos.x - curPos.x;
        double ax = dx * springStrength;
        velocity.x += ax;
        velocity.x *= friction;
        
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
        
        if (abs(dz) < 0.01 && abs(velocity.z) < 0.001) {
            curPos.z = targetPos.z;
            velocity.z = 0;
        } else {
            curPos.z += velocity.z;
        }
        
        // Update radius based on depth
        radius = size * curPos.z;
        if (radius < 1) radius = 1;
    }
    
    void drawPixel(SDL_Surface* surface, int x, int y, Uint32 color) {
        if (x < 0 || x >= surface->w || y < 0 || y >= surface->h) return;
        
        Uint32* pixels = (Uint32*)surface->pixels;
        pixels[y * surface->w + x] = color;
    }
    
    void draw(SDL_Surface* surface) {
        Uint32 pixelColor = SDL_MapRGB(surface->format, color.r, color.g, color.b);
        
        int x0 = (int)(curPos.x);
        int y0 = (int)(curPos.y);
        int r = (int)(radius);
        
        // Filled circle
        for (int x = -r; x <= r; x++) {
            for (int y = -r; y <= r; y++) {
                if (x * x + y * y <= r * r) {
                    drawPixel(surface, x0 + x, y0 + y, pixelColor);
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

class PointCollection {
public:
    Vector3 mousePos;
    Point points[MAX_POINTS];
    int pointCount;
    
    PointCollection() {
        mousePos.x = 240;
        mousePos.y = 136;
        mousePos.z = 0;
        pointCount = 0;
    }
    
    void addPoint(double x, double y, double z, double size, const char* color) {
        if (pointCount < MAX_POINTS) {
            points[pointCount].init(x, y, z, size, color);
            pointCount++;
        }
    }
    
    void update() {
        for (int i = 0; i < pointCount; i++) {
            Point* point = &points[i];
            double dx = mousePos.x - point->curPos.x;
            double dy = mousePos.y - point->curPos.y;
            double dd = (dx * dx) + (dy * dy);
            double d = sqrt(dd);
            
            if (d < 100) {
                point->targetPos.x = point->curPos.x - dx;
                point->targetPos.y = point->curPos.y - dy;
            } else {
                point->targetPos.x = point->originalPos.x;
                point->targetPos.y = point->originalPos.y;
            }
            
            point->update();
        }
    }
    
    void draw(SDL_Surface* surface) {
        for (int i = 0; i < pointCount; i++) {
            points[i].draw(surface);
        }
    }
};

static void computeBounds(const PointData* data, int count, double* w, double* h) {
    int minX = 99999, maxX = -99999;
    int minY = 99999, maxY = -99999;
    
    for (int i = 0; i < count; i++) {
        if (data[i].x < minX) minX = data[i].x;
        if (data[i].x > maxX) maxX = data[i].x;
        if (data[i].y < minY) minY = data[i].y;
        if (data[i].y > maxY) maxY = data[i].y;
    }
    
    *w = maxX - minX;
    *h = maxY - minY;
}

class App {
private:
    SDL_Surface* screen;
    PointCollection pointCollection;
    bool running;
    Vector3 cursorPos;
    double cursorSpeed;
    SceCtrlData pad, oldPad;
    
public:
    App() {
        screen = NULL;
        running = false;
        cursorPos.x = 240;
        cursorPos.y = 136;
        cursorPos.z = 0;
        cursorSpeed = 3.0;
    }
    
    bool init() {
        sceCtrlSetSamplingCycle(0);
        sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);
        
        if (SDL_Init(SDL_INIT_VIDEO) < 0) {
            return false;
        }
        
        screen = SDL_SetVideoMode(480, 272, 32, SDL_SWSURFACE);
        if (!screen) {
            return false;
        }
        
        initPoints();
        running = true;
        return true;
    }
    
    void initPoints() {
        static const PointData pointData[] = {
            {202, 78, 7, "#ed9d33"}, {348, 83, 7, "#d44d61"}, {256, 69, 7, "#4f7af2"},
            {214, 59, 7, "#ef9a1e"}, {265, 36, 7, "#4976f3"}, {300, 78, 7, "#269230"},
            {294, 59, 7, "#1f9e2c"}, {45, 88, 7, "#1c48dd"}, {268, 52, 7, "#2a56ea"},
            {73, 83, 7, "#3355d8"}, {294, 6, 7, "#36b641"}, {235, 62, 7, "#2e5def"},
            {353, 42, 6, "#d53747"}, {336, 52, 6, "#eb676f"}, {208, 41, 6, "#f9b125"},
            {321, 70, 6, "#de3646"}, {8, 60, 6, "#2a59f0"}, {180, 81, 6, "#eb9c31"},
            {146, 65, 6, "#c41731"}, {145, 49, 6, "#d82038"}, {246, 34, 6, "#5f8af8"},
            {169, 69, 6, "#efa11e"}, {273, 99, 6, "#2e55e2"}, {248, 120, 6, "#4167e4"},
            {294, 41, 6, "#0b991a"}, {267, 114, 6, "#4869e3"}, {78, 67, 6, "#3059e3"},
            {294, 23, 6, "#10a11d"}, {117, 83, 6, "#cf4055"}, {137, 80, 6, "#cd4359"},
            {14, 71, 6, "#2855ea"}, {331, 80, 6, "#ca273c"}, {25, 82, 6, "#2650e1"},
            {233, 46, 6, "#4a7bf9"}, {73, 13, 6, "#3d65e7"}, {327, 35, 5, "#f47875"},
            {319, 46, 5, "#f36764"}, {256, 81, 5, "#1d4eeb"}, {244, 88, 5, "#698bf1"},
            {194, 32, 5, "#fac652"}, {97, 56, 5, "#ee5257"}, {105, 75, 5, "#cf2a3f"},
            {42, 4, 5, "#5681f5"}, {10, 27, 5, "#4577f6"}, {166, 55, 5, "#f7b326"},
            {266, 88, 5, "#2b58e8"}, {178, 34, 5, "#facb5e"}, {100, 65, 5, "#e02e3d"},
            {343, 32, 5, "#f16d6f"}, {59, 5, 5, "#507bf2"}, {27, 9, 5, "#5683f7"},
            {233, 116, 5, "#3158e2"}, {123, 32, 5, "#f0696c"}, {6, 38, 5, "#3769f6"},
            {63, 62, 5, "#6084ef"}, {6, 49, 5, "#2a5cf4"}, {108, 36, 5, "#f4716e"},
            {169, 43, 5, "#f8c247"}, {137, 37, 5, "#e74653"}, {318, 58, 5, "#ec4147"},
            {226, 100, 4, "#4876f1"}, {101, 46, 4, "#ef5c5c"}, {226, 108, 4, "#2552ea"},
            {17, 17, 4, "#4779f7"}, {232, 93, 4, "#4b78f1"}
        };
        
        int dataCount = sizeof(pointData) / sizeof(pointData[0]);
        
        double logoW, logoH;
        computeBounds(pointData, dataCount, &logoW, &logoH);
        
        double scale = 0.8;
        double offsetX = (480 / 2.0) - (logoW * scale / 2.0);
        double offsetY = (272 / 2.0) - (logoH * scale / 2.0);
        
        for (int i = 0; i < dataCount; i++) {
            double x = offsetX + pointData[i].x * scale;
            double y = offsetY + pointData[i].y * scale;
            pointCollection.addPoint(x, y, 0.0, (double)(pointData[i].size) * scale, pointData[i].color);
        }
        
        pointCollection.mousePos = cursorPos;
    }
    
    void handleInput() {
        oldPad = pad;
        sceCtrlReadBufferPositive(&pad, 1);
        
        if ((pad.Buttons & PSP_CTRL_START) && (pad.Buttons & PSP_CTRL_SELECT)) {
            running = false;
            return;
        }
        
        if (pad.Buttons & PSP_CTRL_LEFT) {
            cursorPos.x -= cursorSpeed;
        }
        if (pad.Buttons & PSP_CTRL_RIGHT) {
            cursorPos.x += cursorSpeed;
        }
        if (pad.Buttons & PSP_CTRL_UP) {
            cursorPos.y -= cursorSpeed;
        }
        if (pad.Buttons & PSP_CTRL_DOWN) {
            cursorPos.y += cursorSpeed;
        }
        
        float analogX = (pad.Lx - 128) / 128.0f;
        float analogY = (pad.Ly - 128) / 128.0f;
        
        if (abs(analogX) > 0.2f) {
            cursorPos.x += analogX * cursorSpeed * 1.5;
        }
        if (abs(analogY) > 0.2f) {
            cursorPos.y += analogY * cursorSpeed * 1.5;
        }
        
        if (cursorPos.x < 0) cursorPos.x = 0;
        if (cursorPos.x >= 480) cursorPos.x = 479;
        if (cursorPos.y < 0) cursorPos.y = 0;
        if (cursorPos.y >= 272) cursorPos.y = 271;
        
        pointCollection.mousePos = cursorPos;
    }
    
    void drawCursor(SDL_Surface* surface) {
        Uint32 cursorColor = SDL_MapRGB(surface->format, 0, 0, 0);
        int x = (int)(cursorPos.x);
        int y = (int)(cursorPos.y);
        
        for (int i = -8; i <= 8; i++) {
            if (x + i >= 0 && x + i < 480) {
                if (y >= 0 && y < 272) {
                    Uint32* pixels = (Uint32*)surface->pixels;
                    pixels[y * surface->w + (x + i)] = cursorColor;
                }
            }
        }
        
        for (int i = -8; i <= 8; i++) {
            if (y + i >= 0 && y + i < 272) {
                if (x >= 0 && x < 480) {
                    Uint32* pixels = (Uint32*)surface->pixels;
                    pixels[(y + i) * surface->w + x] = cursorColor;
                }
            }
        }
    }
    
    void update() {
        pointCollection.update();
    }
    
    void render() {
        SDL_FillRect(screen, NULL, SDL_MapRGB(screen->format, 255, 255, 255));
        
        pointCollection.draw(screen);
        drawCursor(screen);
        
        SDL_Flip(screen);
    }
    
    void run() {
        while (running) {
            handleInput();
            update();
            render();
            
            SDL_Delay(33);
        }
    }
    
    void cleanup() {
        if (screen) {
            SDL_FreeSurface(screen);
        }
        SDL_Quit();
        sceKernelExitGame();
    }
};

int exit_callback(int arg1, int arg2, void *common) {
    sceKernelExitGame();
    return 0;
}

int callbackThread(SceSize args, void *argp) {
    int cbid = sceKernelCreateCallback("Exit Callback", exit_callback, NULL);
    sceKernelRegisterExitCallback(cbid);
    sceKernelSleepThreadCB();
    return 0;
}

int setupCallbacks(void) {
    int thid = sceKernelCreateThread("update_thread", callbackThread, 0x11, 0xFA0, 0, 0);
    if (thid >= 0) {
        sceKernelStartThread(thid, 0, 0);
    }
    return thid;
}

int main(int argc, char* argv[]) {
    setupCallbacks();
    
    App app;
    
    if (!app.init()) {
        app.cleanup();
        return -1;
    }
    
    app.run();
    app.cleanup();
    
    return 0;
}