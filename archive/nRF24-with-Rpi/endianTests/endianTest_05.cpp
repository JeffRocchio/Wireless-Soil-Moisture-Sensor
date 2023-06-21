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
 *
 * 06-26-2022: Manually load struct from raw bytes received

 */
#define VERSION "06-27-2022 rel 06"


//#include <ctime>       // time()
#include <iostream>    // cin, cout, endl
#include <iomanip>     // format manipulators for use with cout
#include <string>      // string, getline()
//#include <time.h>      // CLOCK_MONOTONIC_RAW, timespec, clock_gettime()
//#include <RF24/RF24.h> // RF24, RF24_PA_LOW, delay()
#include <cstring>
#include <stdio.h>

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

uint8_t rxBytes[40];

struct RxPayloadStruct {
  char statusText[11];          // For use in debugging.
  float testFloat1 = 12.5;      // Intel (little-e): 00004841 | big-e: 41480000
  uint32_t chargeTime = 256;    // little-e: 00010000 | big-e: 00000100
  float testFloat2 = 0.1;       // little-e: cdcccc3d | big-e: 3DCCCCCD
  char units[4];                // nFD, mFD, FD
  float capacitance = 100;      // little-e: 0000C842 | big-e:42C80000
};
RxPayloadStruct rxPayload;

int payloadSize = sizeof(rxPayload);
unsigned char* pPayloadAsChar = (unsigned char*) & rxPayload;

void loadRxStruct(RxPayloadStruct* pStruct, uint8_t* pBytes) {
    size_t offset = 0;
    unsigned int wdthCol1, wdthCol2, wdthCol3, wdthCol4;

    wdthCol1 = 23;
    wdthCol2 = 3;
    wdthCol3 = 25;
    wdthCol4 = 4;

    cout << setfill(' ');

    cout << setw(wdthCol1) << "offset to statusText:" << setw(wdthCol2) << offset << endl;
    memcpy(pStruct->statusText, &pBytes[offset], sizeof(pStruct->statusText));
    offset = offset + sizeof(pStruct->statusText);

    cout << setw(wdthCol1) << "offset to testFloat1:" << setw(wdthCol2) << offset;
    pStruct->testFloat1 = *(float *)&pBytes[offset];
    cout << setw(wdthCol3) << left << " | Value of testFloat1= " << setw(wdthCol4) << right << pStruct->testFloat1 << endl;
    offset = offset + sizeof(pStruct->testFloat1);

    cout << setw(wdthCol1) << "offset to chargeTime:" << setw(wdthCol2) << offset;
    pStruct->chargeTime = *(uint32_t *)&pBytes[offset];
    cout << setw(wdthCol3) << left << " | Value of chargeTime= " << setw(wdthCol4) << right << pStruct->chargeTime << endl;
    offset = offset + sizeof(pStruct->chargeTime);

    cout << setw(wdthCol1) << "offset to testFloat2:" << setw(wdthCol2) << offset;
    pStruct->testFloat2 = *(float *)&pBytes[offset];
    cout << setw(wdthCol3) << left << " | Value of testFloat2= " << setw(wdthCol4) << right << pStruct->testFloat2 << endl;
    offset = offset + sizeof(pStruct->testFloat2);

    cout << setw(wdthCol1) << "offset to units:" << setw(wdthCol2) << offset << endl;
    memcpy((unsigned char *)&pStruct->units, &pBytes[offset], sizeof(pStruct->units));
    offset = offset + sizeof(pStruct->units);

    cout << setw(wdthCol1) << "offset to capacitance:" << setw(wdthCol2) << offset;
    cout << setw(wdthCol3) << left << " | Value of capacitance= " << setw(wdthCol4) << right << pStruct->capacitance << endl;
    pStruct->capacitance = *(float *)&pBytes[offset];

    cout << endl;
}



int main(int argc, char** argv) {
    memcpy(rxPayload.statusText, "Initial", 7);
    memcpy(rxPayload.units, "---", 3);

    memcpy(rxBytes, "\x53\x75\x63\x63\x65\x73\x73\x20\x20\x20\x00\x00\x00\x48\x41\x00\x01\x00\x00\xc9\xcc\x4c\x40\x2d\x2d\x2d\x00\x00\x00\xc8\x42\x42\x00\x00\x00\x00\x00\x00\x00\x00", 40);

    unsigned int wdthVarName = 14;
    unsigned int wdthValue = 12;

    cout << endl;
    cout << setw(0) << setfill(' ');
    cout << "Total Size Of rxPayload struct: " << sizeof(rxPayload) << " || ";
    cout << "Total Bytes Received from nRF24 Transmission: " << sizeof(rxBytes) << endl;
    cout << setfill('-') << setw(sizeof(rxBytes)*3) << "-" << setfill(' ') << endl;
    cout << "Payload bytes in HEX / array index offset ---: " << endl;
    showHexOfBytes((unsigned char *)&rxBytes,sizeof(rxBytes));
    cout << endl;
    for (int k=0; k<sizeof(rxBytes); k++) {
        cout << setw(2) << setfill('0') << k << " ";
    }
    cout << endl;
    cout << setfill('-') << setw(sizeof(rxBytes)*3) << "-" << setfill(' ') << endl;


    // load rxPayload structure
    loadRxStruct(&rxPayload, rxBytes);

    cout << setw(wdthVarName) << setfill(' ') << " statusText: ";
    cout << setw(2) << (unsigned int)sizeof(rxPayload.statusText);
    cout << " | " << setw(wdthValue) << rxPayload.statusText << " | 0x ";
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


