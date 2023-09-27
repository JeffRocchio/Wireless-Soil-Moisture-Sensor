/*  ATTiny84 Moisture Sensor Project - Raspberry Pi Server-Side
 *  -----------------------------------------------------------------------------------------------------
 *
 *  Raspberry Pi C++ program for the 'server side' portion of the capacitance moisture sensor network
 *
 *  This project is under local git version control, in folder:
 *  /home/jroc/Dropbox/projects/MoistureSensor/CapSensor
 *  Refer to git for version history and associated comments.
 *
 * 09/27/2023-rel01:
 *      > Implemented writing received sensor readings to a text log file.
 *
 * 09/26/2023-rel01:
 *      > Removed user prompt to input role (i.e., rx vs tx in setrole()). Hard coded it to always be
 *        in the 'receiving' (a.k.a., 'slave,') role.
 *      > Removed timeout completely. So we are in an infinite loop always listening for Tx's from
 *        the ATTiny's radio.
 *      > Changed field chargeTime to sensorTime in RxPayloadStruct.
 *      > THIS CHANGE TRIGGERED THE ERROR OF ISSUE-1 --->>
 *           Redefined the AckPayloadStruct struct to match up with RadioComms::RxPayloadStruct struct
 *           to support the intended command/response protocol specified in the log entry for milestone #12.
 *              I changed that struct to be only 2 fields, both uint32_t, and with that
 *              change it works. So it's something to do with byte alignment/boundaries
 *              in the struct definitions. I am assuming that misalignment causes overflows,
 *              memory leaks, or other types of corruptions.
 *
 *
 * 09/18/2023-rel01:
 *      > Removed user prompt to input radio number. Hard-coding it to always be transmitting
 *        to the ATTiny's radio chip address - that is, address "2Node."
 *      > Modified the timeout to be 10 minutes.
 *
 * 09/15/2023-rel01:
 *      > Modified the RxPayloadStruct to match a tweak I made on the ATTiny side.
 *        (Moved the units variable down to be with the statusText string so all the numeric
 *        variables would be stacked atop the stings. My thought is to achieve 4-byte
 *        alignment to eventually see if I can cease with custom data load routine for
 *        the structure.)
 *
 * 10/04/2022-rel01:
 *      > Initial program to receive, and display on the console, the data structure
 *        populated, and transmitted, by the ATTiny84/nRF24 prototype device.
 */
#define VERSION "09-27-2023 rel 01"

#define LOG_FILEPATH "/home/readings.txt"
//#define LOG_INTERVAL 60           // This is in seconds. 60=1 minute.
#define LOG_INTERVAL 60 * 60 * 2    // Let's start with once every 2 hours for initial deployment.

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
#include <fstream>     // For writing a log text file.
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
  uint32_t sensorTime;            // Milliseconds on ATTiny clock at time of transmission.
  uint32_t ctSuccess;             // count of success Tx attempts tiny84 has seen since boot
  uint32_t ctErrors;              // count of Tx errors tiny84 saw since last successful transmit
  char units[4];                  // nFD, mFD, FD
  char statusText[12];            // For use in debugging.
};
RxPayloadStruct rxPayload;

    /* Structure to store the outgoing ACK payload
    */
struct AckPayloadStruct {
    uint32_t command;
    uint32_t uliCmdData;

    /*uint8_t command;      // Command ID back to sensor | 1-byte
    uint8_t uiCmdData;    // Command data field: unsigned int | 1-byte
    int iCmdData;         // Command data field: signed int | 2-bytes
    uint32_t uliCmdData;  // Command data field: unsigned long int | 4-bytes
    float fCmdData;       // Command data field: float | 4-bytes
    */
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
void setAckPayload(uint32_t cmd, uint32_t uliData);                                 // Load the ack response data packet
bool logData(RxPayloadStruct* rxData);                                              // Write a log entry.



int main(int argc, char** argv) {
    // perform hardware check
    if (!radio.begin()) {
        cout << "radio hardware is not responding!!" << endl;
        return 0; // quit now
    }

    // Let these addresses be used for the pair
    uint8_t address[2][6] = {"1Node", "2Node"}; // "2Node is the address of ATTiny's nRF24 radio."

    // print example's name
    cout << endl<< argv[0] << " [" << VERSION << "]" << endl;

    // to use ACK payloads, we need to enable dynamic payload lengths
    radio.enableDynamicPayloads();    // ACK payloads are dynamically sized

    // Acknowledgement packets have no payloads by default. We need to enable
    // this feature for all nodes (TX & RX) to use ACK payloads.
    radio.enableAckPayload();

    // Set the PA Level low to try preventing power supply related problems
    // because these examples are likely run with nodes in close proximity to
    // each other.
    radio.setPALevel(RF24_PA_LOW);  // RF24_PA_MAX is default.

    // set the address of the receiving node into the TX pipe
    radio.openWritingPipe(address[1]);     // always uses pipe 0

    // set this nodes's address into a reading pipe
    radio.openReadingPipe(1, address[0]); // using pipe 1

    // For debugging info
    // radio.printDetails();        // (smaller) function that prints raw register values
    radio.printPrettyDetails();     // (larger) function that prints human readable data

    // ready to execute program now
    // setRole(); // calls master() or slave() based on user input <- See below comment.
    slave(); /* 9/26/2023: Removing user input for role. "slave" is bad naming here
    * as I am now treating the RPi as the 'master' and the sensor as
    * the 'slave.' But the RPi code all needs to be cleaned up at some
    * point anyway. */
    return 0;

} // Main()



/* =============================================================================
   Local Functions
   =============================================================================
*/

/* 09/26/20233: This function is now obsolete.
 * Set this node's role from stdin stream.
 *   This only considers the first char as input.
 *   Raspberry Pi will be a receive-only node.
 *   This function then calls slave(), which runs an infinte loop, unless we
 *   timeout not receiving any data from the tiny84.
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
    // Working variables.
    time_t lastLog = time(0);
    time_t logInterval = LOG_INTERVAL;


    setAckPayload(0, 15000);                               // Populate ack payload struct for next Rx/ack cycle.
    DisplayRxPacket dspRx;                                 // create object to display received packets

        /* Load the ackPayload for first received
           transmission on pipe 0. */
    radio.writeAckPayload(1, &ackPayload, sizeof(ackPayload));

