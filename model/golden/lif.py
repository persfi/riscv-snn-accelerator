
import numpy as np

INT16_MIN, INT16_MAX = -32768, 32767
def lif_update(v_prev, weight_sum, v_th, k):

    v_new = v_prev - (v_prev >> k)
    v_new = v_new + weight_sum
    spike = (v_new > v_th).astype(np.int32)
    v_new -= spike * v_th
    v_new = np.clip(v_new, INT16_MIN, INT16_MAX).astype(np.int16)

    return v_new, spike
        
