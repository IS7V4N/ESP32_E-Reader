#include "HalFrontlight.h"

#include <Arduino.h>
#include <Logging.h>

void HalFrontlight::begin(uint8_t brightnessPercent) {
  // Arduino-ESP32 v3.x API: ledcAttach(pin, freq, resolution)
  if (!ledcAttach(FRONTLIGHT_PIN, PWM_FREQ, PWM_RESOLUTION)) {
    LOG_ERR("FLT", "LEDC attach failed on GPIO %d", FRONTLIGHT_PIN);
    return;
  }
  LOG_DBG("FLT", "Frontlight PWM initialised on GPIO %d (max duty %d/255)", FRONTLIGHT_PIN, PWM_DUTY_MAX);
  setBrightness(brightnessPercent);
}

void HalFrontlight::setBrightness(uint8_t percent) {
  if (percent > 100) percent = 100;

  // 0% -> fully off (duty = 0 = constant LOW = LED off on active-high driver)
  if (percent == 0) {
    ledcWrite(FRONTLIGHT_PIN, 0);
    LOG_DBG("FLT", "Brightness 0%% -> off");
    return;
  }

  // Map 1-100% linearly onto [PWM_DUTY_MIN, PWM_DUTY_MAX].
  // LED driver is active-high: higher duty = brighter. No inversion needed.
  const uint8_t duty = static_cast<uint8_t>(
      PWM_DUTY_MIN +
      (static_cast<uint16_t>(percent - 1) * (PWM_DUTY_MAX - PWM_DUTY_MIN)) / 99u);

  ledcWrite(FRONTLIGHT_PIN, duty);
  LOG_DBG("FLT", "Brightness %d%% -> duty %d/255", percent, duty);
}

void HalFrontlight::off() {
  ledcWrite(FRONTLIGHT_PIN, 0);
  LOG_DBG("FLT", "Frontlight off");
}
