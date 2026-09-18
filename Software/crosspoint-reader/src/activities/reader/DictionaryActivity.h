#pragma once

#include <EpdFontFamily.h>
#include <Epub/Page.h>
#include <Epub/Section.h>

#include <memory>
#include <string>
#include <vector>

#include "../Activity.h"
#include "util/DictionaryLookup.h"

// Word picker on top of the current EPUB page with an English -> Hungarian lookup popup.
//   Left / Right      cursor to previous / next word
//   Back / Confirm    cursor one line up / down
//   long Confirm      translate the selected word
//   long Back         leave dictionary mode
class DictionaryActivity final : public Activity {
 public:
  explicit DictionaryActivity(GfxRenderer& renderer, MappedInputManager& mappedInput, Section& section,
                              const int marginLeft, const int marginTop)
      : Activity("Dictionary", renderer, mappedInput), section(section), marginLeft(marginLeft), marginTop(marginTop) {}

  void onEnter() override;
  void onExit() override;
  void loop() override;
  void render(RenderLock&&) override;

 private:
  struct WordRef {
    const std::string* text;
    int16_t x;  // page relative
    int16_t y;
    int16_t width;
    EpdFontFamily::Style style;
    uint16_t line;  // index among non-empty lines of the page
  };

  Section& section;
  const int marginLeft;
  const int marginTop;

  std::unique_ptr<Page> page;
  std::vector<WordRef> words;
  int cursor = 0;
  int cursorCenterX = 0;  // remembered column for up/down moves

  DictionaryLookup dictionary;
  bool popupVisible = false;
  std::string popupTitle;
  std::string popupBody;
  bool firstRender = true;

  void buildWordList();
  void moveHorizontal(int delta);
  void moveVertical(int delta);
  void translateSelectedWord();
  void exit();

  void drawFrame(int fontId) const;
  void drawCursor(int fontId) const;
  void drawPopup(int fontId) const;
};