    radio.startListening();                                             // put radio in RX mode
    while (true) {                                                      // No timeout, infinite loop here.
        uint8_t pipe;
        if (radio.available(&pipe)) {                                   // is there a received payload? get the pipe number that recieved it
            uint8_t bytes = radio.getDynamicPayloadSize();              // <<-- NOTE: Compilier says we never use this anywhere. Myes, get it's size
            radio.read(&rxBytes[0], sizeof(rxBytes));                   // fetch payload from RX FIFO
            loadRxStruct(&rxPayload, rxBytes);                          // Manually' load rxPayload structure from the received bytes array.
            dspRx.displayRxResults(&rxPayload, true);                   // display received transmission info
            setAckPayload(0, 15000);                                    // Populate ack payload struct for next Rx/ack cycle.
            radio.writeAckPayload(1, &ackPayload, sizeof(ackPayload));  // Load the ACK payload into writing pipe for next cycle.
            if(time(0) > lastLog + logInterval) {                       // Time to write log entry?
                logData(&rxPayload);
                lastLog = time(0);
            } //BOTTOM of IF[test if time to write log entry]
        } // BOTTOM of IF[test if payload received]]
    } // BOTTOM of while loop

        /* Handle radio listening timout case. Which, other than a Control-C by
           the user, is the only way the slave() function ends. And upon ending
           we return back to the setRole() function's while loop, allowing the
           user to quite or initiate another try.
        */
    radio.stopListening();                                              // recommended idle behavior is TX mode, tho we never get to this line.
    cout << "Just executed radio.stopListening(); ";                    // But IF we do, notify user of this fact.
} // BOTTOM of slave()



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
   REQUIRES: ackPayload struct has been loaded with intended response data. */
void displayAck(AckPayloadStruct* pStruct) {
            cout << " Sent in Response: (NOT SHOWING AS I DEBUG ISSUE-1)";
            /*cout << (uint8_t)pStruct->command << endl;
            cout << (uint8_t)pStruct->uiCmdData << endl;
            cout << (int)pStruct->iCmdData << endl;
            cout << (uint32_t)pStruct->uliCmdData << endl;
            cout << (float)pStruct->fCmdData << endl;
            cout << setfill('-') << setw(50) << "-" << setfill(' ') << endl << endl;
            */
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

    pStruct->sensorTime = *(uint32_t *)&pBytes[offset];
    offset = offset + sizeof(pStruct->sensorTime);

    pStruct->ctSuccess = *(uint32_t *)&pBytes[offset];
    offset = offset + sizeof(pStruct->ctSuccess);

    pStruct->ctErrors = *(uint32_t *)&pBytes[offset];
    offset = offset + sizeof(pStruct->ctErrors);

    memcpy((unsigned char *)&pStruct->units, &pBytes[offset], sizeof(pStruct->units));
    offset = offset + sizeof(pStruct->units);

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

    cout << setw(wdthVarName) << setfill(' ') << " sensorTime: ";
    cout << setw(2) << (unsigned int)sizeof(rxPayload.sensorTime);
    cout << " | " << setw(wdthValue) << rxPayload.sensorTime << " | 0x ";
    showHexOfBytes((unsigned char*)&rxPayload.sensorTime,sizeof(rxPayload.sensorTime));
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

    cout << setw(wdthVarName) << setfill(' ') << " units: ";
    cout << setw(2) << (unsigned int)sizeof(rxPayload.units);
    cout << " | " << setw(wdthValue) << rxPayload.units << " | 0x ";
    showHexOfBytes((unsigned char*)&rxPayload.units,sizeof(rxPayload.units));
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


void setAckPayload(uint32_t cmd, uint32_t uliData) {
    ackPayload.command = cmd;
    ackPayload.uliCmdData = 0;
    //ackPayload.iCmdData = 0;
    //ackPayload.uliCmdData = uliData;
    //ackPayload.fCmdData = 0;
}

bool logData(RxPayloadStruct* rxData) {

    // Open a file for appending sensor readings
    std::ofstream logFile;
    logFile.open(LOG_FILEPATH, std::ios::app);

    if (!logFile.is_open()) {
        std::cerr << "Error opening the log file." << std::endl;
        return false;
      }

    // Get the current time, in a pretty string format.
    time_t now = time(0);
    tm* localTime = localtime(&now);
    char buffer[80];
    strftime(buffer,80, "%a %R %F", localTime);

    // Write the sensor reading and timestamp to the log file
    //logFile << "Timestamp: " << asctime(localTime);
    logFile << buffer << ":";
    logFile << " Moisture: " << rxData->capacitance;
    logFile << "  ctSuccess: " << rxData->ctSuccess;
    logFile << "  ctErrors: " << rxData->ctErrors;
    logFile << "  SensorTime: " << rxData->sensorTime;
    logFile << std::endl;

    // Close the file
    logFile.close();

    return true;
  }

