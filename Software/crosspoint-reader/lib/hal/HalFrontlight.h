#pragma once

#include <cstdint>

class HalFrontlight {
 public:
  // --- Tunable PWM limits ---
  // User's 0% always maps to fully off (duty = 0).
  // Any non-zero % is clamped to [PWM_DUTY_MIN, PWM_DUTY_MAX].
  // Adjust PWM_DUTY_MIN to set the lowest brightness reachable from the UI.
  // Adjust PWM_DUTY_MAX to cap the maximum brightness.
  // Observed working range: LED responds in roughly [0, 60] duty (8-bit LEDC, active-high).
  // Values above ~60 saturate to the same brightness.
  // Raise PWM_DUTY_MAX if max brightness seems weak; lower PWM_DUTY_MIN if 1% flickers.
  static constexpr uint8_t PWM_DUTY_MIN = 1;   // floor for 1% UI value (dim but stable)
  static constexpr uint8_t PWM_DUTY_MAX = 60;  // ceiling; LED likely saturates above this

  // Initialise LEDC PWM on the frontlight pin and apply the given brightness.
  static void begin(uint8_t brightnessPercent);

  // Set brightness from a 0-100 percentage.
  // 0%  -> duty 0 (fully off).
  // 1-100% -> mapped to [PWM_DUTY_MIN, PWM_DUTY_MAX].
  static void setBrightness(uint8_t percent);

  // Turn off the frontlight (duty = 0).
  static void off();

 private:
  static constexpr uint8_t FRONTLIGHT_PIN = 20;
  static constexpr uint32_t PWM_FREQ = 10000;   // 10 kHz (LED driver: 5–100 kHz)
  static constexpr uint8_t PWM_RESOLUTION = 8;  // 8-bit (0-255)
};
