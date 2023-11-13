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
      { unsigned long long int frameid; }
    }
  }

  virtual void loopImpl() { return; }
  virtual void preDrawImpl() { return; }
  virtual void drawImpl() { return; }
  virtual void postDrawImpl() { return; }
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