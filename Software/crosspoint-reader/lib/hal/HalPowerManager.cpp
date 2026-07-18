#include "HalPowerManager.h"

#include <Logging.h>
#include <WiFi.h>
#include <esp_sleep.h>

#include <cassert>

#include "HalGPIO.h"

HalPowerManager powerManager;  // Singleton instance

void HalPowerManager::begin() {
  pinMode(BAT_GPIO0, INPUT);
  normalFreq = getCpuFrequencyMhz();
  modeMutex = xSemaphoreCreateMutex();
  assert(modeMutex != nullptr);
}

void HalPowerManager::setPowerSaving(bool enabled) {
  if (normalFreq <= 0) {
    return;  // invalid state
  }

  auto wifiMode = WiFi.getMode();
  if (wifiMode != WIFI_MODE_NULL) {
    // Wifi is active, force disabling power saving
    enabled = false;
  }

  // Note: We don't use mutex here to avoid too much overhead,
  // it's not very important if we read a slightly stale value for currentLockMode
  const LockMode mode = currentLockMode;

  if (mode == None && enabled && !isLowPower) {
    LOG_DBG("PWR", "Going to low-power mode");
    if (!setCpuFrequencyMhz(LOW_POWER_FREQ)) {
      LOG_DBG("PWR", "Failed to set CPU frequency = %d MHz", LOW_POWER_FREQ);
      return;
    }
    isLowPower = true;

  } else if ((!enabled || mode != None) && isLowPower) {
    LOG_DBG("PWR", "Restoring normal CPU frequency");
    if (!setCpuFrequencyMhz(normalFreq)) {
      LOG_DBG("PWR", "Failed to set CPU frequency = %d MHz", normalFreq);
      return;
    }
    isLowPower = false;
  }

  // Otherwise, no change needed
}

void HalPowerManager::startDeepSleep(HalGPIO& gpio) const {
  // Ensure that the power button has been released to avoid immediately turning back on if you're holding it
  while (gpio.isPressed(HalGPIO::BTN_POWER)) {
    delay(50);
    gpio.update();
  }
  pinMode(InputManager::POWER_BUTTON_PIN, INPUT_PULLUP);
  // Arm the wakeup trigger *after* the button is released
  // Note: this is only useful for waking up on USB power. On battery, the MCU will be completely powered off, so the
  // power button is hard-wired to briefly provide power to the MCU, waking it up regardless of the wakeup source
  // configuration
  esp_deep_sleep_enable_gpio_wakeup(1ULL << InputManager::POWER_BUTTON_PIN, ESP_GPIO_WAKEUP_GPIO_LOW);
  // Enter Deep Sleep
  esp_deep_sleep_start();
}

uint16_t HalPowerManager::getBatteryPercentage() {
  static const BatteryMonitor battery = BatteryMonitor(BAT_GPIO0);

  // Multi-sample averaging to reduce ADC noise
  static constexpr int NUM_SAMPLES = 8;
  uint32_t mvSum = 0;
  for (int i = 0; i < NUM_SAMPLES; i++) {
    mvSum += battery.readMillivolts();
  }
  const uint16_t avgMv = static_cast<uint16_t>(mvSum / NUM_SAMPLES);
  const float rawPercent = static_cast<float>(BatteryMonitor::percentageFromMillivolts(avgMv));

  // Exponential moving average (alpha = 0.15 → slow, stable response)
  static constexpr float EMA_ALPHA = 0.15f;
  if (smoothedPercentage < 0.0f) {
    smoothedPercentage = rawPercent;  // First reading: initialize immediately
  } else {
    smoothedPercentage += EMA_ALPHA * (rawPercent - smoothedPercentage);
  }

  // Hysteresis: only update displayed value if change >= 2%
  static constexpr float HYSTERESIS = 2.0f;
  const auto rounded = static_cast<uint16_t>(smoothedPercentage + 0.5f);
  const int delta = static_cast<int>(rounded) - static_cast<int>(displayedPercentage);
  if (displayedPercentage == 0 && smoothedPercentage > 0.0f) {
    displayedPercentage = rounded;  // First valid reading
  } else if (delta >= HYSTERESIS || delta <= -HYSTERESIS) {
    displayedPercentage = rounded;
  }

  return displayedPercentage;
}

HalPowerManager::Lock::Lock() {
  xSemaphoreTake(powerManager.modeMutex, portMAX_DELAY);
  // Current limitation: only one lock at a time
  if (powerManager.currentLockMode != None) {
    LOG_ERR("PWR", "Lock already held, ignore");
    valid = false;
  } else {
    powerManager.currentLockMode = NormalSpeed;
    valid = true;
  }
  xSemaphoreGive(powerManager.modeMutex);
  if (valid) {
    // Immediately restore normal CPU frequency if currently in low-power mode
    powerManager.setPowerSaving(false);
  }
}

HalPowerManager::Lock::~Lock() {
  xSemaphoreTake(powerManager.modeMutex, portMAX_DELAY);
  if (valid) {
    powerManager.currentLockMode = None;
  }
  xSemaphoreGive(powerManager.modeMutex);
}
