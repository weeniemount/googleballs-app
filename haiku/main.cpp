#include <AppFileInfo.h>
#include <Application.h>
#include <Bitmap.h>
#include <File.h>
#include <OS.h>
#include <Path.h>
#include <Resources.h>
#include <Roster.h>
#include <Screen.h>
#include <TranslationUtils.h>
#include <View.h>
#include <Window.h>
#include <cmath>
#include <cstring>
#include <string>
#include <vector>

const uint32 MSG_PULSE = 'puls';

struct Vector3 {
  double x, y, z;

  Vector3(double x = 0, double y = 0, double z = 0) : x(x), y(y), z(z) {}

  void set(double newX, double newY, double newZ = 0) {
    x = newX;
    y = newY;
    z = newZ;
  }
};

struct Color {
  uint8 r, g, b, a;

  Color(uint8 r = 255, uint8 g = 255, uint8 b = 255, uint8 a = 255)
      : r(r), g(g), b(b), a(a) {}

  static Color fromHex(const std::string &hex) {
    if (hex[0] == '#') {
      unsigned int value = std::stoul(hex.substr(1), nullptr, 16);
      return Color((value >> 16) & 0xFF, (value >> 8) & 0xFF, value & 0xFF,
                   255);
    }
    return Color();
  }

  rgb_color toRGBColor() const {
    rgb_color c;
    c.red = r;
    c.green = g;
    c.blue = b;
    c.alpha = a;
    return c;
  }
};

class Point {
public:
  Vector3 curPos, originalPos, targetPos, velocity;
  Color color;
  double radius, size;
  double friction = 0.8;
  double springStrength = 0.1;

  Point(double x, double y, double z, double size, const std::string &colorHex)
      : curPos(x, y, z), originalPos(x, y, z), targetPos(x, y, z),
        velocity(0, 0, 0), size(size), radius(size) {
    color = Color::fromHex(colorHex);
  }

  void update(double dt) {
    // Time scaling factor (target 30fps = ~0.033s)
    double timeScale = dt / 0.033;

    // X axis spring physics
    double dx = targetPos.x - curPos.x;
    double ax = dx * springStrength * timeScale;
    velocity.x += ax;
    velocity.x *= std::pow(friction, timeScale);

    if (std::abs(dx) < 0.1 && std::abs(velocity.x) < 0.01) {
      curPos.x = targetPos.x;
      velocity.x = 0;
    } else {
      curPos.x += velocity.x * timeScale;
    }

    // Y axis spring physics
    double dy = targetPos.y - curPos.y;
    double ay = dy * springStrength * timeScale;
    velocity.y += ay;
    velocity.y *= std::pow(friction, timeScale);

    if (std::abs(dy) < 0.1 && std::abs(velocity.y) < 0.01) {
      curPos.y = targetPos.y;
      velocity.y = 0;
    } else {
      curPos.y += velocity.y * timeScale;
    }

    // Z axis (depth)
    double dox = originalPos.x - curPos.x;
    double doy = originalPos.y - curPos.y;
    double dd = (dox * dox) + (doy * doy);
    double d = std::sqrt(dd);

    targetPos.z = d / 100.0 + 1.0;
    double dz = targetPos.z - curPos.z;
    double az = dz * springStrength * timeScale;
    velocity.z += az;
    velocity.z *= std::pow(friction, timeScale);

    if (std::abs(dz) < 0.01 && std::abs(velocity.z) < 0.001) {
      curPos.z = targetPos.z;
      velocity.z = 0;
    } else {
      curPos.z += velocity.z * timeScale;
    }

    radius = size * curPos.z;
    if (radius < 1)
      radius = 1;
  }

  void draw(BView *view) {
    rgb_color c = color.toRGBColor();
    view->SetHighColor(c);
    view->SetPenSize(0); // Fill doesn't use pen size, but good practice

    // Use native FillEllipse for better performance
    // Haiku coordinates are center of pixel, so simple arithmetic works
    BRect rect(curPos.x - radius, curPos.y - radius, curPos.x + radius,
               curPos.y + radius);
    view->FillEllipse(rect);
  }
};

struct PointData {
  int x, y;
  int size;
  std::string color;
};

