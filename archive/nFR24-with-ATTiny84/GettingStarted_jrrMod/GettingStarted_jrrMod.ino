
/*  5/10/2022: Trying to get two chips to talk to each other. This sketch is for the ATtiny84 driven chip
 *  (other side is the raspberry pi driven chip). I have modified the original code to:
 *    1. Set the correct CD and CSN pin numbers.
 *    2. Removed the Serial input/output code as I don't have a serial port/monitor for the ATTiny.
 *    3. Added an LED pin so I can get some debug signals via use of flashing an LED.
 * 
 */


/*
 * See documentation at https://nRF24.github.io/RF24
 * See License information at root directory of this library
 * Author: Brendan Doherty (2bndy5)
 */

/**
 * A simple example of sending data from 1 nRF24L01 transceiver to another.
 *
 * This example was written to be used on 2 devices acting as "nodes".
 * Use the Serial Monitor to change each node's behavior.
 */
#include <SPI.h>
#include "printf.h"
#include "RF24.h"

  //NEW on 5/10/2022: Set up a pin to blink an LED
const int ledPin = PIN_PA1; // the pin that the led is wired to

  //5/10/2022: Changing PIN references per SpenceKonde attiny core recommendation.
#define CE_PIN PIN_PA2
#define CSN_PIN PIN_PA3

  // instantiate an object for the nRF24L01 transceiver
  //  Original Code: RF24 radio(7, 8); // using pin 7 for the CE pin, and pin 8 for the CSN pin
RF24 radio(CE_PIN, CSN_PIN);



// Let these addresses be used for the pair
uint8_t address[][6] = {"1Node", "2Node"};
// It is very helpful to think of an address as a path instead of as
// an identifying device destination

// to use different addresses on a pair of radios, we need a variable to
// uniquely identify which address this radio will use to transmit
bool radioNumber = 1; // 0 uses address[0] to transmit, 1 uses address[1] to transmit

// Used to control whether this node is sending or receiving
bool role = true;  // true = TX role, false = RX role

// For this example, we'll be using a payload containing
// a single float number that will be incremented
// on every successful transmission
float payload = 0.0;

void setup() {

  // Start with debug LED on
  digitalWrite(ledPin, HIGH);
  delay(3000);  // delay so we can see LED is on.

  // initialize the transceiver on the SPI bus
  if (!radio.begin()) {
    while (1) {
        // If the radio fails to initilize with radio.begin(), then we
        // fall into this infinite loop, stuck in this error condition.
        // So below I am signalling that we're in this error condition
        // by turning the LED off.
        digitalWrite(ledPin, LOW);
        delay(1500);  // Half-second delay
      } // end while
  }


  // Set the PA Level low to try preventing power supply related problems
  // because these examples are likely run with nodes in close proximity to
  // each other.
  radio.setPALevel(RF24_PA_LOW);  // RF24_PA_MAX is default.

  // save on transmission time by setting the radio to only transmit the
  // number of bytes we need to transmit a float
  radio.setPayloadSize(sizeof(payload)); // float datatype occupies 4 bytes

  // set the TX address of the RX node into the TX pipe
  radio.openWritingPipe(address[radioNumber]);     // always uses pipe 0

  // set the RX address of the TX node into a RX pipe
  radio.openReadingPipe(1, address[!radioNumber]); // using pipe 1

  // additional setup specific to the node's role
  if (role) {
    radio.stopListening();  // put radio in TX mode
  } else {
    radio.startListening(); // put radio in RX mode
  }


} // setup

void loop() {

  if (role) {
    // This device is a TX node

    // Turn LED on to show we got here.
    digitalWrite(ledPin, HIGH);

    unsigned long start_timer = micros();                    // start the timer
    bool report = radio.write(&payload, sizeof(float));      // transmit & save the report
    unsigned long end_timer = micros();                      // end the timer

    if (report) { // payload was delivered
      // Flash LED to show this
      digitalWrite(ledPin, LOW);
      delay(500);  // Half-second delay
      digitalWrite(ledPin, HIGH);
      delay(500);  // Half-second delay
      digitalWrite(ledPin, LOW);
      delay(500);  // Half-second delay
      digitalWrite(ledPin, HIGH);
      payload += 0.01;                                       // increment float payload
    } else {
      digitalWrite(ledPin, LOW); // payload was not delivered, turn LED off to show this.
      delay(1000);  // slow transmissions down by 1 second
    }
  
  } else {
    // This device is a RX node - for my mods, we should never get to this branch.

    uint8_t pipe;
    if (radio.available(&pipe)) {             // is there a payload? get the pipe number that recieved it
      uint8_t bytes = radio.getPayloadSize(); // get the size of the payload
      radio.read(&payload, bytes);            // fetch payload from FIFO
    }
  } // else, and thus ends role if

} // loop
