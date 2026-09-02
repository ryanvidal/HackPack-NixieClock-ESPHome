#include "color_animations.h"
#include "hack_pack_nixie_clock.h"
#include <cmath>
#include <cstdlib>

namespace esphome {
namespace hack_pack_nixie_clock {

// =============================================================================
// 1. Rainbow Animation
// =============================================================================
class RainbowAnimation : public ColorModeAnimation {
public:
  uint32_t get_frame_ms() const override { return 40; }
  int get_total_steps() const override { return 255; }

  void step(HackPackNixieClock *clock, int step, int total_steps) override {
    for (int p = 0; p < 6; p++) {
      for (int s = 0; s < 7; s++) {
        uint32_t c = HackPackNixieClock::wheel_(25 * p + 5 * s + step);
        clock->set_panel_segment_rgb(p, s, HackPackNixieClock::red_(c),
                                     HackPackNixieClock::green_(c),
                                     HackPackNixieClock::blue_(c));
      }
    }
  }
};

// =============================================================================
// 2. Solid Animation
// =============================================================================
class SolidAnimation : public ColorModeAnimation {
public:
  uint32_t get_frame_ms() const override { return 100; }
  int get_total_steps() const override { return 1; }

  void step(HackPackNixieClock *clock, int step, int total_steps) override {
    uint32_t base_col = clock->get_panel_base_color();
    uint8_t r = HackPackNixieClock::red_(base_col);
    uint8_t g = HackPackNixieClock::green_(base_col);
    uint8_t b = HackPackNixieClock::blue_(base_col);

    for (int p = 0; p < 6; p++) {
      for (int s = 0; s < 7; s++) {
        clock->set_panel_segment_rgb(p, s, r, g, b);
      }
    }
  }
};

// =============================================================================
// 3. Gradient Animation
// =============================================================================
class GradientAnimation : public ColorModeAnimation {
public:
  uint32_t get_frame_ms() const override { return 50; }
  int get_total_steps() const override { return 50; }

  void setup(HackPackNixieClock *clock) override {
    uint8_t base_pos = clock->get_panel_color_pos();
    strt_col1_ = HackPackNixieClock::wheel_(base_pos + (rand() % 61 - 30));
    strt_col2_ = HackPackNixieClock::wheel_(base_pos + (rand() % 61 - 30));
    end_col1_ = HackPackNixieClock::wheel_(base_pos + (rand() % 61 - 30));
    end_col2_ = HackPackNixieClock::wheel_(base_pos + (rand() % 61 - 30));
    now_col1_ = strt_col1_;
    now_col2_ = strt_col2_;
  }

  void step(HackPackNixieClock *clock, int step, int total_steps) override {
    for (int p = 0; p < 6; p++) {
      uint32_t panelColor =
          HackPackNixieClock::color_fade_(now_col1_, now_col2_, p, 5);
      uint8_t r = HackPackNixieClock::red_(panelColor);
      uint8_t g = HackPackNixieClock::green_(panelColor);
      uint8_t b = HackPackNixieClock::blue_(panelColor);
      for (int s = 0; s < 7; s++) {
        clock->set_panel_segment_rgb(p, s, r, g, b);
      }
    }
    now_col1_ = HackPackNixieClock::color_fade_(strt_col1_, end_col1_, step,
                                                total_steps);
    now_col2_ = HackPackNixieClock::color_fade_(strt_col2_, end_col2_, step,
                                                total_steps);
  }

  void on_complete(HackPackNixieClock *clock) override {
    strt_col1_ = end_col1_;
    strt_col2_ = end_col2_;
    now_col1_ = strt_col1_;
    now_col2_ = strt_col2_;
    uint8_t base_pos = clock->get_panel_color_pos();
    end_col1_ = HackPackNixieClock::wheel_(base_pos + (rand() % 61 - 30));
    end_col2_ = HackPackNixieClock::wheel_(base_pos + (rand() % 61 - 30));
  }

protected:
  uint32_t strt_col1_{0}, strt_col2_{0};
  uint32_t end_col1_{0}, end_col2_{0};
  uint32_t now_col1_{0}, now_col2_{0};
};

// =============================================================================
// 4. Flow Animation
// =============================================================================
class FlowAnimation : public ColorModeAnimation {
public:
  uint32_t get_frame_ms() const override { return 25; }
  int get_total_steps() const override { return 50; }

  void setup(HackPackNixieClock *clock) override {
    strt_col1_ = HackPackNixieClock::wheel_(rand() % 256);
    strt_col2_ = HackPackNixieClock::wheel_(rand() % 256);
    end_col1_ = HackPackNixieClock::wheel_(rand() % 256);
    end_col2_ = HackPackNixieClock::wheel_(rand() % 256);
    now_col1_ = strt_col1_;
    now_col2_ = strt_col2_;
  }

