#ifndef ENCODE_H
#define ENCODE_H

unsigned int wang32(unsigned int x);
int encode_timestep(int img, int t, volatile unsigned int *bank);

#endif
