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

int div_round_negative(int a, int b) {
  if (a >= 0) {
    return a / b;
  } else {
    return -1 - (-1 - a) / b;
  }
}

int proper_mod(int a, int b) { return (a % b + b) % b; }

template <typename T, int tile_size>
struct TiledGrid {
  struct Tile {
    Tile() {
      for (int i = 0; i < tile_size * tile_size; i++) {
        data[i] = T();
      }
    }

    T data[tile_size * tile_size];
  };

  static std::tuple<int, int, int, int> indexDecomp(int x, int y) {
    int tile_x = div_round_negative(x, tile_size);
    int tile_y = div_round_negative(y, tile_size);
    int idx_x = proper_mod(x, tile_size);
    int idx_y = proper_mod(y, tile_size);
    return std::make_tuple(tile_x, tile_y, idx_x, idx_y);
  }

  T get(int x, int y) const {
    auto [tileid_x, tileid_y, idx_x, idx_y] = indexDecomp(x, y);
    auto it = tiles.find({tileid_x, tileid_y});
    if (it == tiles.end()) {
      return T();
    }

    return it->second.data[idx_y * tile_size + idx_x];
  }

  void set(int x, int y, const T& value) {
    auto [tileid_x, tileid_y, idx_x, idx_y] = indexDecomp(x, y);
    auto it = tiles.find({tileid_x, tileid_y});
    if (it == tiles.end()) {
      it = tiles.insert({{tileid_x, tileid_y}, Tile()}).first;
    }

    it->second.data[idx_y * tile_size + idx_x] = value;
  }

 private:
  std::map<std::pair<int, int>, Tile> tiles;
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

      const float zoomIncrement = 1.f;
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
  static constexpr int tile_size = 16;

 public:
  DynamicGridWindow(int w, int h, const std::string& winTitle, int fps,
                    std::function<void(void)> t, std::shared_ptr<Grid> g)
      : WindowManagerBase(w, h, winTitle, fps),
        tick(t),
        grid(std::move(g)),
        gui{false},
        camera() {
    int im_w = static_cast<int>(w) + 2 * tile_size;
    int im_h = static_cast<int>(h) + 2 * tile_size;
    buffer.resize(im_w * im_h, WHITE);
    im = {.data = buffer.data(),
          .width = im_w,
          .height = im_h,
          .mipmaps = 1,
          .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8};
  }

  void drawImGuiImpl() override { gui.draw(); }

  void loopImpl() override { tick(); }

  void preDrawImpl() override {
    camera.updateCamera();
    Vector2 dxy = GetScreenToWorld2D({0, 0}, camera.camera);
    int x_start = static_cast<int>(dxy.x);
    int y_start = static_cast<int>(dxy.y);

    for (int x = 0; x < im.width; x++) {
      for (int y = 0; y < im.height; y++) {
        Color res = Colorizer::colorize(
            grid->get(x + x_start - tile_size, y + y_start - tile_size));
        buffer[y * im.width + x] = res;
      }
    }
    im.data = (void*)buffer.data();
    tex = LoadTextureFromImage(im);
  }

  void drawImpl() override {
    Vector2 dxy = GetScreenToWorld2D({0, 0}, camera.camera);
    int x_start = static_cast<int>(dxy.x);
    int y_start = static_cast<int>(dxy.y);

    ClearBackground(WHITE);
    BeginMode2D(camera.camera);
    DrawTexture(tex, x_start - tile_size, y_start - tile_size, WHITE);
    EndMode2D();
  }

  void postDrawImpl() override { UnloadTexture(tex); }

 private:
  ImguiFPS gui;

  CameraModule camera;

  std::function<void(void)> tick;
  std::shared_ptr<Grid> grid;

  std::vector<Color> buffer;
  Image im;
  Texture2D tex;
};
