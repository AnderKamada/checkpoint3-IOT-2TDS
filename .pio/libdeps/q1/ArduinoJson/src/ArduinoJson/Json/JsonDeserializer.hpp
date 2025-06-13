// ArduinoJson - https://arduinojson.org
// Copyright © 2014-2025, Benoit BLANCHON
// MIT License

#pragma once

#include <ArduinoJson/Deserialization/deserialize.hpp>
#include <ArduinoJson/Json/EscapeSequence.hpp>
#include <ArduinoJson/Json/Latch.hpp>
#include <ArduinoJson/Json/Utf16.hpp>
#include <ArduinoJson/Json/Utf8.hpp>
#include <ArduinoJson/Memory/ResourceManager.hpp>
#include <ArduinoJson/Numbers/parseNumber.hpp>
#include <ArduinoJson/Polyfills/assert.hpp>
#include <ArduinoJson/Polyfills/type_traits.hpp>
#include <ArduinoJson/Polyfills/utility.hpp>
#include <ArduinoJson/Variant/VariantData.hpp>

ARDUINOJSON_BEGIN_PRIVATE_NAMESPACE

template <typename TReader>
class JsonDeserializer {
 public:
  JsonDeserializer(ResourceManager* resources, TReader reader)
      : stringBuilder_(resources),
        foundSomething_(false),
        latch_(reader),
        resources_(resources) {}

  template <typename TFilter>
  DeserializationError parse(VariantData& variant, TFilter filter,
                             DeserializationOption::NestingLimit nestingLimit) {
    DeserializationError::Code err;

    err = parseVariant(variant, filter, nestingLimit);

    if (!err && latch_.last() != 0 && variant.isFloat()) {
      // We don't detect trailing characters earlier, so we need to check now
      return DeserializationError::InvalidInput;
    }

    return err;
  }

 private:
  char current() {
    return latch_.current();
  }

  void move() {
    latch_.clear();
  }

  bool eat(char charToSkip) {
    if (current() != charToSkip)
      return false;
    move();
    return true;
  }

  template <typename TFilter>
  DeserializationError::Code parseVariant(
      VariantData& variant, TFilter filter,
      DeserializationOption::NestingLimit nestingLimit) {
    DeserializationError::Code err;

    err = skipSpacesAndComments();
    if (err)
      return err;

    switch (current()) {
      case '[':
        if (filter.allowArray())
          return parseArray(variant.toArray(), filter, nestingLimit);
        else
          return skipArray(nestingLimit);

      case '{':
        if (filter.allowObject())
          return parseObject(variant.toObject(), filter, nestingLimit);
        else
          return skipObject(nestingLimit);

      case '\"':
      case '\'':
        if (filter.allowValue())
          return parseStringValue(variant);
        else
          return skipQuotedString();

      case 't':
        if (filter.allowValue())
          variant.setBoolean(true);
        return skipKeyword("true");

      case 'f':
        if (filter.allowValue())
          variant.setBoolean(false);
        return skipKeyword("false");

      case 'n':
        // the variant should already by null, except if the same object key was
        // used twice, as in {"a":1,"a":null}
        return skipKeyword("null");

      default:
        if (filter.allowValue())
          return parseNumericValue(variant);
        else
          return skipNumericValue();
    }
  }

  DeserializationError::Code skipVariant(
      DeserializationOption::NestingLimit nestingLimit) {
    DeserializationError::Code err;

    err = skipSpacesAndComments();
    if (err)
      return err;

    switch (current()) {
      case '[':
        return skipArray(nestingLimit);

      case '{':
        return skipObject(nestingLimit);

      case '\"':
      case '\'':
        return skipQuotedString();

      case 't':
        return skipKeyword("true");

      case 'f':
        return skipKeyword("false");

      case 'n':
        return skipKeyword("null");

      default:
        return skipNumericValue();
    }
  }

