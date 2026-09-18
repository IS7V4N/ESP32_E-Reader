#include "DictionaryActivity.h"

#include <FontCacheManager.h>
#include <GfxRenderer.h>
#include <I18n.h>

#include <algorithm>
#include <cstdlib>

#include "CrossPointSettings.h"
#include "MappedInputManager.h"
#include "ReaderUtils.h"
#include "components/UITheme.h"

namespace {
constexpr char EM_SPACE[] = "\xe2\x80\x83";  // indent the layout puts in front of the first word of a paragraph

bool startsWithEmSpace(const std::string& s) { return s.compare(0, 3, EM_SPACE) == 0; }

std::vector<std::string> wrapText(const GfxRenderer& renderer, const int fontId, const std::string& text,
                                  const int maxWidth, const EpdFontFamily::Style style) {
  std::vector<std::string> lines;
  std::string line;
  size_t pos = 0;
  while (pos < text.size()) {
    size_t end = text.find(' ', pos);
    if (end == std::string::npos) end = text.size();
    const std::string word = text.substr(pos, end - pos);
    pos = end + 1;
    if (word.empty()) continue;

    const std::string candidate = line.empty() ? word : line + " " + word;
    if (!line.empty() && renderer.getTextWidth(fontId, candidate.c_str(), style) > maxWidth) {
      lines.push_back(line);
      line = word;
    } else {
      line = candidate;
    }
  }
  if (!line.empty()) lines.push_back(line);
  return lines;
}
}  // namespace

void DictionaryActivity::onEnter() {
  Activity::onEnter();
  page = section.loadPageFromSectionFile();
  buildWordList();
  cursor = 0;
  cursorCenterX = words.empty() ? 0 : words[0].x + words[0].width / 2;
  popupVisible = false;
  firstRender = true;
  requestUpdate();
}

void DictionaryActivity::onExit() {
  dictionary.close();
  words.clear();
  page.reset();
  Activity::onExit();
}

void DictionaryActivity::buildWordList() {
  words.clear();
  if (!page) return;

  // One allocation for the whole page (a page holds a few hundred words at most) instead of vector regrowth
  size_t wordTotal = 0;
  for (const auto& element : page->elements) {
    if (element->getTag() == TAG_PageLine) {
      const auto& block = static_cast<const PageLine&>(*element).getBlock();
      if (block) wordTotal += block->wordCount();
    }
  }
  words.reserve(wordTotal);

  const int fontId = SETTINGS.getReaderFontId();
  uint16_t line = 0;
  for (const auto& element : page->elements) {
    if (element->getTag() != TAG_PageLine) continue;
    const auto& pageLine = static_cast<const PageLine&>(*element);
    const auto& block = pageLine.getBlock();
    if (!block || block->isEmpty()) continue;

    const auto& texts = block->getWords();
    const auto& xs = block->getWordXpos();
    const auto& styles = block->getWordStyles();
    if (texts.size() != xs.size() || texts.size() != styles.size()) continue;

    for (size_t i = 0; i < texts.size(); i++) {
      int16_t x = pageLine.xPos + xs[i];
      const char* visible = texts[i].c_str();
      if (startsWithEmSpace(texts[i])) {
        x += renderer.getTextAdvanceX(fontId, EM_SPACE, styles[i]);
        visible += 3;
      }
      if (*visible == '\0') continue;
      const auto width = static_cast<int16_t>(renderer.getTextWidth(fontId, visible, styles[i]));
      words.push_back({&texts[i], x, pageLine.yPos, width, styles[i], line});
    }
    line++;
  }
}

void DictionaryActivity::moveHorizontal(const int delta) {
  const int next = cursor + delta;
  if (next < 0 || next >= static_cast<int>(words.size())) return;
  cursor = next;
  cursorCenterX = words[cursor].x + words[cursor].width / 2;
  requestUpdate();
}

void DictionaryActivity::moveVertical(const int delta) {
  // Wrap around: moving up from the first line lands on the last line and the other way round
  const int lineCount = static_cast<int>(words.back().line) + 1;
  const int targetLine = (static_cast<int>(words[cursor].line) + delta + lineCount) % lineCount;
  int best = -1;
  int bestDistance = 0;
  for (int i = 0; i < static_cast<int>(words.size()); i++) {
    if (words[i].line != targetLine) continue;
    const int distance = std::abs(words[i].x + words[i].width / 2 - cursorCenterX);
    if (best < 0 || distance < bestDistance) {
      best = i;
      bestDistance = distance;
    }
  }
  if (best < 0) return;
  cursor = best;  // keep cursorCenterX so that repeated up/down moves stay in the same column
  requestUpdate();
}

