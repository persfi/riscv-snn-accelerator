#ifndef MMIO_H
#define MMIO_H


#define PRINT_ADDR 0x10000000u
#define EXIT_ADDR  0x10000004u

#define MMIO_PRINT (*(volatile unsigned char *)(PRINT_ADDR))
#define MMIO_EXIT  (*(volatile unsigned int *)(EXIT_ADDR))


#define ACCEL_BASE          0x20000000u
#define ACCEL_T_ADDR        (ACCEL_BASE + 0x10u)   
#define ACCEL_VTH1_ADDR     (ACCEL_BASE + 0x14u)  
#define ACCEL_VTH2_ADDR     (ACCEL_BASE + 0x18u)   
#define ACCEL_K_ADDR        (ACCEL_BASE + 0x1Cu)        
#define ACCEL_START_ADDR    (ACCEL_BASE + 0x20u)  
#define ACCEL_QWDATA_ADDR   (ACCEL_BASE + 0x24u)  
#define ACCEL_QCOMMIT_ADDR  (ACCEL_BASE + 0x28u)   
#define ACCEL_STATUS_ADDR   (ACCEL_BASE + 0x2Cu)  

#define ACCEL_T             (*(volatile unsigned int *)(ACCEL_T_ADDR))
#define ACCEL_VTH1          (*(volatile unsigned int *)(ACCEL_VTH1_ADDR))
#define ACCEL_VTH2          (*(volatile unsigned int *)(ACCEL_VTH2_ADDR))
#define ACCEL_K             (*(volatile unsigned int *)(ACCEL_K_ADDR))
#define ACCEL_START         (*(volatile unsigned int *)(ACCEL_START_ADDR))
#define ACCEL_QWDATA        (*(volatile unsigned int *)(ACCEL_QWDATA_ADDR))
#define ACCEL_QCOMMIT       (*(volatile unsigned int *)(ACCEL_QCOMMIT_ADDR))
#define ACCEL_STATUS        (*(volatile unsigned int *)(ACCEL_STATUS_ADDR))

/* STATUS bits */
#define ACCEL_BUFFER_FREE   0x1u 
//is the ev_idx bank the host is preparing to write to free
#define ACCEL_IMAGE_DONE    0x2u 
//all timesteps done for that image


#endif
