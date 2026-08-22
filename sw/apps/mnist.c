#include "../bsp/mmio.h"
#include "../libsnn/image.h"
#include "../libsnn/snn.h"


int main() {
  snn_config(20, 2, 248, 295);
  int pred = snn_run_image(IMAGE_INDEX, 20);
  for (int i = 0; i < 10; i++) MMIO_PRINT_INT = ACCEL_COUNT(i);
  MMIO_PRINT_INT = pred;
}
