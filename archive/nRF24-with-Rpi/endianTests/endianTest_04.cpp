/* This is an investigative variant of the Raspberry Pi, receive, side of my
 * 'step-5' milestone: * Transmit / Receive capacitance measurement. In this
 * code I am trying to work out what is going on with the receipt and rendering
 * of the non-string and non-8-bit intteger data values. Floats and unsigned long
 * are not coming across correctly. One theory is this is due to differing
 * endian encoding on the tiny84 vs the RPi. So I am herein trying to work this
 * out.
 *
 * 06-19-2022:  Attempting to understand how to use cout instead of printf
 *              for displaying the hex-formatted content of variables.
 */
#define VERSION "06-24-2022 rel 02"


//#include <ctime>       // time()
#include <iostream>    // cin, cout, endl
#include <iomanip>     // format manipulators for use with cout
#include <string>      // string, getline()
//#include <time.h>      // CLOCK_MONOTONIC_RAW, timespec, clock_gettime()
//#include <RF24/RF24.h> // RF24, RF24_PA_LOW, delay()
#include <cstring>
#include <stdio.h>



/* Macro uised to reverse endian byte-order. See @ https://stackoverflow.com/a/41196306 */
#define REVERSE_BYTES(...) do for(size_t REVERSE_BYTES=0; REVERSE_BYTES<sizeof(__VA_ARGS__)>>1; ++REVERSE_BYTES)\
    ((unsigned char*)&(__VA_ARGS__))[REVERSE_BYTES] ^= ((unsigned char*)&(__VA_ARGS__))[sizeof(__VA_ARGS__)-1-REVERSE_BYTES],\
    ((unsigned char*)&(__VA_ARGS__))[sizeof(__VA_ARGS__)-1-REVERSE_BYTES] ^= ((unsigned char*)&(__VA_ARGS__))[REVERSE_BYTES],\
    ((unsigned char*)&(__VA_ARGS__))[REVERSE_BYTES] ^= ((unsigned char*)&(__VA_ARGS__))[sizeof(__VA_ARGS__)-1-REVERSE_BYTES];\
while(0)


using namespace std;


void getHexOfFloat(float f) {
    unsigned char* pFloat = (unsigned char*) & f;
    for (size_t k=0; k<sizeof(f); k++) {
        cout << setfill('0') << setw(2) << hex << (unsigned int)pFloat[k] << " ";
    }
        cout << dec;
}

void getHexOfString(char* s, int iLen) {
    for (int k=0; k<iLen; k++) {
        cout << setfill('0') << setw(2) << hex << (unsigned int)s[k] << " ";
    }
    cout << dec;
}

void showHexOfBytes(unsigned char* b, int iLen) {
    for (int k=0; k<iLen; k++) {
        cout << setfill('0') << setw(2) << hex << (unsigned int)b[k] << " ";
    }
    cout << dec;
}


struct RxPayloadStruct {
  char statusText[9];           // For use in debugging.
  float testFloat1 = 12.5;      // Intel (little-e): 00004841 | big-e: 41480000
  uint32_t chargeTime = 256;    // little-e: 00010000 | big-e: 00000100
  float testFloat2 = 0.1;       // little-e: cdcccc3d | big-e: 3DCCCCCD
  char units[4];                // nFD, mFD, FD
  float capacitance = 100;      // little-e: 0000C842 | big-e:42C80000
};
RxPayloadStruct rxPayload;

int payloadSize = sizeof(rxPayload);
unsigned char* pPayloadAsChar = (unsigned char*) & rxPayload;
//unsigned char* pPayloadAsChar = (unsigned char*) & rxPayload;

/* Approach to displaying the 4 bytes of a float as a string of HEX.
   Found @ https://stackoverflow.com/a/8121951 */
union FloatToChar {
    float f;
    char  c[sizeof(float)];
};
FloatToChar x;



int main(int argc, char** argv) {
    memcpy(rxPayload.statusText, "Initial", 7);
    memcpy(rxPayload.units, "---", 3);

    unsigned int wdthVarName = 14;
    unsigned int wdthValue = 5;

    cout << endl;
    cout << setw(0) << setfill(' ');
    cout << "Total Size Of rxPayload: " << sizeof(rxPayload) << " || ";
    cout << "Payload bytes as HEX --: ";
    showHexOfBytes(pPayloadAsChar,sizeof(rxPayload));
    cout << endl;
    cout << setw(60) << setfill('-') << "-" << endl;

    cout << setw(wdthVarName) << setfill(' ') << " statusText: ";
    cout << setw(2) << (unsigned int)sizeof(rxPayload.statusText);
    cout << " | " << rxPayload.statusText << " | 0x ";
    getHexOfString(rxPayload.statusText, sizeof(rxPayload.statusText));
    cout << endl;

    cout << setw(wdthVarName) << setfill(' ') << " testFloat1: ";
    cout << setw(2) << (unsigned int)sizeof(rxPayload.testFloat1);
    cout << " | " << setw(wdthValue) << rxPayload.testFloat1 << " | 0x ";
    getHexOfFloat(rxPayload.testFloat1);
    cout << endl;

    cout << setw(wdthVarName) << setfill(' ') << " chargeTime: ";
    cout << setw(2) << (unsigned int)sizeof(rxPayload.chargeTime);
    cout << " | " << setw(wdthValue) << rxPayload.chargeTime << " | 0x ";
    showHexOfBytes((unsigned char*)&rxPayload.chargeTime,sizeof(rxPayload.chargeTime));
    //getHexOfLong(rxPayload.testFloat1);
    cout << endl;

    cout << setw(wdthVarName) << setfill(' ') << " testFloat2: ";
    cout << setw(2) << (unsigned int)sizeof(rxPayload.testFloat2);
    cout << " | " << setw(wdthValue) << rxPayload.testFloat2 << " | 0x ";
    getHexOfFloat(rxPayload.testFloat2);
    cout << endl;

    cout << setw(wdthVarName) << setfill(' ') << " units: ";
    cout << setw(2) << (unsigned int)sizeof(rxPayload.units);
    cout << " | " << setw(wdthValue) << rxPayload.units << " | 0x ";
    getHexOfString(rxPayload.units, sizeof(rxPayload.units));
    cout << endl;

    cout << setw(wdthVarName) << setfill(' ') << " capacitance: ";
    cout << setw(2) << (unsigned int)sizeof(rxPayload.capacitance);
    cout << " | " << setw(wdthValue) << rxPayload.capacitance << " | 0x ";
    getHexOfFloat(rxPayload.capacitance);
    cout << endl << endl;



    /*    printf("/n", );
    printf("/n", );
    printf("/n", );
    printf("/n", );
    printf("/n", );
*/


    //REVERSE_BYTES(rxPayload.testFloat1);
    //cout << " Float-1: ";
    //cout << (float)rxPayload.testFloat1;

     return 0;
}


