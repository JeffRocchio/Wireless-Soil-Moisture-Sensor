/* This is an investigative variabnt of the Raspberry Pi, receive, side of my
 * 'step-5' milestone: * Transmit / Receive capacitance measurement. In this
 * code I am trying to work out what is going on with the receipt and rendering
 * of the non-string and non-8-bit intteger data values. Floats and unsigned long
 * are not coming across correctly. One theory is this is due to differing
 * endian encoding on the tiny84 vs the RPi. So I am herein trying to work this
 * out.
 *
 * 06-19-2022: First iteration, using code that has started as a copy of CapDataReceive_02.cpp
 */
#define VERSION "06-19-2022 rel 01"

/*
 * For nRF24 radio chip documentation see https://nRF24.github.io/RF24
 * For the approach on capacitance measurement on the ATTiny84 side --:
 *   RCTiming_capacitance_meter || Paul Badger 2008
 *   @ https://www.arduino.cc/en/Tutorial/Foundations/CapacitanceMeter

 * This source based on "manualAcknowledgements.cpp" in the RPi nRF24 library
 * noted above.
 */

//#include <ctime>       // time()
//#include <iostream>    // cin, cout, endl
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

struct RxPayloadStruct {
  char statusText[11];            // For use in debugging.
  float testFloat1 = 13.2;
  uint32_t chargeTime = 256;
  float testFloat2 = 0.1;
  char units[4];                  // nFD, mFD, FD
  float capacitance = 0;
};
RxPayloadStruct rxPayload;

/* Approach to displaying the 4 bytes of a float as a string of HEX.
   Found @ https://stackoverflow.com/a/8121951 */
union FloatToChar {
    float f;
    char  c[sizeof(float)];
};
FloatToChar x;



int main(int argc, char** argv) {
    memcpy(rxPayload.statusText, "Initial ", 8);
    memcpy(rxPayload.units, "--- ", 4);

    printf("\n");
    printf("Total Size Of rxPayload: %i\n", sizeof(rxPayload));
    printf("statusText %i | %s | %X \n", sizeof(rxPayload.statusText), rxPayload.statusText, rxPayload.statusText);
    printf("testFloat1 %i | %f | %A | %8X | \n", sizeof(rxPayload.testFloat1), rxPayload.testFloat1, rxPayload.testFloat1, (uint32_t)rxPayload.testFloat1);
    printf("chargeTime %i | %u | %X \n", sizeof(rxPayload.chargeTime), rxPayload.chargeTime, rxPayload.chargeTime);
    printf("testFloat2 %i | %f | %A\n", sizeof(rxPayload.testFloat2), rxPayload.testFloat2, rxPayload.testFloat2);
    printf("units %i | %s | | %X \n", sizeof(rxPayload.units), rxPayload.units, rxPayload.units);
    printf("capacitance %i | %f | %A \n", sizeof(rxPayload.capacitance), rxPayload.capacitance, rxPayload.capacitance);
    printf("\n");

    printf("\n");
    printf("Now the example I found online -- \n");
    x.f = 1.3;
    printf("x.f: %i | %f | %A | HEX: ", sizeof(x.f), x.f, x.f);
    for (int i=0; i<sizeof(float); i++) {
        printf("%02hhX ", x.c[i]);
        //printf("i:%i x.c[i]: %X ", i, x.c[i]);
        //printf("  || 0x%02hhX \n", x.c[i]);
    }
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


