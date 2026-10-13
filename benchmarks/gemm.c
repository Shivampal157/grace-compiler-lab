#include <stddef.h>

// Dense Matrix Multiplication Tile Kernel
// Prime candidate for FMA (Fused Multiply-Add) contraction and SVE2 vectorization.
// Key Grace CPU target features: 4x128-bit vector pipelines, dual FMA issue.
void gemm_tile(const float *restrict A, const float *restrict B, float *restrict C,
               size_t M, size_t N, size_t K) {
  for (size_t i = 0; i < M; ++i) {
    for (size_t j = 0; j < N; ++j) {
      float sum = C[i * N + j];
      for (size_t k = 0; k < K; ++k) {
        sum += A[i * K + k] * B[k * N + j];
      }
      C[i * N + j] = sum;
    }
  }
}
