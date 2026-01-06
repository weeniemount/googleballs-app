#include <iostream>
#include <vector>
#include <cmath>
#include <string>
#include <chrono>
#include <thread>
#include <sstream>
#include <cstring>
#include <map>
#include <memory>
#include <csignal>

#ifdef _WIN32
#include <windows.h>
#include <conio.h>
#else
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#endif

// Global flag for window resize
volatile sig_atomic_t g_windowResized = 0;

#ifndef _WIN32
void handleResize(int sig) {
    g_windowResized = 1;
}
#endif

struct Vector3 {
    double x, y, z;
    Vector3(double x = 0, double y = 0, double z = 0) : x(x), y(y), z(z) {}
};

struct Color {
    uint8_t r, g, b;
    
    Color(uint8_t r = 255, uint8_t g = 255, uint8_t b = 255) : r(r), g(g), b(b) {}
    
    static Color fromHex(const std::string& hex) {
        if (hex[0] == '#') {
            unsigned int value = std::stoul(hex.substr(1), nullptr, 16);
            return Color((value >> 16) & 0xFF, (value >> 8) & 0xFF, value & 0xFF);
        }
        return Color();
    }
    
    std::string toAnsi() const {
        return "\033[38;2;" + std::to_string(r) + ";" + std::to_string(g) + ";" + std::to_string(b) + "m";
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
        
        // Z axis (depth)
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
        
        radius = size * curPos.z;
        if (radius < 0.5) radius = 0.5;
    }
};

struct PointData {
    int x, y, size;
    std::string color;
};

class TerminalCanvas {
private:
    int width, height;
    std::vector<std::string> buffer;
    std::vector<Color> colorBuffer;
    std::vector<std::pair<int, int>> dirtyPixels;
    
public:
    TerminalCanvas(int w, int h) : width(w), height(h) {
        buffer.resize(height, std::string(width, ' '));
        colorBuffer.resize(height * width, Color(255, 255, 255));
        dirtyPixels.reserve(1000);
    }
    
    void clear() {
        // Only clear pixels that were actually drawn
        for (const auto& pixel : dirtyPixels) {
            int x = pixel.first;
            int y = pixel.second;
            if (y >= 0 && y < height && x >= 0 && x < width) {
                buffer[y][x] = ' ';
                colorBuffer[y * width + x] = Color(255, 255, 255);
            }
        }
        dirtyPixels.clear();
    }
    
    void setPixel(int x, int y, const std::string& c, const Color& color) {
        if (x >= 0 && x < width && y >= 0 && y < height) {
            buffer[y][x] = c[0];
            colorBuffer[y * width + x] = color;
            dirtyPixels.push_back({x, y});
        }
    }
    
    void drawCircle(int cx, int cy, double radius, const Color& color) {
        // Use different characters based on size for better visual
        const char* chars = "●◉○◌";
        int charIdx = std::min(3, std::max(0, static_cast<int>(radius / 2)));
        std::string c(1, chars[charIdx]);
        
        int r = static_cast<int>(radius);
        for (int y = -r; y <= r; y++) {
            for (int x = -r; x <= r; x++) {
                double dist = std::sqrt(x * x + y * y);
                if (dist <= radius) {
                    setPixel(cx + x, cy + y, c, color);
                }
            }
        }
    }
    
    std::string render() {
        std::ostringstream oss;
        oss << "\033[H"; // Move cursor to home
        
        // Safety check - don't render if size is unreasonable
        if (width * height > 20000) {
            oss << "Terminal too large to render\n";
            return oss.str();
        }
        
        Color lastColor(0, 0, 0);
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                Color c = colorBuffer[y * width + x];
                if (buffer[y][x] != ' ') {
                    if (c.r != lastColor.r || c.g != lastColor.g || c.b != lastColor.b) {
                        oss << c.toAnsi();
                        lastColor = c;
                    }
                    oss << buffer[y][x];
                } else {
                    oss << ' ';
                }
            }
            if (y < height - 1) oss << '\n';
        }
        oss << "\033[0m"; // Reset color
        return oss.str();
    }
};

class Terminal {
public:
#ifdef _WIN32
    static void setupWindows() {
        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        DWORD dwMode = 0;
        GetConsoleMode(hOut, &dwMode);
        dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        SetConsoleMode(hOut, dwMode);
        
        // Hide cursor
        CONSOLE_CURSOR_INFO cursorInfo;
        GetConsoleCursorInfo(hOut, &cursorInfo);
        cursorInfo.bVisible = false;
        SetConsoleCursorInfo(hOut, &cursorInfo);
    }
    
