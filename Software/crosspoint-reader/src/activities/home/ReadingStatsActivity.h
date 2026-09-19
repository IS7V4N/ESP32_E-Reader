#pragma once
#include <cstddef>
#include <cstdint>

#include "activities/Activity.h"

// Read-only screen showing accumulated reading statistics, split into the most recently
// read book and the all-time totals. Data comes from the in-memory ReadingStatsStore,
// so there is no file I/O and nothing to allocate.
class ReadingStatsActivity final : public Activity {
  // Draws a "label .......... value" row and returns the y of the next row.
  int drawStatRow(const char* label, const char* value, int y, int pageWidth) const;
  // Formats a duration as "1h 05m", "12m" or "45s" into the caller's buffer.
  static void formatDuration(uint32_t seconds, char* buf, size_t bufSize);
  // Formats pages per minute, or "-" when no time has been recorded.
  static void formatPagesPerMinute(uint32_t pageTurns, uint32_t seconds, char* buf, size_t bufSize);

 public:
  explicit ReadingStatsActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
      : Activity("ReadingStats", renderer, mappedInput) {}
  void onEnter() override;
  void onExit() override;
  void loop() override;
  void render(RenderLock&&) override;
};
