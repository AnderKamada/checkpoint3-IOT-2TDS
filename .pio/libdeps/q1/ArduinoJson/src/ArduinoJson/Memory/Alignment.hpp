// ArduinoJson - https://arduinojson.org
// Copyright © 2014-2025, Benoit BLANCHON
// MIT License

#pragma once

#include <ArduinoJson/Namespace.hpp>

#include <stddef.h>  // size_t

ARDUINOJSON_BEGIN_PRIVATE_NAMESPACE

#if ARDUINOJSON_ENABLE_ALIGNMENT

inline bool isAligned(size_t value) {
  const size_t mask = sizeof(void*) - 1;
  size_t addr = value;
  return (addr & mask) == 0;
}

inline size_t addPadding(size_t bytes) {
  const size_t mask = sizeof(void*) - 1;
  return (bytes + mask) & ~mask;
}

template <size_t bytes>
struct AddPadding {
  static const size_t mask = sizeof(void*) - 1;
  static const size_t value = (bytes + mask) & ~mask;
};

#else

inline bool isAligned(size_t) {
  return true;
}

inline size_t addPadding(size_t bytes) {
  return bytes;
}

template <size_t bytes>
struct AddPadding {
  static const size_t value = bytes;
};

#endif

template <typename T>
inline bool isAligned(T* ptr) {
  return isAligned(reinterpret_cast<size_t>(ptr));
}

template <typename T>
inline T* addPadding(T* p) {
  size_t address = addPadding(reinterpret_cast<size_t>(p));
  return reinterpret_cast<T*>(address);
}

ARDUINOJSON_END_PRIVATE_NAMESPACE
