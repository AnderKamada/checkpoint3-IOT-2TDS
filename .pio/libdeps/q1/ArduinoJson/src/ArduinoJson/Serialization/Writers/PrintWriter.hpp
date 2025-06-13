// ArduinoJson - https://arduinojson.org
// Copyright © 2014-2025, Benoit BLANCHON
// MIT License

#pragma once

#include <Arduino.h>

ARDUINOJSON_BEGIN_PRIVATE_NAMESPACE

template <typename TDestination>
class Writer<TDestination,
             enable_if_t<is_base_of<::Print, TDestination>::value>> {
 public:
  explicit Writer(::Print& print) : print_(&print) {}

  size_t write(uint8_t c) {
    return print_->write(c);
  }

  size_t write(const uint8_t* s, size_t n) {
    return print_->write(s, n);
  }

 private:
  ::Print* print_;
};

ARDUINOJSON_END_PRIVATE_NAMESPACE
