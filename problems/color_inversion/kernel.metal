#include <metal_stdlib>
using namespace metal;

// TODO: Implement this exercise. The function intentionally writes no output,
// so the correctness suite fails until you supply the Metal algorithm.
kernel void solve(device uchar* image [[buffer(0)]], constant uint& width [[buffer(1)]], constant uint& height [[buffer(2)]], uint gid [[thread_position_in_grid]]) {}