  template <typename TFilter>
  DeserializationError::Code parseArray(
      ArrayData& array, TFilter filter,
      DeserializationOption::NestingLimit nestingLimit) {
    DeserializationError::Code err;

    if (nestingLimit.reached())
      return DeserializationError::TooDeep;

    // Skip opening braket
    ARDUINOJSON_ASSERT(current() == '[');
    move();

    // Skip spaces
    err = skipSpacesAndComments();
    if (err)
      return err;

    // Empty array?
    if (eat(']'))
      return DeserializationError::Ok;

    TFilter elementFilter = filter[0UL];

    // Read each value
    for (;;) {
      if (elementFilter.allow()) {
        // Allocate slot in array
        VariantData* value = array.addElement(resources_);
        if (!value)
          return DeserializationError::NoMemory;

        // 1 - Parse value
        err = parseVariant(*value, elementFilter, nestingLimit.decrement());
        if (err)
          return err;
      } else {
        err = skipVariant(nestingLimit.decrement());
        if (err)
          return err;
      }

      // 2 - Skip spaces
      err = skipSpacesAndComments();
      if (err)
        return err;

      // 3 - More values?
      if (eat(']'))
        return DeserializationError::Ok;
      if (!eat(','))
        return DeserializationError::InvalidInput;
    }
  }

  DeserializationError::Code skipArray(
      DeserializationOption::NestingLimit nestingLimit) {
    DeserializationError::Code err;

    if (nestingLimit.reached())
      return DeserializationError::TooDeep;

    // Skip opening braket
    ARDUINOJSON_ASSERT(current() == '[');
    move();

    // Read each value
    for (;;) {
      // 1 - Skip value
      err = skipVariant(nestingLimit.decrement());
      if (err)
        return err;

      // 2 - Skip spaces
      err = skipSpacesAndComments();
      if (err)
        return err;

      // 3 - More values?
      if (eat(']'))
        return DeserializationError::Ok;
      if (!eat(','))
        return DeserializationError::InvalidInput;
    }
  }

  template <typename TFilter>
  DeserializationError::Code parseObject(
      ObjectData& object, TFilter filter,
      DeserializationOption::NestingLimit nestingLimit) {
    DeserializationError::Code err;

    if (nestingLimit.reached())
      return DeserializationError::TooDeep;

    // Skip opening brace
    ARDUINOJSON_ASSERT(current() == '{');
    move();

    // Skip spaces
    err = skipSpacesAndComments();
    if (err)
      return err;

    // Empty object?
    if (eat('}'))
      return DeserializationError::Ok;

    // Read each key value pair
    for (;;) {
      // Parse key
      err = parseKey();
      if (err)
        return err;

      // Skip spaces
      err = skipSpacesAndComments();
      if (err)
        return err;

      // Colon
      if (!eat(':'))
        return DeserializationError::InvalidInput;

      JsonString key = stringBuilder_.str();

      TFilter memberFilter = filter[key];

      if (memberFilter.allow()) {
        auto member = object.getMember(adaptString(key), resources_);
        if (!member) {
          auto keyVariant = object.addPair(&member, resources_);
          if (!keyVariant)
            return DeserializationError::NoMemory;

          stringBuilder_.save(keyVariant);
        } else {
          member->clear(resources_);
        }

        // Parse value
        err = parseVariant(*member, memberFilter, nestingLimit.decrement());
        if (err)
          return err;
      } else {
        err = skipVariant(nestingLimit.decrement());
        if (err)
          return err;
      }

      // Skip spaces
      err = skipSpacesAndComments();
      if (err)
        return err;

      // More keys/values?
      if (eat('}'))
        return DeserializationError::Ok;
      if (!eat(','))
        return DeserializationError::InvalidInput;

      // Skip spaces
      err = skipSpacesAndComments();
      if (err)
        return err;
    }
  }

  DeserializationError::Code skipObject(
      DeserializationOption::NestingLimit nestingLimit) {
    DeserializationError::Code err;

    if (nestingLimit.reached())
      return DeserializationError::TooDeep;

    // Skip opening brace
    ARDUINOJSON_ASSERT(current() == '{');
    move();

    // Skip spaces
    err = skipSpacesAndComments();
    if (err)
      return err;

    // Empty object?
    if (eat('}'))
      return DeserializationError::Ok;

    // Read each key value pair
    for (;;) {
      // Skip key
      err = skipKey();
      if (err)
        return err;

      // Skip spaces
      err = skipSpacesAndComments();
      if (err)
        return err;

      // Colon
      if (!eat(':'))
        return DeserializationError::InvalidInput;

      // Skip value
      err = skipVariant(nestingLimit.decrement());
      if (err)
        return err;

      // Skip spaces
      err = skipSpacesAndComments();
      if (err)
        return err;

      // More keys/values?
      if (eat('}'))
        return DeserializationError::Ok;
      if (!eat(','))
        return DeserializationError::InvalidInput;

      err = skipSpacesAndComments();
      if (err)
        return err;
    }
  }

