#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <vector>
#include <cmath>
#include <string>
#include <algorithm>
#include "icon/balls.h"

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
    Uint8 r, g, b, a;
    
    Color(Uint8 r = 255, Uint8 g = 255, Uint8 b = 255, Uint8 a = 255) 
        : r(r), g(g), b(b), a(a) {}
    
    // Convert hex string to color
    static Color fromHex(const std::string& hex) {
        if (hex[0] == '#') {
            unsigned int value = std::stoul(hex.substr(1), nullptr, 16);
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
    
    void draw(SDL_Renderer* renderer) {
        SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
        
        // Anti-aliased filled circle using multiple samples
        int x0 = static_cast<int>(curPos.x);
        int y0 = static_cast<int>(curPos.y);
        double r = radius;
        
        // Use subpixel sampling for anti-aliasing
        for (int x = static_cast<int>(-r - 1); x <= static_cast<int>(r + 1); x++) {
            for (int y = static_cast<int>(-r - 1); y <= static_cast<int>(r + 1); y++) {
                double coverage = 0.0;
                const int samples = 4; // 4x4 subpixel sampling
                
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
                    Uint8 alpha = static_cast<Uint8>(coverage * color.a);
                    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, alpha);
                    SDL_RenderDrawPoint(renderer, x0 + x, y0 + y);
                }
            }
        }
    }
};

class PointCollection {
public:
    Vector3 mousePos;
    std::vector<Point> points;
    
    PointCollection() : mousePos(0, 0, 0) {}
    
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
    
    void draw(SDL_Renderer* renderer) {
        for (auto& point : points) {
            point.draw(renderer);
        }
    }
};

void set_icon(SDL_Window *window) {
	SDL_RWops *rw = SDL_RWFromConstMem(icon, icon_size);
	if (!rw) {
		SDL_Log("Failed to create RWops: %s", SDL_GetError());
		return;
	}

	SDL_Surface *icon = IMG_Load_RW(rw, 1); // 1 = auto-free rw
	if (!icon) {
		SDL_Log("Failed to load PNG from memory: %s", IMG_GetError());
		return;
	}

	SDL_SetWindowIcon(window, icon);
	SDL_FreeSurface(icon);
}

class App {
private:
    SDL_Window* window;
    SDL_Renderer* renderer;
    PointCollection pointCollection;
    bool running;
    int windowWidth, windowHeight;
    
public:
    App() : window(nullptr), renderer(nullptr), running(false), 
            windowWidth(800), windowHeight(600) {}
    
    bool init() {
        if (SDL_Init(SDL_INIT_VIDEO) < 0) {
            SDL_Log("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
            return false;
        }
        
        window = SDL_CreateWindow("Google Balls Desktop",
                                SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                windowWidth, windowHeight,
                                SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
        
        if (!window) {
            SDL_Log("Window could not be created! SDL_Error: %s\n", SDL_GetError());
            return false;
        }
        
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
        if (!renderer) {
            SDL_Log("Renderer could not be created! SDL_Error: %s\n", SDL_GetError());
            return false;
        }
        
        // Enable alpha blending for anti-aliasing
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        
        set_icon(window);

        initPoints();
        running = true;
        return true;
    }
    
    void initPoints() {
        // Original point data from JavaScript (adjusted for center positioning)
        struct PointData {
            int x, y;
            int size;
            std::string color;
        };
        
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
        
        // Center the points
        for (const auto& data : pointData) {
            double x = (windowWidth/2 - 180) + data.x;
            double y = (windowHeight/2 - 65) + data.y;
            pointCollection.addPoint(x, y, 0.0, static_cast<double>(data.size), data.color);
        }
    }
    
    void handleEvents() {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            switch (e.type) {
                case SDL_QUIT:
                    running = false;
                    break;
                case SDL_MOUSEMOTION:
                    pointCollection.mousePos.set(e.motion.x, e.motion.y);
                    break;
                case SDL_WINDOWEVENT:
                    if (e.window.event == SDL_WINDOWEVENT_RESIZED) {
                        windowWidth = e.window.data1;
                        windowHeight = e.window.data2;
                        // Recenter points on resize
                        for (auto& point : pointCollection.points) {
                            double relX = point.originalPos.x - (windowWidth/2 - 180);
                            double relY = point.originalPos.y - (windowHeight/2 - 65);
                            point.originalPos.x = (windowWidth/2 - 180) + relX;
                            point.originalPos.y = (windowHeight/2 - 65) + relY;
                            point.curPos.x = point.originalPos.x;
                            point.curPos.y = point.originalPos.y;
                        }
                    }
                    break;
            }
        }
    }
    
    void update() {
        pointCollection.update();
    }
    
    void render() {
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);  // White background
        SDL_RenderClear(renderer);
        
        pointCollection.draw(renderer);
        
        SDL_RenderPresent(renderer);
    }
    
    void run() {
        const int frameDelay = 30;  // Match JavaScript exactly (30ms timeout)
        
        Uint32 frameStart;
        int frameTime;
        
        while (running) {
            frameStart = SDL_GetTicks();
            
            handleEvents();
            update();
            render();
            
            frameTime = SDL_GetTicks() - frameStart;
            
            if (frameDelay > frameTime) {
                SDL_Delay(frameDelay - frameTime);
            }
        }
    }
    
    void cleanup() {
        if (renderer) {
            SDL_DestroyRenderer(renderer);
            renderer = nullptr;
        }
        
        if (window) {
            SDL_DestroyWindow(window);
            window = nullptr;
        }
        
        SDL_Quit();
    }
};

int main(int argc, char* args[]) {
    App app;
    
    if (!app.init()) {
        SDL_Log("Failed to initialize!");
        return -1;
    }
    
    app.run();
    app.cleanup();
    
    return 0;
}