void DictionaryActivity::translateSelectedWord() {
  const std::string& selected = *words[cursor].text;

  // Words split by the layout at a hyphenation point: "trans-" / "lation"
  std::vector<std::string> attempts;
  const bool lastOnLine = cursor + 1 < static_cast<int>(words.size()) && words[cursor + 1].line != words[cursor].line;
  const bool firstOnLine = cursor > 0 && words[cursor - 1].line != words[cursor].line;
  if (lastOnLine && selected.size() > 1 && selected.back() == '-') {
    const std::string head = selected.substr(0, selected.size() - 1);
    attempts.push_back(head + *words[cursor + 1].text);
    attempts.push_back(selected + *words[cursor + 1].text);  // real compound such as "well-known"
  } else if (firstOnLine) {
    const std::string& previous = *words[cursor - 1].text;
    if (previous.size() > 1 && previous.back() == '-') {
      attempts.push_back(previous.substr(0, previous.size() - 1) + selected);
      attempts.push_back(previous + selected);
    }
  }
  attempts.push_back(selected);

  popupTitle = DictionaryLookup::cleanWord(attempts.front());
  popupBody.clear();
  DictionaryLookup::Result result = DictionaryLookup::Result::NotFound;
  for (const auto& attempt : attempts) {
    result = dictionary.lookup(attempt, popupBody);
    if (result != DictionaryLookup::Result::NotFound) break;
  }

  if (result == DictionaryLookup::Result::NoDictionary) {
    popupBody = tr(STR_DICT_MISSING);
  } else if (result == DictionaryLookup::Result::NotFound) {
    popupBody = tr(STR_DICT_NOT_FOUND);
  }
  if (popupTitle.empty()) popupTitle = tr(STR_DICTIONARY);

  popupVisible = true;
  requestUpdate();
}

void DictionaryActivity::exit() {
  ActivityResult cancelled;
  cancelled.isCancelled = true;
  setResult(std::move(cancelled));
  finish();
}

void DictionaryActivity::loop() {
  using Button = MappedInputManager::Button;

  if (popupVisible) {
    if (mappedInput.wasAnyReleased()) {
      popupVisible = false;
      requestUpdate();
    }
    return;
  }

  if (mappedInput.wasReleased(Button::Back)) {
    if (mappedInput.getHeldTime() >= ReaderUtils::DICTIONARY_LONG_PRESS_MS) {
      exit();
    } else if (!words.empty()) {
      moveVertical(-1);
    }
    return;
  }

  if (words.empty()) return;

  if (mappedInput.wasReleased(Button::Confirm)) {
    if (mappedInput.getHeldTime() >= ReaderUtils::DICTIONARY_LONG_PRESS_MS) {
      translateSelectedWord();
    } else {
      moveVertical(1);
    }
    return;
  }

  if (mappedInput.wasReleased(Button::Left)) {
    moveHorizontal(-1);
  } else if (mappedInput.wasReleased(Button::Right)) {
    moveHorizontal(1);
  }
}

void DictionaryActivity::drawFrame(const int fontId) const {
  if (page) {
    page->render(renderer, fontId, marginLeft, marginTop);
  }
  if (!words.empty()) {
    drawCursor(fontId);
  }
  if (popupVisible) {
    drawPopup(fontId);
  }
}

void DictionaryActivity::drawCursor(const int fontId) const {
  const WordRef& w = words[cursor];
  const char* text = w.text->c_str();
  if (startsWithEmSpace(*w.text)) text += 3;

  const int x = marginLeft + w.x;
  const int y = marginTop + w.y;
  renderer.fillRect(x - 2, y, w.width + 4, renderer.getLineHeight(fontId), true);
  renderer.drawText(fontId, x, y, text, false, w.style);
}

void DictionaryActivity::drawPopup(const int fontId) const {
  constexpr int outerMargin = 20;
  constexpr int padding = 12;
  constexpr int frame = 2;
  constexpr int hintsHeight = 60;

  const int screenWidth = renderer.getScreenWidth();
  const int screenHeight = renderer.getScreenHeight();
  const int lineHeight = renderer.getLineHeight(fontId);
  const int maxTextWidth = screenWidth - 2 * (outerMargin + padding);

  const auto bodyLines = wrapText(renderer, fontId, popupBody, maxTextWidth, EpdFontFamily::REGULAR);
  const int textHeight = static_cast<int>(bodyLines.size() + 1) * lineHeight;
  const int boxWidth = screenWidth - 2 * outerMargin;
  const int boxHeight = textHeight + 2 * padding;

  // Keep the popup away from the selected word
  const int wordY = marginTop + words[cursor].y;
  const int boxY = wordY < screenHeight / 2 ? screenHeight - hintsHeight - boxHeight : outerMargin;

  renderer.fillRect(outerMargin - frame, boxY - frame, boxWidth + 2 * frame, boxHeight + 2 * frame, true);
  renderer.fillRect(outerMargin, boxY, boxWidth, boxHeight, false);

  int y = boxY + padding;
  const int textX = outerMargin + padding;
  renderer.drawText(fontId, textX, y, popupTitle.c_str(), true, EpdFontFamily::BOLD);
  y += lineHeight;
  for (const auto& line : bodyLines) {
    renderer.drawText(fontId, textX, y, line.c_str(), true, EpdFontFamily::REGULAR);
    y += lineHeight;
  }
}

void DictionaryActivity::render(RenderLock&&) {
  const int fontId = SETTINGS.getReaderFontId();
  auto* fcm = renderer.getFontCacheManager();

  // Scan pass so the font cache holds every glyph of the page and the popup, then the real render
  renderer.clearScreen();
  auto scope = fcm->createPrewarmScope();
  drawFrame(fontId);
  scope.endScanAndPrewarm();

  renderer.clearScreen();
  drawFrame(fontId);

  if (!popupVisible) {
    const auto labels =
        mappedInput.mapLabels(tr(STR_DIR_UP), tr(STR_DIR_DOWN), tr(STR_DIR_LEFT), tr(STR_DIR_RIGHT));
    GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);
  }

  if (firstRender) {
    // The reader may have left a grayscale image on the panel, refresh it fully once when entering
    firstRender = false;
    renderer.displayBuffer(SETTINGS.textAntiAliasing ? HalDisplay::HALF_REFRESH : HalDisplay::FAST_REFRESH);
  } else {
    renderer.displayBuffer();
  }
}
