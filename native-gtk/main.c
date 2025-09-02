#include <gtk/gtk.h>
#include <cairo.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "icon/balls.h"

typedef struct {
    double x, y, z;
} Vector3;

typedef struct {
    double r, g, b, a; // 0..1
} Color;

typedef struct {
    Vector3 curPos, originalPos, targetPos, velocity;
    Color color;
    double radius, size;
    double friction;
    double springStrength;
} Point;

typedef struct {
    Vector3 mousePos;
    Point* points;
    size_t count;
} PointCollection;

typedef struct {
    GtkWidget* window;
    GtkWidget* drawing_area;
    PointCollection pc;
    gboolean running;
    int width;
    int height;
} App;

static const double TWO_PI = 6.283185307179586;

// Utilities
static Color color_from_hex(const char* hex) {
    Color c = {1.0, 1.0, 1.0, 1.0};
    if (!hex) return c;
    const char* p = hex;
    if (p[0] == '#') p++;
    unsigned int value = 0;
    if (strlen(p) == 6) {
        value = (unsigned int)strtoul(p, NULL, 16);
        c.r = ((value >> 16) & 0xFF) / 255.0;
        c.g = ((value >> 8) & 0xFF) / 255.0;
        c.b = (value & 0xFF) / 255.0;
        c.a = 1.0;
    }
    return c;
}

// Physics and rendering
static void point_update(Point* p) {
    // X axis spring physics
    double dx = p->targetPos.x - p->curPos.x;
    double ax = dx * p->springStrength;
    p->velocity.x += ax;
    p->velocity.x *= p->friction;

    if (fabs(dx) < 0.1 && fabs(p->velocity.x) < 0.01) {
        p->curPos.x = p->targetPos.x;
        p->velocity.x = 0.0;
    } else {
        p->curPos.x += p->velocity.x;
    }

    // Y axis spring physics
    double dy = p->targetPos.y - p->curPos.y;
    double ay = dy * p->springStrength;
    p->velocity.y += ay;
    p->velocity.y *= p->friction;

    if (fabs(dy) < 0.1 && fabs(p->velocity.y) < 0.01) {
        p->curPos.y = p->targetPos.y;
        p->velocity.y = 0.0;
    } else {
        p->curPos.y += p->velocity.y;
    }

    // Z axis (depth) based on distance from original position
    double dox = p->originalPos.x - p->curPos.x;
    double doy = p->originalPos.y - p->curPos.y;
    double dd = (dox * dox) + (doy * doy);
    double d = sqrt(dd);

    p->targetPos.z = d / 100.0 + 1.0;
    double dz = p->targetPos.z - p->curPos.z;
    double az = dz * p->springStrength;
    p->velocity.z += az;
    p->velocity.z *= p->friction;

    if (fabs(dz) < 0.01 && fabs(p->velocity.z) < 0.001) {
        p->curPos.z = p->targetPos.z;
        p->velocity.z = 0.0;
    } else {
        p->curPos.z += p->velocity.z;
    }

    // Update radius based on depth
    p->radius = p->size * p->curPos.z;
    if (p->radius < 1.0) p->radius = 1.0;
}

static void point_draw(Point* p, cairo_t* cr) {
    cairo_set_antialias(cr, CAIRO_ANTIALIAS_BEST);
    cairo_set_source_rgba(cr, p->color.r, p->color.g, p->color.b, p->color.a);
    cairo_new_path(cr);
    cairo_arc(cr, p->curPos.x, p->curPos.y, p->radius, 0, TWO_PI);
    cairo_fill(cr);
}

static void point_collection_update(PointCollection* pc) {
    for (size_t i = 0; i < pc->count; ++i) {
        Point* point = &pc->points[i];

        double dx = pc->mousePos.x - point->curPos.x;
        double dy = pc->mousePos.y - point->curPos.y;
        double d = sqrt(dx * dx + dy * dy);

        if (d < 150.0) {
            point->targetPos.x = point->curPos.x - dx;
            point->targetPos.y = point->curPos.y - dy;
        } else {
            point->targetPos.x = point->originalPos.x;
            point->targetPos.y = point->originalPos.y;
        }

        point_update(point);
    }
}

static void point_collection_draw(PointCollection* pc, cairo_t* cr) {
    for (size_t i = 0; i < pc->count; ++i) {
        point_draw(&pc->points[i], cr);
    }
}

// App setup
static void app_init_points(App* app) {
    typedef struct {
        int x, y;
        int size;
        const char* color;
    } PointData;

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
    static const size_t N = sizeof(pointData) / sizeof(pointData[0]);

    app->pc.points = (Point*)calloc(N, sizeof(Point));
    app->pc.count = N;

    double centerX = (app->width / 2.0) - 180.0;
    double centerY = (app->height / 2.0) - 65.0;

    for (size_t i = 0; i < N; ++i) {
        Point* p = &app->pc.points[i];
        double x = centerX + pointData[i].x;
        double y = centerY + pointData[i].y;

        p->curPos.x = x; p->curPos.y = y; p->curPos.z = 0.0;
        p->originalPos = p->curPos;
        p->targetPos = p->curPos;
        p->velocity.x = p->velocity.y = p->velocity.z = 0.0;
        p->size = (double)pointData[i].size;
        p->radius = p->size;
        p->friction = 0.8;
        p->springStrength = 0.1;
        p->color = color_from_hex(pointData[i].color);
    }
}

