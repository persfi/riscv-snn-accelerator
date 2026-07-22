#include <mmio.h>

int main(){

    const char a[6] = "hello";
    const char *p = " world";
    int i=0;

    while(i<6){
        MMIO_PRINT = a[i];
        i++;
    }
    
    while(*p){
        MMIO_PRINT = *p;
        p++;
    }

    return 0;
}
