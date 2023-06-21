/* This is the Raspberry Pi, receive, side of my 'step-5' milestone:
 * Transmit / Receive capacitance measurement.
 *
 * For nRF24 radio chip documentation see https://nRF24.github.io/RF24
 * For the approach on capacitance measurement on the ATTiny84 side --:
 *   RCTiming_capacitance_meter || Paul Badger 2008
 *   @ https://www.arduino.cc/en/Tutorial/Foundations/CapacitanceMeter

 * This source based on "manualAcknowledgements.cpp" in the RPi nRF24 library
 * noted above.
 */

#include <ctime>       // time()
#include <iostream>    // cin, cout, endl
#include <string>      // string, getline()
#include <time.h>      // CLOCK_MONOTONIC_RAW, timespec, clock_gettime()
#include <RF24/RF24.h> // RF24, RF24_PA_LOW, delay()

using namespace std;

/****************** Linux ***********************/
// Radio CE Pin, CSN Pin, SPI Speed
// CE Pin uses GPIO number with BCM and SPIDEV drivers, other platforms use their own pin numbering
// CS Pin addresses the SPI bus number at /dev/spidev<a>.<b>
// ie: RF24 radio(<ce_pin>, <a>*10+<b>); spidev1.0 is 10, spidev1.1 is 11 etc..

// Generic:
RF24 radio(22, 0);
/****************** Linux (BBB,x86,etc) ***********************/
// See http://nRF24.github.io/RF24/pages.html for more information on usage
// See http://iotdk.intel.com/docs/master/mraa/ for more information on MRAA
// See https://www.kernel.org/doc/Documentation/spi/spidev for more information on SPIDEV

// Make data structures to store the receive and ACK payloads.
struct RxPayloadStruct {
  char statusText[11];            // For use in debugging.
  unsigned long chargeTime;       // Time it took for capacitor to charge.
  char units[4];                  // nFD, mFD, FD
  float capacitance;
};
RxPayloadStruct rxPayload;

// Make a data structure to store the ACK payload
struct AckPayloadStruct {
  char message[11];               // Outgoing message, up to 10 chrs+Null.
  uint8_t counter;
};
AckPayloadStruct ackPayload;

void setRole(); // prototype to set the node's role
void master();  // prototype of the TX node's behavior
void slave();   // prototype of the RX node's behavior

// custom defined timer for evaluating transmission time in microseconds
struct timespec startTimer, endTimer;
uint32_t getMicros(); // prototype to get ellapsed time in microseconds


int main(int argc, char** argv) {

    // perform hardware check
    if (!radio.begin()) {
        cout << "radio hardware is not responding!!" << endl;
        return 0; // quit now
    }

    // Append a NULL terminating 0 for printing text fields as a c-string.
    rxPayload.statusText[10] = 0;
    rxPayload.units[3] =0;
    ackPayload.message[10] = 0;

    // Let these addresses be used for the pair of nodes used in this example
    uint8_t address[2][6] = {"1Node", "2Node"};
    //             the TX address^ ,  ^the RX address
    // It is very helpful to think of an address as a path instead of as
    // an identifying device destination

    // to use different addresses on a pair of radios, we need a variable to
    // uniquely identify which address this radio will use to transmit
    bool radioNumber = 1; // 0 uses address[0] to transmit, 1 uses address[1] to transmit

    // print example's name
    cout << argv[0] << endl;

    // Set the radioNumber via the terminal on startup
    cout << "Which radio is this? Enter '0' or '1'. Defaults to '0' ";
    string input;
    getline(cin, input);
    radioNumber = input.length() > 0 && (uint8_t)input[0] == 49;

    // Set the PA Level low to try preventing power supply related problems
    // because these examples are likely run with nodes in close proximity to
    // each other.
    radio.setPALevel(RF24_PA_LOW);  // RF24_PA_MAX is default.

    // save on transmission time by setting the radio to only transmit the
    // number of bytes we need.
    radio.setPayloadSize(sizeof(ackPayload));

    // set the TX address of the RX node into the TX pipe
    radio.openWritingPipe(address[radioNumber]);     // always uses pipe 0

    // set the RX address of the TX node into a RX pipe
    radio.openReadingPipe(1, address[!radioNumber]); // using pipe 1

    // For debugging info
    // radio.printDetails();       // (smaller) function that prints raw register values
    // radio.printPrettyDetails(); // (larger) function that prints human readable data

    // ready to execute program now
    setRole(); // calls master() or slave() based on user input
    return 0;
} // main


/**
 * set this node's role from stdin stream.
 * this only considers the first char as input.
 */
void setRole() {
    string input = "";
    while (!input.length()) {
        cout << "*** PRESS 't' to begin transmitting to the other node\n";
        cout << "*** PRESS 'r' to begin receiving from the other node\n";
        cout << "*** PRESS 'q' to exit" << endl;
        getline(cin, input);
        if (input.length() >= 1) {
            if (input[0] == 'T' || input[0] == 't')
                master();
            else if (input[0] == 'R' || input[0] == 'r')
                slave();
            else if (input[0] == 'Q' || input[0] == 'q')
                break;
            else
                cout << input[0] << " is an invalid input. Please try again." << endl;
        }
        input = ""; // stay in the while loop
    } // while
} // setRole()