static void computeBounds(const std::vector<PointData> &data, double &w,
                          double &h) {
  int minX = 99999, maxX = -99999;
  int minY = 99999, maxY = -99999;

  for (const auto &p : data) {
    if (p.x < minX)
      minX = p.x;
    if (p.x > maxX)
      maxX = p.x;
    if (p.y < minY)
      minY = p.y;
    if (p.y > maxY)
      maxY = p.y;
  }

  w = maxX - minX;
  h = maxY - minY;
}

class PointCollection {
public:
  Vector3 mousePos;
  std::vector<Point> points;

  PointCollection() : mousePos(0, 0, 0) {}

  void addPoint(double x, double y, double z, double size,
                const std::string &color) {
    points.emplace_back(x, y, z, size, color);
  }

  void update(double dt) {
    for (auto &point : points) {
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

      point.update(dt);
    }
  }

  void draw(BView *view) {
    for (auto &point : points) {
      point.draw(view);
    }
  }
};

class BallsView : public BView {
public:
  BallsView(BRect frame)
      : BView(frame, "BallsView", B_FOLLOW_ALL_SIDES,
              B_WILL_DRAW | B_FRAME_EVENTS | B_PULSE_NEEDED),
        fOffscreenBitmap(nullptr), fOffscreenView(nullptr), fLastTime(0) {
    SetViewColor(B_TRANSPARENT_COLOR);
    initPoints();
  }

  ~BallsView() { delete fOffscreenBitmap; }

  virtual void AttachedToWindow() {
    BView::AttachedToWindow();
    SetEventMask(B_POINTER_EVENTS, B_NO_POINTER_HISTORY);
    fLastTime = system_time();
    _InitDoubleBuffering();
  }

  virtual void Draw(BRect updateRect) {
    if (!fOffscreenBitmap || !fOffscreenBitmap->IsValid())
      return;

    if (fOffscreenBitmap->Lock()) {
      fOffscreenView->SetHighColor(255, 255, 255);
      fOffscreenView->FillRect(fOffscreenView->Bounds());

      fOffscreenView->SetDrawingMode(B_OP_ALPHA);
      pointCollection.draw(fOffscreenView);

      fOffscreenView->Sync();
      fOffscreenBitmap->Unlock();
    }

    DrawBitmap(fOffscreenBitmap, BPoint(0, 0));
  }

  virtual void MouseMoved(BPoint where, uint32 code,
                          const BMessage *dragMessage) {
    pointCollection.mousePos.set(where.x, where.y);
  }

  virtual void FrameResized(float width, float height) {
    BView::FrameResized(width, height);
    recenterPoints(width, height);
    _InitDoubleBuffering();
  }

  virtual void Pulse() {
    bigtime_t now = system_time();
    double dt = (now - fLastTime) / 1000000.0;
    fLastTime = now;

    if (dt > 0.1)
      dt = 0.1;

    pointCollection.update(dt);
    Invalidate();
  }

private:
  PointCollection pointCollection;
  BBitmap *fOffscreenBitmap;
  BView *fOffscreenView;
  bigtime_t fLastTime;

  void _InitDoubleBuffering() {
    delete fOffscreenBitmap;
    fOffscreenBitmap = new BBitmap(Bounds(), B_RGBA32, true);
    if (fOffscreenBitmap->IsValid()) {
      fOffscreenView =
          new BView(Bounds(), "offscreen", B_FOLLOW_ALL, B_WILL_DRAW);
      fOffscreenBitmap->AddChild(fOffscreenView);
    } else {
      delete fOffscreenBitmap;
      fOffscreenBitmap = nullptr;
      fOffscreenView = nullptr;
    }
  }

