#include <stddef.h>

// 2D 5-Point Stencil Diffusion Kernel
// Evaluates multi-dimensional memory strides, cache-line reuse, and neighbor access.
void stencil_2d(const float *restrict in, float *restrict out, size_t rows, size_t cols,
                float c0, float c1) {
  for (size_t r = 1; r < rows - 1; ++r) {
    for (size_t c = 1; c < cols - 1; ++c) {
      float center = in[r * cols + c];
      float north  = in[(r - 1) * cols + c];
      float south  = in[(r + 1) * cols + c];
      float west   = in[r * cols + (c - 1)];
      float east   = in[r * cols + (c + 1)];
      out[r * cols + c] = c0 * center + c1 * (north + south + east + west);
    }
  }
}
