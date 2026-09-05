#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace mlxgym {

struct Grid {
  std::size_t x{1};
  std::size_t y{1};
  std::size_t z{1};
};

struct BufferHandle {
  std::size_t index{};
  std::size_t size{};
};

struct PipelineHandle {
  std::size_t index{};
};

struct BufferSlice {
  BufferHandle buffer;
  std::size_t offset{};
};

struct ConstantArg {
  std::vector<std::uint8_t> bytes;

  template <typename T> static ConstantArg from(const T &value) {
    const auto *begin = reinterpret_cast<const std::uint8_t *>(&value);
    return {{begin, begin + sizeof(T)}};
  }
};

struct Invocation {
  std::vector<BufferSlice> buffers;
  std::vector<ConstantArg> constants;
  Grid grid;
};

struct DispatchTiming {
  double gpu_seconds{};
};

class MetalContext {
public:
  MetalContext();
  ~MetalContext();
  MetalContext(const MetalContext &) = delete;
  MetalContext &operator=(const MetalContext &) = delete;

  std::string device_name() const;
  BufferHandle create_buffer(std::size_t size, const void *initial_data = nullptr);
  void upload(BufferHandle buffer, const void *data, std::size_t size, std::size_t offset = 0);
  void download(BufferHandle buffer, void *data, std::size_t size, std::size_t offset = 0) const;
  PipelineHandle load_pipeline(const std::string &library_path, const std::string &function_name);
  DispatchTiming dispatch(PipelineHandle pipeline, const Invocation &invocation);
  void begin_capture(const std::string &output_path);
  void end_capture();

private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};

DispatchTiming encode_solution(MetalContext &context, PipelineHandle pipeline,
                               const Invocation &invocation);

} // namespace mlxgym
