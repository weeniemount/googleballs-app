#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>

#include <switch.h>

namespace
{

    constexpr int kScreenWidth = 1280;
    constexpr int kScreenHeight = 720;
    constexpr float kPointScale = 2.55f;
    constexpr float kRepelDistance = 155.0f;
    constexpr float kRepelDistanceSquared = kRepelDistance * kRepelDistance;
    constexpr float kAnalogSpeed = 14.0f;
    constexpr float kDpadSpeed = 9.0f;
    constexpr int kStickDeadzone = 4500;
    constexpr float kOffscreenMargin = 240.0f;
    constexpr float kMaximumVelocity = 80.0f;

    struct Vec3
    {
        float x;
        float y;
        float z;
    };

    struct PointData
    {
        int x;
        int y;
        int radius;
        u8 red;
        u8 green;
        u8 blue;
    };

    constexpr std::array<PointData, 65> kPointData{{
        {202, 78, 9, 0xed, 0x9d, 0x33},
        {348, 83, 9, 0xd4, 0x4d, 0x61},
        {256, 69, 9, 0x4f, 0x7a, 0xf2},
        {214, 59, 9, 0xef, 0x9a, 0x1e},
        {265, 36, 9, 0x49, 0x76, 0xf3},
        {300, 78, 9, 0x26, 0x92, 0x30},
        {294, 59, 9, 0x1f, 0x9e, 0x2c},
        {45, 88, 9, 0x1c, 0x48, 0xdd},
        {268, 52, 9, 0x2a, 0x56, 0xea},
        {73, 83, 9, 0x33, 0x55, 0xd8},
        {294, 6, 9, 0x36, 0xb6, 0x41},
        {235, 62, 9, 0x2e, 0x5d, 0xef},
        {353, 42, 8, 0xd5, 0x37, 0x47},
        {336, 52, 8, 0xeb, 0x67, 0x6f},
        {208, 41, 8, 0xf9, 0xb1, 0x25},
        {321, 70, 8, 0xde, 0x36, 0x46},
        {8, 60, 8, 0x2a, 0x59, 0xf0},
        {180, 81, 8, 0xeb, 0x9c, 0x31},
        {146, 65, 8, 0xc4, 0x17, 0x31},
        {145, 49, 8, 0xd8, 0x20, 0x38},
        {246, 34, 8, 0x5f, 0x8a, 0xf8},
        {169, 69, 8, 0xef, 0xa1, 0x1e},
        {273, 99, 8, 0x2e, 0x55, 0xe2},
        {248, 120, 8, 0x41, 0x67, 0xe4},
        {294, 41, 8, 0x0b, 0x99, 0x1a},
        {267, 114, 8, 0x48, 0x69, 0xe3},
        {78, 67, 8, 0x30, 0x59, 0xe3},
        {294, 23, 8, 0x10, 0xa1, 0x1d},
        {117, 83, 8, 0xcf, 0x40, 0x55},
        {137, 80, 8, 0xcd, 0x43, 0x59},
        {14, 71, 8, 0x28, 0x55, 0xea},
        {331, 80, 8, 0xca, 0x27, 0x3c},
        {25, 82, 8, 0x26, 0x50, 0xe1},
        {233, 46, 8, 0x4a, 0x7b, 0xf9},
        {73, 13, 8, 0x3d, 0x65, 0xe7},
        {327, 35, 6, 0xf4, 0x78, 0x75},
        {319, 46, 6, 0xf3, 0x67, 0x64},
        {256, 81, 6, 0x1d, 0x4e, 0xeb},
        {244, 88, 6, 0x69, 0x8b, 0xf1},
        {194, 32, 6, 0xfa, 0xc6, 0x52},
        {97, 56, 6, 0xee, 0x52, 0x57},
        {105, 75, 6, 0xcf, 0x2a, 0x3f},
        {42, 4, 6, 0x56, 0x81, 0xf5},
        {10, 27, 6, 0x45, 0x77, 0xf6},
        {166, 55, 6, 0xf7, 0xb3, 0x26},
        {266, 88, 6, 0x2b, 0x58, 0xe8},
        {178, 34, 6, 0xfa, 0xcb, 0x5e},
        {100, 65, 6, 0xe0, 0x2e, 0x3d},
        {343, 32, 6, 0xf1, 0x6d, 0x6f},
        {59, 5, 6, 0x50, 0x7b, 0xf2},
        {27, 9, 6, 0x56, 0x83, 0xf7},
        {233, 116, 6, 0x31, 0x58, 0xe2},
        {123, 32, 6, 0xf0, 0x69, 0x6c},
        {6, 38, 6, 0x37, 0x69, 0xf6},
        {63, 62, 6, 0x60, 0x84, 0xef},
        {6, 49, 6, 0x2a, 0x5c, 0xf4},
        {108, 36, 6, 0xf4, 0x71, 0x6e},
        {169, 43, 6, 0xf8, 0xc2, 0x47},
        {137, 37, 6, 0xe7, 0x46, 0x53},
        {318, 58, 6, 0xec, 0x41, 0x47},
        {226, 100, 5, 0x48, 0x76, 0xf1},
        {101, 46, 5, 0xef, 0x5c, 0x5c},
        {226, 108, 5, 0x25, 0x52, 0xea},
        {17, 17, 5, 0x47, 0x79, 0xf7},
        {232, 93, 5, 0x4b, 0x78, 0xf1},
    }};

