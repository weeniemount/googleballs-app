
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <time.h>
#include <wayland-client.h>
#include <vector>
#include <cmath>
#include <string>
#include <algorithm>
#include <iostream>

#include "xdg-shell-client-protocol.h"
#include "xdg-decoration-client-protocol.h"

struct Vector3 {
    double x, y, z;
    Vector3(double x = 0, double y = 0, double z = 0) : x(x), y(y), z(z) {}
    void set(double newX, double newY, double newZ = 0) { x = newX; y = newY; z = newZ; }
};

struct Color {
    uint8_t r, g, b, a;
    Color(uint8_t r = 255, uint8_t g = 255, uint8_t b = 255, uint8_t a = 255) 
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
    Vector3 curPos, prevPos, originalPos, targetPos, velocity;
    Color color;
    double radius, size;
    double friction = 0.8;
    double springStrength = 0.1;
    
    Point(double x, double y, double z, double size, const std::string& colorHex)
        : curPos(x, y, z), prevPos(x, y, z), originalPos(x, y, z), targetPos(x, y, z),
          velocity(0, 0, 0), color(Color::fromHex(colorHex)), radius(size), size(size) {
    }
    
    void update() {
        prevPos = curPos; // Store state before update
        
        double dx = targetPos.x - curPos.x;
        double ax = dx * springStrength;
        velocity.x += ax; velocity.x *= friction;
        if (std::abs(dx) < 0.1 && std::abs(velocity.x) < 0.01) { curPos.x = targetPos.x; velocity.x = 0; } else { curPos.x += velocity.x; }
        
        double dy = targetPos.y - curPos.y;
        double ay = dy * springStrength;
        velocity.y += ay; velocity.y *= friction;
        if (std::abs(dy) < 0.1 && std::abs(velocity.y) < 0.01) { curPos.y = targetPos.y; velocity.y = 0; } else { curPos.y += velocity.y; }
        
        double dox = originalPos.x - curPos.x;
        double doy = originalPos.y - curPos.y;
        double dd = (dox * dox) + (doy * doy);
        double d = std::sqrt(dd);
        
        targetPos.z = d / 100.0 + 1.0;
        double dz = targetPos.z - curPos.z;
        double az = dz * springStrength;
        velocity.z += az; velocity.z *= friction;
        if (std::abs(dz) < 0.01 && std::abs(velocity.z) < 0.001) { curPos.z = targetPos.z; velocity.z = 0; } else { curPos.z += velocity.z; }
        
        radius = size * curPos.z;
        if (radius < 1) radius = 1;
    }

    // Helper to blend colors manually with position interpolation
    void draw(uint32_t* buffer, int width, int height, double alpha) {
        // Linear interpolation: state = prev * (1-alpha) + cur * alpha
        double ix = prevPos.x * (1.0 - alpha) + curPos.x * alpha;
        double iy = prevPos.y * (1.0 - alpha) + curPos.y * alpha;
        // Radius interpolation is tricky because z is used, but z is also interpolated, so this should work
        // Using current radius for simplicity, or interpolate radius too?
        // Let's recompute radius from interpolated Z for absolute correctness? 
        // Or simpler: just interpolate curPos.z?
        double iz = prevPos.z * (1.0 - alpha) + curPos.z * alpha;
        double ir = size * iz;
        if (ir < 1) ir = 1;
        
        int x0 = static_cast<int>(ix);
        int y0 = static_cast<int>(iy);
        double r = ir;
        
        int minX = std::max(0, static_cast<int>(x0 - r - 2));
        int maxX = std::min(width - 1, static_cast<int>(x0 + r + 2));
        int minY = std::max(0, static_cast<int>(y0 - r - 2));
        int maxY = std::min(height - 1, static_cast<int>(y0 + r + 2));

        for (int y = minY; y <= maxY; y++) {
            for (int x = minX; x <= maxX; x++) {
                double dx = x - ix;
                double dy = y - iy;
                double dist = std::sqrt(dx*dx + dy*dy);
                
                double alphaFactor = 0.0;
                if (dist < r - 0.5) alphaFactor = 1.0;
                else if (dist < r + 0.5) alphaFactor = 1.0 - (dist - (r - 0.5));
                
                if (alphaFactor > 0.0) {
                    uint32_t& pixel = buffer[y * width + x];
                    
                    uint8_t bgB = pixel & 0xFF;
                    uint8_t bgG = (pixel >> 8) & 0xFF;
                    uint8_t bgR = (pixel >> 16) & 0xFF;
                    
                    uint8_t srcR = color.r;
                    uint8_t srcG = color.g;
                    uint8_t srcB = color.b;
                    
                    double a = (color.a / 255.0) * alphaFactor;
                    
                    uint8_t outR = static_cast<uint8_t>(srcR * a + bgR * (1.0 - a));
                    uint8_t outG = static_cast<uint8_t>(srcG * a + bgG * (1.0 - a));
                    uint8_t outB = static_cast<uint8_t>(srcB * a + bgB * (1.0 - a));
                    
                    pixel = (0xFF << 24) | (outR << 16) | (outG << 8) | outB;
                }
            }
        }
    }
};

