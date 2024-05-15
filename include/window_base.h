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

template <typename T, class Colorizer, int tile_size>
struct GraphicalTiledGrid : TiledGrid<T, tile_size> {
  struct ImageTile {
    ImageTile() : dirty(true), loaded(false) {
      data = (Color*)malloc(tile_size * tile_size * sizeof(Color));
      for (int i = 0; i < tile_size * tile_size; i++) {
        data[i] = Colorizer::colorize(T());
      }
      im = {.data = &data[0],
            .width = tile_size,
            .height = tile_size,
            .mipmaps = 2,
            .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8};
    }

    Color* data;
    Image im;
    Texture2D tex;
    bool dirty;
    bool loaded;
  };

  void set(int x, int y, const T& value) {
    auto [_, __, idx_x, idx_y] = TiledGrid<T, tile_size>::indexDecomp(x, y);
    TiledGrid<T, tile_size>::set(x, y, value);
    ImageTile& tile = get_image_tile(x, y);

    tile.data[idx_y * tile_size + idx_x] = Colorizer::colorize(value);
    tile.dirty = true;
  }

  void load_tile_at(int x, int y) {
    if (!has_tile(x, y)) {
      return;
    }
    ImageTile& tile = get_image_tile(x, y);
    if (!tile.loaded) {
      tile.tex = LoadTextureFromImage(tile.im);
      tile.loaded = true;
    }
    if (tile.dirty) {
      UpdateTexture(tile.tex, tile.im.data);
      GenTextureMipmaps(&tile.tex);
      SetTextureFilter(tile.tex, TEXTURE_FILTER_POINT);
    }
  }

  void draw_tile_at(int x, int y) {
    if (!has_tile(x, y)) {
      return;
    }
    ImageTile& tile = get_image_tile(x, y);
    DrawTexture(tile.tex, x, y, WHITE);
  }

  void unload_tile_at(int x, int y) {
    if (!has_tile(x, y)) {
      return;
    }
    ImageTile& tile = get_image_tile(x, y);
    UnloadTexture(tile.tex);
    tile.loaded = false;
  }

 private:
  ImageTile& get_image_tile(int x, int y) {
    auto [tileid_x, tileid_y, _, __] =
        TiledGrid<T, tile_size>::indexDecomp(x, y);
    auto it = imageTiles.find({tileid_x, tileid_y});
    if (it == imageTiles.end()) {
      it = imageTiles.insert({{tileid_x, tileid_y}, ImageTile()}).first;
    }

    return it->second;
  }

  bool has_tile(int x, int y) {
    auto [tileid_x, tileid_y, _, __] =
        TiledGrid<T, tile_size>::indexDecomp(x, y);
    return imageTiles.find({tileid_x, tileid_y}) != imageTiles.end();
  }

  std::map<std::pair<int, int>, ImageTile> imageTiles;
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

template <typename T, class Colorizer>
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

template <template <class _> class Runtime, typename T, class Colorizer>
class DynamicGridWindow : public WindowManagerBase {
  static constexpr int tile_size = 128;

 public:
  DynamicGridWindow(int w, int h, const std::string& winTitle, int fps)
      : WindowManagerBase(w, h, winTitle, fps), grid{}, gui{false}, camera() {
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

  void loopImpl() override {
    Runtime<GraphicalTiledGrid<T, Colorizer, tile_size>>::tick(grid);
  }

  void preDrawImpl() override {
    camera.updateCamera();
    Vector2 dxy = GetScreenToWorld2D({0, 0}, camera.camera);
    int x_start =
        div_round_negative(static_cast<int>(dxy.x), tile_size) * tile_size;
    int y_start =
        div_round_negative(static_cast<int>(dxy.y), tile_size) * tile_size;

    dxy = GetScreenToWorld2D(
        {(float)GetScreenWidth(), (float)GetScreenHeight()}, camera.camera);
    int x_end =
        div_round_negative(static_cast<int>(dxy.x), tile_size) * tile_size +
        tile_size;
    int y_end =
        div_round_negative(static_cast<int>(dxy.y), tile_size) * tile_size +
        tile_size;
    int drawn = 0;
    for (int x = x_start; x < x_end; x += tile_size) {
      for (int y = y_start; y < y_end; y += tile_size) {
        grid.load_tile_at(x, y);
      }
    }
  }

  void drawImpl() override {
    Vector2 dxy = GetScreenToWorld2D({0, 0}, camera.camera);
    int x_start =
        div_round_negative(static_cast<int>(dxy.x), tile_size) * tile_size;
    int y_start =
        div_round_negative(static_cast<int>(dxy.y), tile_size) * tile_size;
    dxy = GetScreenToWorld2D(
        {(float)GetScreenWidth(), (float)GetScreenHeight()}, camera.camera);
    int x_end =
        div_round_negative(static_cast<int>(dxy.x), tile_size) * tile_size +
        tile_size;
    int y_end =
        div_round_negative(static_cast<int>(dxy.y), tile_size) * tile_size +
        tile_size;

    ClearBackground(Colorizer::colorize(T()));
    BeginMode2D(camera.camera);
    for (int x = x_start; x < x_end; x += tile_size) {
      for (int y = y_start; y < y_end; y += tile_size) {
        grid.draw_tile_at(x, y);
      }
    }
    EndMode2D();
  }

  void postDrawImpl() override {}

 private:
  ImguiFPS gui;

  CameraModule camera;

  GraphicalTiledGrid<T, Colorizer, tile_size> grid;

  std::vector<Color> buffer;
  Image im;
  Texture2D tex;
};
