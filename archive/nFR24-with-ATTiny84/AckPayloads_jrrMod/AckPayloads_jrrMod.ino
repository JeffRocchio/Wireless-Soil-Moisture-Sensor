
/*  5/26/2022: This is my step-3: Two chips to talk back and forth, with the recceiver on the RPi sending back a
 *   receipt acknnowldegement to the ATTiny to confirm receipt. Upon receipt confirm the ATTiny will then send
 *   another data point. An incremented counter will be used on the receiving end 'see' that the bidirectional
 *   communication/ack is working.
 *   
 *   This sketch is for the ATtiny84 driven chip The other side is the raspberry pi driven chip. 
 *   
 *   I have modified the original code, acknowledgementPayloads.ccp, to:
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
const int greenLedPin = PIN_PA1; // the pin that a green led is wired to
  //NEW on 5/26/2022: Set up a pin for an ERROR condition LED
const int errorLedPin = PIN_PB1; // the pin that a red led is wired to


  //5/10/2022: Changing PIN references per SpenceKonde attiny core recommendation.
#define CE_PIN PIN_PA2
#define CSN_PIN PIN_PA3

  // instantiate an object for the nRF24L01 transceiver
  //  Original Code: RF24 radio(7, 8); // using pin 7 for the CE pin, and pin 8 for the CSN pin
RF24 radio(CE_PIN, CSN_PIN);


  // 5/26/2022: I don't really understand how these "addresses" are used.
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
// a string & an integer number that will be incremented
// on every successful transmission.
// Make a data structure to store the entire payload of different datatypes
struct PayloadStruct {
  char message[7];          // only using 6 characters for both the TX & ACK payloads
  uint8_t counter;
};
PayloadStruct payload;


void setup() {

    // initialize LED signal pins.
  pinMode(greenLedPin, OUTPUT);
  pinMode(errorLedPin, OUTPUT);


    // To debug the LEDs - turn them both on and wait 5 seconds to see if I can see them.
  digitalWrite(greenLedPin, HIGH);
  digitalWrite(errorLedPin, HIGH);
  delay(2000); 
  
    // Clear error LED, turn on green LED to signal we are working through the setup process.
  digitalWrite(errorLedPin, LOW);
  digitalWrite(greenLedPin, HIGH);

  // initialize the transceiver on the SPI bus
  if (!radio.begin()) {
    while (1) {
        // If the radio fails to initilize with radio.begin(), then we
        // fall into this infinite loop, stuck in this error condition.
        // So below I am signalling that we're in this error condition
        // by turning the error LED on.
        digitalWrite(greenLedPin, LOW);
        digitalWrite(errorLedPin, HIGH);
        delay(1500);  // Delay to see error LED
      } // end while
  }


    // Set the PA Level low to try preventing power supply related problems
    // because these examples are likely run with nodes in close proximity to
    // each other.
  radio.setPALevel(RF24_PA_LOW);  // RF24_PA_MAX is default.

    // to use ACK payloads, we need to enable dynamic payload lengths (for all nodes)
  radio.enableDynamicPayloads();    // ACK payloads are dynamically sized

    // Acknowledgement packets have no payloads by default. We need to enable
    // this feature for all nodes (TX & RX) to use ACK payloads.
  radio.enableAckPayload();

    // set the TX address of the RX node into the TX pipe
  radio.openWritingPipe(address[radioNumber]);     // always uses pipe 0 for transmit.

    // set the RX address of the TX node into a RX pipe. 
    // (An interesting way to specify the row '0' in the address array.
    // I guess this approach guarantees that transmit and receive addresses
    // will always be 'the opposite' of each other.)
  radio.openReadingPipe(1, address[!radioNumber]); // using pipe 1 to receive.

    // additional setup specific to thhis node's TX role.
  if (role) {
      // setup the TX payload
    memcpy(payload.message, "Hello ", 6);
    payload.counter = 1;

    radio.stopListening();  // put radio in TX mode
  }


} // setup


  //    OK, we've setup the ATTiny84 and the radio chips. Now go into an infinite loop
  // continuously transmitting data, seeking a valid acknowlegement of receipt; then repeat....
  //    5/26/2022: I removed the else portion of code pertaining to receive nodes as here
  // I am only allowing the ATTiny to be a data/sensor transmitter; with the RPi to be
  // the receiver.
void loop() {
  
  digitalWrite(greenLedPin, HIGH);
  
  unsigned long start_timer = micros();                    // start the timer
  bool report = radio.write(&payload, sizeof(payload));    // transmit & save the report
  unsigned long end_timer = micros();                      // end the timer

  if (report) { // IF report is true than payload was transmitted out by the nRF chip and a auto-ack received back.
      digitalWrite(greenLedPin, HIGH);
      digitalWrite(errorLedPin, LOW);

        // See if we got a receipt ACK payload back from the receiver (this would be an auto-ack payload).
        // Declare variable to hold the pipe number that received the acknowledgement payload.
      uint8_t pipe;
      if (radio.available(&pipe)) { // ACK if-then block. Test for ack payload, which will also retrun the pipe number that received it.
          // Declare a variable to hold the Ack payload. Using the same structure we use for transmitting. 
          // (NOTE: is there some reason this declare is 'inline' here vs being defined in the declarations section of the 
          // code per normal coding custom?)
        PayloadStruct received;
        radio.read(&received, sizeof(received));           // get incoming ACK payload
          // save incoming counter & increment for next outgoing
        memcpy(payload.message, "Hello ", 6);
        payload.counter = received.counter + 1;
      } else {
        // Radio chip sent the tranmission out, but did not receive an ACK data payload back.
        // Show this with the LED. But then fall through to bottom of loop and keep transmitting.
        // Note that in this case we've not incremented the counter, so in principle we should see
        // on the receive side the counter not being incremented, which is really our tell that
        // the acknowledgements are not being received.
        digitalWrite(errorLedPin, HIGH); // payload was not acknowledged, turn error LED on to show this.
        delay(1000);  // slow transmissions down by 1 second, but then fall through to bottom of loop and keep transmitting.
        memcpy(payload.message, "yyyyy ", 6);
      } // ACK (i.e., bottom of the test for ack IF-ELSE block)

  } else {
    // Transmission failed or timed out.
    //code here to signal transmit fail. Like, e.g., turn a red LED on. Then we fall through and keep trying.
    digitalWrite(greenLedPin, LOW);
    digitalWrite(errorLedPin, HIGH);
    delay(1000);  // slow transmissions down by 1 second, and time to see LED status.
    memcpy(payload.message, "xxxxx ", 6);
  
  } // report (i.e., bottom of the transmit success/fail IF-ELSE block)

} // data streaming loop