// ... PointData and computeBounds ...
// I will just paste the rest of the file logic but modifying where needed

struct PointData { int x, y; int size; std::string color; };
static void computeBounds(const std::vector<PointData>& data, double& w, double& h) {
    int minX = 99999, maxX = -99999;
    int minY = 99999, maxY = -99999;
    for (const auto& p : data) {
        if (p.x < minX) minX = p.x; 
        if (p.x > maxX) maxX = p.x;
        if (p.y < minY) minY = p.y; 
        if (p.y > maxY) maxY = p.y;
    }
    w = maxX - minX; h = maxY - minY;
}

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
                point.targetPos.x = point.curPos.x - dx;
                point.targetPos.y = point.curPos.y - dy;
            } else {
                point.targetPos.x = point.originalPos.x;
                point.targetPos.y = point.originalPos.y;
            }
            point.update();
        }
    }
    
    void draw(uint32_t* buffer, int width, int height, double alpha) {
        for (auto& point : points) point.draw(buffer, width, height, alpha);
    }
};

static struct wl_display *display;
static struct wl_compositor *compositor;
static struct wl_shm *shm;
static struct xdg_wm_base *xdg_wm_base;
static struct zxdg_decoration_manager_v1 *decoration_manager;
static struct wl_seat *seat;
static struct wl_surface *surface;
static struct xdg_surface *xdg_surface;
static struct xdg_toplevel *xdg_toplevel;
static struct wl_pointer *pointer;

static int width = 800, height = 600;
static bool running = true;
static int32_t pointer_x = 0, pointer_y = 0;

static PointCollection pointCollection;
static bool pointsInitialized = false;

static void randname(char *buf) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    long r = ts.tv_nsec;
    for (int i = 0; i < 6; ++i) {
        buf[i] = 'A' + (r & 15) + (r & 16) * 2;
        r >>= 5;
    }
}

static int create_shm_file(off_t size) {
    char name[] = "/wl_shm-XXXXXX";
    int retries = 100;
    do {
        randname(name + sizeof(name) - 7);
        --retries;
        int fd = shm_open(name, O_RDWR | O_CREAT | O_EXCL, 0600);
        if (fd >= 0) {
            shm_unlink(name);
            if (ftruncate(fd, size) < 0) {
                close(fd);
                return -1;
            }
            return fd;
        }
    } while (retries > 0 && errno == EEXIST);
    return -1;
}

static void xdg_wm_base_ping(void *data, struct xdg_wm_base *xdg_wm_base, uint32_t serial) {
    xdg_wm_base_pong(xdg_wm_base, serial);
}
static const struct xdg_wm_base_listener xdg_wm_base_listener = { xdg_wm_base_ping };

