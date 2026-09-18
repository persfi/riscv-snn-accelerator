#include "../bsp/mmio.h"
#include "../libsnn/image.h"
#include "../libsnn/snn.h"
#include "../libsnn/netcfg.h"


int main() {

  snn_config(NET_T, NET_K, NET_VTH1, NET_VTH2, NET_HIDDEN);
  
  #ifdef BOARD
  for (;;) {
    int idx = MMIO_SW & 0xF;
    if (idx >= IMAGE_COUNT) idx = IMAGE_COUNT - 1;
    MMIO_LED = snn_run_image(idx);
  }
#else
  int idx = MMIO_SW & 0xF;
  if (idx >= IMAGE_COUNT) idx = IMAGE_COUNT - 1;
  int pred = snn_run_image(idx);
  for (int i = 0; i < 10; i++) MMIO_PRINT_INT = ACCEL_COUNT(i);
  MMIO_PRINT_INT = pred;
  MMIO_LED = pred;
#endif

  
}
