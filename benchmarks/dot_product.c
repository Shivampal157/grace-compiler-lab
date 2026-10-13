#include <stddef.h>

// Vector Dot Product with Horizontal Accumulation
// Evaluates reduction patterns, vector accumulator dependencies, and loop unrolling.
float dot_product(const float *restrict a, const float *restrict b, size_t n) {
  float sum = 0.0f;
  for (size_t i = 0; i < n; ++i) {
    sum += a[i] * b[i];
  }
  return sum;
}
