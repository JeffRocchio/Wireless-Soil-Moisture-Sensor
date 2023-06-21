#include <ctime>       // time()
#include <iostream>    // cin, cout, endl
#include <iomanip>     // format manipulators for use with cout
#include <string>      // string, getline()
#include <time.h>      // CLOCK_MONOTONIC_RAW, timespec, clock_gettime()
#include <cstring>

using namespace std;

/* =============================================================================
   Gloabal Variable Declarations
   =============================================================================
*/
    /* Struct to hold the data received in
       from the ATTiny's nRF24
    */
struct RxPayloadStruct {
  float capacitance;
  uint32_t chargeTime;            // Time it took for capacitor to charge.
  char units[4];                  // nFD, mFD, FD
  uint32_t ctSuccess;             // count of success Tx attempts tiny84 has seen since boot
  uint32_t ctErrors;              // count of Tx errors tiny84 saw since last successful transmit
  char statusText[12];            // For use in debugging.
};
RxPayloadStruct rxPayload;


/* =============================================================================
   Class definitions
   =============================================================================
*/
class DisplayRxPacket {
    public:
        DisplayRxPacket();
        void displayRxResults(RxPayloadStruct* pStruct, bool bCurReset=true);

    private:
        bool bFirstTime;
        void showHexOfBytes(unsigned char* b, int iLen);                                    // display hex value of variables

};

/* =============================================================================
   Function prototypes
   =============================================================================
*/



int main(int argc, char** argv)
{
    DisplayRxPacket dspRx;

    rxPayload.capacitance = 0;
    rxPayload.chargeTime = 0;
    memcpy(rxPayload.units, "Mfd", 3);
    rxPayload.units[4] = 0;
    rxPayload.ctSuccess = 0;
    rxPayload.ctErrors = 0;
    memcpy(rxPayload.statusText, "Status Text", 11);


    cout << "argv[0] = " << argv[0] << endl;

    cout << endl;
    cout << "Trying to test the static keywork on a bool in a function" << endl;
    cout << "bFirstTime should show as true the first time, then false thereafter" << endl;
    cout << endl;

    for(int i=1; i<4; i++) {
        cout << "We are in the loop, i=" << i << endl;
        dspRx.displayRxResults(&rxPayload, true);
    }

    //cout << "\033[7A";
    //cout << "\033[" << iLinesConsumed << "A";
    cout << "=== END ====================" << endl;

    return 0;
}



/* CLASS DisplayRxPacket
   ============================================================================
   Function definitions for class
   */

/* Class Constructor
   ----------------------------------------------------------------------------
   */
DisplayRxPacket::DisplayRxPacket() {
    bFirstTime = true;
}

/* Display Results of a received Transmission
   ----------------------------------------------------------------------------
   REQUIRES:    loadRxStruct() has already been called.
   bCurReset:   If TRUE then this proc will move cursor back up the screen so
                that it overwrites a previous call to this function so that
                the display does not scroll. If FALSE it will outputting the
                display below whever the current console cursor is
                (i.e., scroll mode). Defaults to TRUE.
  */
void DisplayRxPacket::displayRxResults(RxPayloadStruct* pStruct, bool bCurReset) {
    unsigned int iLinesConsumed = 7;
    unsigned int wdthVarName = 14;
    unsigned int wdthValue = 14;

    /* Avoid overwriting the prior 7 lines when we perform
     this fuction the first time through. 1st time through it should write out
     to new lines. Then, if bCurRest is passed in as TRUE it should
     subsequently go back up the screen the right number of lines.
     */
    if(bCurReset && !bFirstTime) {
        cout << "\033[" << iLinesConsumed << "A";
    }
    bFirstTime = false;

    cout << "============= Incoming Transmissions ==============" << endl;

    cout << setw(wdthVarName) << setfill(' ') << " capacitance: ";
    cout << setw(2) << (unsigned int)sizeof(rxPayload.capacitance);
    cout << " | " << setw(wdthValue) << rxPayload.capacitance << " | 0x ";
    showHexOfBytes((unsigned char*)&rxPayload.capacitance,sizeof(rxPayload.capacitance));
    cout << endl;

    cout << setw(wdthVarName) << setfill(' ') << " chargeTime: ";
    cout << setw(2) << (unsigned int)sizeof(rxPayload.chargeTime);
    cout << " | " << setw(wdthValue) << rxPayload.chargeTime << " | 0x ";
    showHexOfBytes((unsigned char*)&rxPayload.chargeTime,sizeof(rxPayload.chargeTime));
    cout << endl;

    cout << setw(wdthVarName) << setfill(' ') << " units: ";
    cout << setw(2) << (unsigned int)sizeof(rxPayload.units);
    cout << " | " << setw(wdthValue) << rxPayload.units << " | 0x ";
    showHexOfBytes((unsigned char*)&rxPayload.units,sizeof(rxPayload.units));
    cout << endl;

    cout << setw(wdthVarName) << setfill(' ') << " ctSuccess: ";
    cout << setw(2) << (unsigned int)sizeof(rxPayload.ctSuccess);
    cout << " | " << setw(wdthValue) << rxPayload.ctSuccess << " | 0x ";
    showHexOfBytes((unsigned char*)&rxPayload.ctSuccess,sizeof(rxPayload.ctSuccess));
    cout << endl;

    cout << setw(wdthVarName) << setfill(' ') << " ctErrors: ";
    cout << setw(2) << (unsigned int)sizeof(rxPayload.ctErrors);
    cout << " | " << setw(wdthValue) << rxPayload.ctErrors << " | 0x ";
    showHexOfBytes((unsigned char*)&rxPayload.ctErrors,sizeof(rxPayload.ctErrors));
    cout << endl;

    cout << setw(wdthVarName) << setfill(' ') << " statusText: ";
    cout << setw(2) << (unsigned int)sizeof(rxPayload.statusText);
    cout << " | " << setw(wdthValue) << rxPayload.statusText << " | 0x ";
    showHexOfBytes((unsigned char*)&rxPayload.statusText,sizeof(rxPayload.statusText));
    cout << endl;
}


/* Display the HEX value of the bytes that store a variable.
   ----------------------------------------------------------------------------
   PARMS:      1. The first byte of the variable to show the HEX for is passed in
            as a pointer to an unsigned char.
               2. The 2nd param is the length of the variable, i.e., the length
            of it's data type (or length of the string array if a string).
            E.g., use the sizeof() function on the variable to obtain this
            param.
   */
void DisplayRxPacket::showHexOfBytes(unsigned char* b, int iLen) {
    for (int k=0; k<iLen; k++) {
        cout << setfill('0') << setw(2) << hex << (unsigned int)b[k] << " ";
    }
    cout << dec;
}


