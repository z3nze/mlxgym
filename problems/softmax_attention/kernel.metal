#include <metal_stdlib>
using namespace metal;

// TODO: Implement scaled dot-product attention:
// output = softmax(Q * transpose(K) / sqrt(d)) * V.
// Softmax is applied independently to every row of the M-by-N score matrix.
kernel void solve(device const float *q [[buffer(0)]], device const float *k [[buffer(1)]],
                  device const float *v [[buffer(2)]], device float *output [[buffer(3)]],
                  constant uint &m [[buffer(4)]], constant uint &n [[buffer(5)]],
                  constant uint &d [[buffer(6)]], uint2 gid [[thread_position_in_grid]]) {}
