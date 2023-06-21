
/**
 * 06/17/2022 - This sketch was created during my failed attempt at Milestone #5. This
 * is a 'rollback' to the point of just trying to get the 'scanner' utility on the RPi
 * to see RF carrier waves from the tiny84 connected nRF24 radio.
 * 
 * I started here with a full copy/paste from the "rf24ATtiny84_2rdTry_TxOnly" sketch
 * I used at Milestone #3.
 * 
 */

/**
 * ATTiny24/44/84 Pin map with CE_PIN 8 and CSN_PIN 7 & assuming 1.9V to 3V on VCC
 * Schematic provided and successfully tested by
 * Carmine Pastore (https://github.com/Carminepz)
 *
 *                          +-\/-+
 *  nRF24L01 VCC ---- VCC  1|o   |14 GND --- nRF24L01 GND
 *                    PB0  2|    |13 AREF
 *                    PB1  3|    |12 PA1
 *                    PB3  4|    |11 PA2 --- nRF24L01 CE
 *                    PB2  5|    |10 PA3 --- nRF24L01 CSN
 *                    PA7  6|    |9  PA4 --- nRF24L01 SCK
 *  nRF24L01 MOSI --- PA6  7|    |8  PA5 --- nRF24L01 MISO
 *                          +----+
 */

#include "SPI.h"
#include "RF24.h"

const int ledPin = PIN_PA1;           // green LED
const int errorLedPin = PIN_PB1;      // red LED


#define CE_PIN PIN_PA2
#define CSN_PIN PIN_PA3

  // instantiate an object for the nRF24L01 transceiver
RF24 radio(CE_PIN, CSN_PIN);

  // Address for the transmitter
uint8_t address[][6] = {"1Node"};

  // to use different addresses on a pair of radios, we need a variable to
  // point into the address[] array in order to uniquely identify which 
  // address this radio will use to transmit
bool radioNumber = 0; // 0 uses address[0] to transmit, 1 uses address[1] to transmit

  // Used to control whether this node is sending or receiving
bool role = true;  // true = TX node, false = RX node <4/21/2022 set to 'true'

  // For this example, we'll be using a payload containing
  // a string & an integer number that will be incremented
  // on every successful transmission.
  // Make a data structure to store the entire payload of different datatypes
struct PayloadStruct {
  char message[7];          // Max of 6 characters, plus terminator chr, for TX & RX text payloads
  uint8_t counter;
};
PayloadStruct payload;

void setup() {

  pinMode(ledPin, OUTPUT);
  pinMode(errorLedPin, OUTPUT);
  digitalWrite(errorLedPin, LOW);   // Clear the error LED.
  digitalWrite(ledPin, HIGH);       // Green LED on to signal all ok so far.
  delay(1500);                      // Stop to see LED signals at this point.


  // append a NULL terminating character for printing as a c-string
  payload.message[6] = 0;

  // initialize the transceiver on the SPI bus
  if (!radio.begin()) {
    while (1) {
        // If the radio fails to initilize with radio.begin(), then we
        // fall into this infinite loop, stuck in this error condition.
        // So below I am signalling that we're in this error condition
        // the errorLED.
        errorLED (9);               // Call this error #9.
      } // hold in infinite error-loop
  }

  digitalWrite(ledPin, HIGH);

    // Set the PA Level low to try preventing power supply related problems
    // because these examples are likely run with nodes in close proximity to
    // each other.
  radio.setPALevel(RF24_PA_LOW); // RF24_PA_MAX is default, so override and set it low here.

    // save on transmission time by setting the radio to only transmit the
    // number of bytes we need to transmit.
  radio.setPayloadSize(sizeof(payload)); // char[7] & uint8_t datatypes occupy 8 bytes

  // set the address of the RX node into the TX pipe
  radio.openWritingPipe(address[radioNumber]);     // always use pipe 0 to Transmit

  memcpy(payload.message, "Hello ", 6); // set the outgoing message
  radio.stopListening();                // put radio in TX mode

} // setup()

void loop() {
  // Continuously transmit the payload

  digitalWrite(ledPin, HIGH);                           // green LED on to indicate a transmit start
  bool report = radio.write(&payload, sizeof(payload)); // transmit & save the report

  if (report) {
    payload.counter++; // increment counter for next transmission
    digitalWrite(ledPin, LOW);                          // LED off after tranmit event
  } else {                                              // nRF24 says transmit failed (for reasons unknown)
      digitalWrite(ledPin, LOW);                        // Turn green LED off.
      digitalWrite(errorLedPin, HIGH);                  // red LED on. So steady-red-on means we're falling through to here
  } // report
} // loop


void errorLED (int errorNo) {
  digitalWrite(ledPin, LOW);                // Turn green LED off.
  digitalWrite(errorLedPin, HIGH);          // Show RED on for a bit before counting out the error number.
  delay(1500);
  digitalWrite(errorLedPin, LOW);
  for (int i=errorNo; i>0; i--) {           // Flash LED to count the error number.
    delay(500);
    digitalWrite(errorLedPin, HIGH);
    delay(500);
    digitalWrite(errorLedPin, LOW);
  }
  delay(500);
  digitalWrite(errorLedPin, HIGH);          // Show RED on for a bit before counting out the error number.
} // errorLED
