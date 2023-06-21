
/*  This is my Milestone #7 : Measure capacitance and transmit to nRF24 on RPi. 
 *  The matching "receiver" code on the RPi side is: MS-07_CapDataReceive_01.cpp.
 *  
 *  10/03/2022: UNSOLVED PUZZLE | When I uncomment the pin setups for capacitor charge/discharge, with
 *              no other changes, then this sketch appears to be acting as if the cap measure function
 *              is the void loop() function. This is occurring when the first line under the 
 *              comment: "/* Init capacitance measurement pins." is uncommented. Uncommenting the other
 *              three lines is fine, the code works just fine with those.
 *                  - The failure mode for the above is that both LEDs are lit, stay lit for the delay(5000)
 *                    I have at the start of setup(); then there is a very brief (sub-second) blinking out
 *                    of both LEDs simultanously, then they both light up again....repeat endlessly.
 *                    I get NO nRF24 transmissions over to the RPi (the RPi times out, as designed, after
 *                    the 12 seconds I coded it to do when no packets received from the tiny84).
 *                      * My guess is that this LED pattern is telling me that the CPU is continuously
 *                        rebooting.
 *                  - I think I have it. I initilized both the charge and discharge pins to an OUTPUT
 *                    value of LOW. When I, instead, initilize the charge pin to a value of HIGH the
 *                    sketch runs OK. So my thought is that setting both pins at once to LOW creates
 *                    an internally seen short-circut condition - which the CPU trapped and self-initated
 *                    a reset.
 *                      * BUT this then begs the question as to why this would be so. You can definitely
 *                        have two pins set to LOW at once. And, hmmm..., don't have I have to set them
 *                        both to LOW in order to discharge the cap? So I'm going to have to go back to
 *                        the cap measurement example and double check everything; including my
 *                        breadboard wiring and pin assignments.
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
  digitalWrite(errorLedPin, HIGH);    // LEDs on to confirm we are here and LEDs are working
  digitalWrite(greenLedPin, HIGH);
  delay(5000);                        // Pause for human to verify LEDs & state at this point.

        /* Init pines needed to manage & measure the capacitor. */
  pinMode(chargePin, OUTPUT);         // set chargePin to output      |<- !FAILS IF UNCOMMENTED! (VER 00-05)
  //digitalWrite(chargePin, LOW);     // Set chargePin LOW            |<- CANNOT HAVE BOTH PINS LOW AT SAME TIME! (VER 00-02)
  digitalWrite(chargePin, HIGH);      // Set chargePin HIGH           |<- WORKS OK UNCOMMENTED   (VER 00-06)
  pinMode(dischargePin, OUTPUT);      // Set discharge pin to output  |<- WORKS OK UNCOMMENTED   (VER 00-03)
  digitalWrite(dischargePin, LOW);    // Init discharge pin to LOW    |<- WORKS OK UNCOMMENTED   (VER 00-04)

      /* Init the nRF24 radio on the SPI bus. If this fails, go into an infinite
       * loop, stuck in that error condition. If this does happen, signal that
       * with the LEDs. */
  if (!radio.begin()) {
    while (1) {
        errorLED (9);                 // Call this error #9.
      } // end while
  }

  radio.setPALevel(RF24_PA_LOW);      // RF24_PA_MAX is default, set low while in close proximity on my bench
  radio.enableDynamicPayloads();      // Must set to use ACK payloads, which I wish to do for now.
  radio.enableAckPayload();           // Ack packets have no payload by default. Must enable on all nodes (TX & RX).

      /*    Original source code comment: "set the TX address of the RX node into the TX pipe. 
       * Always uses pipe 0 for transmit."
       *    Jeff Note: Am a bit confused by this. I haven't yet backtracked this to work
       * out what it's really saying. But my guess is that it's saying we need to load an
       * address into the transmit pipe that the receiving node will recognize and accept
       * as a valid "one of us." Anyway, just leaving this as the original demo code has it. */
  radio.openWritingPipe(address[radioNumber]);

      /*    Original Source Comment: "set the RX address of the TX node into a RX pipe."
       *    Jeff: An interesting way to specify the row '0' in the address array.
       * I guess this approach guarantees that transmit and receive addresses
       * will always be 'the opposite' of each other. */
  radio.openReadingPipe(1, address[!radioNumber]);  // using pipe 1 to receive.

      /* Load the txPayload structure with initial defaults. */
  txPayload.capacitance = 0;                        // Calculated capacitance
  txPayload.chargeTime = 0;                         // Time it took for capacitor to charge.
  memcpy(txPayload.units, "---", 3);                // Capacitance units
  txPayload.ctSuccess = 0;                          // Running count of no-error transmissions
  txPayload.ctErrors = 0;                           // Running count of transmissions errors
  memcpy(txPayload.statusText, "ver 00-06  ", 11);  // Status message, helpful for debgging for now.

  radio.stopListening();                            // Put radio in transmit mode.

} // END setup()



// ==== Transmission Loop
/*************************************************************************************************
 *    OK, we've setup the ATTiny84 and the radio chip. Now go into an infinite loop, continuously 
 * transmitting data, seeking a valid acknowlegement of receipt; then repeat....
 *    NOTE: I removed the else portion of the original demo code pertaining to receive nodes as 
 * here I am only allowing the ATTiny to be a data/sensor transmitter; with the RPi to be the 
 * receiver. 
 */
void loop() {
  digitalWrite(greenLedPin, LOW);                               // Added on 10/3 to help with the issue debugging: mGreen LED off Red on to signal we are in loop() function.
  digitalWrite(errorLedPin, HIGH);
  delay(3000);                                                  // Delay to see the LED signal.

  bool report = radio.write(&txPayload, sizeof(txPayload));     // attempt transmit - obtaining 'success' or 'timeout' result


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
  handleCapacitor();                                                   // This is the purpose of this sketch - read voltage and load the tx payload struct.
} // data streaming loop




// ==== Manage the Capacitor
/*************************************************************************************************
 *    In order to not block the CPU I will use a state variable, and a SWITCH structure, to 
 * call out from the main loop(), check the cap state, take appropriate action, populate the
 * data Tx structure, then return back to the loop() function where I'll transmit the in-flight
 * data (i.e., capacitor status) over to the Raspberry Pi.
 *    At the moment, hoever, I simply starting from my known good prior sketch of simply reading
 * the voltage off the analog pin on which the cap will be connected. Once I prove this is 
 * working I will proceed to the cap charge / measure / discharge cycle.
 * 
 * 10/03/2022 --> ISSUE: Something odd going on here. As soon as I uncomment the lines of code to 
 *    initilize any of the four digital pins I am using to charge / discharge the capacitor then the 
 *    sketch acts as if this function is the loop() funcion. Meaning that it keeps looping through 
 *    this function and never returning to the actual loop() function. Yet, with NO OTHER changes, 
 *    if I comment out those lines the sketch works perfectly.
 */
void handleCapacitor() {
  uint32_t iVolts;

  digitalWrite(greenLedPin, HIGH);                  // Added on 10/3 to help with the issue debugging: Green LED on, Red off to signal we are in this function.
  digitalWrite(errorLedPin, LOW);
  delay(3000);                                      // Delay to see the LED signal.

  iVolts = analogRead(analogPin);
  txPayload.capacitance = (float)iVolts;            // The volts on pin #6, as converted by ATTiny into integer relative value
  memcpy(txPayload.units, "Vts", 3);                // Measurement units - indicate we are providing volts, not capacitance at this point.
}




// ==== Error Signalling
/*************************************************************************************************
 *  Can use this function to have the RED LED signal an error code.
*/
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
