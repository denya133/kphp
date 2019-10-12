#pragma once
#include <type_traits>

#include "runtime/include.h"

namespace impl_ {
template<class ReturnType, class FallbackType>
auto perform_fallback_impl(const FallbackType &lambda_fallback,
                                 std::enable_if_t<bool(sizeof((std::declval<FallbackType>()(), 0)))> *) noexcept {
  return ReturnType(lambda_fallback());
}

template<class ReturnType, class FallbackType>
ReturnType perform_fallback_impl(const FallbackType &value_fallback, ...) noexcept {
  return ReturnType(value_fallback);
}

template<class ReturnType, class FallbackType>
ReturnType perform_fallback(const FallbackType &value_fallback) noexcept {
  return perform_fallback_impl<ReturnType>(value_fallback, nullptr);
}

template<class ReturnType, class ValueType, class FallbackType>
ReturnType null_coalesce_resolve_or_false(std::true_type, const Optional<ValueType> &value, const FallbackType &fallback) noexcept {
  php_assert(value.value_state() == OptionalState::null_value);
  return perform_fallback<ReturnType>(fallback);
}

template<class ReturnType, class ValueType, class FallbackType>
ReturnType null_coalesce_resolve_or_false(std::false_type, const Optional<ValueType> &value, const FallbackType &fallback) noexcept {
  return f$is_null(value) ? impl_::perform_fallback<ReturnType>(fallback) : ReturnType(value.val());
}
} // namespace impl_


template<class ReturnType, class ValueType, class FallbackType>
ReturnType null_coalesce(const ValueType &value, const FallbackType &fallback) noexcept {
  return f$is_null(value) ? impl_::perform_fallback<ReturnType>(fallback) : ReturnType(value);
}

template<class ReturnType, class ValueType, class FallbackType>
enable_if_t_is_not_optional<ReturnType, ReturnType>
null_coalesce(const Optional<ValueType> &value, const FallbackType &fallback) noexcept {
  // keep in mind that ReturnType is not an optional here!
  // TODO remove is_class_instance and rpc_connection when forbid mix them with false
  using result_can_be_false = std::integral_constant<bool,
    vk::is_type_in_list<ReturnType, bool, var, rpc_connection>{} ||
    is_class_instance<ReturnType>{}
  >;
  using false_cast_immposible = std::integral_constant<bool, std::is_same<ValueType, bool>{} && !result_can_be_false{}>;

  php_assert((result_can_be_false{} || value.value_state() != OptionalState::false_value));
  return impl_::null_coalesce_resolve_or_false<ReturnType>(false_cast_immposible{}, value, fallback);
}

template<class ReturnType, class ValueType, class KeyType, class FallbackType, class = vk::enable_if_constructible<var, KeyType>>
ReturnType null_coalesce(const array<ValueType> &arr, const KeyType &key, const FallbackType &fallback) noexcept {
  auto *value = arr.find_value(key);
  return value ? null_coalesce<ReturnType>(*value, fallback) : impl_::perform_fallback<ReturnType>(fallback);
}

template<typename ReturnType, class ValueType, typename FallbackType>
ReturnType null_coalesce(const array<ValueType> &arr, const string &string_key, int precomuted_hash, const FallbackType &fallback) noexcept {
  auto *value = arr.find_value(string_key, precomuted_hash);
  return value ? null_coalesce<ReturnType>(*value, fallback) : impl_::perform_fallback<ReturnType>(fallback);
}

template<class ReturnType, class KeyType, class FallbackType, class = vk::enable_if_constructible<var, KeyType>>
ReturnType null_coalesce(const string &str, const KeyType &key, const FallbackType &fallback) noexcept {
  string value = str.get_value(key);
  return value.empty() ? impl_::perform_fallback<ReturnType>(fallback) : std::move(value);
}

template<class ReturnType, class KeyType, class FallbackType, class = vk::enable_if_constructible<var, KeyType>>
ReturnType null_coalesce(const var &v, const KeyType &key, const FallbackType &fallback) noexcept {
  return v.is_string()
         ? null_coalesce<ReturnType>(v.as_string(), key, fallback)
         : null_coalesce<ReturnType>(v.get_value(key), fallback);
}

template<typename ReturnType, typename FallbackType>
ReturnType null_coalesce(const var &v, const string &string_key, int precomuted_hash, const FallbackType &fallback) noexcept {
  return v.is_string()
         ? null_coalesce<ReturnType>(v.as_string(), string_key, fallback)
         : null_coalesce<ReturnType>(v.get_value(string_key, precomuted_hash), fallback);
}