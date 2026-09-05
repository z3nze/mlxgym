#include <metal_stdlib>
using namespace metal;

// TODO: Implement this exercise. The function intentionally writes no output,
// so the correctness suite fails until you supply the Metal algorithm.
kernel void solve(device const float* input [[buffer(0)]], device const float* weights [[buffer(1)]], device float* output [[buffer(2)]], constant uint& input_size [[buffer(3)]], constant uint& kernel_size [[buffer(4)]], uint gid [[thread_position_in_grid]]) {}
