#ifndef MMIO_H
#define MMIO_H


#define PRINT_ADDR 0x10000000u
#define EXIT_ADDR  0x10000004u

#define MMIO_PRINT (*(volatile unsigned char *)(PRINT_ADDR))
#define MMIO_EXIT  (*(volatile unsigned int *)(EXIT_ADDR))

#define ACCEL_BASE          0x20000000u
#define ACCEL_EVA_BASE      0x20001000u
#define ACCEL_EVB_BASE      0x20002000u
#define ACCEL_W1_BASE       0x20020000u
#define ACCEL_W2_BASE       0x20040000u

#define ACCEL_T_ADDR        (ACCEL_BASE + 0x10u)  
//host writes T to accel
#define ACCEL_VTH1_ADDR     (ACCEL_BASE + 0x14u)
#define ACCEL_VTH2_ADDR     (ACCEL_BASE + 0x18u)
#define ACCEL_K_ADDR        (ACCEL_BASE + 0x1Cu)
#define ACCEL_START_ADDR    (ACCEL_BASE + 0x20u)
#define ACCEL_EVA_LEN_ADDR  (ACCEL_BASE + 0x24u)
#define ACCEL_EVB_LEN_ADDR  (ACCEL_BASE + 0x28u)
#define ACCEL_STATUS_ADDR   (ACCEL_BASE + 0x2Cu)
#define ACCEL_L1_SHIFT_ADDR (ACCEL_BASE + 0x30u) 
#define ACCEL_COUNT_ADDR    (ACCEL_BASE + 0x40u) // base of the ten output spike counters

#define ACCEL_T             (*(volatile unsigned int *)(ACCEL_T_ADDR))
#define ACCEL_VTH1          (*(volatile unsigned int *)(ACCEL_VTH1_ADDR))
#define ACCEL_VTH2          (*(volatile unsigned int *)(ACCEL_VTH2_ADDR))
#define ACCEL_K             (*(volatile unsigned int *)(ACCEL_K_ADDR))
#define ACCEL_START         (*(volatile unsigned int *)(ACCEL_START_ADDR))
#define ACCEL_STATUS        (*(volatile unsigned int *)(ACCEL_STATUS_ADDR))
/* reserved:  hidden size is fixed at 128 */
#define ACCEL_L1_SHIFT      (*(volatile unsigned int *)(ACCEL_L1_SHIFT_ADDR))

#define ACCEL_EVA_LEN       (*(volatile unsigned int *)(ACCEL_EVA_LEN_ADDR))
#define ACCEL_EVB_LEN       (*(volatile unsigned int *)(ACCEL_EVB_LEN_ADDR))

#define ACCEL_EVA(i)        (*(volatile unsigned int *)(ACCEL_EVA_BASE + 4u*(i)))
#define ACCEL_EVB(i)        (*(volatile unsigned int *)(ACCEL_EVB_BASE + 4u*(i)))
#define ACCEL_W1(i)         (*(volatile unsigned int *)(ACCEL_W1_BASE  + 4u*(i)))
#define ACCEL_W2(i)         (*(volatile unsigned int *)(ACCEL_W2_BASE  + 4u*(i)))
/* four per word. */
#define ACCEL_COUNT_W(w)    (*(volatile unsigned int *)(ACCEL_COUNT_ADDR + 4u*(w)))
#define ACCEL_COUNT(i)      ((ACCEL_COUNT_W((i) >> 2) >> (8u*((i) & 3u))) & 0xFFu)

/* STATUS bits, read-only */
#define ACCEL_BANK_A_FREE   (1u << 0)
#define ACCEL_BANK_B_FREE   (1u << 1)
#define ACCEL_IMAGE_DONE    (1u << 2)
//all timesteps done for that image

#endif
