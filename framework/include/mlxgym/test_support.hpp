#pragma once

#include "mlxgym/metal_context.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstring>
#include <iostream>
#include <limits>
#include <random>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

namespace mlxgym {

constexpr std::size_t kGuardElements = 32;

template <typename T> class GuardedBuffer {
public:
  explicit GuardedBuffer(std::size_t count, T initial = T{})
      : count_(count), storage_(count + 2 * kGuardElements, guard_value()) {
    std::fill(data(), data() + count_, initial);
  }

  T *data() { return storage_.data() + kGuardElements; }
  const T *data() const { return storage_.data() + kGuardElements; }
  std::size_t size() const { return count_; }

  BufferHandle upload(MetalContext &context) {
    handle_ = context.create_buffer(storage_.size() * sizeof(T), storage_.data());
    return handle_;
  }

  BufferSlice slice() const { return {handle_, kGuardElements * sizeof(T)}; }

  void download(MetalContext &context) {
    context.download(handle_, storage_.data(), storage_.size() * sizeof(T));
  }

  bool guards_intact() const {
    return std::all_of(storage_.begin(), storage_.begin() + kGuardElements,
                       [](T v) { return bit_equal(v, guard_value()); }) &&
           std::all_of(storage_.end() - kGuardElements, storage_.end(),
                       [](T v) { return bit_equal(v, guard_value()); });
  }

private:
  static T guard_value() {
    if constexpr (std::is_floating_point_v<T>)
      return static_cast<T>(-98765.25);
    return static_cast<T>(0xA5);
  }
  static bool bit_equal(T a, T b) { return std::memcmp(&a, &b, sizeof(T)) == 0; }

  std::size_t count_;
  std::vector<T> storage_;
  BufferHandle handle_{};
};

inline bool close(float actual, float expected, float rtol = 1.0e-4F, float atol = 1.0e-5F) {
  return std::isfinite(actual) &&
         std::fabs(actual - expected) <=
             atol + rtol * std::max(std::fabs(actual), std::fabs(expected));
}

class TestLog {
public:
  void expect(bool condition, const std::string &message) {
    ++checks_;
    if (!condition) {
      ++failures_;
      if (failures_ <= 20)
        std::cerr << "FAIL: " << message << '\n';
      if (failures_ == 21)
        std::cerr << "FAIL: further failures suppressed\n";
    }
  }
  int failures() const { return failures_; }
  int checks() const { return checks_; }

private:
  int checks_{};
  int failures_{};
};

std::vector<std::size_t> boundary_sizes();
std::mt19937 make_rng();
int run_task(const std::string &task, MetalContext &context, PipelineHandle pipeline,
             bool benchmark);

} // namespace mlxgym
