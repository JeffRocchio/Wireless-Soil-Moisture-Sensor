
/*  This is my Milestone #6: Measure capacitance on the ATTiny84, and transmit that info over 
 *  to the nRF24 hooked up to the Raspberry Pi. The matching "receiver" code on the RPi side is 
 *  MS-06_CapDataReceive_05.cpp.
 *  
 *  07/08/2022: My initial attempt failed to work at all. So I am here starting all over from scratch. This time I plan to 
 *  proceed in smaller increments, proving each one is working before advancing.
 *  
 *  Increment-1: Simply modify the TxPayload struct for the fields we'll use for the live capacator measurement.
 *    07/08/20222: Ok, good, WORKED.
 *  
 *  Increment-2: Improve the 'success' rate of the transmissions received by the RPi. Too many 'Error 3' receipts.
 *    07/18/2022: Extending the TxPayload struct to add counters for Tx 'success' and 'Error' attempts.
 *    
 *    
 *  Increment-3: Use a stub for the measure() function. Not actually attempting a measurement, but just filling
 *               out the data structure with dummy data. And to include how I want the signalling LEDs to work.
 *  Increment-4: TBD.
 *  
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

// ==== Pull in Required Libraries
#include <SPI.h>
#include "RF24.h"

  /* Using PIN references per SpenceKonde attiny core recommendation. */


// ==== ATTiny84 Pin Reference Definitions
/*************************************************************************************************
 *    Define references to the logical pins we'll be using.
 *    NOTE: for RF24 I am using the PIN references per SpenceKonde attiny core recommendation. The
 * pin references in the original demo code did not work for the tiny84. After working out the
 * whole Arduino core thing, and how the logical-physical pin mappings are actually defined by
 * the core's author, I went to the SpenceKonde github and inspected his documentation, in
 * which he states to use the 'logical' port numbering schema - e.g., "PIN_PA#" or PIN_PB#."
 */

  /* Logical pin names representing connection of ATTiny84 to nRF24 chip */
#define CE_PIN PIN_PA2
#define CSN_PIN PIN_PA3

  /* LEDs to signal status. */
const int greenLedPin = PIN_PA1;  // the pin that a green led is wired to
const int errorLedPin = PIN_PB1;  // the pin that a red led is wired to

  /* For capacitance measurement */
#define analogPin      PIN_PA7    // Physical pin#6 - analog pin for measuring capacitor voltage
#define chargePin      PIN_PB2    // Physical pin#5 - pin to charge the capacitor - connected to one end of the charging resistor
#define dischargePin   PIN_PB3    // Physical pin#4 - pin to discharge the capacitor
#define resistorValue  100.0F     // change this to whatever resistor value you are using


// ==== Global Variables
/*************************************************************************************************
 *    Admidittly being sloppy here as I experiment and learn, relative to relying heavily on
 * globals. Although, my mindset clearly comes from desktop/server coding. Perhaps efficient
 * coding on an MCU is paradigmatically different vis-a-vis globals. */

  /* Some useful variables as I experiment with cap measurement. */
unsigned long startTime;
unsigned long elapsedTime;
float microFarads;                // floating point variable to preserve precision, make calculations
float nanoFarads;

  /* Declare, and instantiate, an object for the nRF24L01 transceiver. */
RF24 radio(CE_PIN, CSN_PIN);

  /*    Declare an array to hold node addresses. In this case we have only two nodes -
   * the RPi and this node, the tiny84.
   *    NOTE from original source: "It is very helpful to think of an address as a path 
   * instead of as an identifying device destination."
   *    Jeff Comment: I don't really understand how these "addresses" are used, this 
   * needs further rabbit-holing my me at some point. */
uint8_t address[][6] = {"1Node", "2Node"};

  /*    Comment in original source: "To use different addresses on a pair of radios
   * we need a variable to uniquely identify which address this radio will use to 
   * transmit."
   *    This variable is used later to de-reference an entry in the address[][] array
   * declared above. */
bool radioNumber = 1;             // 0 uses address[0] to transmit, 1 uses address[1] to transmit

  /* Used to control whether this node is sending or receiving. */
bool role = true;                 // true = TX role, false = RX role

  /*    Define, and declare, a struct to hold the transmit payload.
   *    NOTE: This struct must match the 'receive' payload structure declared on
   * the RPi side. */
struct TxPayloadStruct {
  float capacitance;
  uint32_t chargeTime;            // Time it took for capacitor to charge.
  char units[4];                  // nFD, mFD, FD
  uint32_t ctSuccess;             // count of success Tx attempts tiny84 has seen since boot
  uint32_t ctErrors;              // count of Tx errors tiny84 saw since last successful transmit
  char statusText[11];            // For use in debugging.
};
TxPayloadStruct txPayload;

  /*    Define, and declare, a struct to hold the acknowledgement payload.
   *    NOTE: This struct must match the 'ack' payload structure declared on
   * the RPi side. */
struct RxPayloadStruct {
  char message[11];               // Incoming message up to 10 chrs+Null.
  uint8_t counter;
};
RxPayloadStruct rxAckPayload;




// ==== Setup Procedure
/*************************************************************************************************
 *  Make everything ready so that we can begin to sense and transmit.
 */
