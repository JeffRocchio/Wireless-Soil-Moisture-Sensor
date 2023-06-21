#include <stdio.h>

#define REVERSE_BYTES(...) do for(size_t REVERSE_BYTES=0; REVERSE_BYTES<sizeof(__VA_ARGS__)>>1; ++REVERSE_BYTES)\
    ((unsigned char*)&(__VA_ARGS__))[REVERSE_BYTES] ^= ((unsigned char*)&(__VA_ARGS__))[sizeof(__VA_ARGS__)-1-REVERSE_BYTES],\
    ((unsigned char*)&(__VA_ARGS__))[sizeof(__VA_ARGS__)-1-REVERSE_BYTES] ^= ((unsigned char*)&(__VA_ARGS__))[REVERSE_BYTES],\
    ((unsigned char*)&(__VA_ARGS__))[REVERSE_BYTES] ^= ((unsigned char*)&(__VA_ARGS__))[sizeof(__VA_ARGS__)-1-REVERSE_BYTES];\
while(0)



int main(){
    unsigned long long x = 0xABCDEF0123456789;
    printf("Before: %llX\n",x);
    REVERSE_BYTES(x);
    printf("After : %llX\n",x);

    char c[8]="nametag";
    printf("Before: %c%c%c%c%c%c%c\n",c[0],c[1],c[2],c[3],c[4],c[5],c[6]);
    REVERSE_BYTES(c);
    printf("After : %c%c%c%c%c%c%c\n",c[0],c[1],c[2],c[3],c[4],c[5],c[6]);

    float f = 1.2;
    printf("Before: %f\n",f);
    REVERSE_BYTES(f);
    printf("After : %f\n",f);

}