    class Point
    {
    public:
        Point() = default;
        Point(float x, float y, float radius, u8 red, u8 green, u8 blue)
            : position_{x, y, 1.0f}, original_{x, y, 1.0f}, target_{x, y, 1.0f}, velocity_{0, 0, 0},
              base_radius_(radius), radius_(radius), color_(RGBA8_MAXALPHA(red, green, blue)) {}

        void update(float cursor_x, float cursor_y, float delta_seconds)
        {
            const float dx = cursor_x - position_.x;
            const float dy = cursor_y - position_.y;
            if (dx * dx + dy * dy < kRepelDistanceSquared)
            {
                target_.x = position_.x - dx;
                target_.y = position_.y - dy;
            }
            else
            {
                target_.x = original_.x;
                target_.y = original_.y;
            }

            const float time_scale = delta_seconds / 0.03f;
            updateAxis(position_.x, target_.x, velocity_.x, time_scale, 0.1f, 0.01f);
            updateAxis(position_.y, target_.y, velocity_.y, time_scale, 0.1f, 0.01f);
            const float origin_dx = original_.x - position_.x;
            const float origin_dy = original_.y - position_.y;
            target_.z = std::sqrt(origin_dx * origin_dx + origin_dy * origin_dy) / 100.0f + 1.0f;
            updateAxis(position_.z, target_.z, velocity_.z, time_scale, 0.01f, 0.001f);

            if (!std::isfinite(position_.x) || !std::isfinite(position_.y) ||
                !std::isfinite(position_.z) || !std::isfinite(velocity_.x) ||
                !std::isfinite(velocity_.y) || !std::isfinite(velocity_.z))
            {
                reset();
                return;
            }

            position_.x = std::clamp(position_.x, -kOffscreenMargin,
                                     kScreenWidth + kOffscreenMargin);
            position_.y = std::clamp(position_.y, -kOffscreenMargin,
                                     kScreenHeight + kOffscreenMargin);
            velocity_.x = std::clamp(velocity_.x, -kMaximumVelocity, kMaximumVelocity);
            velocity_.y = std::clamp(velocity_.y, -kMaximumVelocity, kMaximumVelocity);
            velocity_.z = std::clamp(velocity_.z, -kMaximumVelocity, kMaximumVelocity);
            radius_ = std::max(1.0f, std::min(base_radius_ * position_.z, 120.0f));
        }

        void reset()
        {
            position_ = original_;
            target_ = original_;
            velocity_ = {0, 0, 0};
            radius_ = base_radius_;
        }

        float x() const { return position_.x; }
        float y() const { return position_.y; }
        float radius() const { return radius_; }
        u32 color() const { return color_; }

