#pragma once
/// \file Contains the \p UniqueID type.

#include <cassert>
#include <cstdint>
#include <functional>

class UniqueID {
public:
  enum class Kind : char {
    SOURCE_FILE = 's',
    FUNCTION = 'f',
    SCOPE_FILE_WRITE = 'w',
  };

public:
  /// Generates a new unique ID object. Every \p UniqueID has a different ID
  /// value.
  /// \param kind The kind of object that this ID is for.
  UniqueID(Kind kind);

  /// \warning Do not use the default constructor for this class. It is only
  /// here so that you can use this class in data structures that require it.
  UniqueID() { assert(false && "Do not use the UniqueID default constructor."); };

  /// String representation of this ID (e.g. 's_00001' is a \p SOURCE_FILE ID).
  const char* str() const;

  /// The kind/type of this ID (e.g. \p SOURCE_FILE ).
  Kind kind() const;

  /// The ID number.
  uint64_t value() const;

  bool operator==(UniqueID other) const;
  bool operator!=(UniqueID other) const;

private:
  union {
    char m_idStr[8];
    uint64_t m_idValue;
  };
  static_assert(sizeof(char[8]) == sizeof(uint64_t));
};

static_assert(sizeof(UniqueID) == sizeof(uint64_t));

/// This allows \p UniqueID to be used in data structures like
/// \p std::unordered_map as a key.
template <> struct std::hash<UniqueID> {
  std::size_t operator()(UniqueID id) const { return std::hash<uint64_t>()(id.value()); }
};
