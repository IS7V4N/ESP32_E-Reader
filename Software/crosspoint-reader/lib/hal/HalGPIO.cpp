#include <HalGPIO.h>
#include <SPI.h>

void HalGPIO::begin() {
  inputMgr.begin();
  SPI.begin(EPD_SCLK, SPI_MISO, EPD_MOSI, EPD_CS);
}

void HalGPIO::update() { inputMgr.update(); }

bool HalGPIO::wasUsbStateChanged() const { return false; }

bool HalGPIO::isPressed(uint8_t buttonIndex) const { return inputMgr.isPressed(buttonIndex); }

bool HalGPIO::wasPressed(uint8_t buttonIndex) const { return inputMgr.wasPressed(buttonIndex); }

bool HalGPIO::wasAnyPressed() const { return inputMgr.wasAnyPressed(); }

bool HalGPIO::wasReleased(uint8_t buttonIndex) const { return inputMgr.wasReleased(buttonIndex); }

bool HalGPIO::wasAnyReleased() const { return inputMgr.wasAnyReleased(); }

void HalGPIO::consumeRelease(const uint8_t buttonIndex) { inputMgr.consumeRelease(buttonIndex); }

void HalGPIO::forceRelease(const uint8_t buttonIndex) { inputMgr.forceRelease(buttonIndex); }

unsigned long HalGPIO::getHeldTime() const { return inputMgr.getHeldTime(); }

bool HalGPIO::isUsbConnected() const {
  // GPIO 20 repurposed for frontlight PWM; USB detection no longer available
  return false;
}

HalGPIO::WakeupReason HalGPIO::getWakeupReason() const {
  const auto wakeupCause = esp_sleep_get_wakeup_cause();
  const auto resetReason = esp_reset_reason();

  if ((wakeupCause == ESP_SLEEP_WAKEUP_UNDEFINED && resetReason == ESP_RST_POWERON) ||
      (wakeupCause == ESP_SLEEP_WAKEUP_GPIO && resetReason == ESP_RST_DEEPSLEEP)) {
    return WakeupReason::PowerButton;
  }
  if (wakeupCause == ESP_SLEEP_WAKEUP_UNDEFINED && resetReason == ESP_RST_UNKNOWN) {
    return WakeupReason::AfterFlash;
  }
  return WakeupReason::Other;
}