static struct wl_buffer *create_buffer(int width, int height, uint32_t **data_out) {
    int stride = width * 4;
    int size = stride * height;
    int fd = create_shm_file(size);
    if (fd < 0) {
        fprintf(stderr, "creating a buffer file for %d B failed: %m\n", size);
        return NULL;
    }
    void *data = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (data == MAP_FAILED) {
        fprintf(stderr, "mmap failed: %m\n");
        close(fd);
        return NULL;
    }
    struct wl_shm_pool *pool = wl_shm_create_pool(shm, fd, size);
    struct wl_buffer *buffer = wl_shm_pool_create_buffer(pool, 0, width, height, stride, WL_SHM_FORMAT_ARGB8888);
    wl_shm_pool_destroy(pool);
    close(fd);
    *data_out = (uint32_t*)data;
    return buffer;
}

static void initPoints() {
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
    
    double logoW, logoH;
    computeBounds(pointData, logoW, logoH);
    double offsetX = (width / 2.0) - (logoW / 2.0);
    double offsetY = (height / 2.0) - (logoH / 2.0);

    pointCollection.points.clear();
    for (const auto& data : pointData) {
        double x = offsetX + data.x;
        double y = offsetY + data.y;
        pointCollection.addPoint(x, y, 0.0, static_cast<double>(data.size), data.color);
    }
}

static void xdg_surface_configure(void *data, struct xdg_surface *xdg_surface, uint32_t serial) {
    xdg_surface_ack_configure(xdg_surface, serial);
    if (!pointsInitialized) {
        initPoints();
        pointsInitialized = true;
    }
}
static const struct xdg_surface_listener xdg_surface_listener = { xdg_surface_configure };

static void xdg_toplevel_configure(void *data, struct xdg_toplevel *xdg_toplevel, int32_t w, int32_t h, struct wl_array *states) {
    if (w > 0 && h > 0 && (w != width || h != height)) {
        width = w;
        height = h;
        initPoints();
    }
}
static void xdg_toplevel_close(void *data, struct xdg_toplevel *xdg_toplevel) { running = false; }
static const struct xdg_toplevel_listener xdg_toplevel_listener = { xdg_toplevel_configure, xdg_toplevel_close };

static void pointer_enter(void *data, struct wl_pointer *wl_pointer, uint32_t serial, struct wl_surface *surface, wl_fixed_t surface_x, wl_fixed_t surface_y) {
    pointer_x = wl_fixed_to_int(surface_x);
    pointer_y = wl_fixed_to_int(surface_y);
    pointCollection.mousePos.set(pointer_x, pointer_y);
}
static void pointer_leave(void *data, struct wl_pointer *wl_pointer, uint32_t serial, struct wl_surface *surface) {}
static void pointer_motion(void *data, struct wl_pointer *wl_pointer, uint32_t time, wl_fixed_t surface_x, wl_fixed_t surface_y) {
    pointer_x = wl_fixed_to_int(surface_x);
    pointer_y = wl_fixed_to_int(surface_y);
    pointCollection.mousePos.set(pointer_x, pointer_y);
}
static void pointer_button(void *data, struct wl_pointer *wl_pointer, uint32_t serial, uint32_t time, uint32_t button, uint32_t state) {}
static void pointer_axis(void *data, struct wl_pointer *wl_pointer, uint32_t time, uint32_t axis, wl_fixed_t value) {}
static const struct wl_pointer_listener pointer_listener = {
    pointer_enter, pointer_leave, pointer_motion, pointer_button, pointer_axis,
};

static void seat_capabilities(void *data, struct wl_seat *wl_seat, uint32_t capabilities) {
    if (capabilities & WL_SEAT_CAPABILITY_POINTER) {
        pointer = wl_seat_get_pointer(wl_seat);
        wl_pointer_add_listener(pointer, &pointer_listener, NULL);
    }
}
static void seat_name(void *data, struct wl_seat *wl_seat, const char *name) {}
static const struct wl_seat_listener seat_listener = { seat_capabilities, seat_name };

