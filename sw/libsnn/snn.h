#ifndef SNN_H
#define SNN_H

void snn_config(int t, int k, int vth1, int vth2, int hidden);
int snn_run_image(int img, int t_max);

#endif
