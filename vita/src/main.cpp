#include <SDL2/SDL.h>
#include <vector>
#include <cmath>
#include <string>
#include <algorithm>

struct Vector3 {
    float x, y, z;
    
    Vector3(float x = 0, float y = 0, float z = 0) : x(x), y(y), z(z) {}
    
    void set(float newX, float newY, float newZ = 0) {
        x = newX; y = newY; z = newZ;
    }
};

struct Color {
    Uint8 r, g, b, a;
    
    Color(Uint8 r = 255, Uint8 g = 255, Uint8 b = 255, Uint8 a = 255) 
        : r(r), g(g), b(b), a(a) {}
    
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
    float radius, size;
    float friction = 0.8f;
    float springStrength = 0.1f;
    
    // Optimization: Pre-calculate squared radius for distance checks
    float radiusSquared;
    
    Point(float x, float y, float z, float size, const std::string& colorHex)
        : curPos(x, y, z), originalPos(x, y, z), targetPos(x, y, z),
          velocity(0, 0, 0), size(size), radius(size) {
        color = Color::fromHex(colorHex);
        radiusSquared = radius * radius;
    }
    
    void update() {
        // X axis spring physics
        float dx = targetPos.x - curPos.x;
        float ax = dx * springStrength;
        velocity.x += ax;
        velocity.x *= friction;
        
        // Stop small oscillations
        if (std::abs(dx) < 0.1f && std::abs(velocity.x) < 0.01f) {
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
        
        // Stop small oscillations
        if (std::abs(dy) < 0.1f && std::abs(velocity.y) < 0.01f) {
            curPos.y = targetPos.y;
            velocity.y = 0;
        } else {
            curPos.y += velocity.y;
        }
        
        // Z axis (depth) based on distance from original position
        float dox = originalPos.x - curPos.x;
        float doy = originalPos.y - curPos.y;
        float dd = (dox * dox) + (doy * doy);
        float d = std::sqrt(dd);
        
        targetPos.z = d / 100.0f + 1.0f;
        float dz = targetPos.z - curPos.z;
        float az = dz * springStrength;
        velocity.z += az;
        velocity.z *= friction;
        
        // Stop small Z oscillations
        if (std::abs(dz) < 0.01f && std::abs(velocity.z) < 0.001f) {
            curPos.z = targetPos.z;
            velocity.z = 0;
        } else {
            curPos.z += velocity.z;
        }
        
        // Update radius based on depth
        radius = size * curPos.z;
        if (radius < 1) radius = 1;
        radiusSquared = radius * radius;
    }
    
    // Optimized circle drawing using Bresenham's algorithm with simple anti-aliasing
    void drawOptimized(SDL_Renderer* renderer) {
        SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
        
        int x0 = static_cast<int>(curPos.x);
        int y0 = static_cast<int>(curPos.y);
        int r = static_cast<int>(radius);
        
        if (r <= 1) {
            // Just draw a point for very small circles
            SDL_RenderDrawPoint(renderer, x0, y0);
            return;
        }
        
        // Use simple filled circle algorithm
        for (int y = -r; y <= r; y++) {
            int width = static_cast<int>(std::sqrt(r * r - y * y));
            SDL_RenderDrawLine(renderer, x0 - width, y0 + y, x0 + width, y0 + y);
        }
    }
};

struct PointData {
    int x, y;
    int size;
    std::string color;
};

static void computeBounds(const std::vector<PointData>& data, float& w, float& h) {
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
    
    // Optimization: Cache squared distance threshold
    static constexpr float INTERACTION_DIST_SQ = 150.0f * 150.0f;
    
    PointCollection() : mousePos(0, 0, 0) {}
    
    void addPoint(float x, float y, float z, float size, const std::string& color) {
        points.emplace_back(x, y, z, size, color);
    }
    
    void update() {
        for (auto& point : points) {
            float dx = mousePos.x - point.curPos.x;
            float dy = mousePos.y - point.curPos.y;
            float dd = (dx * dx) + (dy * dy);
            
            if (dd < INTERACTION_DIST_SQ) {
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
            point.drawOptimized(renderer);
        }
    }
};

class App {
private:
    SDL_Window* window;
    SDL_Renderer* renderer;
    PointCollection pointCollection;
    bool running;
    int windowWidth, windowHeight;
    
    // Optimization: Frame timing
    Uint32 lastFrameTime;
    static constexpr Uint32 TARGET_FRAME_TIME = 33; // ~30 FPS for better performance
    
public:
    App() : window(nullptr), renderer(nullptr), running(false), 
            windowWidth(960), windowHeight(544), lastFrameTime(0) {}
    
    bool init() {
        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK) < 0) {
            SDL_Log("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
            return false;
        }
        
        window = SDL_CreateWindow("Google Balls PS Vita",
                                SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                windowWidth, windowHeight,
                                SDL_WINDOW_SHOWN | SDL_WINDOW_FULLSCREEN);
        
        if (!window) {
            SDL_Log("Window could not be created! SDL_Error: %s\n", SDL_GetError());
            return false;
        }
        
        // Optimization: Use software renderer for better compatibility on Vita
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
        if (!renderer) {
            SDL_Log("Renderer could not be created! SDL_Error: %s\n", SDL_GetError());
            return false;
        }
        
        // Initialize joystick for touch input
        if (SDL_NumJoysticks() > 0) {
            SDL_JoystickOpen(0);
        }

        initPoints();
        running = true;
        lastFrameTime = SDL_GetTicks();
        return true;
    }
    
    void initPoints() {
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
		
        float logoW, logoH;
        computeBounds(pointData, logoW, logoH);

        float offsetX = (windowWidth / 2.0f) - (logoW / 2.0f);
        float offsetY = (windowHeight / 2.0f) - (logoH / 2.0f);

        for (const auto& data : pointData) {
            float x = offsetX + data.x;
            float y = offsetY + data.y;
            pointCollection.addPoint(x, y, 0.0f, static_cast<float>(data.size), data.color);
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
                case SDL_FINGERDOWN:
                case SDL_FINGERMOTION:
                    pointCollection.mousePos.set(
                        e.tfinger.x * windowWidth, 
                        e.tfinger.y * windowHeight
                    );
                    break;
                case SDL_JOYBUTTONDOWN:
                    if (e.jbutton.button == SDL_CONTROLLER_BUTTON_START || 
                        e.jbutton.button == SDL_CONTROLLER_BUTTON_BACK) {
                        running = false;
                    }
                    break;
                case SDL_JOYAXISMOTION:
                    if (e.jaxis.axis == 2) { // Right stick X
                        pointCollection.mousePos.x = (e.jaxis.value + 32768) * windowWidth / 65535;
                    }
                    if (e.jaxis.axis == 3) { // Right stick Y
                        pointCollection.mousePos.y = (e.jaxis.value + 32768) * windowHeight / 65535;
                    }
                    break;
            }
        }
    }
    
    void update() {
        pointCollection.update();
    }
    
    void render() {
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderClear(renderer);
        
        pointCollection.draw(renderer);
        
        SDL_RenderPresent(renderer);
    }
    
    void run() {
        while (running) {
            Uint32 frameStart = SDL_GetTicks();
            
            handleEvents();
            update();
            render();
            
            // Cap framerate
            Uint32 frameTime = SDL_GetTicks() - frameStart;
            if (TARGET_FRAME_TIME > frameTime) {
                SDL_Delay(TARGET_FRAME_TIME - frameTime);
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