// ArduinoJson - https://arduinojson.org
// Copyright © 2014-2025, Benoit BLANCHON
// MIT License

#pragma once

#include <ArduinoJson/Numbers/arithmeticCompare.hpp>
#include <ArduinoJson/Polyfills/attributes.hpp>
#include <ArduinoJson/Polyfills/type_traits.hpp>
#include <ArduinoJson/Variant/VariantTag.hpp>

ARDUINOJSON_BEGIN_PUBLIC_NAMESPACE

class JsonVariantConst;

ARDUINOJSON_END_PUBLIC_NAMESPACE

ARDUINOJSON_BEGIN_PRIVATE_NAMESPACE

template <typename T>
CompareResult compare(JsonVariantConst lhs,
                      const T& rhs);  // VariantCompare.cpp

struct VariantOperatorTag {};

template <typename TVariant>
struct VariantOperators : VariantOperatorTag {
  // Returns the default value if the JsonVariant is unbound or incompatible
  //
  // int operator|(JsonVariant, int)
  // float operator|(JsonVariant, float)
  // bool operator|(JsonVariant, bool)
  template <typename T,
            enable_if_t<!IsVariant<T>::value && !is_array<T>::value, int> = 0>
  friend T operator|(const TVariant& variant, const T& defaultValue) {
    if (variant.template is<T>())
      return variant.template as<T>();
    else
      return defaultValue;
  }
  //
  // const char* operator|(JsonVariant, const char*)
  friend const char* operator|(const TVariant& variant,
                               const char* defaultValue) {
    if (variant.template is<const char*>())
      return variant.template as<const char*>();
    else
      return defaultValue;
  }
  //
  // JsonVariant operator|(JsonVariant, JsonVariant)
  template <typename T>
  friend enable_if_t<IsVariant<T>::value, JsonVariantConst> operator|(
      const TVariant& variant, const T& defaultValue) {
    if (variant)
      return variant;
    else
      return defaultValue;
  }

  // value == TVariant
  template <typename T>
  friend bool operator==(T* lhs, const TVariant& rhs) {
    return compare(rhs, lhs) == COMPARE_RESULT_EQUAL;
  }
  template <typename T>
  friend bool operator==(const T& lhs, const TVariant& rhs) {
    return compare(rhs, lhs) == COMPARE_RESULT_EQUAL;
  }

  // TVariant == value
  template <typename T>
  friend bool operator==(const TVariant& lhs, T* rhs) {
    return compare(lhs, rhs) == COMPARE_RESULT_EQUAL;
  }
  template <typename T,
            enable_if_t<!is_base_of<VariantOperatorTag, T>::value, int> = 0>
  friend bool operator==(const TVariant& lhs, const T& rhs) {
    return compare(lhs, rhs) == COMPARE_RESULT_EQUAL;
  }

  // value != TVariant
  template <typename T>
  friend bool operator!=(T* lhs, const TVariant& rhs) {
    return compare(rhs, lhs) != COMPARE_RESULT_EQUAL;
  }
  template <typename T>
  friend bool operator!=(const T& lhs, const TVariant& rhs) {
    return compare(rhs, lhs) != COMPARE_RESULT_EQUAL;
  }

  // TVariant != value
  template <typename T>
  friend bool operator!=(const TVariant& lhs, T* rhs) {
    return compare(lhs, rhs) != COMPARE_RESULT_EQUAL;
  }
  template <typename T,
            enable_if_t<!is_base_of<VariantOperatorTag, T>::value, int> = 0>
  friend bool operator!=(TVariant lhs, const T& rhs) {
    return compare(lhs, rhs) != COMPARE_RESULT_EQUAL;
  }

  // value < TVariant
  template <typename T>
  friend bool operator<(T* lhs, const TVariant& rhs) {
    return compare(rhs, lhs) == COMPARE_RESULT_GREATER;
  }
  template <typename T>
  friend bool operator<(const T& lhs, const TVariant& rhs) {
    return compare(rhs, lhs) == COMPARE_RESULT_GREATER;
  }

  // TVariant < value
  template <typename T>
  friend bool operator<(const TVariant& lhs, T* rhs) {
    return compare(lhs, rhs) == COMPARE_RESULT_LESS;
  }
  template <typename T,
            enable_if_t<!is_base_of<VariantOperatorTag, T>::value, int> = 0>
  friend bool operator<(TVariant lhs, const T& rhs) {
    return compare(lhs, rhs) == COMPARE_RESULT_LESS;
  }

  // value <= TVariant
  template <typename T>
  friend bool operator<=(T* lhs, const TVariant& rhs) {
    return (compare(rhs, lhs) & COMPARE_RESULT_GREATER_OR_EQUAL) != 0;
  }
  template <typename T>
  friend bool operator<=(const T& lhs, const TVariant& rhs) {
    return (compare(rhs, lhs) & COMPARE_RESULT_GREATER_OR_EQUAL) != 0;
  }

  // TVariant <= value
  template <typename T>
  friend bool operator<=(const TVariant& lhs, T* rhs) {
    return (compare(lhs, rhs) & COMPARE_RESULT_LESS_OR_EQUAL) != 0;
  }
  template <typename T,
            enable_if_t<!is_base_of<VariantOperatorTag, T>::value, int> = 0>
  friend bool operator<=(TVariant lhs, const T& rhs) {
    return (compare(lhs, rhs) & COMPARE_RESULT_LESS_OR_EQUAL) != 0;
  }

  // value > TVariant
  template <typename T>
  friend bool operator>(T* lhs, const TVariant& rhs) {
    return compare(rhs, lhs) == COMPARE_RESULT_LESS;
  }
  template <typename T>
  friend bool operator>(const T& lhs, const TVariant& rhs) {
    return compare(rhs, lhs) == COMPARE_RESULT_LESS;
  }

  // TVariant > value
  template <typename T>
  friend bool operator>(const TVariant& lhs, T* rhs) {
    return compare(lhs, rhs) == COMPARE_RESULT_GREATER;
  }
  template <typename T,
            enable_if_t<!is_base_of<VariantOperatorTag, T>::value, int> = 0>
  friend bool operator>(TVariant lhs, const T& rhs) {
    return compare(lhs, rhs) == COMPARE_RESULT_GREATER;
  }

  // value >= TVariant
  template <typename T>
  friend bool operator>=(T* lhs, const TVariant& rhs) {
    return (compare(rhs, lhs) & COMPARE_RESULT_LESS_OR_EQUAL) != 0;
  }
  template <typename T>
  friend bool operator>=(const T& lhs, const TVariant& rhs) {
    return (compare(rhs, lhs) & COMPARE_RESULT_LESS_OR_EQUAL) != 0;
  }

  // TVariant >= value
  template <typename T>
  friend bool operator>=(const TVariant& lhs, T* rhs) {
    return (compare(lhs, rhs) & COMPARE_RESULT_GREATER_OR_EQUAL) != 0;
  }
  template <typename T,
            enable_if_t<!is_base_of<VariantOperatorTag, T>::value, int> = 0>
  friend bool operator>=(const TVariant& lhs, const T& rhs) {
    return (compare(lhs, rhs) & COMPARE_RESULT_GREATER_OR_EQUAL) != 0;
  }
};

ARDUINOJSON_END_PRIVATE_NAMESPACE
