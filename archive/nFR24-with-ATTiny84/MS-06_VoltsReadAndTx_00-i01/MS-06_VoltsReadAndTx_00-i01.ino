
/*  This is my Milestone #6 (recast from cap measure to just try to measure a voltage): VoltsRead Tx/Rx. 
 *  The matching "receiver" code on the RPi side is: MS-06_VoltsRead_0#.cpp.
 *  
 *  09/29/2022: Success!
 *    Initial Attempt. Starting on the theory that I need to work out how to specify the analog pin
 *    I have the voltage source connected to. I am using ATtiny84 physical pin#6; which is 
 *    "analog channel ADC7." In digging into the SpenceKonde attiny core github, as well as a bunch of
 *    Google searched for how to use the analog pins, I have learned that you have to reference the analog
 *    pins as "channels," using the 'channel number.' And as with the digital pins, how you reference them
 *    in the Arduino IDE code depends on how the core you are using for the ATTiny chip has defined (i.e.,
 *    exposed) them to the Arduino platform. It appears this is done via a header file that the arduino IDE
 *    uses, which may be called "pins_arduino.h," tho I don't know where it's located. I am assuming it
 *    gets installed somewhere as part of the initial process of downloading and installing the 'core,' 
 *    which I did way back when in getting myself set up with the Arduino UNO as a programmer, the IDE and
 *    burning the SpenceKonde core onto my ATTiny84. Now, all that being said, the documentation still has
 *    me confused on just how I should define and reference the analog pins in the code. On the SpenceKonde
 *    github, in the (huge!) readme file it talks about using a macro. And in pins_arduino.h there are 
 *    variable definitions for them, e.g.: | static const uint8_t A7 = ADC_CH(7); | So I think I'm in for
 *    some trial-and-error here.
 *  
 *
 *  Hardware setup:
 *   Voltage applied to physical pin #6, using a voltage divider through a pot to vary it. Voltage
 *   source is the VCC rail on the breadboad.
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

  /* For capacitance measurement, tho in this iteration I am only going to measure a voltage, not capacitance. */
#define analogPin      A7         // Attempting form implied by line 65 on pins_arduino.h at SpenceKonde github. | Physical pin#6 - analog pin for measuring voltage
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
  uint32_t ctErrors;              // count of Tx errors since boot
  char statusText[12];            // For use in debugging. [12]=11 text chars + NULL.
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
  digitalWrite(greenLedPin, HIGH);  // Green LED on to signal in setup and all ok so far.
  delay(5000);                      // Pause to verify LEDs at this point.

    /*    Init capacitance measurement pins. */
  //pinMode(chargePin, OUTPUT);       // set chargePin to output
  //digitalWrite(chargePin, LOW);     // Set chargePin LOW
  //pinMode(dischargePin, OUTPUT);    // Set discharge pin to output
  //digitalWrite(dischargePin, LOW);  // Init discharge pin to LOW

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
  memcpy(txPayload.statusText, "00-i01     ", 11);  // status message

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
    txPayload.ctErrors = txPayload.ctErrors + 1;                // we have a Tx/ack failure, increment the errors tracking counter.
  } // if(report) -i.e., bottom of the transmit success/fail IF-ELSE block

  delay(1000);                                                  // Introduce a bit of delay each loop cycle.
  getVolts();                                                   // This is the purpose of this sketch - read voltage and load the tx payload struct.
} // data streaming loop



void getVolts() {
  uint32_t iVolts;
  iVolts = analogRead(analogPin);
  txPayload.capacitance = 0;                        // calculated capacitance
  txPayload.chargeTime = iVolts;                    // The volts on pin #6, as converted by ATTiny into integer relative value
  memcpy(txPayload.units, "Vts", 3);                // Measurement units
  memcpy(txPayload.statusText, "Volts   ", 8);      // status message
}


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
