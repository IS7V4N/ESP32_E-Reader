#include "ReadingStatsActivity.h"

#include <GfxRenderer.h>
#include <I18n.h>

#include <cstdio>

#include "MappedInputManager.h"
#include "ReadingStatsStore.h"
#include "components/UITheme.h"
#include "fontIds.h"

namespace {
constexpr int SUB_HEADER_HEIGHT = 30;
constexpr int STAT_ROW_HEIGHT = 30;
// Longest value is a duration like "1234h 59m"; 32 bytes is ample and keeps us off the heap.
constexpr size_t VALUE_BUF_SIZE = 32;
}  // namespace

void ReadingStatsActivity::formatDuration(const uint32_t seconds, char* buf, const size_t bufSize) {
  const uint32_t hours = seconds / 3600;
  const uint32_t minutes = (seconds % 3600) / 60;

  if (hours > 0) {
    snprintf(buf, bufSize, "%uh %02um", hours, minutes);
  } else if (minutes > 0) {
    snprintf(buf, bufSize, "%um", minutes);
  } else {
    snprintf(buf, bufSize, "%us", seconds);
  }
}

void ReadingStatsActivity::formatPagesPerMinute(const uint32_t pageTurns, const uint32_t seconds, char* buf,
                                                const size_t bufSize) {
  if (seconds == 0 || pageTurns == 0) {
    snprintf(buf, bufSize, "-");
    return;
  }
  const float pagesPerMinute = static_cast<float>(pageTurns) / (static_cast<float>(seconds) / 60.0f);
  snprintf(buf, bufSize, "%.1f", pagesPerMinute);
}

int ReadingStatsActivity::drawStatRow(const char* label, const char* value, const int y, const int pageWidth) const {
  const auto& metrics = UITheme::getInstance().getMetrics();

  renderer.drawText(UI_10_FONT_ID, metrics.contentSidePadding, y, label);

  const int valueWidth = renderer.getTextWidth(UI_10_FONT_ID, value);
  renderer.drawText(UI_10_FONT_ID, pageWidth - metrics.contentSidePadding - valueWidth, y, value);

  return y + STAT_ROW_HEIGHT;
}

void ReadingStatsActivity::onEnter() {
  Activity::onEnter();
  requestUpdate();
}

void ReadingStatsActivity::onExit() { Activity::onExit(); }

void ReadingStatsActivity::loop() {
  if (mappedInput.wasReleased(MappedInputManager::Button::Back) ||
      mappedInput.wasReleased(MappedInputManager::Button::Confirm)) {
    onGoHome();
  }
}

void ReadingStatsActivity::render(RenderLock&&) {
  renderer.clearScreen();

  const auto pageWidth = renderer.getScreenWidth();
  const auto& metrics = UITheme::getInstance().getMetrics();

  GUI.drawHeader(renderer, Rect{0, metrics.topPadding, pageWidth, metrics.headerHeight}, tr(STR_READING_STATS));

  int y = metrics.topPadding + metrics.headerHeight + metrics.verticalSpacing;

  if (!READING_STATS.hasData()) {
    renderer.drawText(UI_10_FONT_ID, metrics.contentSidePadding, y + metrics.verticalSpacing, tr(STR_STATS_NO_DATA));
    const auto emptyLabels = mappedInput.mapLabels(tr(STR_HOME), "", "", "");
    GUI.drawButtonHints(renderer, emptyLabels.btn1, emptyLabels.btn2, emptyLabels.btn3, emptyLabels.btn4);
    renderer.displayBuffer();
    return;
  }

  char value[VALUE_BUF_SIZE];

  // ---- Last book ----
  const BookStats& last = READING_STATS.getLastBook();
  const std::string& lastTitle = READING_STATS.getLastBookTitle();

  GUI.drawSubHeader(renderer, Rect{0, y, pageWidth, SUB_HEADER_HEIGHT}, tr(STR_STATS_LAST_BOOK));
  y += SUB_HEADER_HEIGHT + metrics.verticalSpacing;

  if (!lastTitle.empty()) {
    const auto truncatedTitle =
        renderer.truncatedText(UI_10_FONT_ID, lastTitle.c_str(), pageWidth - metrics.contentSidePadding * 2);
    renderer.drawText(UI_10_FONT_ID, metrics.contentSidePadding, y, truncatedTitle.c_str(), true, EpdFontFamily::BOLD);
    y += STAT_ROW_HEIGHT;
  }

  formatDuration(last.seconds, value, VALUE_BUF_SIZE);
  y = drawStatRow(tr(STR_STATS_READING_TIME), value, y, pageWidth);

  snprintf(value, VALUE_BUF_SIZE, "%u", last.pageTurns);
  y = drawStatRow(tr(STR_STATS_PAGE_TURNS), value, y, pageWidth);

  formatPagesPerMinute(last.pageTurns, last.seconds, value, VALUE_BUF_SIZE);
  y = drawStatRow(tr(STR_STATS_PAGES_PER_MIN), value, y, pageWidth);

  // ---- All books ----
  y += metrics.verticalSpacing;
  GUI.drawSubHeader(renderer, Rect{0, y, pageWidth, SUB_HEADER_HEIGHT}, tr(STR_STATS_ALL_BOOKS));
  y += SUB_HEADER_HEIGHT + metrics.verticalSpacing;

  formatDuration(READING_STATS.getTotalSeconds(), value, VALUE_BUF_SIZE);
  y = drawStatRow(tr(STR_STATS_READING_TIME), value, y, pageWidth);

  snprintf(value, VALUE_BUF_SIZE, "%u", READING_STATS.getTotalPageTurns());
  y = drawStatRow(tr(STR_STATS_PAGE_TURNS), value, y, pageWidth);

  formatPagesPerMinute(READING_STATS.getTotalPageTurns(), READING_STATS.getTotalSeconds(), value, VALUE_BUF_SIZE);
  y = drawStatRow(tr(STR_STATS_PAGES_PER_MIN), value, y, pageWidth);

  snprintf(value, VALUE_BUF_SIZE, "%u", READING_STATS.getFinishedCount());
  drawStatRow(tr(STR_STATS_BOOKS_FINISHED), value, y, pageWidth);

  const auto labels = mappedInput.mapLabels(tr(STR_HOME), "", "", "");
  GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);

  renderer.displayBuffer();
}