    static void restoreWindows() {
        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        CONSOLE_CURSOR_INFO cursorInfo;
        GetConsoleCursorInfo(hOut, &cursorInfo);
        cursorInfo.bVisible = true;
        SetConsoleCursorInfo(hOut, &cursorInfo);
    }
    
    static void getSize(int& w, int& h) {
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi)) {
            w = csbi.srWindow.Right - csbi.srWindow.Left + 1;
            h = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
        } else {
            // Fallback to default size if call fails
            w = 80;
            h = 24;
        }
    }
#else
    static struct termios orig_termios;
    
    static void setupLinux() {
        struct termios raw;
        tcgetattr(STDIN_FILENO, &orig_termios);
        raw = orig_termios;
        raw.c_lflag &= ~(ECHO | ICANON);
        raw.c_cc[VMIN] = 0;  // Non-blocking read
        raw.c_cc[VTIME] = 0;
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
        
        // Hide cursor
        std::cout << "\033[?25l" << std::flush;
        
        // Set non-blocking input
        int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
        fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
        
        // Setup resize signal handler
        signal(SIGWINCH, handleResize);
    }
    
    static void restoreLinux() {
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
        std::cout << "\033[?25h" << std::flush; // Show cursor
    }
    
    static void getSize(int& w, int& h) {
        struct winsize ws;
        if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0 || ws.ws_row == 0) {
            // Fallback to default size if ioctl fails
            w = 80;
            h = 24;
        } else {
            w = ws.ws_col;
            h = ws.ws_row;
        }
    }
#endif
    
    static void setup() {
#ifdef _WIN32
        setupWindows();
#else
        setupLinux();
#endif
        std::cout << "\033[2J"; // Clear screen
    }
    
    static void restore() {
#ifdef _WIN32
        restoreWindows();
#else
        restoreLinux();
#endif
    }
};

#ifndef _WIN32
struct termios Terminal::orig_termios;
#endif

class InputManager {
private:
    std::map<int, bool> keyStates;
    bool shiftPressed = false;
    
