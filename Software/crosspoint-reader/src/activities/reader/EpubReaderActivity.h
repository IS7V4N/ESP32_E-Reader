#pragma once
#include <Epub.h>
#include <Epub/FootnoteEntry.h>
#include <Epub/Section.h>

#include "EpubReaderMenuActivity.h"
#include "activities/Activity.h"

class EpubReaderActivity final : public Activity {
  std::shared_ptr<Epub> epub;
  std::unique_ptr<Section> section = nullptr;
  int currentSpineIndex = 0;
  int nextPageNumber = 0;
  // Set when navigating to a footnote href with a fragment (e.g. #note1).
  // Cleared on the next render after the new section loads and resolves it to a page.
  std::string pendingAnchor;
  int pagesUntilFullRefresh = 0;
  int cachedSpineIndex = 0;
  int cachedChapterTotalPageCount = 0;
  unsigned long lastPageTurnTime = 0UL;
  unsigned long pageTurnDuration = 0UL;
  // Signals that the next render should reposition within the newly loaded section
  // based on a cross-book percentage jump.
  bool pendingPercentJump = false;
  // Normalized 0.0-1.0 progress within the target spine item, computed from book percentage.
  float pendingSpineProgress = 0.0f;
  bool pendingScreenshot = false;
  bool skipNextButtonCheck = false;  // Skip button processing for one frame after subactivity exit
  bool automaticPageTurnActive = false;

  // Reading stats. Accumulated in RAM during the session and flushed once in onExit(),
  // which also covers deep sleep (enterDeepSleep -> goToSleep -> exitActivity -> onExit).
  unsigned long statsSessionMs = 0UL;
  unsigned long statsLastTick = 0UL;
  unsigned long statsLastInput = 0UL;
  // False until onEnter() accepted the book, so a failed open flushes nothing.
  bool statsSessionActive = false;
  bool bookAlreadyFinished = false;
  // Set by render() (render task), consumed by loop() (main task) so the SD write happens
  // off the render path. Single writer, single reader, idempotent - no mutex needed.
  volatile bool pendingFinishedMark = false;
  // Time on a page beyond this without a page turn does not count as reading time.
  static constexpr unsigned long STATS_IDLE_CUTOFF_MS = 3UL * 60 * 1000;
  // Swallows pathological loop gaps (e.g. a long blocking operation) instead of counting them.
  static constexpr unsigned long STATS_MAX_TICK_MS = 5000UL;
  // A book counts as read once it reaches this fraction of its total size.
  static constexpr float BOOK_FINISHED_THRESHOLD = 0.99f;

  void updateReadingTime();
  bool flushFinishedMark();

  // Footnote support
  std::vector<FootnoteEntry> currentPageFootnotes;
  struct SavedPosition {
    int spineIndex;
    int pageNumber;
  };
  static constexpr int MAX_FOOTNOTE_DEPTH = 3;
  SavedPosition savedPositions[MAX_FOOTNOTE_DEPTH] = {};
  int footnoteDepth = 0;

  void renderContents(std::unique_ptr<Page> page, int orientedMarginTop, int orientedMarginRight,
                      int orientedMarginBottom, int orientedMarginLeft);
  void renderStatusBar() const;
  void silentIndexNextChapterIfNeeded(uint16_t viewportWidth, uint16_t viewportHeight);
  void saveProgress(int spineIndex, int currentPage, int pageCount);
  // Jump to a percentage of the book (0-100), mapping it to spine and page.
  void jumpToPercent(int percent);
  void onReaderMenuConfirm(EpubReaderMenuActivity::MenuAction action);
  void applyOrientation(uint8_t orientation);
  void toggleAutoPageTurn(uint8_t selectedPageTurnOption);
  void pageTurn(bool isForwardTurn);

  // Footnote navigation
  void navigateToHref(const std::string& href, bool savePosition = false);
  void restoreSavedPosition();

 public:
  explicit EpubReaderActivity(GfxRenderer& renderer, MappedInputManager& mappedInput, std::unique_ptr<Epub> epub)
      : Activity("EpubReader", renderer, mappedInput), epub(std::move(epub)) {}
  void onEnter() override;
  void onExit() override;
  void loop() override;
  void render(RenderLock&& lock) override;
  bool isReaderActivity() const override { return true; }
};