  DeserializationError::Code parseKey() {
    stringBuilder_.startString();
    if (isQuote(current())) {
      return parseQuotedString();
    } else {
      return parseNonQuotedString();
    }
  }

  DeserializationError::Code parseStringValue(VariantData& variant) {
    DeserializationError::Code err;

    stringBuilder_.startString();

    err = parseQuotedString();
    if (err)
      return err;

    stringBuilder_.save(&variant);

    return DeserializationError::Ok;
  }

  DeserializationError::Code parseQuotedString() {
#if ARDUINOJSON_DECODE_UNICODE
    Utf16::Codepoint codepoint;
    DeserializationError::Code err;
#endif
    const char stopChar = current();

    move();
    for (;;) {
      char c = current();
      move();
      if (c == stopChar)
        break;

      if (c == '\0')
        return DeserializationError::IncompleteInput;

      if (c == '\\') {
        c = current();

        if (c == '\0')
          return DeserializationError::IncompleteInput;

        if (c == 'u') {
#if ARDUINOJSON_DECODE_UNICODE
          move();
          uint16_t codeunit;
          err = parseHex4(codeunit);
          if (err)
            return err;
          if (codepoint.append(codeunit))
            Utf8::encodeCodepoint(codepoint.value(), stringBuilder_);
#else
          stringBuilder_.append('\\');
#endif
          continue;
        }

        // replace char
        c = EscapeSequence::unescapeChar(c);
        if (c == '\0')
          return DeserializationError::InvalidInput;
        move();
      }

      stringBuilder_.append(c);
    }

    if (!stringBuilder_.isValid())
      return DeserializationError::NoMemory;

    return DeserializationError::Ok;
  }

  DeserializationError::Code parseNonQuotedString() {
    char c = current();
    ARDUINOJSON_ASSERT(c);

    if (canBeInNonQuotedString(c)) {  // no quotes
      do {
        move();
        stringBuilder_.append(c);
        c = current();
      } while (canBeInNonQuotedString(c));
    } else {
      return DeserializationError::InvalidInput;
    }

    if (!stringBuilder_.isValid())
      return DeserializationError::NoMemory;

    return DeserializationError::Ok;
  }

  DeserializationError::Code skipKey() {
    if (isQuote(current())) {
      return skipQuotedString();
    } else {
      return skipNonQuotedString();
    }
  }

  DeserializationError::Code skipQuotedString() {
    const char stopChar = current();

    move();
    for (;;) {
      char c = current();
      move();
      if (c == stopChar)
        break;
      if (c == '\0')
        return DeserializationError::IncompleteInput;
      if (c == '\\') {
        if (current() != '\0')
          move();
      }
    }

    return DeserializationError::Ok;
  }

  DeserializationError::Code skipNonQuotedString() {
    char c = current();
    while (canBeInNonQuotedString(c)) {
      move();
      c = current();
    }
    return DeserializationError::Ok;
  }

  DeserializationError::Code parseNumericValue(VariantData& result) {
    uint8_t n = 0;

    char c = current();
    while (canBeInNumber(c) && n < 63) {
      move();
      buffer_[n++] = c;
      c = current();
    }
    buffer_[n] = 0;

    auto number = parseNumber(buffer_);
    switch (number.type()) {
      case NumberType::UnsignedInteger:
        if (result.setInteger(number.asUnsignedInteger(), resources_))
          return DeserializationError::Ok;
        else
          return DeserializationError::NoMemory;

      case NumberType::SignedInteger:
        if (result.setInteger(number.asSignedInteger(), resources_))
          return DeserializationError::Ok;
        else
          return DeserializationError::NoMemory;

      case NumberType::Float:
        if (result.setFloat(number.asFloat(), resources_))
          return DeserializationError::Ok;
        else
          return DeserializationError::NoMemory;

#if ARDUINOJSON_USE_DOUBLE
      case NumberType::Double:
        if (result.setFloat(number.asDouble(), resources_))
          return DeserializationError::Ok;
        else
          return DeserializationError::NoMemory;
#endif

      default:
        return DeserializationError::InvalidInput;
    }
  }

  DeserializationError::Code skipNumericValue() {
    char c = current();
    while (canBeInNumber(c)) {
      move();
      c = current();
    }
    return DeserializationError::Ok;
  }

