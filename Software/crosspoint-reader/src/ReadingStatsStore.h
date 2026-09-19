#pragma once
#include <cstdint>
#include <string>
#include <vector>

// Aggregated reading statistics for a single book.
// Books are identified by a hash of their filepath - the same key the Epub cache directory
// is derived from (see lib/Epub/Epub.h) - so no path strings are kept in DRAM.
struct BookStats {
  uint32_t key = 0;
  uint32_t seconds = 0;
  uint32_t pageTurns = 0;
};

class ReadingStatsStore;
namespace JsonSettingsIO {
bool loadReadingStats(ReadingStatsStore& store, const char* json);
}  // namespace JsonSettingsIO

class ReadingStatsStore {
  // Static instance
  static ReadingStatsStore instance;

  // Stats for the most recently opened book only ("last book" breakdown).
  BookStats lastBook;
  // Title of the last book, kept for display purposes only.
  std::string lastBookTitle;

  // Totals across every book ever read. Monotonic, never reset.
  uint32_t totalSeconds = 0;
  uint32_t totalPageTurns = 0;
  uint32_t finishedCount = 0;

  // Hashes of books that reached the finished threshold, used to avoid double counting
  // and to mark them in the recent books list. Oldest entry is dropped when full;
  // finishedCount stays correct because it is never decremented.
  std::vector<uint32_t> finishedKeys;

  friend bool JsonSettingsIO::loadReadingStats(ReadingStatsStore&, const char*);

 public:
  static constexpr size_t MAX_FINISHED_KEYS = 64;

  ~ReadingStatsStore() = default;

  // Get singleton instance
  static ReadingStatsStore& getInstance() { return instance; }

  // Book identity key, matching the Epub cache directory hash
  static uint32_t keyForPath(const std::string& path);

  // Start tracking a book. Resets the "last book" counters when a different book is opened.
  void beginBook(const std::string& path, const std::string& title);

  // In-memory accumulation only - no SD writes. Flushed by saveToFile() on reader exit.
  void addPageTurn();
  void addSeconds(uint32_t seconds);

  // Returns true if the book was not previously finished (i.e. the caller should persist).
  bool markFinished(const std::string& path);
  bool isFinished(const std::string& path) const;

  const BookStats& getLastBook() const { return lastBook; }
  const std::string& getLastBookTitle() const { return lastBookTitle; }
  uint32_t getTotalSeconds() const { return totalSeconds; }
  uint32_t getTotalPageTurns() const { return totalPageTurns; }
  uint32_t getFinishedCount() const { return finishedCount; }
  const std::vector<uint32_t>& getFinishedKeys() const { return finishedKeys; }

  bool hasData() const { return totalSeconds > 0 || totalPageTurns > 0 || finishedCount > 0; }

  bool saveToFile() const;
  bool loadFromFile();
};

// Helper macro to access reading stats store
#define READING_STATS ReadingStatsStore::getInstance()
