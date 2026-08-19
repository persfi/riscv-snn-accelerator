
#include "../bsp/mmio.h"
#include "../libsnn/encode.h"
#include "../libsnn/image.h"

#define T 20

int main(void) {
  for (int t = 0; t < T; t++)
    MMIO_PRINT_INT = encode_timestep(IMAGE_INDEX, t, &ACCEL_EVA(0));
  return 0;
}
