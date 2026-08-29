#include "../bsp/mmio.h"
#include "../libsnn/image.h"
#include "../libsnn/snn.h"
#include "../libsnn/netcfg.h"


int main() {
  snn_config(NET_T, NET_K, NET_VTH1, NET_VTH2, NET_HIDDEN);
  int pred = snn_run_image(IMAGE_INDEX);
  for (int i = 0; i < 10; i++) MMIO_PRINT_INT = ACCEL_COUNT(i);
  MMIO_PRINT_INT = pred;
}