    enum Keys {
        KEY_UP = 1,
        KEY_DOWN = 2,
        KEY_LEFT = 3,
        KEY_RIGHT = 4,
        KEY_SHIFT = 5,
        KEY_QUIT = 6
    };
    
public:
    void update() {
#ifdef _WIN32
        if (_kbhit()) {
            int c = _getch();
            
            if (c == 'q' || c == 'Q' || c == 27) {
                keyStates[KEY_QUIT] = true;
            }
            
            if (c == 224 || c == 0) { // Arrow keys
                c = _getch();
                if (c == 72) keyStates[KEY_UP] = true;
                if (c == 80) keyStates[KEY_DOWN] = true;
                if (c == 75) keyStates[KEY_LEFT] = true;
                if (c == 77) keyStates[KEY_RIGHT] = true;
            }
            
            // Check for shift (capital WASD or actual shift detection)
            if (c == 'W' || c == 'A' || c == 'S' || c == 'D') {
                shiftPressed = true;
                if (c == 'W') keyStates[KEY_UP] = true;
                if (c == 'S') keyStates[KEY_DOWN] = true;
                if (c == 'A') keyStates[KEY_LEFT] = true;
                if (c == 'D') keyStates[KEY_RIGHT] = true;
            } else if (c == 'w' || c == 'a' || c == 's' || c == 'd') {
                shiftPressed = false;
                if (c == 'w') keyStates[KEY_UP] = true;
                if (c == 's') keyStates[KEY_DOWN] = true;
                if (c == 'a') keyStates[KEY_LEFT] = true;
                if (c == 'd') keyStates[KEY_RIGHT] = true;
            }
        }
#else
        // Clear shift state each frame, will be set if shift+key detected
        bool newShift = false;
        
        char c;
        while (read(STDIN_FILENO, &c, 1) > 0) {
            if (c == 'q' || c == 'Q') {
                keyStates[KEY_QUIT] = true;
            }
            
            if (c == '\033') {
                char seq[5];
                int bytesRead = read(STDIN_FILENO, &seq[0], 1);
                if (bytesRead == 1) {
                    if (seq[0] == '[') {
                        bytesRead = read(STDIN_FILENO, &seq[1], 1);
                        if (bytesRead == 1) {
                            // Regular arrow keys
                            if (seq[1] == 'A') { keyStates[KEY_UP] = true; continue; }
                            if (seq[1] == 'B') { keyStates[KEY_DOWN] = true; continue; }
                            if (seq[1] == 'D') { keyStates[KEY_LEFT] = true; continue; }
                            if (seq[1] == 'C') { keyStates[KEY_RIGHT] = true; continue; }
                            
                            // Check for modified keys (shift+arrow)
                            if (seq[1] == '1') {
                                bytesRead = read(STDIN_FILENO, &seq[2], 1);
                                if (bytesRead == 1 && seq[2] == ';') {
                                    bytesRead = read(STDIN_FILENO, &seq[3], 1);
                                    if (bytesRead == 1 && seq[3] == '2') {
                                        bytesRead = read(STDIN_FILENO, &seq[4], 1);
                                        if (bytesRead == 1) {
                                            newShift = true;
                                            if (seq[4] == 'A') { keyStates[KEY_UP] = true; continue; }
                                            if (seq[4] == 'B') { keyStates[KEY_DOWN] = true; continue; }
                                            if (seq[4] == 'D') { keyStates[KEY_LEFT] = true; continue; }
                                            if (seq[4] == 'C') { keyStates[KEY_RIGHT] = true; continue; }
                                        }
                                    }
                                }
                            }
                        }
                    } else {
                        // ESC key pressed alone - quit
                        keyStates[KEY_QUIT] = true;
                    }
                } else {
                    // ESC with no following characters - quit
                    keyStates[KEY_QUIT] = true;
                }
            }
            
            // WASD keys
            if (c == 'W' || c == 'A' || c == 'S' || c == 'D') {
                newShift = true;
                if (c == 'W') keyStates[KEY_UP] = true;
                if (c == 'S') keyStates[KEY_DOWN] = true;
                if (c == 'A') keyStates[KEY_LEFT] = true;
                if (c == 'D') keyStates[KEY_RIGHT] = true;
            } else if (c == 'w' || c == 'a' || c == 's' || c == 'd') {
                if (c == 'w') keyStates[KEY_UP] = true;
                if (c == 's') keyStates[KEY_DOWN] = true;
                if (c == 'a') keyStates[KEY_LEFT] = true;
                if (c == 'd') keyStates[KEY_RIGHT] = true;
            }
        }
        
        shiftPressed = newShift;
#endif
    }
    
    void processMovement(double& x, double& y) {
        double speed = shiftPressed ? 4.0 : 2.0;
        double speedX = shiftPressed ? 2.0 : 1.0;
        
        if (keyStates[KEY_UP]) y -= speed;
        if (keyStates[KEY_DOWN]) y += speed;
        if (keyStates[KEY_LEFT]) x -= speedX;
        if (keyStates[KEY_RIGHT]) x += speedX;
        
        // Clear key states after processing
        keyStates.clear();
    }
    
    bool shouldQuit() {
        return keyStates[KEY_QUIT];
    }
};

class App {
private:
    std::vector<Point> points;
    Vector3 mousePos;
    int termWidth, termHeight;
    bool running;
    InputManager input;
    bool sizeChanged;
    
public:
    App() : mousePos(0, 0, 0), termWidth(80), termHeight(24), running(true), sizeChanged(false) {}
    
    void init() {
        Terminal::setup();
        Terminal::getSize(termWidth, termHeight);
        
        // Cap terminal size to prevent performance issues
        const int MAX_WIDTH = 200;
        const int MAX_HEIGHT = 60;
        termWidth = std::min(termWidth, MAX_WIDTH);
        termHeight = std::min(termHeight, MAX_HEIGHT);
        
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
        
        // Scale to terminal and center
        double scaleX = termWidth / 360.0;
        double scaleY = (termHeight * 2.0) / 120.0; // Account for char aspect ratio
        double scale = std::min(scaleX, scaleY) * 0.6;
        
        double offsetX = termWidth / 2.0 - 180 * scale;
        double offsetY = termHeight - 60 * scale;
        
        for (const auto& data : pointData) {
            double x = offsetX + data.x * scale;
            double y = offsetY + data.y * scale;
            double size = data.size * scale * 0.3;
            points.emplace_back(x, y, 0.0, size, data.color);
        }
        
        // Start mouse in center
        mousePos.x = termWidth / 2.0;
        mousePos.y = termHeight;
    }
    