  DeserializationError::Code parseHex4(uint16_t& result) {
    result = 0;
    for (uint8_t i = 0; i < 4; ++i) {
      char digit = current();
      if (!digit)
        return DeserializationError::IncompleteInput;
      uint8_t value = decodeHex(digit);
      if (value > 0x0F)
        return DeserializationError::InvalidInput;
      result = uint16_t((result << 4) | value);
      move();
    }
    return DeserializationError::Ok;
  }

  static inline bool isBetween(char c, char min, char max) {
    return min <= c && c <= max;
  }

  static inline bool canBeInNumber(char c) {
    return isBetween(c, '0', '9') || c == '+' || c == '-' || c == '.' ||
#if ARDUINOJSON_ENABLE_NAN || ARDUINOJSON_ENABLE_INFINITY
           isBetween(c, 'A', 'Z') || isBetween(c, 'a', 'z');
#else
           c == 'e' || c == 'E';
#endif
  }

  static inline bool canBeInNonQuotedString(char c) {
    return isBetween(c, '0', '9') || isBetween(c, '_', 'z') ||
           isBetween(c, 'A', 'Z');
  }

  static inline bool isQuote(char c) {
    return c == '\'' || c == '\"';
  }

  static inline uint8_t decodeHex(char c) {
    if (c < 'A')
      return uint8_t(c - '0');
    c = char(c & ~0x20);  // uppercase
    return uint8_t(c - 'A' + 10);
  }

  DeserializationError::Code skipSpacesAndComments() {
    for (;;) {
      switch (current()) {
        // end of string
        case '\0':
          return foundSomething_ ? DeserializationError::IncompleteInput
                                 : DeserializationError::EmptyInput;

        // spaces
        case ' ':
        case '\t':
        case '\r':
        case '\n':
          move();
          continue;

#if ARDUINOJSON_ENABLE_COMMENTS
        // comments
        case '/':
          move();  // skip '/'
          switch (current()) {
            // block comment
            case '*': {
              move();  // skip '*'
              bool wasStar = false;
              for (;;) {
                char c = current();
                if (c == '\0')
                  return DeserializationError::IncompleteInput;
                if (c == '/' && wasStar) {
                  move();
                  break;
                }
                wasStar = c == '*';
                move();
              }
              break;
            }

            // trailing comment
            case '/':
              // no need to skip "//"
              for (;;) {
                move();
                char c = current();
                if (c == '\0')
                  return DeserializationError::IncompleteInput;
                if (c == '\n')
                  break;
              }
              break;

            // not a comment, just a '/'
            default:
              return DeserializationError::InvalidInput;
          }
          break;
#endif

        default:
          foundSomething_ = true;
          return DeserializationError::Ok;
      }
    }
  }

  DeserializationError::Code skipKeyword(const char* s) {
    while (*s) {
      char c = current();
      if (c == '\0')
        return DeserializationError::IncompleteInput;
      if (*s != c)
        return DeserializationError::InvalidInput;
      ++s;
      move();
    }
    return DeserializationError::Ok;
  }

  StringBuilder stringBuilder_;
  bool foundSomething_;
  Latch<TReader> latch_;
  ResourceManager* resources_;
  char buffer_[64];  // using a member instead of a local variable because it
                     // ended in the recursive path after compiler inlined the
                     // code
};

ARDUINOJSON_END_PRIVATE_NAMESPACE

ARDUINOJSON_BEGIN_PUBLIC_NAMESPACE

// Parses a JSON input, filters, and puts the result in a JsonDocument.
// https://arduinojson.org/v7/api/json/deserializejson/
template <typename TDestination, typename... Args,
          detail::enable_if_t<
              detail::is_deserialize_destination<TDestination>::value, int> = 0>
inline DeserializationError deserializeJson(TDestination&& dst,
                                            Args&&... args) {
  using namespace detail;
  return deserialize<JsonDeserializer>(detail::forward<TDestination>(dst),
                                       detail::forward<Args>(args)...);
}

// Parses a JSON input, filters, and puts the result in a JsonDocument.
// https://arduinojson.org/v7/api/json/deserializejson/
template <typename TDestination, typename TChar, typename... Args,
          detail::enable_if_t<
              detail::is_deserialize_destination<TDestination>::value, int> = 0>
inline DeserializationError deserializeJson(TDestination&& dst, TChar* input,
                                            Args&&... args) {
  using namespace detail;
  return deserialize<JsonDeserializer>(detail::forward<TDestination>(dst),
                                       input, detail::forward<Args>(args)...);
}

ARDUINOJSON_END_PUBLIC_NAMESPACE