  void step(HackPackNixieClock *clock, int step, int total_steps) override {
    for (int p = 0; p < 6; p++) {
      uint32_t panelColor =
          HackPackNixieClock::color_fade_(now_col1_, now_col2_, p, 5);
      uint8_t r = HackPackNixieClock::red_(panelColor);
      uint8_t g = HackPackNixieClock::green_(panelColor);
      uint8_t b = HackPackNixieClock::blue_(panelColor);
      for (int s = 0; s < 7; s++) {
        clock->set_panel_segment_rgb(p, s, r, g, b);
      }
    }
    now_col1_ = HackPackNixieClock::color_fade_(strt_col1_, end_col1_, step,
                                                total_steps);
    now_col2_ = HackPackNixieClock::color_fade_(strt_col2_, end_col2_, step,
                                                total_steps);
  }

  void on_complete(HackPackNixieClock *clock) override {
    strt_col1_ = end_col1_;
    strt_col2_ = end_col2_;
    now_col1_ = strt_col1_;
    now_col2_ = strt_col2_;
    end_col1_ = HackPackNixieClock::wheel_(rand() % 256);
    end_col2_ = HackPackNixieClock::wheel_(rand() % 256);
  }

protected:
  uint32_t strt_col1_{0}, strt_col2_{0};
  uint32_t end_col1_{0}, end_col2_{0};
  uint32_t now_col1_{0}, now_col2_{0};
};

// =============================================================================
// 5. Wipe Animation
// =============================================================================
class WipeAnimation : public ColorModeAnimation {
public:
  uint32_t get_frame_ms() const override { return 20; }
  int get_total_steps() const override { return 30; }

  void setup(HackPackNixieClock *clock) override {
    wipe_index_ = 0;
    wipe_col_ = true;
    uint8_t col = rand() % 256;
    uint8_t base_pos = clock->get_panel_color_pos();
    while (std::abs((int)col - (int)base_pos) < 15)
      col = rand() % 256;
    strt_col1_ = HackPackNixieClock::wheel_(col);
  }

  void step(HackPackNixieClock *clock, int step, int total_steps) override {
    uint32_t base_col = clock->get_panel_base_color();
    uint32_t fade_col =
        wipe_col_ ? HackPackNixieClock::color_fade_(base_col, strt_col1_, step,
                                                    total_steps)
                  : HackPackNixieClock::color_fade_(strt_col1_, base_col, step,
                                                    total_steps);

    for (int p = 0; p < 6; p++) {
      uint32_t c =
          (p == wipe_index_)
              ? fade_col
              : (wipe_col_ ? (p < wipe_index_ ? strt_col1_ : base_col)
                           : (p > wipe_index_ ? strt_col1_ : base_col));
      uint8_t r = HackPackNixieClock::red_(c);
      uint8_t g = HackPackNixieClock::green_(c);
      uint8_t b = HackPackNixieClock::blue_(c);
      for (int s = 0; s < 7; s++) {
        clock->set_panel_segment_rgb(p, s, r, g, b);
      }
    }
  }

  void on_complete(HackPackNixieClock *clock) override {
    wipe_index_++;
    if (wipe_index_ > 5) {
      wipe_col_ = !wipe_col_;
      wipe_index_ = 0;
      if (wipe_col_) {
        uint8_t col = rand() % 256;
        uint8_t base_pos = clock->get_panel_color_pos();
        while (std::abs((int)col - (int)base_pos) < 15)
          col = rand() % 256;
        strt_col1_ = HackPackNixieClock::wheel_(col);
      }
    }
  }

protected:
  uint32_t strt_col1_{0};
  int wipe_index_{0};
  bool wipe_col_{true};
};

// =============================================================================
// 6. Pulse Animation
// =============================================================================
class PulseAnimation : public ColorModeAnimation {
public:
  uint32_t get_frame_ms() const override { return 15; }
  int get_total_steps() const override { return 40; }

  void setup(HackPackNixieClock *clock) override {
    pulse_index_ = 0;
    pulse_dir_ = true;
    uint8_t col = rand() % 256;
    uint8_t base_pos = clock->get_panel_color_pos();
    while (std::abs((int)col - (int)base_pos) < 15)
      col = rand() % 256;
    strt_col1_ = HackPackNixieClock::wheel_(col);
  }

