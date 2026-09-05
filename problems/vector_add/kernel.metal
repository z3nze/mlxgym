#include <metal_stdlib>
using namespace metal;

// TODO: Implement this exercise. The function intentionally writes no output,
// so the correctness suite fails until you supply the Metal algorithm.
kernel void solve(device const float *a [[buffer(0)]], device const float *b [[buffer(1)]],
                  device float *output [[buffer(2)]], constant uint &n [[buffer(3)]],
                  uint gid [[thread_position_in_grid]]) {
  if (gid < n) {
    output[gid] = a[gid] + b[gid];
  }
}
