/* Having solved the data struct alignment issue, I am now attampting to actually
 * use an analog pin to measure a voltage, and have that info sent over to
 * Raspberry Pi.
 *
 * This is my 'Step-6' milestone.

 * 09-27-2022:  First iteration, using code that has started as a copy of MS-06_CapDataReceive_06.cpp
                This worked just fine. And in fact I used this version for my 1st attempt to
                perform an actual capacitor measurement, and it worked fine for that as well (in
                terms of showing the data that is; I do still have some issues to work out in getting
                the cap measurement to be correct, but those issues are to be worked out on the
                ATTiny side).
 */
#define VERSION "09-27-2022 rel 01"

/*
 * For nRF24 radio chip documentation see https://nRF24.github.io/RF24
 * For the approach on capacitance measurement on the ATTiny84 side --:
 *   RCTiming_capacitance_meter || Paul Badger 2008
 *   @ https://www.arduino.cc/en/Tutorial/Foundations/CapacitanceMeter
 */

#include <ctime>       // time()
#include <iostream>    // cin, cout, endl
#include <iomanip>     // format manipulators for use with cout
#include <string>      // string, getline()
#include <time.h>      // CLOCK_MONOTONIC_RAW, timespec, clock_gettime()
#include <RF24/RF24.h> // RF24, RF24_PA_LOW, delay()

using namespace std;

/* =============================================================================
   Gloabal Variable Declarations
   =============================================================================
*/

    /* Construct nRF24 Radio object. */
RF24 radio(22, 0);

    /*      Using a struct to directly load the received payload may fail
       due to boundary-alignment issues. See details in the Evernote note for
       Milestone #5.
            SO - below declare is a variable to read the received, raw, bytes into.
       These bytes are then 'manually' loaded into the payload struct using
       loadRxStruct().
    */
uint8_t rxBytes[40];

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

    /* Structure to store the outgoing ACK payload
    */
struct AckPayloadStruct {
  char message[11];               // Outgoing message, up to 10 chrs+Null.
  uint8_t counter;
};
AckPayloadStruct ackPayload;

    /* Custom defined timer for evaluating transmission
       time in microseconds.
    */
struct timespec startTimer, endTimer;



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
void setRole();                                                                     // to set the node's role
void slave();                                                                       // RX node's behavior
void displayRxResults(RxPayloadStruct* pStruct, bool bCurReset=true);               // display received transmission info
void displayRxStruct(RxPayloadStruct* pStruct);                                     // outputs received payload to console
void displayRxbuffer(uint8_t* rxBytes, uint8_t size_rxBytes, uint8_t ctRawBytes);   // outputs raw received data to console
void loadRxStruct(RxPayloadStruct* pStruct, uint8_t* pBytes);                       // loads raw data into structure
void showHexOfBytes(unsigned char* b, int iLen);                                    // display hex value of variables
void displayAck(AckPayloadStruct* pStruct);                                         // display ack response data



int main(int argc, char** argv) {
    // perform hardware check
    if (!radio.begin()) {
        cout << "radio hardware is not responding!!" << endl;
        return 0; // quit now
    }

    // Let these addresses be used for the pair
    uint8_t address[2][6] = {"1Node", "2Node"};
    bool radioNumber = 1; // 0 uses address[0] to transmit, 1 uses address[1] to transmit

    // print example's name
    cout << endl<< argv[0] << " [" << VERSION << "]" << endl;

    // Set the radioNumber via the terminal on startup
    cout << "Which radio is this? Enter '0' or '1'. Defaults to '0' ";
    string input;
    getline(cin, input);
    radioNumber = input.length() > 0 && (uint8_t)input[0] == 49;

    // to use ACK payloads, we need to enable dynamic payload lengths
    radio.enableDynamicPayloads();    // ACK payloads are dynamically sized

    // Acknowledgement packets have no payloads by default. We need to enable
    // this feature for all nodes (TX & RX) to use ACK payloads.
    radio.enableAckPayload();

    // Set the PA Level low to try preventing power supply related problems
    // because these examples are likely run with nodes in close proximity to
    // each other.
    radio.setPALevel(RF24_PA_LOW);  // RF24_PA_MAX is default.

    // set the TX address of the RX node into the TX pipe
    radio.openWritingPipe(address[radioNumber]);     // always uses pipe 0

    // set the RX address of the TX node into a RX pipe
    radio.openReadingPipe(1, address[!radioNumber]); // using pipe 1

    // For debugging info
    // radio.printDetails();        // (smaller) function that prints raw register values
    radio.printPrettyDetails();     // (larger) function that prints human readable data

    // ready to execute program now
    setRole(); // calls master() or slave() based on user input
    return 0;

} // Main()



/* =============================================================================
   Local Functions
   =============================================================================
*/

/* Set this node's role from stdin stream.
   This only considers the first char as input.
   Raspberry Pi will be a receive-only node.
   This function then calls slave(), which runs an infinte loop, unless we
   timeout not receiving any data from the tiny84.
*/
void setRole() {
    string input = "";
    while (!input.length()) {
        cout << "*** PRESS 'r' to begin receiving from the other node\n";
        cout << "*** PRESS 'q' to exit" << endl;
        getline(cin, input);
        if (input.length() >= 1) {
            if (input[0] == 'R' || input[0] == 'r')
                slave(); // <-- This is were we listen for, and process, incoming data.
            else if (input[0] == 'Q' || input[0] == 'q')
                break;
            else
                cout << input[0] << " is an invalid input. Please try again." << endl;
        }
        input = ""; // stay in the while loop
    } // while
} // setRole()


