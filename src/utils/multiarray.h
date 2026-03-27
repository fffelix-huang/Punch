#ifndef PUNCH_UTILS_MULTIARRAY_H_
#define PUNCH_UTILS_MULTIARRAY_H_

#include <array>

namespace punch {

namespace internal {

template <typename T, size_t... Dims>
struct MultiArrayImpl;

template <typename T, size_t First, size_t... Rest>
struct MultiArrayImpl<T, First, Rest...> {
  using type = std::array<typename MultiArrayImpl<T, Rest...>::type, First>;
};

template <typename T, size_t Last>
struct MultiArrayImpl<T, Last> {
  using type = std::array<T, Last>;
};

}  // namespace internal

template <typename T, size_t... Dims>
using MultiArray = typename internal::MultiArrayImpl<T, Dims...>::type;

}  // namespace punch

#endif  // PUNCH_UTILS_MULTIARRAY_H_
