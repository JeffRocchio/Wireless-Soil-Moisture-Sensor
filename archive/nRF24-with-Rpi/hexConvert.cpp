#include <cstdio>
#include <algorithm>    // std::min
#include <iostream>


int main()
{

    char formatStr[] = "Result: %s";
    char crCts[] = "-";
    int dumVal = 0;

    char buffer[50];
    int a = 10, b = 20, c;
    c = a + b;
    sprintf(buffer, "Sum of %d and %d is %d", a, b, c);

    // The string "sum of 10 and 20 is 30" is stored
    // into buffer instead of printing on stdout
    printf("%s", buffer);
    printf("\n");

    if (dumVal > 0) {
        sprintf(crCts, "%X", std::min(0xf, (dumVal & 0xf)));
        printf("crCts=%s", crCts);
        printf("\n");
    }
    else {
        crCts[0] = '-';
        printf("crCts=%s", crCts);
        printf("\n");
    }
    printf(formatStr, crCts); // ORIGINAL: printf(formatStr, min(0xf, (values[i] & 0xf)));
    printf("\n");


    if (dumVal > 0) {
        sprintf(crCts, "%X", std::min(0xf, (dumVal & 0xf)));
    }
    else crCts[0] = '-';
    printf(formatStr, crCts); // ORIGINAL: printf(formatStr, min(0xf, (values[i] & 0xf)));
    printf("\n");

    return 0;
}

