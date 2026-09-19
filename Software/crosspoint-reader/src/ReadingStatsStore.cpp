#include "ReadingStatsStore.h"

#include <HalStorage.h>
#include <JsonSettingsIO.h>
#include <Logging.h>

#include <algorithm>
#include <functional>

namespace {
constexpr char READING_STATS_FILE_JSON[] = "/.crosspoint/stats.json";
}  // namespace

ReadingStatsStore ReadingStatsStore::instance;

uint32_t ReadingStatsStore::keyForPath(const std::string& path) {
  // Matches the cache directory key in lib/Epub/Epub.h. std::hash returns size_t (32-bit on
  // the ESP32-C3), so the truncation below is a no-op on target and only matters for host builds.
  return static_cast<uint32_t>(std::hash<std::string>{}(path));
}

void ReadingStatsStore::beginBook(const std::string& path, const std::string& title) {
  const uint32_t key = keyForPath(path);
  if (lastBook.key != key) {
    lastBook = BookStats{key, 0, 0};
  }
  lastBookTitle = title;
}

void ReadingStatsStore::addPageTurn() {
  lastBook.pageTurns++;
  totalPageTurns++;
}

void ReadingStatsStore::addSeconds(const uint32_t seconds) {
  if (seconds == 0) {
    return;
  }
  lastBook.seconds += seconds;
  totalSeconds += seconds;
}

bool ReadingStatsStore::isFinished(const std::string& path) const {
  const uint32_t key = keyForPath(path);
  return std::find(finishedKeys.begin(), finishedKeys.end(), key) != finishedKeys.end();
}

bool ReadingStatsStore::markFinished(const std::string& path) {
  if (isFinished(path)) {
    return false;
  }

  if (finishedKeys.size() >= MAX_FINISHED_KEYS) {
    // Drop the oldest key. finishedCount is not touched, so the total stays correct.
    finishedKeys.erase(finishedKeys.begin());
  }
  finishedKeys.push_back(keyForPath(path));
  finishedCount++;

  LOG_DBG("RST", "Book finished, total finished: %u", finishedCount);
  return true;
}

bool ReadingStatsStore::saveToFile() const {
  Storage.mkdir("/.crosspoint");
  return JsonSettingsIO::saveReadingStats(*this, READING_STATS_FILE_JSON);
}

bool ReadingStatsStore::loadFromFile() {
  if (!Storage.exists(READING_STATS_FILE_JSON)) {
    return false;
  }

  const String json = Storage.readFile(READING_STATS_FILE_JSON);
  if (json.isEmpty()) {
    return false;
  }

  return JsonSettingsIO::loadReadingStats(*this, json.c_str());
}
