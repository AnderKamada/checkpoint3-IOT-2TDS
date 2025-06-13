// ArduinoJson - https://arduinojson.org
// Copyright © 2014-2025, Benoit BLANCHON
// MIT License

#pragma once

#include <ArduinoJson/Polyfills/type_traits.hpp>

ARDUINOJSON_BEGIN_PRIVATE_NAMESPACE

template <typename T>
struct IsCharOrVoid {
  static const bool value =
      is_same<T, void>::value || is_same<T, char>::value ||
      is_same<T, unsigned char>::value || is_same<T, signed char>::value;
};

template <typename T>
struct IsCharOrVoid<const T> : IsCharOrVoid<T> {};

template <typename TSource>
struct Reader<TSource*, enable_if_t<IsCharOrVoid<TSource>::value>> {
  const char* ptr_;

 public:
  explicit Reader(const void* ptr)
      : ptr_(ptr ? reinterpret_cast<const char*>(ptr) : "") {}

  int read() {
    return static_cast<unsigned char>(*ptr_++);
  }

  size_t readBytes(char* buffer, size_t length) {
    for (size_t i = 0; i < length; i++)
      buffer[i] = *ptr_++;
    return length;
  }
};

template <typename TSource>
struct BoundedReader<TSource*, enable_if_t<IsCharOrVoid<TSource>::value>>
    : public IteratorReader<const char*> {
 public:
  explicit BoundedReader(const void* ptr, size_t len)
      : IteratorReader<const char*>(reinterpret_cast<const char*>(ptr),
                                    reinterpret_cast<const char*>(ptr) + len) {}
};

ARDUINOJSON_END_PRIVATE_NAMESPACE
