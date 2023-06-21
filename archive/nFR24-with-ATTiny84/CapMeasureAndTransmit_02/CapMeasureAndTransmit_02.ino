
/*  6/18/2022: This is my Milestone #5: Measure capacitance on the ATTiny84, and transmit that info over 
 *  to the nRF24 hooked up to the Raspberry Pi. The "receiver" code on the RPi side is CapDataReceive_02.cpp.
 *  
 *  6/26/2022: Modified initial values in the transmit data structure as part of my trying to work out
 *  the how and why of the data values not showing correctly on the RPi, receive, side. My current theory
 *  is that its is due to different endian architecture (even tho the internet says that both the ATmega
 *  chips and the RPi are both little-endian).
 *   
 *  This sketch is for the ATtiny84.
 *  For the nRF24 elements, see documentation at https://nRF24.github.io/RF24 
 * 
 *  For the approach on capacitance measurement --: 
 *  RCTiming_capacitance_meter || Paul Badger 2008
 *  @ https://www.arduino.cc/en/Tutorial/Foundations/CapacitanceMeter
 *
 *  Use RC time constants to measure the value of a capacitor.
 *  Theory   A capcitor will charge, through a resistor, in one time constant, defined as T seconds where
 *    TC = R * C
 *    TC = time constant period in seconds
 *    R = resistance in ohms
 *    C = capacitance in farads (1 microfarad (ufd) = .000001 farad = 10^-6 farads )
 *
 *    The capacitor's voltage at one time constant is defined as 63.2% of the charging voltage.
 *
 *  Hardware setup:
 *  Test Capacitor between common point and ground (positive side of an electrolytic capacitor  to common)
 *  Test Resistor between chargePin and common point
 *  220 ohm resistor between dischargePin and common point
 *  Wire between common point and analogPin (A/D input)
 */

#include <SPI.h>
#include "RF24.h"

  /* Using PIN references per SpenceKonde attiny core recommendation. */

  // LEDs to signal status.
const int greenLedPin = PIN_PA1; // the pin that a green led is wired to
const int errorLedPin = PIN_PB1; // the pin that a red led is wired to

  // For nRF24 chip:
#define CE_PIN PIN_PA2
#define CSN_PIN PIN_PA3

  // For capatance measurement:
#define analogPin      PIN_PA7    // Physical pin#6 - analog pin for measuring capacitor voltage
#define chargePin      PIN_PB2    // Physical pin#5 - pin to charge the capacitor - connected to one end of the charging resistor
#define dischargePin   PIN_PB3    // Physical pin#4 - pin to discharge the capacitor
#define resistorValue  100.0F     // change this to whatever resistor value you are using
unsigned long startTime;
unsigned long elapsedTime;
float microFarads;                // floating point variable to preserve precision, make calculations
float nanoFarads;

RF24 radio(CE_PIN, CSN_PIN);      // instantiate an object for the nRF24L01 transceiver

  // Jeff Comment: I don't really understand how these "addresses" are used.
  /* Let these addresses be used for the pair It is very helpful to think of 
  an address as a path instead of as an identifying device destination. */
uint8_t address[][6] = {"1Node", "2Node"};

  /* to use different addresses on a pair of radios, we need a variable to uniquely 
   * identify which address this radio will use to transmit. */
bool radioNumber = 1;             // 0 uses address[0] to transmit, 1 uses address[1] to transmit

  /* Used to control whether this node is sending or receiving. */
bool role = true;                 // true = TX role, false = RX role

   /*  Make a data structure to store the entire payload of different datatypes. */
struct TxPayloadStruct {
  char statusText[11];            // For use in debugging.
  float testFloat1;
  uint32_t chargeTime;            // Time it took for capacitor to charge.
  float testFloat2;
  char units[4];                  // nFD, mFD, FD
  float capacitance;
};
TxPayloadStruct txPayload;

// Make a data structure to store the ACK payload
struct RxPayloadStruct {
  char message[11];               // Incoming message up to 10 chrs+Null.
  uint8_t counter;
};
RxPayloadStruct rxAckPayload;



