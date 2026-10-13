#include <stddef.h>

int saxpy(const int *restrict x, int *restrict y, int a, size_t n) {
  for (size_t i = 0; i < n; ++i)
    y[i] = a * x[i] + y[i];
  return y[n - 1];
}

int branchy_sum(const int *values, size_t n) {
  int result = 0;
  for (size_t i = 0; i < n; ++i)
    result += values[i] > 0 ? values[i] : -values[i];
  return result;
}
