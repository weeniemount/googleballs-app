#include <SDL2/SDL.h>
#include <whb/proc.h>
#include <whb/sdcard.h>
#include <vector>
#include <cmath>
#include <algorithm>

struct Vector3 {
    float x, y, z;
    Vector3(float x = 0, float y = 0, float z = 0) : x(x), y(y), z(z) {}
};

struct Color {
    Uint8 r, g, b;
    
    static Color fromHex(unsigned int hex) {
        return {static_cast<Uint8>((hex >> 16) & 0xFF),
                static_cast<Uint8>((hex >> 8) & 0xFF),
                static_cast<Uint8>(hex & 0xFF)};
    }
};

class Point {
public:
    Vector3 curPos, originalPos, targetPos, velocity;
    Color color;
    float size, radius;
    
    static constexpr float FRICTION = 0.8f;
    static constexpr float SPRING = 0.1f;
    static constexpr float THRESHOLD_POS = 0.1f;
    static constexpr float THRESHOLD_VEL = 0.01f;
    
    Point(float x, float y, float size, unsigned int colorHex)
        : curPos(x, y, 0), originalPos(x, y, 0), targetPos(x, y, 0),
          velocity(0, 0, 0), size(size), radius(size) {
        color = Color::fromHex(colorHex);
    }
    
    inline void update() {
        // X axis
        float dx = targetPos.x - curPos.x;
        velocity.x = (velocity.x + dx * SPRING) * FRICTION;
        
        if (std::abs(dx) < THRESHOLD_POS && std::abs(velocity.x) < THRESHOLD_VEL) {
            curPos.x = targetPos.x;
            velocity.x = 0;
        } else {
            curPos.x += velocity.x;
        }
        
        // Y axis
        float dy = targetPos.y - curPos.y;
        velocity.y = (velocity.y + dy * SPRING) * FRICTION;
        
        if (std::abs(dy) < THRESHOLD_POS && std::abs(velocity.y) < THRESHOLD_VEL) {
            curPos.y = targetPos.y;
            velocity.y = 0;
        } else {
            curPos.y += velocity.y;
        }
        
        // Z axis (depth based on distance from origin)
        float dox = originalPos.x - curPos.x;
        float doy = originalPos.y - curPos.y;
        float distSq = dox * dox + doy * doy;
        
        targetPos.z = std::sqrt(distSq) * 0.01f + 1.0f;
        float dz = targetPos.z - curPos.z;
        velocity.z = (velocity.z + dz * SPRING) * FRICTION;
        
        if (std::abs(dz) < 0.01f && std::abs(velocity.z) < 0.001f) {
            curPos.z = targetPos.z;
            velocity.z = 0;
        } else {
            curPos.z += velocity.z;
        }
        
        radius = size * curPos.z;
        if (radius < 1.0f) radius = 1.0f;
    }
    
    void draw(SDL_Renderer* renderer) {
        int x0 = static_cast<int>(curPos.x);
        int y0 = static_cast<int>(curPos.y);
        int r = static_cast<int>(radius);
        
        // Optimized circle drawing with 2x2 supersampling
        int rSq = r * r;
        for (int y = -r; y <= r; y++) {
            int ySq = y * y;
            for (int x = -r; x <= r; x++) {
                int distSq = x * x + ySq;
                if (distSq <= rSq) {
                    // Simple distance-based alpha for anti-aliasing
                    float dist = std::sqrt(static_cast<float>(distSq));
                    float alpha = 1.0f;
                    if (dist > r - 1) {
                        alpha = r - dist;
                    }
                    
                    Uint8 a = static_cast<Uint8>(std::max(0.0f, std::min(255.0f, alpha * 255.0f)));
                    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, a);
                    SDL_RenderDrawPoint(renderer, x0 + x, y0 + y);
                }
            }
        }
    }
};

struct PointData {
    short x, y;
    Uint8 size;
    unsigned int color;
};

class PointCollection {
public:
    Vector3 mousePos;
    std::vector<Point> points;
    
    static constexpr float REPEL_DIST = 150.0f;
    static constexpr float REPEL_DIST_SQ = REPEL_DIST * REPEL_DIST;
    
    PointCollection() : mousePos(0, 0, 0) {
        points.reserve(64);
    }
    
    void addPoint(float x, float y, float size, unsigned int color) {
        points.emplace_back(x, y, size, color);
    }
    
