#include "../bsp/mmio.h"
#include "encode.h"
#include "snn.h"

void snn_config(int t, int k, int vth1, int vth2) {
  ACCEL_T = t;
  ACCEL_K = k;
  ACCEL_VTH1 = vth1;
  ACCEL_VTH2 = vth2;
}

int snn_run_image(int img, int t_max) {
  ACCEL_EVA_LEN = encode_timestep(img, 0, &ACCEL_EVA(0));
  ACCEL_START = 1;

  for (int i = 1; i < t_max; i++) {
    while (1) {
      if (i % 2 == 0 && (ACCEL_BANK_A_FREE & ACCEL_STATUS)) {
        ACCEL_EVA_LEN = encode_timestep(img, i, &ACCEL_EVA(0));
        break;
      } else if (i % 2 == 1 && (ACCEL_BANK_B_FREE & ACCEL_STATUS)) {
        ACCEL_EVB_LEN = encode_timestep(img, i, &ACCEL_EVB(0));
        break;
      }
    }
  }

  while (!(ACCEL_STATUS & ACCEL_IMAGE_DONE));
  return snn_argmax();
}

int snn_argmax(void) {
  int max = 0;
  int max_idx = 0;
  for (int i = 0; i < 3; i++) {
    unsigned int word = ACCEL_COUNT_W(i); //use count w save a tiny bit of cycles because there's 7 less volatile loads
    for (int byte = 0; byte < 4; byte++) {
      int digit = i * 4 + byte;
      int count = (word >> (8 * byte)) & 0xFF;
      if (digit < 10 && count > max) {
        max_idx = digit;
        max = count;
      }
    }
  }

  return max_idx;
}
