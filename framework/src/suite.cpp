#include "mlxgym/test_support.hpp"

#include <array>
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <iomanip>
#include <numeric>
#include <sstream>
#include <string_view>

namespace mlxgym {
namespace {

constexpr float kNaN = std::numeric_limits<float>::quiet_NaN();

std::string at(std::string_view task, std::size_t n, std::size_t i) {
  std::ostringstream out;
  out << task << " n=" << n << " index=" << i;
  return out.str();
}

template <typename... Ts> std::vector<ConstantArg> constants(const Ts &...values) {
  return {ConstantArg::from(values)...};
}

std::vector<float> patterned(std::size_t n) {
  static constexpr std::array<float, 15> edges{0.0F,     -0.0F,   1.0F,  -1.0F, 1.0e-4F,
                                               -1.0e-4F, 8.0F,    -8.0F, 20.0F, -20.0F,
                                               100.0F,   -100.0F, 0.5F,  -2.5F, 3.25F};
  std::vector<float> values(n);
  for (std::size_t i = 0; i < n; ++i)
    values[i] = i < edges.size()
                    ? edges[i]
                    : static_cast<float>(static_cast<int>((i * 37) % 257) - 128) / 13.0F;
  return values;
}

void load(GuardedBuffer<float> &buffer, const std::vector<float> &values) {
  std::copy(values.begin(), values.end(), buffer.data());
}

int finish(const std::string &task, const TestLog &log) {
  if (log.failures()) {
    std::cerr << task << ": FAIL (" << log.failures() << "/" << log.checks() << " checks failed)\n";
    return 1;
  }
  std::cout << task << ": PASS (" << log.checks() << " checks)\n";
  return 0;
}

int test_unary(const std::string &task, MetalContext &context, PipelineHandle pipeline,
               const std::function<float(float)> &op, bool exact = false) {
  TestLog log;
  for (const std::size_t n : boundary_sizes()) {
    const auto input_values = patterned(n);
    GuardedBuffer<float> input(n), output(n, kNaN);
    load(input, input_values);
    input.upload(context);
    output.upload(context);
    const std::uint32_t count = static_cast<std::uint32_t>(n);
    Invocation invocation{{input.slice(), output.slice()}, constants(count), {n, 1, 1}};
    encode_solution(context, pipeline, invocation);
    input.download(context);
    output.download(context);
    log.expect(input.guards_intact() && output.guards_intact(), task + " guard regions");
    for (std::size_t i = 0; i < n; ++i) {
      const float expected = op(input_values[i]);
      const bool ok = exact ? std::memcmp(&output.data()[i], &expected, sizeof(float)) == 0
                            : close(output.data()[i], expected, 2.0e-4F, 2.0e-5F);
      log.expect(ok, at(task, n, i));
    }
  }
  return finish(task, log);
}

int test_gated(const std::string &task, MetalContext &context, PipelineHandle pipeline,
               const std::function<float(float, float)> &op) {
  TestLog log;
  const std::vector<std::size_t> sizes{2, 4, 62, 64, 66, 126, 128, 130, 510, 512, 514, 8198};
  for (const std::size_t n : sizes) {
    const std::size_t half = n / 2;
    auto values = patterned(n);
    for (std::size_t i = half; i < n; ++i)
      values[i] *= 0.37F;
    GuardedBuffer<float> input(n), output(half, kNaN);
    load(input, values);
    input.upload(context);
    output.upload(context);
    const std::uint32_t count = static_cast<std::uint32_t>(n);
    Invocation invocation{{input.slice(), output.slice()}, constants(count), {half, 1, 1}};
    encode_solution(context, pipeline, invocation);
    input.download(context);
    output.download(context);
    log.expect(input.guards_intact() && output.guards_intact(), task + " guard regions");
    for (std::size_t i = 0; i < half; ++i)
      log.expect(close(output.data()[i], op(values[i], values[i + half]), 4.0e-4F, 3.0e-5F),
                 at(task, n, i));
  }
  return finish(task, log);
}

int test_clipping(MetalContext &context, PipelineHandle pipeline) {
  TestLog log;
  struct Bounds {
    float lo, hi;
  };
  const std::vector<Bounds> bounds{{-1, 1}, {-2.5F, 3.5F}, {0, 4}, {-4, -1}, {.5F, .5F}};
  const auto sizes = boundary_sizes();
  for (std::size_t case_index = 0; case_index < bounds.size(); ++case_index) {
    const auto n = sizes[case_index + 6];
    const auto b = bounds[case_index];
    auto values = patterned(n);
    if (n > 3) {
      values[0] = b.lo;
      values[1] = b.hi;
      values[2] = b.lo - 1;
      values[3] = b.hi + 1;
    }
    GuardedBuffer<float> input(n), output(n, kNaN);
    load(input, values);
    input.upload(context);
    output.upload(context);
    const std::uint32_t count = static_cast<std::uint32_t>(n);
    Invocation inv{{input.slice(), output.slice()}, constants(b.lo, b.hi, count), {n, 1, 1}};
    encode_solution(context, pipeline, inv);
    output.download(context);
    log.expect(output.guards_intact(), "value_clipping guard regions");
    for (std::size_t i = 0; i < n; ++i)
      log.expect(output.data()[i] == std::max(b.lo, std::min(b.hi, values[i])),
                 at("value_clipping", n, i));
  }
  return finish("value_clipping", log);
}

int test_vector_add(MetalContext &context, PipelineHandle pipeline) {
  TestLog log;
  for (const std::size_t n : boundary_sizes()) {
    auto a_values = patterned(n), b_values = patterned(n);
    std::reverse(b_values.begin(), b_values.end());
    GuardedBuffer<float> a(n), b(n), out(n, kNaN);
    load(a, a_values);
    load(b, b_values);
    a.upload(context);
    b.upload(context);
    out.upload(context);
    const std::uint32_t count = static_cast<std::uint32_t>(n);
    Invocation invocation{{a.slice(), b.slice(), out.slice()}, constants(count), {n, 1, 1}};
    encode_solution(context, pipeline, invocation);
    a.download(context);
    b.download(context);
    out.download(context);
    log.expect(a.guards_intact() && b.guards_intact() && out.guards_intact(),
               "vector_add guard regions");
    for (std::size_t i = 0; i < n; ++i)
      log.expect(close(out.data()[i], a_values[i] + b_values[i]), at("vector_add", n, i));
  }
  return finish("vector_add", log);
}

int test_reverse(MetalContext &context, PipelineHandle pipeline) {
  TestLog log;
  for (const std::size_t n : boundary_sizes()) {
    auto values = patterned(n);
    auto expected = values;
    std::reverse(expected.begin(), expected.end());
    GuardedBuffer<float> input(n);
    load(input, values);
    input.upload(context);
    const std::uint32_t count = static_cast<std::uint32_t>(n);
    Invocation invocation{{input.slice()}, constants(count), {(n + 1) / 2, 1, 1}};
    encode_solution(context, pipeline, invocation);
    input.download(context);
    log.expect(input.guards_intact(), "reverse_array guard regions");
    for (std::size_t i = 0; i < n; ++i)
      log.expect(std::memcmp(&input.data()[i], &expected[i], sizeof(float)) == 0,
                 at("reverse_array", n, i));
  }
  return finish("reverse_array", log);
}

int test_interleave(MetalContext &context, PipelineHandle pipeline) {
  TestLog log;
  for (const std::size_t n : boundary_sizes()) {
    auto a_values = patterned(n), b_values = patterned(n);
    for (float &value : b_values)
      value = value * -3.0F + 0.75F;
    GuardedBuffer<float> a(n), b(n), out(2 * n, kNaN);
    load(a, a_values);
    load(b, b_values);
    a.upload(context);
    b.upload(context);
    out.upload(context);
    const std::uint32_t count = static_cast<std::uint32_t>(n);
    Invocation invocation{{a.slice(), b.slice(), out.slice()}, constants(count), {n, 1, 1}};
    encode_solution(context, pipeline, invocation);
    out.download(context);
    log.expect(out.guards_intact(), "interleave_arrays guard regions");
    for (std::size_t i = 0; i < n; ++i) {
      log.expect(out.data()[2 * i] == a_values[i], at("interleave_arrays/A", n, i));
      log.expect(out.data()[2 * i + 1] == b_values[i], at("interleave_arrays/B", n, i));
    }
  }
  return finish("interleave_arrays", log);
}

int test_matrix_copy_or_add(const std::string &task, MetalContext &context, PipelineHandle pipeline,
                            bool addition) {
  TestLog log;
  const std::vector<std::uint32_t> sizes{1, 2, 15, 16, 17, 31, 32, 33, 65, 127};
  for (const auto n : sizes) {
    const std::size_t count = static_cast<std::size_t>(n) * n;
    auto a_values = patterned(count), b_values = patterned(count);
    std::rotate(b_values.begin(), b_values.begin() + (b_values.empty() ? 0 : 1), b_values.end());
    GuardedBuffer<float> a(count), b(count), out(count, kNaN);
    load(a, a_values);
    load(b, b_values);
    a.upload(context);
    if (addition)
      b.upload(context);
    out.upload(context);
    Invocation invocation;
    invocation.buffers = addition ? std::vector<BufferSlice>{a.slice(), b.slice(), out.slice()}
                                  : std::vector<BufferSlice>{a.slice(), out.slice()};
    invocation.constants = constants(n);
    invocation.grid = {n, n, 1};
    encode_solution(context, pipeline, invocation);
    out.download(context);
    log.expect(out.guards_intact(), task + " guard regions");
    for (std::size_t i = 0; i < count; ++i) {
      const float expected = addition ? a_values[i] + b_values[i] : a_values[i];
      log.expect(addition ? close(out.data()[i], expected) : out.data()[i] == expected,
                 at(task, count, i));
    }
  }
  return finish(task, log);
}

int test_transpose(MetalContext &context, PipelineHandle pipeline) {
  TestLog log;
  const std::vector<std::pair<std::uint32_t, std::uint32_t>> shapes{
      {1, 1}, {1, 67}, {71, 1}, {4, 7}, {15, 17}, {16, 16}, {17, 31}, {32, 33}, {67, 61}};
  for (const auto [rows, cols] : shapes) {
    const std::size_t count = static_cast<std::size_t>(rows) * cols;
    auto values = patterned(count);
    GuardedBuffer<float> input(count), output(count, kNaN);
    load(input, values);
    input.upload(context);
    output.upload(context);
    Invocation invocation{{input.slice(), output.slice()}, constants(rows, cols), {cols, rows, 1}};
    encode_solution(context, pipeline, invocation);
    output.download(context);
    log.expect(output.guards_intact(), "matrix_transpose guard regions");
    for (std::uint32_t r = 0; r < rows; ++r)
      for (std::uint32_t c = 0; c < cols; ++c)
        log.expect(output.data()[static_cast<std::size_t>(c) * rows + r] ==
                       values[static_cast<std::size_t>(r) * cols + c],
                   "matrix_transpose shape=" + std::to_string(rows) + "x" + std::to_string(cols));
  }
  return finish("matrix_transpose", log);
}

int test_matmul(MetalContext &context, PipelineHandle pipeline) {
  TestLog log;
  struct Shape {
    std::uint32_t m, n, k;
  };
  const std::vector<Shape> shapes{{1, 1, 1},    {1, 17, 3},   {19, 1, 23}, {2, 3, 4},
                                  {15, 16, 17}, {16, 16, 16}, {17, 5, 19}, {31, 33, 7}};
  for (const auto s : shapes) {
    const std::size_t ac = static_cast<std::size_t>(s.m) * s.n;
    const std::size_t bc = static_cast<std::size_t>(s.n) * s.k;
    const std::size_t cc = static_cast<std::size_t>(s.m) * s.k;
    auto av = patterned(ac), bv = patterned(bc);
    for (float &v : av)
      v *= 0.125F;
    for (float &v : bv)
      v *= 0.0625F;
    GuardedBuffer<float> a(ac), b(bc), out(cc, kNaN);
    load(a, av);
    load(b, bv);
    a.upload(context);
    b.upload(context);
    out.upload(context);
    Invocation invocation{
        {a.slice(), b.slice(), out.slice()}, constants(s.m, s.n, s.k), {s.k, s.m, 1}};
    encode_solution(context, pipeline, invocation);
    out.download(context);
    log.expect(out.guards_intact(), "matrix_multiplication guard regions");
    for (std::uint32_t r = 0; r < s.m; ++r)
      for (std::uint32_t c = 0; c < s.k; ++c) {
        double sum = 0.0;
        for (std::uint32_t p = 0; p < s.n; ++p)
          sum += static_cast<double>(av[static_cast<std::size_t>(r) * s.n + p]) *
                 bv[static_cast<std::size_t>(p) * s.k + c];
        log.expect(close(out.data()[static_cast<std::size_t>(r) * s.k + c], static_cast<float>(sum),
                         8.0e-4F, 2.0e-4F),
                   "matrix_multiplication shape=" + std::to_string(s.m) + "x" +
                       std::to_string(s.n) + "x" + std::to_string(s.k));
      }
  }
  return finish("matrix_multiplication", log);
}

int test_softmax_attention(MetalContext &context, PipelineHandle pipeline) {
  TestLog log;
  struct Shape {
    std::uint32_t m, n, d;
  };
  const std::vector<Shape> shapes{{1, 1, 1}, {1, 3, 2},  {3, 1, 5},    {2, 3, 4},
                                  {3, 5, 7}, {7, 9, 16}, {17, 13, 31}, {33, 35, 17}};

  for (std::size_t case_index = 0; case_index < shapes.size(); ++case_index) {
    const auto shape = shapes[case_index];
    const std::size_t q_count = std::size_t(shape.m) * shape.d;
    const std::size_t kv_count = std::size_t(shape.n) * shape.d;
    std::vector<float> q_values(q_count), k_values(kv_count), v_values(kv_count);
    for (std::size_t i = 0; i < q_count; ++i)
      q_values[i] = 0.75F * std::sin(float(i) * 0.37F + 0.11F);
    for (std::size_t i = 0; i < kv_count; ++i) {
      k_values[i] = 0.8F * std::cos(float(i) * 0.23F - 0.19F);
      v_values[i] = std::sin(float(i) * 0.13F) - 0.3F * std::cos(float(i) * 0.41F);
    }
    // Exercise stable softmax rather than allowing an implementation to rely on small logits.
    if (case_index + 1 == shapes.size()) {
      for (float &value : q_values)
        value *= 8.0F;
      for (float &value : k_values)
        value *= 8.0F;
    }

    std::vector<float> expected(q_count);
    std::vector<double> scores(shape.n);
    const double scale = 1.0 / std::sqrt(double(shape.d));
    for (std::uint32_t row = 0; row < shape.m; ++row) {
      double maximum = -std::numeric_limits<double>::infinity();
      for (std::uint32_t key = 0; key < shape.n; ++key) {
        double dot = 0.0;
        for (std::uint32_t col = 0; col < shape.d; ++col)
          dot += double(q_values[std::size_t(row) * shape.d + col]) *
                 k_values[std::size_t(key) * shape.d + col];
        scores[key] = dot * scale;
        maximum = std::max(maximum, scores[key]);
      }
      double denominator = 0.0;
      for (double &score : scores) {
        score = std::exp(score - maximum);
        denominator += score;
      }
      for (std::uint32_t col = 0; col < shape.d; ++col) {
        double value = 0.0;
        for (std::uint32_t key = 0; key < shape.n; ++key)
          value += (scores[key] / denominator) * v_values[std::size_t(key) * shape.d + col];
        expected[std::size_t(row) * shape.d + col] = float(value);
      }
    }

    GuardedBuffer<float> q(q_count), k(kv_count), v(kv_count), output(q_count, kNaN);
    load(q, q_values);
    load(k, k_values);
    load(v, v_values);
    q.upload(context);
    k.upload(context);
    v.upload(context);
    output.upload(context);
    Invocation invocation{{q.slice(), k.slice(), v.slice(), output.slice()},
                          constants(shape.m, shape.n, shape.d),
                          {shape.d, shape.m, 1}};
    encode_solution(context, pipeline, invocation);
    q.download(context);
    k.download(context);
    v.download(context);
    output.download(context);

    const std::string label = "softmax_attention shape=" + std::to_string(shape.m) + "x" +
                              std::to_string(shape.n) + "x" + std::to_string(shape.d);
    log.expect(q.guards_intact() && k.guards_intact() && v.guards_intact() &&
                   output.guards_intact(),
               label + " guard regions");
    log.expect(std::equal(q.data(), q.data() + q_count, q_values.begin()) &&
                   std::equal(k.data(), k.data() + kv_count, k_values.begin()) &&
                   std::equal(v.data(), v.data() + kv_count, v_values.begin()),
               label + " input immutability");
    for (std::size_t i = 0; i < q_count; ++i)
      log.expect(close(output.data()[i], expected[i], 1.5e-3F, 2.0e-4F),
                 label + " index=" + std::to_string(i));
  }
  return finish("softmax_attention", log);
}

int test_color(MetalContext &context, PipelineHandle pipeline) {
  TestLog log;
  const std::vector<std::pair<std::uint32_t, std::uint32_t>> shapes{{1, 1},   {1, 2},   {2, 1},
                                                                    {17, 19}, {63, 65}, {257, 3}};
  for (const auto [width, height] : shapes) {
    const std::size_t pixels = static_cast<std::size_t>(width) * height;
    GuardedBuffer<std::uint8_t> image(pixels * 4);
    std::vector<std::uint8_t> before(pixels * 4);
    for (std::size_t i = 0; i < pixels; ++i) {
      before[4 * i] = static_cast<std::uint8_t>((17 * i + 3) % 256);
      before[4 * i + 1] = static_cast<std::uint8_t>((29 * i + 11) % 256);
      before[4 * i + 2] = static_cast<std::uint8_t>((43 * i + 19) % 256);
      before[4 * i + 3] = static_cast<std::uint8_t>((7 * i + 23) % 256);
    }
    std::copy(before.begin(), before.end(), image.data());
    image.upload(context);
    Invocation invocation{{image.slice()}, constants(width, height), {pixels, 1, 1}};
    encode_solution(context, pipeline, invocation);
    image.download(context);
    log.expect(image.guards_intact(), "color_inversion guard regions");
    for (std::size_t i = 0; i < pixels; ++i)
      for (std::size_t c = 0; c < 4; ++c) {
        const auto expected =
            c == 3 ? before[4 * i + c] : static_cast<std::uint8_t>(255 - before[4 * i + c]);
        log.expect(image.data()[4 * i + c] == expected,
                   "color_inversion pixel=" + std::to_string(i));
      }
  }
  return finish("color_inversion", log);
}

int test_rgb(MetalContext &context, PipelineHandle pipeline) {
  TestLog log;
  const std::vector<std::pair<std::uint32_t, std::uint32_t>> shapes{{1, 1},   {2, 3},   {31, 1},
                                                                    {16, 16}, {17, 17}, {65, 63}};
  for (const auto [width, height] : shapes) {
    const std::size_t pixels = static_cast<std::size_t>(width) * height;
    GuardedBuffer<float> input(3 * pixels), output(pixels, kNaN);
    for (std::size_t i = 0; i < pixels; ++i) {
      input.data()[3 * i] = static_cast<float>((i * 3) % 257) / 256.0F;
      input.data()[3 * i + 1] = static_cast<float>((i * 5) % 257) / 256.0F;
      input.data()[3 * i + 2] = static_cast<float>((i * 7) % 257) / 256.0F;
    }
    if (pixels) {
      input.data()[0] = 1;
      input.data()[1] = 0;
      input.data()[2] = 0;
    }
    input.upload(context);
    output.upload(context);
    Invocation invocation{
        {input.slice(), output.slice()}, constants(width, height), {pixels, 1, 1}};
    encode_solution(context, pipeline, invocation);
    output.download(context);
    log.expect(output.guards_intact(), "rgb_to_grayscale guard regions");
    for (std::size_t i = 0; i < pixels; ++i) {
      const float expected = .299F * input.data()[3 * i] + .587F * input.data()[3 * i + 1] +
                             .114F * input.data()[3 * i + 2];
      log.expect(close(output.data()[i], expected, 2.0e-5F, 2.0e-6F),
                 "rgb_to_grayscale pixel=" + std::to_string(i));
    }
  }
  return finish("rgb_to_grayscale", log);
}

int test_conv1d(MetalContext &context, PipelineHandle pipeline) {
  TestLog log;
  const std::vector<std::pair<std::uint32_t, std::uint32_t>> shapes{
      {1, 1}, {2, 1}, {3, 3}, {5, 2}, {17, 5}, {31, 31}, {64, 7}, {263, 7}, {300, 45}};
  for (const auto [n, k] : shapes) {
    const std::size_t out_n = n - k + 1;
    auto iv = patterned(n), kv = patterned(k);
    for (float &v : kv)
      v *= .03125F;
    GuardedBuffer<float> input(n), kernel(k), output(out_n, kNaN);
    load(input, iv);
    load(kernel, kv);
    input.upload(context);
    kernel.upload(context);
    output.upload(context);
    Invocation invocation{
        {input.slice(), kernel.slice(), output.slice()}, constants(n, k), {out_n, 1, 1}};
    encode_solution(context, pipeline, invocation);
    output.download(context);
    log.expect(output.guards_intact(), "convolution_1d guard regions");
    for (std::size_t i = 0; i < out_n; ++i) {
      double sum = 0;
      for (std::size_t j = 0; j < k; ++j)
        sum += double(iv[i + j]) * kv[j];
      log.expect(close(output.data()[i], float(sum), 8e-4F, 2e-4F),
                 "convolution_1d shape=" + std::to_string(n) + "," + std::to_string(k));
    }
  }
  return finish("convolution_1d", log);
}

int test_conv2d(MetalContext &context, PipelineHandle pipeline) {
  TestLog log;
  struct Shape {
    std::uint32_t ir, ic, kr, kc;
  };
  const std::vector<Shape> shapes{{1, 1, 1, 1}, {3, 4, 1, 1},   {3, 3, 3, 3},
                                  {5, 7, 2, 3}, {17, 19, 5, 4}, {33, 31, 7, 7}};
  for (const auto s : shapes) {
    const auto orows = s.ir - s.kr + 1, ocols = s.ic - s.kc + 1;
    const std::size_t in_n = std::size_t(s.ir) * s.ic, k_n = std::size_t(s.kr) * s.kc,
                      out_n = std::size_t(orows) * ocols;
    auto iv = patterned(in_n), kv = patterned(k_n);
    for (float &v : kv)
      v *= .03125F;
    GuardedBuffer<float> input(in_n), kernel(k_n), output(out_n, kNaN);
    load(input, iv);
    load(kernel, kv);
    input.upload(context);
    kernel.upload(context);
    output.upload(context);
    Invocation invocation{{input.slice(), kernel.slice(), output.slice()},
                          constants(s.ir, s.ic, s.kr, s.kc),
                          {ocols, orows, 1}};
    encode_solution(context, pipeline, invocation);
    output.download(context);
    log.expect(output.guards_intact(), "convolution_2d guard regions");
    for (std::uint32_t r = 0; r < orows; ++r)
      for (std::uint32_t c = 0; c < ocols; ++c) {
        double sum = 0;
        for (std::uint32_t y = 0; y < s.kr; ++y)
          for (std::uint32_t x = 0; x < s.kc; ++x)
            sum += double(iv[std::size_t(r + y) * s.ic + c + x]) * kv[std::size_t(y) * s.kc + x];
        log.expect(close(output.data()[std::size_t(r) * ocols + c], float(sum), 1e-3F, 3e-4F),
                   "convolution_2d shape check");
      }
  }
  return finish("convolution_2d", log);
}

int test_rainbow(MetalContext &context, PipelineHandle pipeline) {
  TestLog log;
  struct Case {
    std::uint32_t n, r;
  };
  const std::vector<Case> cases{{1, 0},   {1, 1},   {4, 2},   {31, 3},
                                {255, 4}, {256, 8}, {257, 5}, {4099, 17}};
  auto hash = [](std::uint32_t value) {
    std::uint32_t h = 2166136261u;
    for (int b = 0; b < 4; ++b)
      h = (h ^ ((value >> (8 * b)) & 255u)) * 16777619u;
    return h;
  };
  for (const auto c : cases) {
    GuardedBuffer<std::int32_t> input(c.n);
    GuardedBuffer<std::uint32_t> output(c.n, 0xDEADBEEFu);
    for (std::size_t i = 0; i < c.n; ++i)
      input.data()[i] = static_cast<std::int32_t>(i * 2654435761u);
    if (c.n > 0)
      input.data()[0] = 0;
    if (c.n > 1)
      input.data()[1] = -1;
    if (c.n > 2)
      input.data()[2] = std::numeric_limits<std::int32_t>::min();
    input.upload(context);
    output.upload(context);
    Invocation inv{{input.slice(), output.slice()}, constants(c.n, c.r), {c.n, 1, 1}};
    encode_solution(context, pipeline, inv);
    output.download(context);
    log.expect(output.guards_intact(), "rainbow_table guard regions");
    for (std::size_t i = 0; i < c.n; ++i) {
      std::uint32_t expected = static_cast<std::uint32_t>(input.data()[i]);
      for (std::uint32_t r = 0; r < c.r; ++r)
        expected = hash(expected);
      log.expect(output.data()[i] == expected, at("rainbow_table", c.n, i));
    }
  }
  return finish("rainbow_table", log);
}

int test_reduction(MetalContext &context, PipelineHandle pipeline) {
  TestLog log;
  const std::vector<std::size_t> sizes{1, 2, 31, 32, 33, 255, 256, 257, 4099, 100000};
  for (const auto n : sizes) {
    auto values = patterned(n);
    for (std::size_t i = 0; i < n; ++i)
      values[i] = ((i & 1) ? -1.0F : 1.0F) * (1.0F + float(i % 7) * .25F);
    if (n > 3) {
      values[0] = 1000;
      values[1] = -1000;
      values[2] = .125F;
    }
    values[0] += .375F;
    GuardedBuffer<float> input(n), output(1, 0.0F);
    load(input, values);
    input.upload(context);
    output.upload(context);
    const std::uint32_t count = static_cast<std::uint32_t>(n);
    Invocation inv{{input.slice(), output.slice()}, constants(count), {n, 1, 1}};
    encode_solution(context, pipeline, inv);
    output.download(context);
    double expected = 0;
    for (float v : values)
      expected += v;
    const float tolerance = 2e-4F * std::max(1.0F, float(std::fabs(expected))) + 5e-7F * float(n);
    log.expect(std::isfinite(output.data()[0]) &&
                   std::fabs(output.data()[0] - float(expected)) <= tolerance,
               "reduction n=" + std::to_string(n));
    log.expect(output.guards_intact(), "reduction guard regions");
  }
  return finish("reduction", log);
}

int test_softmax(MetalContext &context, PipelineHandle pipeline) {
  TestLog log;
  const std::vector<std::size_t> sizes{1, 2, 31, 32, 33, 255, 256, 257, 4099};
  for (const auto n : sizes) {
    auto values = patterned(n);
    for (float &v : values)
      v *= .25F;
    if (n > 2) {
      values[0] = 100;
      values[1] = -100;
      values[2] = 99;
    }
    GuardedBuffer<float> input(n), output(n, kNaN);
    load(input, values);
    input.upload(context);
    output.upload(context);
    const std::uint32_t count = static_cast<std::uint32_t>(n);
    Invocation inv{{input.slice(), output.slice()}, constants(count), {n, 1, 1}};
    encode_solution(context, pipeline, inv);
    output.download(context);
    const float maximum = *std::max_element(values.begin(), values.end());
    double denom = 0;
    for (float v : values)
      denom += std::exp(double(v - maximum));
    double sum = 0;
    for (std::size_t i = 0; i < n; ++i) {
      const float expected = float(std::exp(double(values[i] - maximum)) / denom);
      sum += output.data()[i];
      log.expect(close(output.data()[i], expected, 5e-4F, 2e-6F) && output.data()[i] >= 0,
                 at("softmax", n, i));
    }
    log.expect(std::fabs(sum - 1.0) <= 5e-5, "softmax sum n=" + std::to_string(n));
    log.expect(output.guards_intact(), "softmax guard regions");
  }
  return finish("softmax", log);
}

int test_batchnorm(MetalContext &context, PipelineHandle pipeline) {
  TestLog log;
  struct Shape {
    std::uint32_t n, c;
  };
  const std::vector<Shape> shapes{{1, 1}, {1, 7}, {2, 3}, {7, 1}, {17, 5}, {32, 16}, {65, 33}};
  for (const auto s : shapes) {
    const std::size_t count = std::size_t(s.n) * s.c;
    auto values = patterned(count);
    if (s.n > 1 && s.c > 1)
      for (std::uint32_t n = 0; n < s.n; ++n)
        values[std::size_t(n) * s.c] = 3.5F;
    GuardedBuffer<float> input(count), gamma(s.c), beta(s.c), output(count, kNaN);
    load(input, values);
    for (std::size_t c = 0; c < s.c; ++c) {
      gamma.data()[c] = .5F + float(c % 5) * .25F;
      beta.data()[c] = float(int(c % 3) - 1) * .2F;
    }
    input.upload(context);
    gamma.upload(context);
    beta.upload(context);
    output.upload(context);
    const float eps = 1e-5F;
    Invocation inv{{input.slice(), gamma.slice(), beta.slice(), output.slice()},
                   constants(s.n, s.c, eps),
                   {s.c, s.n, 1}};
    encode_solution(context, pipeline, inv);
    output.download(context);
    std::vector<double> mean(s.c, 0), var(s.c, 0);
    for (std::uint32_t n = 0; n < s.n; ++n)
      for (std::uint32_t c = 0; c < s.c; ++c)
        mean[c] += values[std::size_t(n) * s.c + c];
    for (double &m : mean)
      m /= s.n;
    for (std::uint32_t n = 0; n < s.n; ++n)
      for (std::uint32_t c = 0; c < s.c; ++c) {
        double d = values[std::size_t(n) * s.c + c] - mean[c];
        var[c] += d * d;
      }
    for (double &v : var)
      v /= s.n;
    for (std::uint32_t n = 0; n < s.n; ++n)
      for (std::uint32_t c = 0; c < s.c; ++c) {
        const auto i = std::size_t(n) * s.c + c;
        const float expected =
            gamma.data()[c] * float((values[i] - mean[c]) / std::sqrt(var[c] + eps)) +
            beta.data()[c];
        log.expect(close(output.data()[i], expected, 8e-4F, 3e-4F),
                   "batch_normalization shape=" + std::to_string(s.n) + "x" + std::to_string(s.c));
      }
    log.expect(output.guards_intact(), "batch_normalization guard regions");
  }
  return finish("batch_normalization", log);
}

struct BenchBuffers {
  std::vector<BufferHandle> handles;
  Invocation invocation;
  double units{};
  double bytes{};
  double flops{};
};

void append_benchmark_record(const std::string &line) {
  const char *path = std::getenv("MLXGYM_BENCHMARK_RECORD_FILE");
  if (!path || !*path)
    return;
  std::ofstream output(path, std::ios::app);
  if (!output)
    throw std::runtime_error("Unable to write benchmark record file");
  output << line << '\n';
}

void print_bench(const std::string &task, const std::string &label, bool primary,
                 MetalContext &context, PipelineHandle pipeline, const Invocation &invocation,
                 double units, double bytes, double flops) {
  for (int i = 0; i < 5; ++i)
    encode_solution(context, pipeline, invocation);
  std::vector<double> samples;
  for (int i = 0; i < 30; ++i)
    samples.push_back(encode_solution(context, pipeline, invocation).gpu_seconds);
  std::sort(samples.begin(), samples.end());
  const double sec = samples[samples.size() / 2];
  const std::size_t p95_index = (samples.size() * 95 + 99) / 100 - 1;
  const double p95 = samples[p95_index];
  std::cout << std::left << std::setw(24) << label << " median=" << std::fixed
            << std::setprecision(3) << sec * 1e6 << " us p95=" << p95 * 1e6 << " us";
  if (bytes > 0)
    std::cout << "  " << bytes / sec / 1e9 << " GB/s";
  if (flops > 0)
    std::cout << "  " << flops / sec / 1e9 << " GFLOP/s";
  if (units > 0)
    std::cout << "  " << units / sec / 1e6 << " Munit/s";
  std::cout << '\n';

  // A stable, tab-separated record for scripts. Keep the normal output above pleasant for humans.
  std::ostringstream record;
  record << std::setprecision(17) << "MLXGYM_BENCH_V1\t" << task << '\t' << label << '\t'
         << (primary ? 1 : 0) << '\t' << sec * 1e6 << '\t' << p95 * 1e6 << '\t'
         << (bytes > 0 ? bytes / sec / 1e9 : 0.0) << '\t' << (flops > 0 ? flops / sec / 1e9 : 0.0)
         << '\t' << (units > 0 ? units / sec / 1e6 : 0.0);
  append_benchmark_record(record.str());
}

int benchmark_task(const std::string &task, MetalContext &context, PipelineHandle pipeline) {
  auto buffer = [&](std::size_t bytes) { return context.create_buffer(bytes); };
  auto run_1d = [&](std::size_t n, std::vector<BufferSlice> buffers, std::vector<ConstantArg> args,
                    std::size_t grid, double byte_count, double flop_count = 0.0,
                    const std::string &suffix = " elements") {
    Invocation inv{std::move(buffers), std::move(args), {grid, 1, 1}};
    print_bench(task, std::to_string(n) + suffix, n == (std::size_t(1) << 24), context, pipeline,
                inv, double(n), byte_count, flop_count);
  };

  const bool plain_unary = task == "relu" || task == "leaky_relu" || task == "sigmoid_activation" ||
                           task == "sigmoid_linear_unit";
  if (task == "vector_add" || task == "interleave_arrays" || plain_unary ||
      task == "reverse_array" || task == "value_clipping" || task == "swish_gated_linear_unit" ||
      task == "gaussian_error_gated_linear_unit") {
    for (const std::size_t n : {std::size_t(4096), std::size_t(1) << 20, std::size_t(1) << 24}) {
      const std::uint32_t count = static_cast<std::uint32_t>(n);
      auto a = buffer(n * 4);
      std::vector<BufferSlice> bs;
      std::vector<ConstantArg> cs;
      double bytes = n * 8.0;
      std::size_t grid = n;
      if (task == "vector_add") {
        auto b = buffer(n * 4), out = buffer(n * 4);
        bs = {{a, 0}, {b, 0}, {out, 0}};
        bytes = n * 12.0;
        cs = constants(count);
      } else if (task == "interleave_arrays") {
        auto b = buffer(n * 4), out = buffer(n * 8);
        bs = {{a, 0}, {b, 0}, {out, 0}};
        bytes = n * 16.0;
        cs = constants(count);
      } else if (task == "reverse_array") {
        bs = {{a, 0}};
        cs = constants(count);
        grid = (n + 1) / 2;
      } else {
        auto out = buffer(n * 4);
        bs = {{a, 0}, {out, 0}};
        cs = constants(count);
        if (task == "value_clipping") {
          const float lo = -2.5F, hi = 3.5F;
          cs = constants(lo, hi, count);
        }
        if (task.find("gated_linear_unit") != std::string::npos) {
          grid = n / 2;
          bytes = n * 6.0;
        }
      }
      run_1d(n, std::move(bs), std::move(cs), grid, bytes);
    }
    return 0;
  }
  if (task == "matrix_copy" || task == "matrix_addition") {
    for (const std::uint32_t n : {512u, 2048u, 4096u}) {
      const std::size_t count = std::size_t(n) * n;
      auto a = buffer(count * 4), out = buffer(count * 4);
      std::vector<BufferSlice> bs{{a, 0}};
      double bytes = count * 8.0;
      if (task == "matrix_addition") {
        auto b = buffer(count * 4);
        bs.push_back({b, 0});
        bytes = count * 12.0;
      }
      bs.push_back({out, 0});
      Invocation inv{bs, constants(n), {n, n, 1}};
      print_bench(task, std::to_string(n) + "x" + std::to_string(n), n == 4096, context, pipeline,
                  inv, count, bytes, 0);
    }
    return 0;
  }
  if (task == "matrix_transpose") {
    for (const auto shape : {std::pair{512u, 512u}, std::pair{2048u, 2048u},
                             std::pair{4096u, 1024u}, std::pair{4093u, 1021u}}) {
      auto [r, c] = shape;
      const std::size_t n = std::size_t(r) * c;
      auto in = buffer(n * 4), out = buffer(n * 4);
      Invocation inv{{{in, 0}, {out, 0}}, constants(r, c), {c, r, 1}};
      print_bench(task, std::to_string(r) + "x" + std::to_string(c), r == 4096 && c == 1024,
                  context, pipeline, inv, n, n * 8.0, 0);
    }
    return 0;
  }
  if (task == "matrix_multiplication") {
    for (const std::uint32_t n : {256u, 512u, 1024u}) {
      const std::size_t count = std::size_t(n) * n;
      auto a = buffer(count * 4), b = buffer(count * 4), out = buffer(count * 4);
      Invocation inv{{{a, 0}, {b, 0}, {out, 0}}, constants(n, n, n), {n, n, 1}};
      print_bench(task, std::to_string(n) + " cubed", n == 1024, context, pipeline, inv, count, 0,
                  2.0 * n * n * n);
    }
    return 0;
  }
  if (task == "softmax_attention") {
    for (const auto shape :
         {std::array<std::uint32_t, 3>{128, 128, 64}, std::array<std::uint32_t, 3>{512, 512, 64},
          std::array<std::uint32_t, 3>{1024, 1024, 128}}) {
      const auto [m, n, d] = shape;
      const std::size_t q_count = std::size_t(m) * d;
      const std::size_t kv_count = std::size_t(n) * d;
      auto q = buffer(q_count * 4), k = buffer(kv_count * 4), v = buffer(kv_count * 4);
      auto output = buffer(q_count * 4);
      Invocation inv{{{q, 0}, {k, 0}, {v, 0}, {output, 0}}, constants(m, n, d), {d, m, 1}};
      const std::string label =
          "M=" + std::to_string(m) + " N=" + std::to_string(n) + " d=" + std::to_string(d);
      const double flops = 4.0 * double(m) * n * d;
      const double bytes = 4.0 * (q_count * 2.0 + kv_count * 2.0);
      print_bench(task, label, m == 1024 && n == 1024 && d == 128, context, pipeline, inv, q_count,
                  bytes, flops);
    }
    return 0;
  }
  if (task == "color_inversion" || task == "rgb_to_grayscale") {
    for (const auto shape :
         {std::pair{1920u, 1080u}, std::pair{3840u, 2160u}, std::pair{1919u, 1079u}}) {
      auto [w, h] = shape;
      const std::size_t pixels = std::size_t(w) * h;
      if (task == "color_inversion") {
        auto image = buffer(pixels * 4);
        Invocation inv{{{image, 0}}, constants(w, h), {pixels, 1, 1}};
        print_bench(task, std::to_string(w) + "x" + std::to_string(h), w == 3840 && h == 2160,
                    context, pipeline, inv, pixels, pixels * 8.0, 0);
      } else {
        auto in = buffer(pixels * 12), out = buffer(pixels * 4);
        Invocation inv{{{in, 0}, {out, 0}}, constants(w, h), {pixels, 1, 1}};
        print_bench(task, std::to_string(w) + "x" + std::to_string(h), w == 3840 && h == 2160,
                    context, pipeline, inv, pixels, pixels * 16.0, 0);
      }
    }
    return 0;
  }
  if (task == "convolution_1d") {
    const std::uint32_t n = 1u << 20;
    for (const std::uint32_t k : {3u, 7u, 31u, 127u}) {
      const std::size_t out_n = n - k + 1;
      auto in = buffer(n * 4), weights = buffer(k * 4), out = buffer(out_n * 4);
      Invocation inv{{{in, 0}, {weights, 0}, {out, 0}}, constants(n, k), {out_n, 1, 1}};
      print_bench(task, "N=1M K=" + std::to_string(k), k == 127, context, pipeline, inv, out_n, 0,
                  2.0 * out_n * k);
    }
    return 0;
  }
  if (task == "convolution_2d") {
    for (const std::uint32_t side : {512u, 1024u}) {
      for (const std::uint32_t k : {3u, 5u, 7u}) {
        const auto os = side - k + 1;
        const std::size_t in_n = std::size_t(side) * side, out_n = std::size_t(os) * os;
        auto in = buffer(in_n * 4), weights = buffer(k * k * 4), out = buffer(out_n * 4);
        Invocation inv{{{in, 0}, {weights, 0}, {out, 0}}, constants(side, side, k, k), {os, os, 1}};
        print_bench(task, std::to_string(side) + "^2 K=" + std::to_string(k),
                    side == 1024 && k == 7, context, pipeline, inv, out_n, 0, 2.0 * out_n * k * k);
      }
    }
    return 0;
  }
  if (task == "rainbow_table") {
    const std::uint32_t n = 1u << 20;
    for (const std::uint32_t r : {1u, 100u, 1000u}) {
      auto in = buffer(n * 4), out = buffer(n * 4);
      Invocation inv{{{in, 0}, {out, 0}}, constants(n, r), {n, 1, 1}};
      print_bench(task, "N=1M R=" + std::to_string(r), r == 1000, context, pipeline, inv,
                  double(n) * r, n * 8.0, 0);
    }
    return 0;
  }
  if (task == "reduction" || task == "softmax") {
    for (const std::size_t n : {std::size_t(1024), std::size_t(1) << 20, std::size_t(1) << 24}) {
      const auto count = static_cast<std::uint32_t>(n);
      auto in = buffer(n * 4), out = buffer((task == "reduction" ? 1 : n) * 4);
      Invocation inv{{{in, 0}, {out, 0}}, constants(count), {n, 1, 1}};
      print_bench(task, std::to_string(n) + " elements", n == (std::size_t(1) << 24), context,
                  pipeline, inv, n, n * 4.0 + (task == "reduction" ? 4.0 : n * 4.0), 0);
    }
    return 0;
  }
  if (task == "batch_normalization") {
    for (const auto shape :
         {std::pair{1024u, 256u}, std::pair{4096u, 768u}, std::pair{8192u, 1024u}}) {
      auto [n, c] = shape;
      const std::size_t count = std::size_t(n) * c;
      auto in = buffer(count * 4), g = buffer(c * 4), b = buffer(c * 4), out = buffer(count * 4);
      const float eps = 1e-5F;
      Invocation inv{{{in, 0}, {g, 0}, {b, 0}, {out, 0}}, constants(n, c, eps), {c, n, 1}};
      print_bench(task, std::to_string(n) + "x" + std::to_string(c), n == 8192 && c == 1024,
                  context, pipeline, inv, count, count * 8.0 + c * 8.0, 0);
    }
    return 0;
  }
  throw std::runtime_error("No benchmark for task: " + task);
}

int run_correctness(const std::string &task, MetalContext &context, PipelineHandle pipeline) {
  if (task == "vector_add")
    return test_vector_add(context, pipeline);
  if (task == "reverse_array")
    return test_reverse(context, pipeline);
  if (task == "interleave_arrays")
    return test_interleave(context, pipeline);
  if (task == "matrix_copy")
    return test_matrix_copy_or_add(task, context, pipeline, false);
  if (task == "matrix_addition")
    return test_matrix_copy_or_add(task, context, pipeline, true);
  if (task == "matrix_transpose")
    return test_transpose(context, pipeline);
  if (task == "matrix_multiplication")
    return test_matmul(context, pipeline);
  if (task == "softmax_attention")
    return test_softmax_attention(context, pipeline);
  if (task == "color_inversion")
    return test_color(context, pipeline);
  if (task == "rgb_to_grayscale")
    return test_rgb(context, pipeline);
  if (task == "convolution_1d")
    return test_conv1d(context, pipeline);
  if (task == "convolution_2d")
    return test_conv2d(context, pipeline);
  if (task == "rainbow_table")
    return test_rainbow(context, pipeline);
  if (task == "reduction")
    return test_reduction(context, pipeline);
  if (task == "softmax")
    return test_softmax(context, pipeline);
  if (task == "batch_normalization")
    return test_batchnorm(context, pipeline);
  if (task == "relu")
    return test_unary(task, context, pipeline, [](float x) { return x > 0 ? x : 0; }, true);
  if (task == "leaky_relu")
    return test_unary(task, context, pipeline, [](float x) { return x >= 0 ? x : .01F * x; });
  if (task == "value_clipping")
    return test_clipping(context, pipeline);
  if (task == "sigmoid_activation")
    return test_unary(task, context, pipeline,
                      [](float x) { return 1.0F / (1.0F + std::exp(-x)); });
  if (task == "sigmoid_linear_unit")
    return test_unary(task, context, pipeline, [](float x) { return x / (1.0F + std::exp(-x)); });
  if (task == "swish_gated_linear_unit")
    return test_gated(task, context, pipeline,
                      [](float a, float b) { return a / (1 + std::exp(-a)) * b; });
  if (task == "gaussian_error_gated_linear_unit")
    return test_gated(task, context, pipeline, [](float a, float b) {
      return a * .5F * b * (1 + std::erf(b * .7071067811865475F));
    });
  throw std::runtime_error("Unknown task: " + task);
}

} // namespace

std::vector<std::size_t> boundary_sizes() {
  return {0, 1, 2, 31, 32, 33, 63, 64, 65, 127, 128, 129, 255, 256, 257, 4099};
}
std::mt19937 make_rng() { return std::mt19937(0x4D4C5847u); }

int run_task(const std::string &task, MetalContext &context, PipelineHandle pipeline,
             bool benchmark) {
  const int correctness = run_correctness(task, context, pipeline);
  if (correctness || !benchmark)
    return correctness;
  std::cout << "Benchmarking on " << context.device_name() << "\n";
  append_benchmark_record("MLXGYM_DEVICE_V1\t" + context.device_name());
  return benchmark_task(task, context, pipeline);
}

} // namespace mlxgym
