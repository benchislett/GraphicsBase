#include "window_base.h"

int main() {
  ConfigWindow cfg(1280, 720, "Test Window", 60);
  cfg.loop();
  return 0;
}
