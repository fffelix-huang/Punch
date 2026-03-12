#ifndef PUNCH_CHESS_UTILS_STATIC_VECTOR_H_
#define PUNCH_CHESS_UTILS_STATIC_VECTOR_H_

#include <array>
#include <cassert>
#include <iterator>

namespace punch {

template <typename T, size_t kCapacity>
class StaticVector {
 public:
  using value_type = T;
  using size_type = size_t;
  using difference_type = std::ptrdiff_t;
  using reference = value_type&;
  using const_reference = const value_type&;
  using pointer = value_type*;
  using const_pointer = const value_type*;

  using iterator = value_type*;
  using const_iterator = const value_type*;

  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  [[nodiscard]] constexpr reference operator[](size_type pos) noexcept {
    return data_[pos];
  }
  [[nodiscard]] constexpr const_reference operator[](
      size_type pos) const noexcept {
    return data_[pos];
  }

  [[nodiscard]] constexpr reference front() noexcept { return data_[0]; }
  [[nodiscard]] constexpr const_reference front() const noexcept {
    return data_[0];
  }

  [[nodiscard]] constexpr reference back() noexcept { return data_[size_ - 1]; }
  [[nodiscard]] constexpr const_reference back() const noexcept {
    return data_[size_ - 1];
  }

  [[nodiscard]] constexpr iterator begin() noexcept { return data_.data(); }
  [[nodiscard]] constexpr const_iterator begin() const noexcept {
    return data_.data();
  }
  [[nodiscard]] constexpr const_iterator cbegin() const noexcept {
    return data_.data();
  }

  [[nodiscard]] constexpr iterator end() noexcept {
    return data_.data() + size_;
  }
  [[nodiscard]] constexpr const_iterator end() const noexcept {
    return data_.data() + size_;
  }
  [[nodiscard]] constexpr const_iterator cend() const noexcept {
    return data_.data() + size_;
  }

  [[nodiscard]] constexpr bool empty() const noexcept { return size_ == 0; }
  [[nodiscard]] constexpr size_type size() const noexcept { return size_; }

  constexpr void clear() noexcept { size_ = 0; }

  constexpr void push_back(const_reference value) noexcept {
    assert(size_ < kCapacity);
    data_[size_++] = value;
  }
  constexpr void push_back(T&& value) noexcept(
      std::is_nothrow_move_assignable_v<T>) {
    assert(size_ < kCapacity);
    data_[size_++] = std::move(value);
  }

  constexpr void pop_back() noexcept {
    assert(size_ > 0);
    size_--;
  }

 private:
  std::array<T, kCapacity> data_;
  size_t size_ = 0;
};

}  // namespace punch

#endif  // PUNCH_CHESS_UTILS_STATIC_VECTOR_H_