void setup() {

  /* Init LED signal pins. */
  pinMode(greenLedPin, OUTPUT);
  pinMode(errorLedPin, OUTPUT);
  digitalWrite(errorLedPin, LOW);   // Clear the error LED.
  digitalWrite(greenLedPin, HIGH);  // Green LED on to signal all ok so far.
  delay(1500);                      // Stop to see LED signals at this point.

  /*  Init capacitance measurement pins. 
      For now we're just going to create dummy data tho */
  //pinMode(chargePin, OUTPUT);       // set chargePin to output
  //digitalWrite(chargePin, LOW);     // Set chargePin LOW

  /* Init the nRF24 Radio. */
  if (!radio.begin()) {             // initialize the transceiver on the SPI bus
    while (1) {
        /* If the radio fails to initilize with radio.begin(), then we
         * fall into this infinite loop, stuck in this error condition.
         * So below I am signalling that we're in this error condition
         * by turning the error LED on. */
        errorLED (9);               // Call this error #9.
      } // end while
  }

    /* Set the PA Level low to try preventing power supply related problems
     * because these examples are likely run with nodes in close proximity to
     * each other. */
  radio.setPALevel(RF24_PA_LOW);    // RF24_PA_MAX is default.

    /* to use ACK payloads, we need to enable dynamic payload lengths (for all nodes) */
  radio.enableDynamicPayloads();

    /* Acknowledgement packets have no payloads by default. We need to enable
     * this feature for all nodes (TX & RX) to use ACK payloads. */
  radio.enableAckPayload();

    /* set the TX address of the RX node into the TX pipe. 
     * Always uses pipe 0 for transmit.*/
  radio.openWritingPipe(address[radioNumber]);

    /* set the RX address of the TX node into a RX pipe. 
     * (An interesting way to specify the row '0' in the address array.
     * Jeff: I guess this approach guarantees that transmit and receive addresses
     * will always be 'the opposite' of each other.) */
  radio.openReadingPipe(1, address[!radioNumber]); // using pipe 1 to receive.

    /* additional setup specific to this node's TX role. */
  memcpy(txPayload.statusText, "Initial ", 8);    // setup the TX payload
  txPayload.testFloat1 = 12.5;
  txPayload.chargeTime = 256;                     // Time it took for capacitor to charge.
  txPayload.testFloat2 = 0.1;
  memcpy(txPayload.units, "---", 3);
  txPayload.capacitance = 100;
  radio.stopListening();                          // put radio in TX mode


} // END setup()

  /*    OK, we've setup the ATTiny84 and the radio chips. Now go into an infinite loop
   * continuously transmitting data, seeking a valid acknowlegement of receipt; then repeat....
   *    I removed the else portion of code pertaining to receive nodes as here
   * I am only allowing the ATTiny to be a data/sensor transmitter; with the RPi to be
   * the receiver. */
void loop() {
  digitalWrite(greenLedPin, HIGH);
  unsigned long start_timer = micros();                         // start the timer
  bool report = radio.write(&txPayload, sizeof(txPayload));     // transmit & save the report
  unsigned long end_timer = micros();                           // end the timer

  if (report) { // IF report is true than nRF24 chip believes it sent the payload out.
      /* Since we have full success here, clear all error conditions. */
      digitalWrite(greenLedPin, HIGH);
      digitalWrite(errorLedPin, LOW);
      memcpy(txPayload.statusText, "Success   ", 10);          // clear error status text
      txPayload.testFloat2 = txPayload.testFloat2 + 0.1;        // 6-18: working on chargetime value bug

        /* See if we got a receipt ACK payload back from the receiver (this would be an auto-ack payload).
         * Declare variable to hold the pipe number that received the acknowledgement payload. */
      uint8_t pipe;
      if (radio.available(&pipe)) {                             // ACK if-then block. Test for ack payload, which will also return the pipe number that received it.

          /* Declare a variable to hold the Ack payload. Using the same structure we use for transmitting. 
           * (NOTE: is there some reason this declare is 'inline' here vs being defined in the declarations 
           * section of the code per normal coding custom?) */
        radio.read(&rxAckPayload, sizeof(rxAckPayload));        // get incoming ACK payload. Tho doing nothing with it on this side of the conversation.
        delay(20);                                              // brief pause, then fall through to bottom of loop and keep transmitting.

      } else {
        /* Radio chip sent the tranmission out, but did not receive an ACK data payload back.
         * Show this with the LED. But then fall through to bottom of loop and keep transmitting. */
        errorLED (2);               // Call this error #2.
        memcpy(txPayload.statusText, "ERROR 02 ", 9);        // Set text to send back to RPi to show error.
      } // ACK i.e., bottom of the test to see if we got an ACK back

  } else {
    /* Transmission failed or timed out. Code here to signal transmit fail. Like, e.g., turn a red LED on. 
     * Then we fall through and keep trying. */
    errorLED (3);               // Call this error #3.
    memcpy(txPayload.statusText, "ERROR 03 ", 9);        // Set text to send back to RPi to show error.
  } // if(report) -i.e., bottom of the transmit success/fail IF-ELSE block

} // data streaming loop


void errorLED (int errorNo) {
  digitalWrite(greenLedPin, LOW);           // Turn green LED off.
  digitalWrite(errorLedPin, HIGH);          // Show RED on for a bit before counting out the error number.
  delay(1500);
  digitalWrite(errorLedPin, LOW);
  for (int i=errorNo; i>0; i--) {               // Flash LED to count the error number.
    delay(500);
    digitalWrite(errorLedPin, HIGH);
    delay(500);
    digitalWrite(errorLedPin, LOW);
  }
  delay(500);
  digitalWrite(errorLedPin, HIGH);          // Show RED on for a bit before counting out the error number.
} // errorLED