/* Performs receiver-role tasks */
void slave() {
    memcpy(ackPayload.message, "Pkt Count ", 10);          // set the ackPayload message
    ackPayload.counter = 0;                                // set the ackPayload counter
    DisplayRxPacket dspRx;                                 // create object to display received packets



        /* Load the ackPayload for first received
           transmission on pipe 0.
        */
    radio.writeAckPayload(1, &ackPayload, sizeof(ackPayload));

    radio.startListening();                                             // put radio in RX mode
    time_t startTimer = time(nullptr);                                  // start a timer
    while (time(nullptr) - startTimer < 12) {                           // use 12 second timeout
        uint8_t pipe;
        if (radio.available(&pipe)) {                                   // is there a received payload? get the pipe number that recieved it
            uint8_t bytes = radio.getDynamicPayloadSize();              // <<-- NOTE: Compilier says we never use this anywhere. Myes, get it's size
            radio.read(&rxBytes[0], sizeof(rxBytes));                   // fetch payload from RX FIFO
            loadRxStruct(&rxPayload, rxBytes);                          // Manually' load rxPayload structure from the received bytes array.
            dspRx.displayRxResults(&rxPayload, true);                                    // display received transmission info
            startTimer = time(nullptr);                                 // Reset the timer
            ackPayload.counter = ackPayload.counter + 1;                // Increment the 'payloads received' counter.
            radio.writeAckPayload(1, &ackPayload, sizeof(ackPayload));  // Load the ACK payload for use on the next received Tx
        } // if received something
    } // while

        /* Handle radio listening timout case. Which, other than a Control-C by
           the user, is the only way the slave() function ends. And upon ending
           we return back to the setRole() function's while loop, allowing the
           user to quite or initiate another try.
        */
    cout << "Timeout While Waiting to Receive Data. Leaving RX role." << endl;
    cout << "You may quite or press r to try again." << endl;
    radio.stopListening();                                              // recommended idle behavior is TX mode
} // slave



/* Inspect the received data received from ATTiny.
   ----------------------------------------------------------------------------
   REQUIRES: radio.read() has already been called.
 */
void displayRxbuffer(uint8_t* rxBytes, uint8_t size_rxBytes, uint8_t ctRawBytes) {

    cout << endl;
    cout << setw(0) << setfill(' ');
    cout << fixed;

        /* Show the sizes of things.
        */
    cout << "Recieved " << (unsigned int)ctRawBytes << " bytes | ";
    cout << "Size of rxBytes[]: " << (unsigned int)size_rxBytes << " || ";
    cout << "Size Of rxPayload struct: " << sizeof(rxPayload) << " | " << endl;

        /* Inspect the received data received from ATTiny.
        */
    cout << setfill('-') << setw(size_rxBytes*3) << "-" << setfill(' ') << endl;
    cout << "rxBytes in HEX / rxBytes[] index offset ---: " << endl;
    //showHexOfBytes((unsigned char *)&rxBytes, size_rxBytes);
    showHexOfBytes((unsigned char *)rxBytes, size_rxBytes);
    cout << endl;
    for (unsigned int k=0; k<size_rxBytes; k++) {
        cout << setw(2) << setfill('0') << k << " ";
    }
    cout << endl;
    cout << setfill('-') << setw(size_rxBytes*3) << "-" << setfill(' ') << endl;

}



/* Display onto the console what we are going to ACK back with.
   ----------------------------------------------------------------------------
   REQUIRES: ackPayload struct has been loaded with intended response data.
 */
void displayAck(AckPayloadStruct* pStruct) {
            cout << " Sent in Response: ";
            cout << pStruct->message;
            cout << (unsigned int)pStruct->counter << endl;
            cout << setfill('-') << setw(50) << "-" << setfill(' ') << endl << endl;
}





/* Manually load the incoming data into a structure.
   ----------------------------------------------------------------------------
    REQUIRES: The RxPayloadStruct defintion on this node exactly match the
              definition created on the transmit node.

    07/19/202: Removed the code that writes the hex output to the console so
    that this function only loads the structure and does not affect the
    console display. If console output needs to be reimplemented, go back
    to MS-06_CapDataReceived_05.cpp.
 */
void loadRxStruct(RxPayloadStruct* pStruct, uint8_t* pBytes) {
    size_t offset = 0;

    pStruct->capacitance = *(float *)&pBytes[offset];
    offset = offset + sizeof(pStruct->capacitance);

    pStruct->chargeTime = *(uint32_t *)&pBytes[offset];
    offset = offset + sizeof(pStruct->chargeTime);

    memcpy((unsigned char *)&pStruct->units, &pBytes[offset], sizeof(pStruct->units));
    offset = offset + sizeof(pStruct->units);

    pStruct->ctSuccess = *(uint32_t *)&pBytes[offset];
    offset = offset + sizeof(pStruct->ctSuccess);

    pStruct->ctErrors = *(uint32_t *)&pBytes[offset];
    offset = offset + sizeof(pStruct->ctErrors);

    memcpy(pStruct->statusText, &pBytes[offset], sizeof(pStruct->statusText));
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
void showHexOfBytes(unsigned char* b, int iLen) {
    for (int k=0; k<iLen; k++) {
        cout << setfill('0') << setw(2) << hex << (unsigned int)b[k] << " ";
    }
    cout << dec;
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
    static bool bFirstTime = true;
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