// GTK callbacks
static gboolean on_draw(GtkWidget* widget, cairo_t* cr, gpointer user_data) {
    App* app = (App*)user_data;
    (void)widget;

    // Background white
    cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
    cairo_paint(cr);

    // Draw points
    point_collection_draw(&app->pc, cr);

    return FALSE;
}

static gboolean on_motion_notify(GtkWidget* widget, GdkEventMotion* event, gpointer user_data) {
    App* app = (App*)user_data;
    (void)widget;
    app->pc.mousePos.x = event->x;
    app->pc.mousePos.y = event->y;
    return TRUE;
}

static void on_size_allocate(GtkWidget* widget, GdkRectangle* allocation, gpointer user_data) {
    App* app = (App*)user_data;
    (void)widget;
    int oldW = app->width;
    int oldH = app->height;

    app->width = allocation->width;
    app->height = allocation->height;

    if (app->pc.points && app->pc.count > 0 && (oldW != 0 && oldH != 0)) {
        double oldCenterX = (oldW / 2.0) - 180.0;
        double oldCenterY = (oldH / 2.0) - 65.0;
        double newCenterX = (app->width / 2.0) - 180.0;
        double newCenterY = (app->height / 2.0) - 65.0;

        for (size_t i = 0; i < app->pc.count; ++i) {
            Point* p = &app->pc.points[i];
            double relX = p->originalPos.x - oldCenterX;
            double relY = p->originalPos.y - oldCenterY;

            p->originalPos.x = newCenterX + relX;
            p->originalPos.y = newCenterY + relY;

            p->curPos.x = p->originalPos.x;
            p->curPos.y = p->originalPos.y;

            // Reset target and velocity to avoid sudden jumps on resize
            p->targetPos = p->originalPos;
            p->velocity.x = p->velocity.y = p->velocity.z = 0.0;
        }
    }
}

static gboolean on_timeout(gpointer user_data) {
    App* app = (App*)user_data;
    if (!app->running) return FALSE;

    point_collection_update(&app->pc);
    if (app->drawing_area) {
        gtk_widget_queue_draw(app->drawing_area);
    }
    return TRUE;
}

static void on_destroy(GtkWidget* widget, gpointer user_data) {
    App* app = (App*)user_data;
    (void)widget;
    app->running = FALSE;
    if (app->pc.points) {
        free(app->pc.points);
        app->pc.points = NULL;
    }
    gtk_main_quit();
}

int main(int argc, char** argv) {
    gtk_init(&argc, &argv);

    App app;
    memset(&app, 0, sizeof(App));
    app.width = 800;
    app.height = 600;
    app.running = TRUE;

    app.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(app.window), "Google Balls Desktop");
    gtk_window_set_default_size(GTK_WINDOW(app.window), app.width, app.height);

    // Load icon from embedded data
    GInputStream *stream = g_memory_input_stream_new_from_data(
        icon, icon_size, NULL);
    
    GError *error = NULL;
    GdkPixbuf *icon_pixbuf = gdk_pixbuf_new_from_stream(stream, NULL, &error);
    g_object_unref(stream);
    
    if (icon_pixbuf) {
        gtk_window_set_icon(GTK_WINDOW(app.window), icon_pixbuf);
        g_object_unref(icon_pixbuf);
    } else if (error) {
        g_warning("Failed to load icon: %s", error->message);
        g_error_free(error);
    }

    app.drawing_area = gtk_drawing_area_new();
    gtk_widget_set_hexpand(app.drawing_area, TRUE);
    gtk_widget_set_vexpand(app.drawing_area, TRUE);

    gtk_container_add(GTK_CONTAINER(app.window), app.drawing_area);

    // Signals
    g_signal_connect(app.window, "destroy", G_CALLBACK(on_destroy), &app);
    g_signal_connect(app.drawing_area, "draw", G_CALLBACK(on_draw), &app);
    g_signal_connect(app.drawing_area, "size-allocate", G_CALLBACK(on_size_allocate), &app);

    // Mouse motion events
    gtk_widget_add_events(app.drawing_area, GDK_POINTER_MOTION_MASK);
    g_signal_connect(app.drawing_area, "motion-notify-event", G_CALLBACK(on_motion_notify), &app);

    gtk_widget_show_all(app.window);

    // Initialize points after widget is realized to get actual size
    GtkAllocation alloc;
    gtk_widget_get_allocation(app.drawing_area, &alloc);
    app.width = alloc.width > 0 ? alloc.width : app.width;
    app.height = alloc.height > 0 ? alloc.height : app.height;
    app.pc.mousePos.x = app.width / 2.0;
    app.pc.mousePos.y = app.height / 2.0;

    app_init_points(&app);

    g_timeout_add(30, on_timeout, &app);

    gtk_main();
    return 0;
}