/**
 * make this node act as the transmitter
 */
void master() {

    memcpy(ackPayload.message, "Hello ", 6);                        // set the outgoing message
    radio.stopListening();                                          // put in TX mode

    unsigned int failures = 0;                                      // keep track of failures
    while (failures < 6) {
        clock_gettime(CLOCK_MONOTONIC_RAW, &startTimer);            // start the timer
        bool report = radio.write(&ackPayload, sizeof(ackPayload)); // transmit & save the report

        if (report) {
            // transmission successful; wait for response and print results

            radio.startListening();                             // put in RX mode
            unsigned long start_timeout = millis();             // timer to detect no response
            while (!radio.available()) {                        // wait for response
                if (millis() - start_timeout > 200)             // only wait 200 ms
                    break;
            }
            unsigned long ellapsedTime = getMicros();           // end the timer
            radio.stopListening();                              // put back in TX mode

            // print summary of transactions
            uint8_t pipe;
            cout << "Transmission successful! ";
            if (radio.available(&pipe)) {                       // is there a payload received? grab the pipe number that received it
                uint8_t bytes = radio.getPayloadSize();         // grab the incoming payload size
                cout << "Round trip delay = ";
                cout << ellapsedTime;                           // print the timer result
                cout << " us. Sent: " << ackPayload.message;    // print outgoing message
                cout << (unsigned int)ackPayload.counter;       // print outgoing counter
                radio.read(&rxPayload, sizeof(rxPayload));      // get incoming payload
                cout << " Recieved " << (unsigned int)bytes;    // print incoming payload size
                cout << " on pipe " << (unsigned int)pipe;      // print RX pipe number
                cout << ": " << rxPayload.statusText;           // print the incoming message
                cout << " charge time: ";                       // time to charge cap
                cout << (unsigned long)rxPayload.chargeTime;
                cout << " capacitance: ";                       // capacitance value
                cout << (float)rxPayload.capacitance;
                cout << " " << rxPayload.units;                 // capacitance units
                cout << endl;
                ackPayload.counter = ackPayload.counter + 1;    // increment counter
            }
            else {
                cout << "Recieved no response." << endl;        // no response received
            }
        }
        else {
            cout << "Transmission failed or timed out";         // payload was not delivered
            cout << endl;
            failures++;                                         // increment failure counter
        } // report

        // to make this example readable in the terminal
        delay(1000);  // slow transmissions down by 1 second
    } // while

    cout << failures << " failures detected. Leaving TX role." << endl;
} // master


/**
 * make this node act as the receiver
 */
void slave() {
    memcpy(ackPayload.message, "World ", 6);                    // set the response message
    radio.startListening();                                     // put in RX mode

    time_t startTimer = time(nullptr);                          // start a timer
    while (time(nullptr) - startTimer < 6) {                    // use 6 second timeout
        uint8_t pipe;
        if (radio.available(&pipe)) {                           // is there a payload? get the pipe number that recieved it
            uint8_t bytes = radio.getPayloadSize();             // get size of incoming payload
            radio.read(&rxPayload, sizeof(rxPayload));          // get incoming payload
            ackPayload.counter = ackPayload.counter + 1;        // increment counter

            // transmit response & save result to `report`
            radio.stopListening();                              // put in TX mode
            radio.writeFast(&ackPayload, sizeof(ackPayload));   // load response into TX FIFO
            bool report = radio.txStandBy(150);                 // keep retrying for 150 ms
            radio.startListening();                             // put back in RX mode

            // print summary of transactions
            cout << " Recieved " << (unsigned int)bytes;        // print incoming payload size
            cout << " bytes on pipe " << (unsigned int)pipe;    // print RX pipe number
            cout << ": " << rxPayload.statusText;               // print the incoming message
            cout << " charge time: ";                           // time to charge cap
            cout << (unsigned long)rxPayload.chargeTime;
            cout << " capacitance: ";                           // capacitance value
            cout << (float)rxPayload.capacitance;
            cout << " " << rxPayload.units ;                    // capacitance units
            cout << endl;

            if (report) {
                cout << " Sent: " << ackPayload.message;        // print outgoing message
                cout << (unsigned int)ackPayload.counter;       // print outgoing counter
                cout << endl;
            }
            else {
                cout << " Response failed to send." << endl;    // failed to send response
            }
            startTimer = time(nullptr);                         // reset timer
        } // available
    } // while

    cout << "Nothing received in 6 seconds. Leaving RX role." << endl;
    radio.stopListening(); // recommended idle mode is TX mode
} // slave


/**
 * Calculate the ellapsed time in microseconds
 */
uint32_t getMicros() {
    // this function assumes that the timer was started using
    // `clock_gettime(CLOCK_MONOTONIC_RAW, &startTimer);`

    clock_gettime(CLOCK_MONOTONIC_RAW, &endTimer);
    uint32_t seconds = endTimer.tv_sec - startTimer.tv_sec;
    uint32_t useconds = (endTimer.tv_nsec - startTimer.tv_nsec) / 1000;

    return ((seconds) * 1000 + useconds) + 0.5;
}