  void initPoints() {
    std::vector<PointData> pointData = {
        {202, 78, 9, "#ed9d33"},  {348, 83, 9, "#d44d61"},
        {256, 69, 9, "#4f7af2"},  {214, 59, 9, "#ef9a1e"},
        {265, 36, 9, "#4976f3"},  {300, 78, 9, "#269230"},
        {294, 59, 9, "#1f9e2c"},  {45, 88, 9, "#1c48dd"},
        {268, 52, 9, "#2a56ea"},  {73, 83, 9, "#3355d8"},
        {294, 6, 9, "#36b641"},   {235, 62, 9, "#2e5def"},
        {353, 42, 8, "#d53747"},  {336, 52, 8, "#eb676f"},
        {208, 41, 8, "#f9b125"},  {321, 70, 8, "#de3646"},
        {8, 60, 8, "#2a59f0"},    {180, 81, 8, "#eb9c31"},
        {146, 65, 8, "#c41731"},  {145, 49, 8, "#d82038"},
        {246, 34, 8, "#5f8af8"},  {169, 69, 8, "#efa11e"},
        {273, 99, 8, "#2e55e2"},  {248, 120, 8, "#4167e4"},
        {294, 41, 8, "#0b991a"},  {267, 114, 8, "#4869e3"},
        {78, 67, 8, "#3059e3"},   {294, 23, 8, "#10a11d"},
        {117, 83, 8, "#cf4055"},  {137, 80, 8, "#cd4359"},
        {14, 71, 8, "#2855ea"},   {331, 80, 8, "#ca273c"},
        {25, 82, 8, "#2650e1"},   {233, 46, 8, "#4a7bf9"},
        {73, 13, 8, "#3d65e7"},   {327, 35, 6, "#f47875"},
        {319, 46, 6, "#f36764"},  {256, 81, 6, "#1d4eeb"},
        {244, 88, 6, "#698bf1"},  {194, 32, 6, "#fac652"},
        {97, 56, 6, "#ee5257"},   {105, 75, 6, "#cf2a3f"},
        {42, 4, 6, "#5681f5"},    {10, 27, 6, "#4577f6"},
        {166, 55, 6, "#f7b326"},  {266, 88, 6, "#2b58e8"},
        {178, 34, 6, "#facb5e"},  {100, 65, 6, "#e02e3d"},
        {343, 32, 6, "#f16d6f"},  {59, 5, 6, "#507bf2"},
        {27, 9, 6, "#5683f7"},    {233, 116, 6, "#3158e2"},
        {123, 32, 6, "#f0696c"},  {6, 38, 6, "#3769f6"},
        {63, 62, 6, "#6084ef"},   {6, 49, 6, "#2a5cf4"},
        {108, 36, 6, "#f4716e"},  {169, 43, 6, "#f8c247"},
        {137, 37, 6, "#e74653"},  {318, 58, 6, "#ec4147"},
        {226, 100, 5, "#4876f1"}, {101, 46, 5, "#ef5c5c"},
        {226, 108, 5, "#2552ea"}, {17, 17, 5, "#4779f7"},
        {232, 93, 5, "#4b78f1"}};

    BRect bounds = Bounds();
    double logoW, logoH;
    computeBounds(pointData, logoW, logoH);

    double offsetX = (bounds.Width() / 2.0) - (logoW / 2.0);
    double offsetY = (bounds.Height() / 2.0) - (logoH / 2.0);

    for (const auto &data : pointData) {
      double x = offsetX + data.x;
      double y = offsetY + data.y;
      pointCollection.addPoint(x, y, 0.0, static_cast<double>(data.size),
                               data.color);
    }
  }

  void recenterPoints(float width, float height) {
    for (auto &point : pointCollection.points) {
      double relX = point.originalPos.x - (width / 2 - 180);
      double relY = point.originalPos.y - (height / 2 - 65);
      point.originalPos.x = (width / 2 - 180) + relX;
      point.originalPos.y = (height / 2 - 65) + relY;
      point.curPos.x = point.originalPos.x;
      point.curPos.y = point.originalPos.y;
    }
  }
};

class BallsWindow : public BWindow {
public:
  BallsWindow()
      : BWindow(BRect(100, 100, 900, 700), "Google Balls", B_TITLED_WINDOW,
                B_ASYNCHRONOUS_CONTROLS | B_QUIT_ON_WINDOW_CLOSE) {

    BRect bounds = Bounds();
    fView = new BallsView(bounds);
    AddChild(fView);

    // Start pulse - 30ms = ~33 fps
    SetPulseRate(16000);
  }

  virtual void MessageReceived(BMessage *message) {
    switch (message->what) {
    default:
      BWindow::MessageReceived(message);
      break;
    }
  }

  virtual bool QuitRequested() {
    be_app->PostMessage(B_QUIT_REQUESTED);
    return true;
  }

private:
  BallsView *fView;
};

class BallsApp : public BApplication {
public:
  BallsApp() : BApplication("application/x-vnd.GoogleBalls") {
    fWindow = new BallsWindow();
    fWindow->Show();
  }

private:
  BallsWindow *fWindow;
};

int main() {
  BallsApp app;
  app.Run();
  return 0;
}
