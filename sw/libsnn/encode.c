#include "encode.h"

#include "image.h"

unsigned int wang32(unsigned int x) {
  x = ~x + (x << 15);
  x ^= x >> 12;
  x = x + (x << 2);
  x ^= x >> 4;
  x = (x + (x << 3)) + (x << 11);
  x ^= x >> 16;
  return x;
}

int encode_timestep(int img, int t, volatile unsigned int* bank) {
  unsigned int key = wang32(0u);
  int n = 0;

  for (int p = 0; p < IMAGE_PIXELS; p++) {
    unsigned int ctr =
        ((unsigned int)img << 15) | ((unsigned int)t << 10) | (unsigned int)p;
    if ((wang32(key ^ ctr) & 0xFFu) < image[p]) {
      bank[n] = (unsigned int)p;
      n++;
    }
  }
  return n;
}
