// ArduinoJson - https://arduinojson.org
// Copyright © 2014-2025, Benoit BLANCHON
// MIT License

#pragma once

#include <ArduinoJson/Namespace.hpp>

ARDUINOJSON_BEGIN_PRIVATE_NAMESPACE

// A meta-function that return the type T without the const modifier
template <typename T>
struct remove_const {
  using type = T;
};
template <typename T>
struct remove_const<const T> {
  using type = T;
};

template <typename T>
using remove_const_t = typename remove_const<T>::type;

ARDUINOJSON_END_PRIVATE_NAMESPACE