    void update() {
        for (auto& point : points) {
            float dx = mousePos.x - point.curPos.x;
            float dy = mousePos.y - point.curPos.y;
            float distSq = dx * dx + dy * dy;
            
            if (distSq < REPEL_DIST_SQ) {
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

class App {
private:
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_GameController* gamepad;
    PointCollection pointCollection;
    bool running;
    int windowWidth, windowHeight;
    bool showCursor;
    
    static constexpr int CURSOR_SIZE = 12;
    static constexpr int DEADZONE = 8000;
    static constexpr float ANALOG_SPEED = 10.0f;
    
public:
    App() : window(nullptr), renderer(nullptr), gamepad(nullptr),
            running(false), windowWidth(1280), windowHeight(720), showCursor(true) {}
    
    bool init() {
        WHBProcInit();
        WHBMountSdCard();
        
        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) < 0) {
            SDL_Log("SDL init failed: %s\n", SDL_GetError());
            return false;
        }
        
        window = SDL_CreateWindow("Google Balls (Wii U)",
                                SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                windowWidth, windowHeight, SDL_WINDOW_SHOWN);
        
        if (!window) {
            SDL_Log("Window creation failed: %s\n", SDL_GetError());
            return false;
        }
        
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
        if (!renderer) {
            SDL_Log("Renderer creation failed: %s\n", SDL_GetError());
            return false;
        }
        
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        
        if (SDL_NumJoysticks() > 0) {
            gamepad = SDL_GameControllerOpen(0);
            if (gamepad) {
                SDL_Log("Gamepad connected\n");
            }
        }
        
        initPoints();
        
        // Center cursor
        pointCollection.mousePos.x = windowWidth / 2.0f;
        pointCollection.mousePos.y = windowHeight / 2.0f;
        
        running = true;
        return true;
    }
    
    void initPoints() {
        static const PointData pointData[] = {
            {202, 78, 9, 0xed9d33}, {348, 83, 9, 0xd44d61}, {256, 69, 9, 0x4f7af2},
            {214, 59, 9, 0xef9a1e}, {265, 36, 9, 0x4976f3}, {300, 78, 9, 0x269230},
            {294, 59, 9, 0x1f9e2c}, {45, 88, 9, 0x1c48dd}, {268, 52, 9, 0x2a56ea},
            {73, 83, 9, 0x3355d8}, {294, 6, 9, 0x36b641}, {235, 62, 9, 0x2e5def},
            {353, 42, 8, 0xd53747}, {336, 52, 8, 0xeb676f}, {208, 41, 8, 0xf9b125},
            {321, 70, 8, 0xde3646}, {8, 60, 8, 0x2a59f0}, {180, 81, 8, 0xeb9c31},
            {146, 65, 8, 0xc41731}, {145, 49, 8, 0xd82038}, {246, 34, 8, 0x5f8af8},
            {169, 69, 8, 0xefa11e}, {273, 99, 8, 0x2e55e2}, {248, 120, 8, 0x4167e4},
            {294, 41, 8, 0x0b991a}, {267, 114, 8, 0x4869e3}, {78, 67, 8, 0x3059e3},
            {294, 23, 8, 0x10a11d}, {117, 83, 8, 0xcf4055}, {137, 80, 8, 0xcd4359},
            {14, 71, 8, 0x2855ea}, {331, 80, 8, 0xca273c}, {25, 82, 8, 0x2650e1},
            {233, 46, 8, 0x4a7bf9}, {73, 13, 8, 0x3d65e7}, {327, 35, 6, 0xf47875},
            {319, 46, 6, 0xf36764}, {256, 81, 6, 0x1d4eeb}, {244, 88, 6, 0x698bf1},
            {194, 32, 6, 0xfac652}, {97, 56, 6, 0xee5257}, {105, 75, 6, 0xcf2a3f},
            {42, 4, 6, 0x5681f5}, {10, 27, 6, 0x4577f6}, {166, 55, 6, 0xf7b326},
            {266, 88, 6, 0x2b58e8}, {178, 34, 6, 0xfacb5e}, {100, 65, 6, 0xe02e3d},
            {343, 32, 6, 0xf16d6f}, {59, 5, 6, 0x507bf2}, {27, 9, 6, 0x5683f7},
            {233, 116, 6, 0x3158e2}, {123, 32, 6, 0xf0696c}, {6, 38, 6, 0x3769f6},
            {63, 62, 6, 0x6084ef}, {6, 49, 6, 0x2a5cf4}, {108, 36, 6, 0xf4716e},
            {169, 43, 6, 0xf8c247}, {137, 37, 6, 0xe74653}, {318, 58, 6, 0xec4147},
            {226, 100, 5, 0x4876f1}, {101, 46, 5, 0xef5c5c}, {226, 108, 5, 0x2552ea},
            {17, 17, 5, 0x4779f7}, {232, 93, 5, 0x4b78f1}
        };
        
        // Compute bounds
        short minX = 32767, maxX = -32768, minY = 32767, maxY = -32768;
        for (const auto& p : pointData) {
            minX = std::min(minX, p.x);
            maxX = std::max(maxX, p.x);
            minY = std::min(minY, p.y);
            maxY = std::max(maxY, p.y);
        }
        
        float offsetX = (windowWidth - (maxX - minX)) * 0.5f;
        float offsetY = (windowHeight - (maxY - minY)) * 0.5f;

        for (const auto& data : pointData) {
            pointCollection.addPoint(
                offsetX + data.x,
                offsetY + data.y,
                static_cast<float>(data.size),
                data.color
            );
        }
    }
    
    void drawCursor(SDL_Renderer* renderer, int x, int y) {
        // Draw crosshair cursor
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 200);
        
        // Horizontal line
        SDL_RenderDrawLine(renderer, x - CURSOR_SIZE, y, x - 3, y);
        SDL_RenderDrawLine(renderer, x + 3, y, x + CURSOR_SIZE, y);
        
        // Vertical line
        SDL_RenderDrawLine(renderer, x, y - CURSOR_SIZE, x, y - 3);
        SDL_RenderDrawLine(renderer, x, y + 3, x, y + CURSOR_SIZE);
        
        // Center dot
        SDL_RenderDrawPoint(renderer, x, y);
        SDL_RenderDrawPoint(renderer, x + 1, y);
        SDL_RenderDrawPoint(renderer, x, y + 1);
        SDL_RenderDrawPoint(renderer, x + 1, y + 1);
    }
    
    void handleEvents() {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            switch (e.type) {
                case SDL_QUIT:
                    running = false;
                    break;
                    
                case SDL_FINGERDOWN:
                case SDL_FINGERMOTION:
                    pointCollection.mousePos.x = e.tfinger.x * windowWidth;
                    pointCollection.mousePos.y = e.tfinger.y * windowHeight;
                    showCursor = true;
                    break;
                    
                case SDL_MOUSEMOTION:
                    pointCollection.mousePos.x = static_cast<float>(e.motion.x);
                    pointCollection.mousePos.y = static_cast<float>(e.motion.y);
                    showCursor = true;
                    break;
                    
                case SDL_CONTROLLERBUTTONDOWN:
                    if (e.cbutton.button == SDL_CONTROLLER_BUTTON_START ||
                        e.cbutton.button == SDL_CONTROLLER_BUTTON_B) {
                        running = false;
                    }
                    break;
            }
        }
        
        // Handle analog stick
        if (gamepad) {
            Sint16 x = SDL_GameControllerGetAxis(gamepad, SDL_CONTROLLER_AXIS_LEFTX);
            Sint16 y = SDL_GameControllerGetAxis(gamepad, SDL_CONTROLLER_AXIS_LEFTY);
            
            if (std::abs(x) > DEADZONE || std::abs(y) > DEADZONE) {
                pointCollection.mousePos.x += (x / 32768.0f) * ANALOG_SPEED;
                pointCollection.mousePos.y += (y / 32768.0f) * ANALOG_SPEED;
                
                // Clamp
                pointCollection.mousePos.x = std::max(0.0f, std::min(static_cast<float>(windowWidth), pointCollection.mousePos.x));
                pointCollection.mousePos.y = std::max(0.0f, std::min(static_cast<float>(windowHeight), pointCollection.mousePos.y));
                
                showCursor = true;
            }
        }
        
        if (!WHBProcIsRunning()) {
            running = false;
        }
    }
    
    void update() {
        pointCollection.update();
    }
    
    void render() {
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderClear(renderer);
        
        pointCollection.draw(renderer);
        
        if (showCursor) {
            drawCursor(renderer, 
                      static_cast<int>(pointCollection.mousePos.x),
                      static_cast<int>(pointCollection.mousePos.y));
        }
        
        SDL_RenderPresent(renderer);
    }
    
    void run() {
        while (running) {
            handleEvents();
            update();
            render();
        }
    }
    
    void cleanup() {
        if (gamepad) {
            SDL_GameControllerClose(gamepad);
        }
        if (renderer) {
            SDL_DestroyRenderer(renderer);
        }
        if (window) {
            SDL_DestroyWindow(window);
        }
        SDL_Quit();
        WHBUnmountSdCard();
        WHBProcShutdown();
    }
};

int main(int argc, char* args[]) {
    App app;
    
    if (!app.init()) {
        SDL_Log("Failed to initialize!\n");
        return -1;
    }
    
    app.run();
    app.cleanup();
    
    return 0;
}