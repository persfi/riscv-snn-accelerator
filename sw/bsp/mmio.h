#ifndef MMIO_H
#define MMIO_H


#define PRINT_ADDR 0x10000000u
#define EXIT_ADDR  0x10000004u

#define MMIO_PRINT (*(volatile unsigned char *)(PRINT_ADDR))
#define MMIO_EXIT  (*(volatile unsigned int *)(EXIT_ADDR))

#endif