  void step(HackPackNixieClock *clock, int step, int total_steps) override {
    uint32_t base_col = clock->get_panel_base_color();
    for (int p = 0; p < 6; p++) {
      uint32_t c;
      if (p < 3 - pulse_index_ || p > 2 + pulse_index_) {
        c = base_col;
      } else if (p == 3 - pulse_index_ || p == 2 + pulse_index_) {
        c = pulse_dir_ ? HackPackNixieClock::color_fade_(base_col, strt_col1_,
                                                         step, total_steps)
                       : HackPackNixieClock::color_fade_(strt_col1_, base_col,
                                                         step, total_steps);
      } else {
        c = strt_col1_;
      }
      uint8_t r = HackPackNixieClock::red_(c);
      uint8_t g = HackPackNixieClock::green_(c);
      uint8_t b = HackPackNixieClock::blue_(c);
      for (int s = 0; s < 7; s++) {
        clock->set_panel_segment_rgb(p, s, r, g, b);
      }
    }
  }

  void on_complete(HackPackNixieClock *clock) override {
    pulse_index_++;
    if (pulse_index_ > 3) {
      pulse_index_ = 0;
      pulse_dir_ = !pulse_dir_;
      if (pulse_dir_) {
        uint8_t col = rand() % 256;
        uint8_t base_pos = clock->get_panel_color_pos();
        while (std::abs((int)col - (int)base_pos) < 15)
          col = rand() % 256;
        strt_col1_ = HackPackNixieClock::wheel_(col);
      }
    }
  }

protected:
  uint32_t strt_col1_{0};
  int pulse_index_{0};
  bool pulse_dir_{true};
};

// =============================================================================
// 7. Bounce Animation
// =============================================================================
class BounceAnimation : public ColorModeAnimation {
public:
  uint32_t get_frame_ms() const override { return 15; }
  int get_total_steps() const override { return 40; }

  void setup(HackPackNixieClock *clock) override {
    bounce_index_ = 0;
    bounce_dir_ = true;
    uint8_t col = rand() % 256;
    uint8_t base_pos = clock->get_panel_color_pos();
    while (std::abs((int)col - (int)base_pos) < 15)
      col = rand() % 256;
    strt_col1_ = HackPackNixieClock::wheel_(base_pos);
    strt_col2_ = HackPackNixieClock::wheel_(col);
  }

  void step(HackPackNixieClock *clock, int step, int total_steps) override {
    uint32_t base_col = clock->get_panel_base_color();
    for (int p = 0; p < 6; p++) {
      uint32_t c = base_col;
      if (p == bounce_index_) {
        c = HackPackNixieClock::color_fade_(base_col, strt_col2_, step,
                                            total_steps);
      } else if (bounce_dir_ && p == bounce_index_ - 1) {
        c = HackPackNixieClock::color_fade_(strt_col2_, base_col, step,
                                            total_steps);
      } else if (!bounce_dir_ && p == bounce_index_ + 1) {
        c = HackPackNixieClock::color_fade_(strt_col2_, base_col, step,
                                            total_steps);
      }
      uint8_t r = HackPackNixieClock::red_(c);
      uint8_t g = HackPackNixieClock::green_(c);
      uint8_t b = HackPackNixieClock::blue_(c);
      for (int s = 0; s < 7; s++) {
        clock->set_panel_segment_rgb(p, s, r, g, b);
      }
    }
  }

  void on_complete(HackPackNixieClock *clock) override {
    bounce_index_ += bounce_dir_ ? 1 : -1;
    if (bounce_index_ >= 5) {
      bounce_dir_ = false;
    } else if (bounce_index_ <= 0) {
      bounce_dir_ = true;
      uint8_t col = rand() % 256;
      uint8_t base_pos = clock->get_panel_color_pos();
      while (std::abs((int)col - (int)base_pos) < 15)
        col = rand() % 256;
      strt_col2_ = HackPackNixieClock::wheel_(col);
    }
  }

protected:
  uint32_t strt_col1_{0}, strt_col2_{0};
  int bounce_index_{0};
  bool bounce_dir_{true};
};

// =============================================================================
// Factory Function
// =============================================================================
std::unique_ptr<ColorModeAnimation> create_color_animation(ColorMode mode) {
  switch (mode) {
  case COLOR_SOLID:
    return std::make_unique<SolidAnimation>();
  case COLOR_GRADIENT:
    return std::make_unique<GradientAnimation>();
  case COLOR_FLOW:
    return std::make_unique<FlowAnimation>();
  case COLOR_WIPE:
    return std::make_unique<WipeAnimation>();
  case COLOR_PULSE:
    return std::make_unique<PulseAnimation>();
  case COLOR_BOUNCE:
    return std::make_unique<BounceAnimation>();
  case COLOR_RAINBOW:
  default:
    return std::make_unique<RainbowAnimation>();
  }
}

} // namespace hack_pack_nixie_clock
} // namespace esphome
