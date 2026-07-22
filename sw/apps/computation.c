#include <mmio.h>
#include <stdbool.h>

//dot product
int main(){

    int a[5] = {1,3,5,7,2};
    int b[5] =  {3,2,6,23,7};
    int res = 214;
    int sum=0;
    bool flag=0;

    for(int i=0;i<5;i++){
        sum+= a[i] *b[i];
    }

    if(sum!=res) flag=1;

    return flag; // EXIT code = 0 -> computation correct

}