static void registry_global(void *data, struct wl_registry *wl_registry, uint32_t name, const char *interface, uint32_t version) {
    if (strcmp(interface, wl_compositor_interface.name) == 0) {
        compositor = (struct wl_compositor*)wl_registry_bind(wl_registry, name, &wl_compositor_interface, 1);
    } else if (strcmp(interface, wl_shm_interface.name) == 0) {
        shm = (struct wl_shm*)wl_registry_bind(wl_registry, name, &wl_shm_interface, 1);
    } else if (strcmp(interface, xdg_wm_base_interface.name) == 0) {
        xdg_wm_base = (struct xdg_wm_base*)wl_registry_bind(wl_registry, name, &xdg_wm_base_interface, 1);
        xdg_wm_base_add_listener(xdg_wm_base, &xdg_wm_base_listener, NULL);
    } else if (strcmp(interface, wl_seat_interface.name) == 0) {
        seat = (struct wl_seat*)wl_registry_bind(wl_registry, name, &wl_seat_interface, 1);
        wl_seat_add_listener(seat, &seat_listener, NULL);
    } else if (strcmp(interface, zxdg_decoration_manager_v1_interface.name) == 0) {
        decoration_manager = (struct zxdg_decoration_manager_v1*)wl_registry_bind(wl_registry, name, &zxdg_decoration_manager_v1_interface, 1);
    }
}
static void registry_global_remove(void *data, struct wl_registry *wl_registry, uint32_t name) {}
static const struct wl_registry_listener registry_listener = { registry_global, registry_global_remove };

static uint64_t get_time_ms() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000 + (uint64_t)ts.tv_nsec / 1000000;
}

int main(int argc, char **argv) {
    display = wl_display_connect(NULL);
    if (!display) { fprintf(stderr, "Failed to connect to Wayland display\n"); return 1; }
    
    struct wl_registry *registry = wl_display_get_registry(display);
    wl_registry_add_listener(registry, &registry_listener, NULL);
    wl_display_roundtrip(display);
    
    if (!compositor || !shm || !xdg_wm_base) {
        fprintf(stderr, "Missing wayland globals\n");
        return 1;
    }
    
    surface = wl_compositor_create_surface(compositor);
    xdg_surface = xdg_wm_base_get_xdg_surface(xdg_wm_base, surface);
    xdg_surface_add_listener(xdg_surface, &xdg_surface_listener, NULL);
    
    xdg_toplevel = xdg_surface_get_toplevel(xdg_surface);
    xdg_toplevel_add_listener(xdg_toplevel, &xdg_toplevel_listener, NULL);
    xdg_toplevel_set_title(xdg_toplevel, "Google Balls");
    xdg_toplevel_set_app_id(xdg_toplevel, "google-balls-wayland");

    if (decoration_manager) {
        struct zxdg_toplevel_decoration_v1 *toplevel_decoration = zxdg_decoration_manager_v1_get_toplevel_decoration(decoration_manager, xdg_toplevel);
        zxdg_toplevel_decoration_v1_set_mode(toplevel_decoration, ZXDG_TOPLEVEL_DECORATION_V1_MODE_SERVER_SIDE);
    }
    
    wl_surface_commit(surface);
    
    wl_display_roundtrip(display);
    
    uint64_t last_time = get_time_ms();
    const uint64_t physics_step_ms = 30;
    uint64_t accumulator = 0;

    while (running) {
        while (wl_display_prepare_read(display) != 0) wl_display_dispatch_pending(display);
        wl_display_flush(display);
        wl_display_read_events(display);
        wl_display_dispatch_pending(display);
        
        uint64_t current_time = get_time_ms();
        uint64_t frame_time = current_time - last_time;
        if (frame_time > 250) frame_time = 250; // Cap spiral of death
        last_time = current_time;
        
        accumulator += frame_time;
        
        while (accumulator >= physics_step_ms) {
            pointCollection.update();
            accumulator -= physics_step_ms;
        }
        
        double alpha = (double)accumulator / physics_step_ms;
        
        uint32_t *pixel_data;
        struct wl_buffer *buffer = create_buffer(width, height, &pixel_data);
        if (!buffer) continue;
        
        for (int i = 0; i < width * height; i++) pixel_data[i] = 0xFFFFFFFF;
        
        pointCollection.draw(pixel_data, width, height, alpha);
        
        wl_surface_attach(surface, buffer, 0, 0);
        wl_surface_damage(surface, 0, 0, width, height);
        wl_surface_commit(surface);
        
        wl_buffer_destroy(buffer);
        munmap(pixel_data, width * height * 4);
    }
    
    return 0;
}
