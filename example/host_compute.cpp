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

template <int tile_size>
struct TiledGrid {
  struct Tile {
    Tile() {
      for (int i = 0; i < tile_size * tile_size; i++) {
        data[i] = 0;
      }
    }

    int data[tile_size * tile_size];
  };

  static std::tuple<int, int, int, int> indexDecomp(int x, int y) {
    int tile_x = (x >= 0) ? x / tile_size : -1 - (-1 - y) / tile_size;
    int tile_y = (y >= 0) ? y / tile_size : -1 - (-1 - y) / tile_size;
    int idx_x = (x >= 0) ? (x % tile_size) : -(x % tile_size);
    int idx_y = (y >= 0) ? (y % tile_size) : -(y % tile_size);
    return std::make_tuple(tile_x, tile_y, idx_x, idx_y);
  }

  int get(int x, int y) const {
    auto [tileid_x, tileid_y, idx_x, idx_y] = indexDecomp(x, y);
    auto it = tiles.find({tileid_x, tileid_y});
    if (it == tiles.end()) {
      return 0;
    }

    return it->second.data[idx_y * tile_size + idx_x];
  }

  void set(int x, int y, int value) {
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

constexpr int width = 512;
constexpr int height = 512;

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
  auto grid = std::make_shared<TiledGrid<16>>();
  auto tick = [&]() {
    for (int x = -128; x < 128; x++) {
      for (int y = -128; y < 128; y++) {
        grid->set(x, y, std::abs(x + y) % 10);
      }
    }
  };

  DynamicGridWindow<TiledGrid<16>, IntColorizer> window(
      width, height, "Test Window", 0, tick, grid);
  window.loop();
  return 0;
}
