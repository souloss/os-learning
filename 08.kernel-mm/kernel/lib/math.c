#include "math.h"

uint32_t max(uint32_t x, uint32_t y) {
  return x > y ? x : y;
}

uint32_t min(uint32_t x, uint32_t y) {
  return x < y ? x : y;
}

int32_t div(int32_t x, int32_t N) {
  if (x >= 0) {
    return x / N;
  } else {
    return (x - N + 1) / N;
  }
}

int32_t mod(int32_t x, int32_t N) {
  return (x % N + N) %N;
}
