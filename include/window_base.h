#include <string>

#include "imgui.h"
#include "imgui_stdlib.h"
#include "raylib.h"
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
    setFpsLimit(fps);
  }

  ~WindowManagerBase() {
    rlImGuiShutdown();
    CloseWindow();
  }

  int getFpsLimit() const { return fpsLimit; };
  void setFpsLimit(int fps) {
    fpsLimit = fps;
    SetTargetFPS(fps);
  }

  int getWinWidth() const { return winWidth; }
  int getWinHeight() const { return winHeight; }

  virtual void initImpl() { return; }

  void loop() {
    while (!WindowShouldClose()) {
      draw();
      loopImpl();
      frameid++;
      { unsigned long long int frameid; }
    }
  }

  virtual void loopImpl() { return; }

  virtual void drawImpl() { return; }

  virtual void drawImGuiImpl() { return; }

 private:
  void drawBegin() const {
    BeginDrawing();
    ClearBackground(RAYWHITE);
  }

  void drawEnd() const { EndDrawing(); }

  void drawImGuiBegin() const { rlImGuiBegin(); }

  void drawImGuiEnd() const { rlImGuiEnd(); }

  void draw() {
    drawBegin();
    drawImpl();
    drawImGuiBegin();
    drawImGuiImpl();
    drawImGuiEnd();
    drawEnd();
  }

  int winWidth;
  int winHeight;

  int fpsLimit;

  unsigned long long int frameid;
};