// ArduinoJson - https://arduinojson.org
// Copyright © 2014-2025, Benoit BLANCHON
// MIT License

#pragma once

#include <ArduinoJson/Namespace.hpp>

ARDUINOJSON_BEGIN_PRIVATE_NAMESPACE

// A meta-function that return the type T if Condition is true.
template <bool Condition, typename T = void>
struct enable_if {};

template <typename T>
struct enable_if<true, T> {
  using type = T;
};

template <bool Condition, typename T = void>
using enable_if_t = typename enable_if<Condition, T>::type;

ARDUINOJSON_END_PRIVATE_NAMESPACE
