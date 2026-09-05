#include <metal_stdlib>
using namespace metal;

// TODO: Implement this exercise. The function intentionally writes no output,
// so the correctness suite fails until you supply the Metal algorithm.
kernel void solve(device const int* input [[buffer(0)]], device uint* output [[buffer(1)]], constant uint& n [[buffer(2)]], constant uint& rounds [[buffer(3)]], uint gid [[thread_position_in_grid]]) {}
