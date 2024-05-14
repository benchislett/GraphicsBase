#include <functional>
#include <string>
#include <vector>

#include "imgui.h"
#include "imgui_stdlib.h"
#include "raylib.h"
#include "raymath.h"
#include "rlImGui.h"

class WindowManagerBase {
 public:
  WindowManagerBase(int w, int h,
                    const std::string& winTitle = "example window",
                    int fps = 60)
      : winWidth(w), winHeight(h), frameid(0ULL) {
    SetTraceLogLevel(LOG_NONE);
    InitWindow(winWidth, winHeight, winTitle.c_str());
    rlImGuiSetup(true);
    initImpl();
    SetTargetFPS(fps);
  }

  ~WindowManagerBase() {
    rlImGuiShutdown();
    CloseWindow();
  }

  int getWinWidth() const { return winWidth; }
  int getWinHeight() const { return winHeight; }

  virtual void initImpl() { return; }

  void loop() {
    while (!WindowShouldClose()) {
      draw();
      loopImpl();
      frameid++;
    }
  }

  virtual void loopImpl() { return; }
  virtual void preDrawImpl() { return; }
  virtual void drawImpl() { return; }
  virtual void postDrawImpl() { return; }
  virtual void drawImGuiImpl() { return; }

 protected:
  void drawBegin() const {
    BeginDrawing();
    ClearBackground(RAYWHITE);
  }

  void drawEnd() const { EndDrawing(); }

  void drawImGuiBegin() const { rlImGuiBegin(); }

  void drawImGuiEnd() const { rlImGuiEnd(); }

  void draw() {
    preDrawImpl();
    drawBegin();
    drawImpl();
    drawImGuiBegin();
    drawImGuiImpl();
    drawImGuiEnd();
    drawEnd();
    postDrawImpl();
  }

  int winWidth;
  int winHeight;

  unsigned long long int frameid;
};

template <typename T, class Colorizer>
struct SimpleGrid {
  int width, height;
  std::vector<T> data;
  std::vector<Color> imageBuffer;

  SimpleGrid(int w, int h) : width(w), height(h) {
    data.resize(width * height, T());
    imageBuffer.resize(width * height, WHITE);
  }

  const T& get(int x, int y) const { return get(y * width + x); }
  const T& get(int idx) const { return data[idx]; }

  void set(int x, int y, const T& value) { set(y * width + x, value); }
  void set(int idx, const T& value) {
    data[idx] = value;
    imageBuffer[idx] = Colorizer::colorize(value);
  }
};

struct CameraModule {
  Camera2D camera;
  CameraModule()
      : camera{.offset = {0, 0},
               .target = {0, 0},
               .rotation = 0.0f,
               .zoom = 1.0f} {}

  void updateCamera() {
    if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
      Vector2 delta = GetMouseDelta();
      delta = Vector2Scale(delta, -1.0f / camera.zoom);
      camera.target = Vector2Add(camera.target, delta);
    }

    float wheel = GetMouseWheelMove();
    if (wheel != 0) {
      Vector2 mouseWorldPos = GetScreenToWorld2D(GetMousePosition(), camera);

      camera.offset = GetMousePosition();
      camera.target = mouseWorldPos;

      const float zoomIncrement = 0.125f;
      camera.zoom =
          std::max(camera.zoom + (wheel * zoomIncrement), zoomIncrement);
    }
  }
};

struct ImguiFPS {
  void draw() {
    ImGui::Begin("FPS", &guiIsOpen);
    ImGui::Text("FPS: %d", GetFPS());
    ImGui::End();
  }

  bool guiIsOpen;
};

template <typename T, typename Colorizer>
class SimpleGridWindow : public WindowManagerBase {
  using Grid = SimpleGrid<T, Colorizer>;

 public:
  SimpleGridWindow(int w, int h, const std::string& winTitle, int fps,
                   std::function<void(void)> t, std::shared_ptr<Grid> g)
      : WindowManagerBase(w, h, winTitle, fps),
        tick(t),
        grid(std::move(g)),
        gui{false} {
    im = {.data = nullptr,
          .width = w,
          .height = h,
          .mipmaps = 1,
          .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8};
  }

  void drawImGuiImpl() override { gui.draw(); }

  void loopImpl() override { tick(); }

  void preDrawImpl() override {
    im.data = (void*)grid->imageBuffer.data();
    tex = LoadTextureFromImage(im);
  }

  void drawImpl() override {
    ClearBackground(WHITE);
    DrawTexture(tex, 0, 0, WHITE);
  }

  void postDrawImpl() override { UnloadTexture(tex); }

 private:
  ImguiFPS gui;

  Image im;
  Texture2D tex;

  std::function<void(void)> tick;
  std::shared_ptr<Grid> grid;
};

template <typename Grid, typename Colorizer>
class DynamicGridWindow : public WindowManagerBase {
 public:
  DynamicGridWindow(int w, int h, const std::string& winTitle, int fps,
                    std::function<void(void)> t, std::shared_ptr<Grid> g)
      : WindowManagerBase(w, h, winTitle, fps),
        tick(t),
        grid(std::move(g)),
        gui{false},
        camera() {
    buffer.resize(w * h, WHITE);
    im = {.data = buffer.data(),
          .width = w,
          .height = h,
          .mipmaps = 1,
          .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8};
  }

  void drawImGuiImpl() override { gui.draw(); }

  void loopImpl() override { tick(); }

  void preDrawImpl() override {
    for (int x = 0; x < im.width; x++) {
      for (int y = 0; y < im.height; y++) {
        Color res = Colorizer::colorize(grid->get(x, y));
        buffer[y * im.width + x] = res;
      }
    }
    im.data = (void*)buffer.data();
    tex = LoadTextureFromImage(im);
    camera.updateCamera();
  }

  void drawImpl() override {
    ClearBackground(WHITE);
    BeginMode2D(camera.camera);
    DrawTexture(tex, 0, 0, WHITE);
    EndMode2D();
  }

  void postDrawImpl() override { UnloadTexture(tex); }

 private:
  ImguiFPS gui;

  CameraModule camera;
  std::vector<Color> buffer;
  Image im;
  Texture2D tex;

  std::function<void(void)> tick;
  std::shared_ptr<Grid> grid;
};