    private:
        static void updateAxis(float &position, float target, float &velocity, float time_scale,
                               float position_threshold, float velocity_threshold)
        {
            const float delta = target - position;
            velocity += delta * 0.1f * time_scale;
            velocity *= std::pow(0.8f, time_scale);
            if (std::fabs(delta) < position_threshold && std::fabs(velocity) < velocity_threshold)
            {
                position = target;
                velocity = 0;
            }
            else
            {
                position += velocity * time_scale;
            }
        }

        Vec3 position_{};
        Vec3 original_{};
        Vec3 target_{};
        Vec3 velocity_{};
        float base_radius_ = 0;
        float radius_ = 0;
        u32 color_ = 0;
    };

    class App
    {
    public:
        App()
        {
            const float offset_x = (kScreenWidth - 353.0f * kPointScale) * 0.5f;
            const float offset_y = (kScreenHeight - 120.0f * kPointScale) * 0.5f;
            for (std::size_t i = 0; i < kPointData.size(); ++i)
            {
                const PointData &p = kPointData[i];
                points_[i] = Point(offset_x + p.x * kPointScale, offset_y + p.y * kPointScale,
                                   p.radius * kPointScale, p.red, p.green, p.blue);
            }
        }

        bool initialize()
        {
            const Result result = framebufferCreate(&framebuffer_, nwindowGetDefault(), kScreenWidth,
                                                    kScreenHeight, PIXEL_FORMAT_RGBA_8888, 2);
            if (R_FAILED(result))
                return false;
            framebufferMakeLinear(&framebuffer_);
            padConfigureInput(1, HidNpadStyleSet_NpadStandard);
            padInitializeDefault(&pad_);
            return true;
        }

        void run()
        {
            u64 last_tick = armGetSystemTick();
            while (appletMainLoop())
            {
                if (!handleInput())
                    break;
                const u64 current_tick = armGetSystemTick();
                const float delta_seconds = std::min(
                    static_cast<float>(armTicksToNs(current_tick - last_tick)) / 1000000000.0f,
                    0.1f);
                last_tick = current_tick;
                for (Point &point : points_)
                    point.update(cursor_x_, cursor_y_, delta_seconds);
                render();
            }
        }

        void shutdown() { framebufferClose(&framebuffer_); }

    private:
        bool handleInput()
        {
            padUpdate(&pad_);
            const u64 down = padGetButtonsDown(&pad_);
            const u64 held = padGetButtons(&pad_);
            if (down & HidNpadButton_Plus)
                return false;
            if (down & HidNpadButton_Minus)
                dark_background_ = !dark_background_;
            if (down & HidNpadButton_A)
                for (Point &point : points_)
                    point.reset();

            HidTouchScreenState touch{};
            if (hidGetTouchScreenStates(&touch, 1) > 0 && touch.count > 0)
            {
                cursor_x_ = touch.touches[0].x;
                cursor_y_ = touch.touches[0].y;
                cursor_active_ = true;
            }
            else
            {
                bool moved = moveFromStick(padGetStickPos(&pad_, 0));
                moved |= moveFromStick(padGetStickPos(&pad_, 1));
                if (held & HidNpadButton_Left)
                {
                    cursor_x_ -= kDpadSpeed;
                    moved = true;
                }
                if (held & HidNpadButton_Right)
                {
                    cursor_x_ += kDpadSpeed;
                    moved = true;
                }
                if (held & HidNpadButton_Up)
                {
                    cursor_y_ -= kDpadSpeed;
                    moved = true;
                }
                if (held & HidNpadButton_Down)
                {
                    cursor_y_ += kDpadSpeed;
                    moved = true;
                }
                cursor_active_ |= moved;
            }

            cursor_x_ = std::max(0.0f, std::min(cursor_x_, float(kScreenWidth - 1)));
            cursor_y_ = std::max(0.0f, std::min(cursor_y_, float(kScreenHeight - 1)));
            return true;
        }

