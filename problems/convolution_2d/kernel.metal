#include <metal_stdlib>
using namespace metal;

// TODO: Implement this exercise. The function intentionally writes no output,
// so the correctness suite fails until you supply the Metal algorithm.
kernel void solve(device const float* input [[buffer(0)]], device const float* weights [[buffer(1)]], device float* output [[buffer(2)]], constant uint& input_rows [[buffer(3)]], constant uint& input_cols [[buffer(4)]], constant uint& kernel_rows [[buffer(5)]], constant uint& kernel_cols [[buffer(6)]], uint2 gid [[thread_position_in_grid]]) {}
