// ArduinoJson - https://arduinojson.org
// Copyright © 2014-2025, Benoit BLANCHON
// MIT License

#pragma once

#include <ArduinoJson/Array/JsonArray.hpp>
#include <ArduinoJson/Object/JsonObject.hpp>
#include <ArduinoJson/Variant/VariantRefBase.hpp>

ARDUINOJSON_BEGIN_PRIVATE_NAMESPACE

template <typename TDerived>
inline JsonVariant VariantRefBase<TDerived>::add() const {
  return add<JsonVariant>();
}

template <typename TDerived>
template <typename T>
inline T VariantRefBase<TDerived>::as() const {
  using variant_type =  // JsonVariantConst or JsonVariant?
      typename function_traits<decltype(&Converter<T>::fromJson)>::arg1_type;
  return Converter<T>::fromJson(getVariant<variant_type>());
}

template <typename TDerived>
inline JsonArray VariantRefBase<TDerived>::createNestedArray() const {
  return add<JsonArray>();
}

template <typename TDerived>
template <typename TChar>
inline JsonArray VariantRefBase<TDerived>::createNestedArray(TChar* key) const {
  return operator[](key).template to<JsonArray>();
}

template <typename TDerived>
template <typename TString>
inline JsonArray VariantRefBase<TDerived>::createNestedArray(
    const TString& key) const {
  return operator[](key).template to<JsonArray>();
}

template <typename TDerived>
inline JsonObject VariantRefBase<TDerived>::createNestedObject() const {
  return add<JsonObject>();
}

template <typename TDerived>
template <typename TChar>
inline JsonObject VariantRefBase<TDerived>::createNestedObject(
    TChar* key) const {
  return operator[](key).template to<JsonObject>();
}

template <typename TDerived>
template <typename TString>
inline JsonObject VariantRefBase<TDerived>::createNestedObject(
    const TString& key) const {
  return operator[](key).template to<JsonObject>();
}

template <typename TDerived>
inline void convertToJson(const VariantRefBase<TDerived>& src,
                          JsonVariant dst) {
  dst.set(src.template as<JsonVariantConst>());
}

template <typename TDerived>
template <typename T, enable_if_t<is_same<T, JsonVariant>::value, int>>
inline T VariantRefBase<TDerived>::add() const {
  return JsonVariant(
      detail::VariantData::addElement(getOrCreateData(), getResourceManager()),
      getResourceManager());
}

template <typename TDerived>
template <typename TString, enable_if_t<IsString<TString>::value, int>>
inline bool VariantRefBase<TDerived>::containsKey(const TString& key) const {
  return VariantData::getMember(getData(), adaptString(key),
                                getResourceManager()) != 0;
}

template <typename TDerived>
template <typename TChar, enable_if_t<IsString<TChar*>::value, int>>
inline bool VariantRefBase<TDerived>::containsKey(TChar* key) const {
  return VariantData::getMember(getData(), adaptString(key),
                                getResourceManager()) != 0;
}

template <typename TDerived>
template <typename TVariant, enable_if_t<IsVariant<TVariant>::value, int>>
inline bool VariantRefBase<TDerived>::containsKey(const TVariant& key) const {
  return containsKey(key.template as<const char*>());
}

template <typename TDerived>
inline JsonVariant VariantRefBase<TDerived>::getVariant() const {
  return JsonVariant(getData(), getResourceManager());
}

template <typename TDerived>
inline JsonVariant VariantRefBase<TDerived>::getOrCreateVariant() const {
  return JsonVariant(getOrCreateData(), getResourceManager());
}

template <typename TDerived>
template <typename T>
inline bool VariantRefBase<TDerived>::is() const {
  using variant_type =  // JsonVariantConst or JsonVariant?
      typename function_traits<decltype(&Converter<T>::checkJson)>::arg1_type;
  return Converter<T>::checkJson(getVariant<variant_type>());
}

template <typename TDerived>
inline ElementProxy<TDerived> VariantRefBase<TDerived>::operator[](
    size_t index) const {
  return {derived(), index};
}

template <typename TDerived>
template <typename TChar,
          enable_if_t<IsString<TChar*>::value && !is_const<TChar>::value, int>>
inline MemberProxy<TDerived, AdaptedString<TChar*>>
VariantRefBase<TDerived>::operator[](TChar* key) const {
  return {derived(), adaptString(key)};
}

template <typename TDerived>
template <typename TString, enable_if_t<IsString<TString>::value, int>>
inline MemberProxy<TDerived, AdaptedString<TString>>
VariantRefBase<TDerived>::operator[](const TString& key) const {
  return {derived(), adaptString(key)};
}

template <typename TDerived>
template <typename TConverter, typename T>
inline bool VariantRefBase<TDerived>::doSet(const T& value, false_type) const {
  TConverter::toJson(value, getOrCreateVariant());
  auto resources = getResourceManager();
  return resources && !resources->overflowed();
}

template <typename TDerived>
template <typename TConverter, typename T>
inline bool VariantRefBase<TDerived>::doSet(const T& value, true_type) const {
  return TConverter::toJson(value, getOrCreateVariant());
}

template <typename TDerived>
template <typename T, enable_if_t<is_same<T, JsonArray>::value, int>>
inline JsonArray VariantRefBase<TDerived>::to() const {
  return JsonArray(
      VariantData::toArray(getOrCreateData(), getResourceManager()),
      getResourceManager());
}

template <typename TDerived>
template <typename T, enable_if_t<is_same<T, JsonObject>::value, int>>
JsonObject VariantRefBase<TDerived>::to() const {
  return JsonObject(
      VariantData::toObject(getOrCreateData(), getResourceManager()),
      getResourceManager());
}

template <typename TDerived>
template <typename T, enable_if_t<is_same<T, JsonVariant>::value, int>>
JsonVariant VariantRefBase<TDerived>::to() const {
  auto data = getOrCreateData();
  auto resources = getResourceManager();
  detail::VariantData::clear(data, resources);
  return JsonVariant(data, resources);
}

ARDUINOJSON_END_PRIVATE_NAMESPACE
