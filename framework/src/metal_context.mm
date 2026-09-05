#import <Foundation/Foundation.h>
#import <Metal/Metal.h>

#include "mlxgym/metal_context.hpp"

#include <algorithm>
#include <cstring>
#include <stdexcept>

namespace mlxgym {
namespace {

std::runtime_error metal_error(NSString *prefix, NSError *error = nil) {
  NSString *message =
      error ? [NSString stringWithFormat:@"%@: %@", prefix, error.localizedDescription] : prefix;
  return std::runtime_error(message.UTF8String);
}

} // namespace

class MetalContext::Impl {
public:
  id<MTLDevice> device;
  id<MTLCommandQueue> queue;
  NSMutableArray<id<MTLBuffer>> *buffers;
  NSMutableArray<id<MTLComputePipelineState>> *pipelines;
  bool capturing{false};
};

MetalContext::MetalContext() : impl_(std::make_unique<Impl>()) {
  @autoreleasepool {
    impl_->device = MTLCreateSystemDefaultDevice();
    if (!impl_->device)
      throw metal_error(@"No Metal device is available");
    impl_->queue = [impl_->device newCommandQueue];
    if (!impl_->queue)
      throw metal_error(@"Unable to create a Metal command queue");
    impl_->buffers = [NSMutableArray array];
    impl_->pipelines = [NSMutableArray array];
  }
}

MetalContext::~MetalContext() = default;

std::string MetalContext::device_name() const { return impl_->device.name.UTF8String; }

BufferHandle MetalContext::create_buffer(std::size_t size, const void *initial_data) {
  @autoreleasepool {
    const std::size_t allocation_size = std::max<std::size_t>(size, 1);
    id<MTLBuffer> buffer = [impl_->device newBufferWithLength:allocation_size
                                                      options:MTLResourceStorageModeShared];
    if (!buffer)
      throw metal_error(@"Unable to allocate a Metal buffer");
    if (initial_data && size)
      std::memcpy(buffer.contents, initial_data, size);
    const std::size_t index = impl_->buffers.count;
    [impl_->buffers addObject:buffer];
    return {index, size};
  }
}

void MetalContext::upload(BufferHandle buffer, const void *data, std::size_t size,
                          std::size_t offset) {
  if (offset + size > buffer.size)
    throw std::out_of_range("Metal upload exceeds buffer");
  id<MTLBuffer> native = impl_->buffers[buffer.index];
  std::memcpy(static_cast<std::uint8_t *>(native.contents) + offset, data, size);
}

void MetalContext::download(BufferHandle buffer, void *data, std::size_t size,
                            std::size_t offset) const {
  if (offset + size > buffer.size)
    throw std::out_of_range("Metal download exceeds buffer");
  id<MTLBuffer> native = impl_->buffers[buffer.index];
  std::memcpy(data, static_cast<const std::uint8_t *>(native.contents) + offset, size);
}

PipelineHandle MetalContext::load_pipeline(const std::string &library_path,
                                           const std::string &function_name) {
  @autoreleasepool {
    NSError *error = nil;
    NSURL *url = [NSURL fileURLWithPath:[NSString stringWithUTF8String:library_path.c_str()]];
    id<MTLLibrary> library = [impl_->device newLibraryWithURL:url error:&error];
    if (!library)
      throw metal_error(@"Unable to load Metal library", error);
    NSString *name = [NSString stringWithUTF8String:function_name.c_str()];
    id<MTLFunction> function = [library newFunctionWithName:name];
    if (!function)
      throw metal_error([NSString stringWithFormat:@"Metal function '%@' is missing", name]);
    id<MTLComputePipelineState> pipeline =
        [impl_->device newComputePipelineStateWithFunction:function error:&error];
    if (!pipeline)
      throw metal_error(@"Unable to create compute pipeline", error);
    const std::size_t index = impl_->pipelines.count;
    [impl_->pipelines addObject:pipeline];
    return {index};
  }
}

DispatchTiming MetalContext::dispatch(PipelineHandle pipeline_handle,
                                      const Invocation &invocation) {
  @autoreleasepool {
    if (invocation.grid.x == 0 || invocation.grid.y == 0 || invocation.grid.z == 0)
      return {};

    id<MTLComputePipelineState> pipeline = impl_->pipelines[pipeline_handle.index];
    id<MTLCommandBuffer> command_buffer = [impl_->queue commandBuffer];
    command_buffer.label = @"mlxgym dispatch";
    id<MTLComputeCommandEncoder> encoder = [command_buffer computeCommandEncoder];
    [encoder setComputePipelineState:pipeline];
    for (std::size_t i = 0; i < invocation.buffers.size(); ++i) {
      const BufferSlice &slice = invocation.buffers[i];
      id<MTLBuffer> buffer = impl_->buffers[slice.buffer.index];
      [encoder setBuffer:buffer offset:slice.offset atIndex:i];
    }
    const std::size_t constant_base = invocation.buffers.size();
    for (std::size_t i = 0; i < invocation.constants.size(); ++i) {
      const auto &bytes = invocation.constants[i].bytes;
      [encoder setBytes:bytes.data() length:bytes.size() atIndex:constant_base + i];
    }

    MTLSize threads;
    if (invocation.grid.y > 1 || invocation.grid.z > 1) {
      std::size_t side = 16;
      while (side * side > pipeline.maxTotalThreadsPerThreadgroup)
        side /= 2;
      threads = MTLSizeMake(side, side, 1);
    } else {
      const std::size_t width = std::min<std::size_t>(256, pipeline.maxTotalThreadsPerThreadgroup);
      threads = MTLSizeMake(width, 1, 1);
    }
    [encoder dispatchThreads:MTLSizeMake(invocation.grid.x, invocation.grid.y, invocation.grid.z)
        threadsPerThreadgroup:threads];
    [encoder endEncoding];
    [command_buffer commit];
    [command_buffer waitUntilCompleted];
    if (command_buffer.status == MTLCommandBufferStatusError)
      throw metal_error(@"Metal command buffer failed", command_buffer.error);
    return {command_buffer.GPUEndTime - command_buffer.GPUStartTime};
  }
}

void MetalContext::begin_capture(const std::string &output_path) {
  @autoreleasepool {
    MTLCaptureManager *manager = MTLCaptureManager.sharedCaptureManager;
    if (![manager supportsDestination:MTLCaptureDestinationGPUTraceDocument])
      throw metal_error(@"GPU trace documents are not supported");
    MTLCaptureDescriptor *descriptor = [[MTLCaptureDescriptor alloc] init];
    descriptor.captureObject = impl_->device;
    descriptor.destination = MTLCaptureDestinationGPUTraceDocument;
    descriptor.outputURL =
        [NSURL fileURLWithPath:[NSString stringWithUTF8String:output_path.c_str()]];
    NSError *error = nil;
    if (![manager startCaptureWithDescriptor:descriptor error:&error])
      throw metal_error(@"Unable to begin Metal capture", error);
    impl_->capturing = true;
  }
}

void MetalContext::end_capture() {
  if (impl_->capturing) {
    [MTLCaptureManager.sharedCaptureManager stopCapture];
    impl_->capturing = false;
  }
}

} // namespace mlxgym
