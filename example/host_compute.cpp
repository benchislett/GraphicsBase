#include <iostream>
#include <map>

#include "window_base.h"

struct UniformRedColorizer {
  static Color colorize(float x) {
    assert(0.f <= x && x <= 1.f);
    unsigned char charval = static_cast<unsigned char>(x * 255.9999f);
    return Color{charval, 0, 0, 255};
  }
};

struct IntColorizer {
  static Color colorize(int x) {
    assert(x >= 0);

    // select from a shuffled palette
    static const std::array<Color, 7> palette = {
        Color{255, 0, 0, 255},    Color{0, 255, 0, 255},
        Color{0, 0, 255, 255},    Color{255, 255, 0, 255},
        Color{255, 0, 255, 255},  Color{0, 255, 255, 255},
        Color{255, 255, 255, 255}};
    // scale so that different values are more easily distinguishable
    Color selection = palette[x % 7];
    int times = x / 7 + 1;
    return Color{static_cast<unsigned char>(selection.r / float(times)),
                 static_cast<unsigned char>(selection.g / float(times)),
                 static_cast<unsigned char>(selection.b / float(times)), 255};
  }
};

template <class Grid>
struct Runtime {
  static void tick(Grid &grid) {
    for (int x = -128; x < 128; x++) {
      for (int y = -128; y < 128; y++) {
        grid.set(x, y, proper_mod(x + y, 10));
      }
    }
  }
};

constexpr int width = 1500;
constexpr int height = 800;

constexpr float inc = 0.001f;

int main() {
  // auto grid =
  //     std::make_shared<SimpleGrid<float, UniformRedColorizer>>(width,
  //     height);
  // float t = 0.f;
  // auto tick = [&]() {
  //   t = fmodf(t + inc, 1.f);
  //   for (int x = 0; x < width; x++) {
  //     for (int y = 0; y < height; y++) {
  //       grid->set(x, y, t);
  //     }
  //   }
  // };
  // SimpleGridWindow window(width, height, "Test Window", 0, tick, grid);

  // dynamic grid window example
  DynamicGridWindow<Runtime, int, IntColorizer> window(width, height,
                                                       "Test Window", 0);
  window.loop();
  return 0;
}
