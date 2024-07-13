#include <compiler/UniqueID.h>

#include <cstdint>
#include <atomic>
#include <cassert>
#include <cstdlib>
#include <cstring>

/// Get the next ID value (uses \p std::atomic so that it's thread safe.)
static uint32_t getNextIdValue() {
  static std::atomic<uint32_t> nextIdValue{1};
  return nextIdValue.fetch_add(1);
}

UniqueID::UniqueID(const Kind kind) {
  const uint32_t val = getNextIdValue();

  // there's is a maximum ID value of 1,048,575
  if(val > 0xfffff)
    std::abort();

  std::strcpy(m_idStr, "?_00000");

  // the 'Kind' enum is 'char' under the hood, the enum values are set to the
  // character that represents the type (like 'S' for 'SOURCE_FILE').
  m_idStr[0] = static_cast<char>(kind);

  for (char i = 0; i < 5; i++) {
    char nibble = (val >> (16 - i * 4)) & 0xf;
    m_idStr[2 + i] = (nibble < 10) ? '0' + nibble : 'a' + (nibble - 10);
  }
}

UniqueID::Kind UniqueID::kind() const { return static_cast<Kind>(m_idStr[0]); }

uint64_t UniqueID::value() const { return (m_idValue & 0x0000ffffffffffff00) >> 8; }

bool UniqueID::operator==(const UniqueID& other) const { return m_idValue == other.m_idValue; }
bool UniqueID::operator!=(const UniqueID& other) const { return m_idValue != other.m_idValue; }

const char* UniqueID::str() const { return m_idStr; }
