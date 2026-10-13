#include <stddef.h>

// Single-Precision A*X + Y (SAXPY)
// Memory-bandwidth bounded streaming vector kernel.
// Evaluates Grace CPU LPDDR5X (546 GB/s) streaming throughput and prefetchers.
void saxpy(const float *restrict x, float *restrict y, float a, size_t n) {
  for (size_t i = 0; i < n; ++i) {
    y[i] = a * x[i] + y[i];
  }
}
