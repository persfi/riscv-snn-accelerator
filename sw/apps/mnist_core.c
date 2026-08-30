#include "../bsp/mmio.h"
#include "../libsnn/encode.h"
#include "../libsnn/image.h"
#include "../libsnn/netcfg.h"
#include "../libsnn/snn.h"

static const signed char* const w1 = (const signed char*)0x4000;
static const signed char* const w2 = (const signed char*)0x1D000;

static unsigned int bank[784];
static int acc[NET_HIDDEN], v1[NET_HIDDEN], v2[10], count[10],
    spike[NET_HIDDEN];

/* Phase markers for the harness to compute how many cycles used in encode and evaluation stage */
#define MARK_START 0
#define MARK_ENCODE 1
#define MARK_EVAL 2

int main() {
  MMIO_PRINT = MARK_START;

  for (int i = 0; i < NET_T; i++) {
    int ev_len = encode_timestep(IMAGE_INDEX, i, bank);
    MMIO_PRINT = MARK_ENCODE;
    int n = 0;

    for (int j = 0; j < ev_len; j++) {
      const signed char* row = w1 + bank[j] * NET_HIDDEN;
      for (int k = 0; k < NET_HIDDEN; k++) {
        acc[k] += row[k];
      }
    }

    for (int k = 0; k < NET_HIDDEN; k++) {
      v1[k] = v1[k] - (v1[k] >> NET_K);

      if (v1[k] + acc[k] > NET_VTH1) {
        v1[k] = v1[k] - NET_VTH1 + acc[k];
        spike[n++] = k;
      } else {
        v1[k] += acc[k];
      }

      if (v1[k] > 32767)
        v1[k] = 32767;
      else if (v1[k] < -32768)
        v1[k] = -32768;

      acc[k] = 0;
    }

    for (int j = 0; j < n; j++) {
      const signed char* row = w2 + spike[j] * 16;
      for (int k = 0; k < 10; k++) {
        acc[k] += row[k];
      }
    }

    for (int k = 0; k < 10; k++) {
      v2[k] = v2[k] - (v2[k] >> NET_K);

      if (v2[k] + acc[k] > NET_VTH2) {
        v2[k] = v2[k] - NET_VTH2 + acc[k];
        count[k]++;
      } else {
        v2[k] += acc[k];
      }

      if (v2[k] > 32767)
        v2[k] = 32767;
      else if (v2[k] < -32768)
        v2[k] = -32768;

      acc[k] = 0;
    }

    MMIO_PRINT = MARK_EVAL;
  }

  int max = 0, idx = 0;
  for (int i = 0; i < 10; i++) {
    MMIO_PRINT_INT = count[i];
    if (count[i] > max) {
      max = count[i];
      idx = i;
    }
  }

  MMIO_PRINT_INT = idx;

  return 0;
}
