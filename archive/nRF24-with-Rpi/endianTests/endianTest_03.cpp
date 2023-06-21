/* This is an investigative variabnt of the Raspberry Pi, receive, side of my
 * 'step-5' milestone: * Transmit / Receive capacitance measurement. In this
 * code I am trying to work out what is going on with the receipt and rendering
 * of the non-string and non-8-bit intteger data values. Floats and unsigned long
 * are not coming across correctly. One theory is this is due to differing
 * endian encoding on the tiny84 vs the RPi. So I am herein trying to work this
 * out.
 *
 * 06-19-2022: First iteration, using code that has started as a copy of endianTest_02.cpp
 */
#define VERSION "06-20-2022 rel 01"

/*
 * For nRF24 radio chip documentation see https://nRF24.github.io/RF24
 * For the approach on capacitance measurement on the ATTiny84 side --:
 *   RCTiming_capacitance_meter || Paul Badger 2008
 *   @ https://www.arduino.cc/en/Tutorial/Foundations/CapacitanceMeter

 * This source based on "manualAcknowledgements.cpp" in the RPi nRF24 library
 * noted above.
 */

//#include <ctime>       // time()
#include <iostream>    // cin, cout, endl
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



void getHex(float f) {
    unsigned char* pFloat = (unsigned char*) & f;

    for (int k=0; k<sizeof(f); k++) {
        printf("%02X ", pFloat[k]);
    }

}

void getHexOfString(char* s, int iLen) {
    for (int k=0; k<iLen; k++) {
        printf("%02X ", s[k]);
    }
}


struct RxPayloadStruct {
  char statusText[9];            // For use in debugging.
  float testFloat1 = 12.5;
  uint32_t chargeTime = 256;
  float testFloat2 = 0.1;
  char units[4];                  // nFD, mFD, FD
  float capacitance = 0;
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

    printf("\n");
    printf("==== Inspect Payload Structure:\n");
    printf("\n");
    printf("Total Size Of rxPayload in Memory: %zu\n", sizeof(rxPayload));
    printf("statusText %i | %s | 0x ", sizeof(rxPayload.statusText), rxPayload.statusText);
    getHexOfString(rxPayload.statusText, sizeof(rxPayload.statusText));
    printf("\n");
    //printf("statusText %i | %s | %X \n", sizeof(rxPayload.statusText), rxPayload.statusText, rxPayload.statusText);
    printf("testFloat1 %i | %f | %A | 0x ", sizeof(rxPayload.testFloat1), rxPayload.testFloat1, rxPayload.testFloat1);
    getHex(rxPayload.testFloat1);
    printf("\n");
    //printf("testFloat1 %i | %f | %A | %8X | \n", sizeof(rxPayload.testFloat1), rxPayload.testFloat1, rxPayload.testFloat1, (uint32_t)rxPayload.testFloat1);
    printf("chargeTime %i | %u | %X \n", sizeof(rxPayload.chargeTime), rxPayload.chargeTime, rxPayload.chargeTime);
    printf("testFloat2 %i | %f | %A | 0x ", sizeof(rxPayload.testFloat2), rxPayload.testFloat2, rxPayload.testFloat2);
    getHex(rxPayload.testFloat2);
    printf("\n");
    //printf("testFloat2 %i | %f | %A\n", sizeof(rxPayload.testFloat2), rxPayload.testFloat2, rxPayload.testFloat2);
    printf("units %i | %s | 0x ", sizeof(rxPayload.units), rxPayload.units);
    getHexOfString(rxPayload.units, sizeof(rxPayload.units));
    printf("\n");
    //printf("units %i | %s | | %X \n", sizeof(rxPayload.units), rxPayload.units, rxPayload.units);

    printf("capacitance %i | %f | %A | 0x ", sizeof(rxPayload.capacitance), rxPayload.capacitance, rxPayload.capacitance);
    getHex(rxPayload.capacitance);
    printf("\n");
    //printf("capacitance %i | %f | %A \n", sizeof(rxPayload.capacitance), rxPayload.capacitance, rxPayload.capacitance);
    printf("\n");
    printf("\n");
    printf("The 'Union' Way -- \n");
    x.f = 0.1;
    printf("x.f: %zu | %f | %A | HEX: ", sizeof(x.f), x.f, x.f);
    for (int i=0; i<sizeof(float); i++) {
        printf("%02hhX ", x.c[i]);
        //printf("i:%i x.c[i]: %X ", i, x.c[i]);
        //printf("  || 0x%02hhX \n", x.c[i]);
    }
    printf("\n");
    printf("\n");
    printf("Full Structure as Sting of HEX:\n");
    printf("Total Size Of rxPayload: %zu\n", sizeof(rxPayload));
    printf("Payload bytes as HEX --:\n  ");
    for (int k=0; k<payloadSize; k++) {
        printf("%02X", pPayloadAsChar[k]);
    }
    printf("\n");
    printf("\n");


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