        bool moveFromStick(const HidAnalogStickState &stick)
        {
            if (std::abs(stick.x) <= kStickDeadzone && std::abs(stick.y) <= kStickDeadzone)
                return false;
            cursor_x_ += float(stick.x) / 32768.0f * kAnalogSpeed;
            cursor_y_ -= float(stick.y) / 32768.0f * kAnalogSpeed;
            return true;
        }

        void render()
        {
            u32 stride = 0;
            u32 *pixels = static_cast<u32 *>(framebufferBegin(&framebuffer_, &stride));
            const int pitch = stride / sizeof(u32);
            const u32 background = dark_background_ ? RGBA8_MAXALPHA(26, 26, 26)
                                                    : RGBA8_MAXALPHA(255, 255, 255);
            for (int y = 0; y < kScreenHeight; ++y)
                std::fill_n(pixels + y * pitch, kScreenWidth, background);
            for (const Point &point : points_)
                drawCircle(pixels, pitch, std::lround(point.x()), std::lround(point.y()),
                           std::lround(point.radius()), point.color());
            if (cursor_active_)
                drawCursor(pixels, pitch, int(cursor_x_), int(cursor_y_));
            framebufferEnd(&framebuffer_);
        }

        static void drawCircle(u32 *pixels, int pitch, int cx, int cy, int radius, u32 color)
        {
            radius = std::clamp(radius, 1, 120);
            if (cx + radius < 0 || cx - radius >= kScreenWidth ||
                cy + radius < 0 || cy - radius >= kScreenHeight)
            {
                return;
            }

            const int min_y = std::max(-radius, -cy);
            const int max_y = std::min(radius, kScreenHeight - 1 - cy);
            if (min_y > max_y)
                return;

            const int radius_squared = radius * radius;
            for (int y = min_y; y <= max_y; ++y)
            {
                const int remaining = radius_squared - y * y;
                if (remaining < 0)
                    continue;
                const int half_width = std::sqrt(float(remaining));
                const int start = std::max(0, cx - half_width);
                const int end = std::min(kScreenWidth - 1, cx + half_width);
                if (start > end)
                    continue;
                std::fill(pixels + (cy + y) * pitch + start,
                          pixels + (cy + y) * pitch + end + 1, color);
            }
        }

        void drawCursor(u32 *pixels, int pitch, int x, int y) const
        {
            const u32 outline = dark_background_ ? RGBA8_MAXALPHA(255, 255, 255)
                                                 : RGBA8_MAXALPHA(0, 0, 0);
            drawLine(pixels, pitch, x - 13, y, x - 4, y, outline);
            drawLine(pixels, pitch, x + 4, y, x + 13, y, outline);
            drawLine(pixels, pitch, x, y - 13, x, y - 4, outline);
            drawLine(pixels, pitch, x, y + 4, x, y + 13, outline);
            drawCircle(pixels, pitch, x, y, 2, RGBA8_MAXALPHA(128, 128, 128));
        }

        static void drawLine(u32 *pixels, int pitch, int x0, int y0, int x1, int y1, u32 color)
        {
            if (y0 == y1 && y0 >= 0 && y0 < kScreenHeight)
            {
                const int start = std::max(0, std::min(x0, x1));
                const int end = std::min(kScreenWidth - 1, std::max(x0, x1));
                if (start <= end)
                    std::fill(pixels + y0 * pitch + start, pixels + y0 * pitch + end + 1, color);
            }
            else if (x0 == x1 && x0 >= 0 && x0 < kScreenWidth)
            {
                const int start = std::max(0, std::min(y0, y1));
                const int end = std::min(kScreenHeight - 1, std::max(y0, y1));
                for (int y = start; y <= end; ++y)
                    pixels[y * pitch + x0] = color;
            }
        }

        Framebuffer framebuffer_{};
        PadState pad_{};
        std::array<Point, kPointData.size()> points_{};
        float cursor_x_ = kScreenWidth * 0.5f;
        float cursor_y_ = kScreenHeight * 0.5f;
        bool cursor_active_ = true;
        bool dark_background_ = false;
    };

} // namespace

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;
    App app;
    if (!app.initialize())
        return EXIT_FAILURE;
    app.run();
    app.shutdown();
    return EXIT_SUCCESS;
}