void setup() {

    /* Init LED signal pins. */
  pinMode(greenLedPin, OUTPUT);
  pinMode(errorLedPin, OUTPUT);
  digitalWrite(errorLedPin, LOW);   // Clear the error LED.
  digitalWrite(greenLedPin, HIGH);  // Green LED on to signal all ok so far.
  delay(1500);                      // Pause to verify LEDs at this point.

    /*    Init capacitance measurement pins.
     *    For now we're just going to create dummy data tho */
  //pinMode(chargePin, OUTPUT);       // set chargePin to output
  //digitalWrite(chargePin, LOW);     // Set chargePin LOW

    /* Init the nRF24 radio on the SPI bus. If this fails, go into an infinite
     * loop, stuck in that error condition. If this does happen, signal that
     * with the LEDs. */
  if (!radio.begin()) {
    while (1) {
        errorLED (9);               // Call this error #9.
      } // end while
  }

    /* Set the PA Level low to try preventing power supply related problems
     * because these examples are likely run with nodes in close proximity to
     * each other. */
  radio.setPALevel(RF24_PA_LOW);    // RF24_PA_MAX is default.

    /* To use ACK payloads, we need to enable dynamic payload lengths (for all nodes) */
  radio.enableDynamicPayloads();

    /*    We wish to receive Acknowledgement payloads back from the RPi. 
     *    To be able to do so We need to enable this feature for all nodes (TX & RX). 
     * (akk packets have no payloads by default.) */
  radio.enableAckPayload();

    /*    Original source code comment: "set the TX address of the RX node into the TX pipe. 
     * Always uses pipe 0 for transmit."
     *    Jeff Note: This is quite confusing. I haven't yet backtracked this to work
     * out what it's really saying. But I assume it's saying that we need to load an
     * address into the transmit pipe that the receiving node will recognize and accept
     * as a valid "one of us." */
  radio.openWritingPipe(address[radioNumber]);

    /*    Original Source Comment: "set the RX address of the TX node into a RX pipe."
     *    Jeff: An interesting way to specify the row '0' in the address array.
     * I guess this approach guarantees that transmit and receive addresses
     * will always be 'the opposite' of each other. */
  radio.openReadingPipe(1, address[!radioNumber]);  // using pipe 1 to receive.

    /* Load the txPayload structure with initial defaults. */
  txPayload.capacitance = 0;                        // calculated capacitance
  txPayload.chargeTime = 0;                         // Time it took for capacitor to charge.
  memcpy(txPayload.units, "---", 3);                // capacitance units
  txPayload.ctSuccess = 0;                          // running count of no-error transmissions
  txPayload.ctErrors = 0;                           // running count of transmissions errors
  memcpy(txPayload.statusText, "Initial ", 8);      // status message

    /* Put radio in transmit mode. */
  radio.stopListening();


} // END setup()



// ==== Transmission Loop
/*************************************************************************************************
 *    OK, we've setup the ATTiny84 and the radio chip. Now go into an infinite loop, continuously 
 * transmitting data, seeking a valid acknowlegement of receipt; then repeat....
 *    NOTE: I removed the else portion of code pertaining to receive nodes as here I am only 
 *  allowing the ATTiny to be a data/sensor transmitter; with the RPi to be the receiver. 
 */
void loop() {
  digitalWrite(greenLedPin, HIGH);
  unsigned long start_timer = micros();                         // time the Tx cycle - obtain start time
  bool report = radio.write(&txPayload, sizeof(txPayload));     // attempt transmit - obtaining 'success' or 'timeout' result
  unsigned long end_timer = micros();                           // time the Tx cycle - obtain end time

  if (report) { // IF report is true than nRF24 chip sent the payload out & got an ack back
        /* report is TRUE: meaning the nRF24 chip sent the payload out & got an ack back. 
         * Since we have full success, clear any prior error conditions. */
      digitalWrite(greenLedPin, HIGH);
      digitalWrite(errorLedPin, LOW);
      memcpy(txPayload.statusText, "Success   ", 10);           // clear error status text
      txPayload.chargeTime = 0;                                 // using for now to help understand error rates; resetting to zero upon success.
      txPayload.ctSuccess = txPayload.ctSuccess + 1;            // we have a Tx/ack success, increment the success tracking counter.

        /* See if we got a receipt ACK payload back from the receiver (this would be an auto-ack payload).
         * Declare variable to hold the pipe number that received the acknowledgement payload. */
      uint8_t pipe;
      if (radio.available(&pipe)) {                             // ACK if-then block. Test for ack payload, which will also return the pipe number that received it.
        radio.read(&rxAckPayload, sizeof(rxAckPayload));        // get incoming ACK payload. Tho doing nothing with it on this side of the conversation.
        delay(20);                                              // brief pause, then fall through to bottom of loop and keep transmitting.
      } else {
        /* Radio chip sent the tranmission out, but did not receive an ACK data payload back.
         * Show this with the LED. But then fall through to bottom of loop and keep transmitting. */
        errorLED (2);                                           // Call this error #2.
        memcpy(txPayload.statusText, "ERROR 02 ", 9);           // Set text to send back to RPi to show error.
        txPayload.ctErrors++;                                   // we have a Tx/ack failure, increment the errors tracking counter.
      } // ACK i.e., bottom of the test to see if we got an ACK back

  } else {
    /* Transmission failed. Report, then fall through and keep trying. */
    /* errorLED (3); <- Let's try skipping the delay this reporting introduces and see what happens */ // Call this error #3.
    digitalWrite(errorLedPin, HIGH); // Instead just show LED red to signal an error
    memcpy(txPayload.statusText, "ERROR 03 ", 9);
    txPayload.chargeTime = txPayload.chargeTime + 1;            // Let's track how many error retries we've got on-the-go.
    txPayload.ctErrors = txPayload.ctErrors + 1;               // we have a Tx/ack failure, increment the errors tracking counter.
  } // if(report) -i.e., bottom of the transmit success/fail IF-ELSE block

  delay(1000);                                                   // Introduce a bit of delay each loop cycle.
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
