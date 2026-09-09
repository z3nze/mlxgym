#include <metal_stdlib>
using namespace metal;

// TODO: Implement this exercise. The function intentionally writes no output,
// so the correctness suite fails until you supply the Metal algorithm.
kernel void solve(device const float *input [[buffer(0)]], device float *output [[buffer(1)]],
                  constant float &lo [[buffer(2)]], constant float &hi [[buffer(3)]],
                  constant uint &n [[buffer(4)]], uint gid [[thread_position_in_grid]]) {
  output[gid] = min(max(lo, input[gid]), hi);
}
