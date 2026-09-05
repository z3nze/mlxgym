#include <metal_stdlib>
using namespace metal;

// TODO: Implement this exercise. The function intentionally writes no output,
// so the correctness suite fails until you supply the Metal algorithm.
kernel void solve(device const float* input [[buffer(0)]], device const float* gamma [[buffer(1)]], device const float* beta [[buffer(2)]], device float* output [[buffer(3)]], constant uint& n [[buffer(4)]], constant uint& channels [[buffer(5)]], constant float& eps [[buffer(6)]], uint2 gid [[thread_position_in_grid]]) {}