    void update() {
        for (auto& point : points) {
            double dx = mousePos.x - point.curPos.x;
            double dy = mousePos.y - point.curPos.y;
            double dd = (dx * dx) + (dy * dy);
            double d = std::sqrt(dd);
            
            if (d < 15) {
                point.targetPos.x = point.curPos.x - dx;
                point.targetPos.y = point.curPos.y - dy;
            } else {
                point.targetPos.x = point.originalPos.x;
                point.targetPos.y = point.originalPos.y;
            }
            
            point.update();
        }
    }
    
    void render(TerminalCanvas& canvas) {
        canvas.clear();
        
        // Debug: print some info at top
        static int frameCount = 0;
        if (frameCount++ % 30 == 0) {  // Every 30 frames
            FILE* f = fopen("/tmp/balls_debug.txt", "a");
            if (f) {
                fprintf(f, "Mouse: %.1f,%.1f Term: %dx%d Points: %zu\n",
                       mousePos.x, mousePos.y, termWidth, termHeight, points.size());
                if (!points.empty()) {
                    fprintf(f, "First point at: %.1f,%.1f\n", 
                           points[0].curPos.x, points[0].curPos.y);
                }
                fclose(f);
            }
        }
        
        for (auto& point : points) {
            // Adjust for terminal character aspect ratio (chars are ~2x taller than wide)
            canvas.drawCircle(
                static_cast<int>(point.curPos.x),
                static_cast<int>(point.curPos.y / 2.0),
                point.radius * 0.5,
                point.color
            );
        }
        
        // Draw cursor position
        int cx = static_cast<int>(mousePos.x);
        int cy = static_cast<int>(mousePos.y / 2.0);
        canvas.setPixel(cx, cy, "✦", Color(128, 128, 128));
    }
    
    void run() {
        FILE* f = fopen("/tmp/balls_debug.txt", "w");
        if (f) {
            fprintf(f, "Starting run() with termWidth=%d, termHeight=%d\n", termWidth, termHeight);
            fclose(f);
        }
        
        std::unique_ptr<TerminalCanvas> canvas(new TerminalCanvas(termWidth, termHeight));
        
        std::cout << "\033[2J\033[H"; // Clear and home
        std::cout << "Google Balls Terminal Edition - Arrow keys/WASD to move | Hold Shift for speed boost | Q to quit\n" << std::flush;
        std::this_thread::sleep_for(std::chrono::milliseconds(1500));
        
        auto lastFrame = std::chrono::steady_clock::now();
        
        while (running) {
            // Cap terminal size to prevent performance issues
            const int MAX_WIDTH = 200;
            const int MAX_HEIGHT = 60;
            
            // Check for terminal resize
            int newWidth, newHeight;
            Terminal::getSize(newWidth, newHeight);
            
            // Cap the size
            newWidth = std::min(newWidth, MAX_WIDTH);
            newHeight = std::min(newHeight, MAX_HEIGHT);
            
            // Check if window was resized or size changed
            if (g_windowResized || newWidth != termWidth || newHeight != termHeight) {
                g_windowResized = 0;
                termWidth = newWidth;
                termHeight = newHeight;
                canvas.reset(new TerminalCanvas(termWidth, termHeight));
                std::cout << "\033[2J\033[H" << std::flush; // Clear on resize
            }
            
            // Handle input
            input.update();
            
            if (input.shouldQuit()) {
                running = false;
                break;
            }
            
            input.processMovement(mousePos.x, mousePos.y);
            
            // Clamp mouse
            mousePos.x = std::max(0.0, std::min(static_cast<double>(termWidth - 1), mousePos.x));
            mousePos.y = std::max(0.0, std::min(static_cast<double>(termHeight * 2 - 1), mousePos.y));
            
            update();
            render(*canvas);
            
            std::cout << canvas->render() << std::flush;
            
            // Frame timing (30ms like original)
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastFrame);
            if (elapsed.count() < 30) {
                std::this_thread::sleep_for(std::chrono::milliseconds(30 - elapsed.count()));
            }
            lastFrame = std::chrono::steady_clock::now();
        }
    }
    
    void cleanup() {
        Terminal::restore();
        std::cout << "\033[2J\033[H"; // Clear screen
        std::cout << "buh bye\n";
    }
};

int main() {
    App app;
    app.init();
    app.run();
    app.cleanup();
    return 0;
}