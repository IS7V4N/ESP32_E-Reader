#pragma once

#include <HalStorage.h>

#include <cstdint>
#include <string>

// English -> Hungarian word lookup in a sorted binary dictionary stored on the SD card.
// The file is produced by scripts/build_dict.py, see the format description there.
class DictionaryLookup {
 public:
  enum class Result { Found, NotFound, NoDictionary };

  static constexpr const char* DICT_PATH = "/dict/en-hu.dic";

  ~DictionaryLookup() { close(); }

  bool open();
  void close();

  // Looks up a word as it appears on the page (punctuation, capitals and typographic quotes are tolerated,
  // simple English inflections are stripped when the exact form is missing).
  Result lookup(const std::string& rawWord, std::string& translation);

  // Lowercase ASCII word without surrounding punctuation, or an empty string if the text is not an English word.
  static std::string cleanWord(const std::string& raw);

 private:
  static constexpr size_t MAX_KEY_LEN = 24;
  static constexpr size_t HEADER_SIZE = 32;

  bool find(const std::string& key, std::string& translation);

  HalFile file;
  bool opened = false;
  uint32_t count = 0;
  uint32_t indexOffset = 0;
  uint32_t dataOffset = 0;
  uint16_t keyLen = 0;
};
