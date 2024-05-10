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

constexpr int IMAGE_WIDTH = 128;

class ConfigWindow : public WindowManagerBase {
 public:
  ConfigWindow(int w, int h, const std::string& winTitle, int fps = 60)
      : WindowManagerBase(w, h, winTitle, fps), hostImageBuffer{} {
    hostImageBuffer.resize(IMAGE_WIDTH * IMAGE_WIDTH);
    im = {.data = (void*)hostImageBuffer.data(),  // sharing host data, no need
                                                  // to deallocate this Image
          .width = IMAGE_WIDTH,
          .height = IMAGE_WIDTH,
          .mipmaps = 1,
          .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8};

    camera = {.offset = {0.f, 0.f},
              .rotation = 0.f,
              .target = {0.f, 0.f},
              .zoom = 1.f};
    // camera = {0};
    // camera.zoom = 1.0f;
  }

  void drawImGuiImpl() override {
    ImGui::Begin("Config", &guiIsOpen);

    ImGui::Text("FPS: %d", GetFPS());

    if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
      Vector2 delta = GetMouseDelta();
      delta = Vector2Scale(delta, -1.0f / camera.zoom);

      camera.target = Vector2Add(camera.target, delta);
    }

    // Zoom based on mouse wheel
    float wheel = GetMouseWheelMove();
    if (wheel != 0) {
      // Get the world point that is under the mouse
      Vector2 mouseWorldPos = GetScreenToWorld2D(GetMousePosition(), camera);

      // Set the offset to where the mouse is
      camera.offset = GetMousePosition();

      // Set the target to match, so that the camera maps the world space point
      // under the cursor to the screen space point under the cursor at any zoom
      camera.target = mouseWorldPos;

      // Zoom increment
      const float zoomIncrement = 0.125f;

      camera.zoom += (wheel * zoomIncrement);
      if (camera.zoom < zoomIncrement) camera.zoom = zoomIncrement;
    }

    ImGui::End();
  }

  void loopImpl() override {}

  void preDrawImpl() override {
    for (int y = 0; y < 128; y++) {
      for (int x = 0; x < 128; x++) {
        hostImageBuffer[y * 128 + x] =
            (((x + frameid / 4) % 4) ^ (y % 4)) ? WHITE : BLACK;
      }
    }

    // im's memory should point to hostImageBuffer
    tex = LoadTextureFromImage(im);
  }

  void drawImpl() override {
    ClearBackground(WHITE);
    BeginMode2D(camera);
    DrawTexture(tex, 0, 0, WHITE);
    EndMode2D();
  }

  void postDrawImpl() override { UnloadTexture(tex); }

 private:
  bool guiIsOpen;

  Image im;
  Texture2D tex;

  Camera2D camera;

  std::vector<Color> hostImageBuffer;
};
