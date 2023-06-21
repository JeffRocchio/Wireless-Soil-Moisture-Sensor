/*   ATTiny84 Moisture Sensor Project
 *   -----------------------------------------------------------------------------------------------------
 *   
 *  Arduino IDE 'sketch' for the ATTiny84 MCU prototype capitance moisture sensor.
 *   
 *  This project is under local git version control, in folder:
 *  /home/jroc/Dropbox/projects/MoistureSensor/CapSensor
 *  Refer to git for version history and associated comments.
 *
 *      06/20/2023: Had to change some pin mappings. I built a version of the sensor circut onto a
 *  perfBoard. But in doing so I mixed up one of the pins for the cap sensor with the pin for the
 *  Red LED. So I needed to change those pin definitions in here in the code.
 *
 *      06/05/2023: Success!! No change to code from 6/3 below; just this note that this code 
 *  worked. It is licking away, showing, after about 15 minutes, 1768 successful transmissions of 
 *  the cap value for my home-made cap sensor ranging between 8-9 pF. It also successfully 
 *  reported the value of the 10pF cap (showing it as 8xpF, close to what my multimeter also 
 *  measured). NOTE that I have not done anything to calibrate the stray capacitiance constant, 
 *  which I left at the value of 24.48 that was in the original author's code.
 *
 *      06/03/2023: Initial 'sketch' to implement approach for measuring pico-farad level
 *  capacitance. This approach uses the methodology described by the article "Capacitance measurement 
 *  with the Arduino Uno" by Jonathan Nethercott
 *  @ https://wordpress.codewrite.co.uk/pic/2014/01/21/cap-meter-with-arduino-uno/
 *      NOTE tho that the above post is from 2014 and Jonathan does not appear to be active
 *  any longer. However, this approach of his is referenced, and used, all over the internet and on
 *  YouTube.
 *      To be safe, I made a pdf of the blog post on this approach and saved it into the docs and
 *  references folder as Pico-farad-cap-measurement-with-Arduino.pdf.
 *   
 *  A detailed log of this project is being recorded in my Evernote account, in the notebook
 *  "Moisture Sensors."
 */

// ==== Pull in Required Libraries
#include <SPI.h>
#include "RF24.h"

// ==== ATTiny84 Pin Reference Definitions
/*************************************************************************************************
 *    Define references to the logical pins we'll be using.
 *    NOTE: I am using the PIN references per SpenceKonde attiny core recommendation. The
 * pin references in the original demo code did not work for the tiny84. After working out the
 * whole Arduino core thing, and how the logical-physical pin mappings are actually defined by
 * the core's author, I went to the SpenceKonde github and inspected his documentation, in
 * which he states that for digital pins, use the 'logical' port numbering schema - 
 *  e.g., "PIN_PA#" or PIN_PB#." For the analog pins use the analog channel number as shown on
 *  the pinout diagram. E.g., us 'A7' for ADC7, physical pin #6; 'A0' for ADC0, physical pin 13.
 *    BUT note that I am currently using core 1.5.2; for version 2.0.0 of the core the pin
 *  reference scheme is being updated to a more commonly used schema. So watch for this.
 */

  /* Logical pin names representing connection of ATTiny84 to nRF24 chip */
#define CE_PIN PIN_PA2
#define CSN_PIN PIN_PA3

  /* LEDs to signal status. */
const int greenLedPin = PIN_PA1;  // Physical pin #12 The pin that a green led is wired to
const int errorLedPin = PIN_PB2;  // Physical pin #5. The pin that a red led is wired to

  /* For capacitance measurement */
const float IN_STRAY_CAP_TO_GND = 24.48;  // Stray capacitance, used as 'C1' in schematic. 
const float IN_EXTRA_CAP_TO_GND = 0.0;    // Extra capacitance can be added to measure higher values.
const float IN_CAP_TO_GND  = IN_STRAY_CAP_TO_GND + IN_EXTRA_CAP_TO_GND;
const int MAX_ADC_VALUE = 1023;

#define voltReadPin    A0                 // Physical pin#13 - analog 'channel 0' for measuring voltage
#define chargePin      PIN_PB1            // Physical pin#3 - pin to charge the capacitor - connected to one end of the charging resistor
//#define dischargePin   PIN_PB0            // NOT USED FOR pF level measurements. [Physical pin#2 - pin to discharge the capacitor]
#define resistorValue  3000000.0F         // 3 Meg Ohm. For accurate calculation must match actual charging resistor value

// ==== Global Variables
/*************************************************************************************************
 *    Being sloppy here as I experiment and learn, relative to relying heavily on
 * globals. Although, my mindset clearly comes from desktop/server coding. Perhaps efficient
 * coding on an MCU is paradigmatically different vis-a-vis globals? */

  /* Some needed variables for cap measurement. */
unsigned long startTime;
unsigned long elapsedTime;
float microFarads;                // floating point to preserve precision during calculations
float nanoFarads;

  /* nRF24: Declare, and instantiate, an object for the nRF24L01 transceiver. */
RF24 radio(CE_PIN, CSN_PIN);

  /* nRF24:    
   *    Declare an array to hold node addresses. In this case we have only two nodes -
   * the RPi and this node, the tiny84.
   *    NOTE from original source: "It is very helpful to think of an address as a path 
   * instead of as an identifying device destination."
   *    Jeff Comment: I don't really understand how these "addresses" are used, this 
   * needs further rabbit-holing my me at some point. */
uint8_t address[][6] = {"1Node", "2Node"};

  /* nRF24:    
   *    Comment in original source: "To use different addresses on a pair of radios
   * we need a variable to uniquely identify which address this radio will use to 
   * transmit."
   *    This variable is used later to de-reference an entry in the address[][] array
   * declared above. */
bool radioNumber = 1;             // 0 uses address[0] to transmit, 1 uses address[1] to transmit

  /* nRF24: Used to control whether this node is sending or receiving. */
bool role = true;                 // true = TX role, false = RX role

  /* nRF24:     
   *    Define, and declare, a struct to hold the transmit payload.
   *    NOTE: This struct must match the 'receive' payload structure declared on
   * the RPi side. */
struct TxPayloadStruct {
  float capacitance;
  uint32_t chargeTime;            // Time it took for capacitor to charge.
  char units[4];                  // nFD, mFD, FD
  uint32_t ctSuccess;             // count of success Tx attempts tiny84 has seen since boot
  uint32_t ctErrors;              // count of Tx errors tiny84 saw since last successful transmit
  char statusText[12];            // For use in debugging. Be sure there is space for a NULL terminating char

};
TxPayloadStruct txPayload;

  /* nRF24:     
   *    Define, and declare, a struct to hold the acknowledgement payload.
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
  digitalWrite(errorLedPin, HIGH);    // Turn LEDs on the verify operation and see where we are at this point
  digitalWrite(greenLedPin, HIGH);
  delay(3000);                        // Pause for humans to verify LEDs at this point.

    /*    Init capacitance measurement pins. */
  pinMode(chargePin, OUTPUT);
  digitalWrite(chargePin, LOW);       // Start with 0 volts on charge pin.
  pinMode(voltReadPin, OUTPUT);
  digitalWrite(voltReadPin, LOW);     // Start with the analog pin low, 0 volts.

    /* Init the nRF24 radio on the SPI bus. If this fails, go into an infinite
     * loop, stuck in that error condition. If this does happen, signal that
     * with the LEDs. */
  if (!radio.begin()) {
    while (1) {
        errorLED (9);                 // Call this error #9.
      } // end while
  }

    /* Set the PA Level low to try preventing power supply related problems
     * because these examples are likely run with nodes in close proximity to
     * each other. */
  radio.setPALevel(RF24_PA_LOW);      // RF24_PA_MAX is default.

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
  unsigned long start_timer = micros();                         // time the Tx cycle - obtain start time
  bool report = radio.write(&txPayload, sizeof(txPayload));     // attempt transmit - obtaining 'success' or 'timeout' result
  unsigned long end_timer = micros();                           // time the Tx cycle - obtain end time

  if (report) { // IF report is true than nRF24 chip sent the payload out & got an ack back
        /* report is TRUE: meaning the nRF24 chip sent the payload out & got an ack back. 
         * Since we have full success, clear any prior error conditions. */
      digitalWrite(errorLedPin, LOW);
      //memcpy(txPayload.statusText, "Success    ", 11);        // clear error status text
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
        //memcpy(txPayload.statusText, "ERROR 02   ", 11);      // Set text to send back to RPi to show error.
        txPayload.ctErrors++;                                   // we have a Tx/ack failure, increment the errors tracking counter.
      } // ACK i.e., bottom of the test to see if we got an ACK back

  } else {
    /* Transmission failed. Report, then fall through and keep trying. */
    /* errorLED (3); <- Let's try skipping the delay this reporting introduces and see what happens */ // Call this error #3.
    digitalWrite(errorLedPin, HIGH);                            // Instead just show LED red to signal an error
    //memcpy(txPayload.statusText, "ERROR 03   ", 11);
    txPayload.ctErrors = txPayload.ctErrors + 1;                // we have a Tx/ack failure, increment the errors tracking counter.
  } // if(report) -i.e., bottom of the transmit success/fail IF-ELSE block


  delay(500);                                                   // Introduce a bit of delay each loop cycle, tho this will mess with timing measurement, so in the end this must go away.
  capacitorMeasurement(&txPayload);                            // Manage and measure the capacitor.

} // data streaming loop



// ==== Capacitor Functions

/*************************************************************************************************
 *    Measure capacitance using Jon's approach. Put the measurement data into the payload
 * structure and return. 
 *    The capacitor under test is connected between chargePin and voltReadPin.
 */
void capacitorMeasurement(TxPayloadStruct * pLoad) {
  startTime = millis();
  pinMode(voltReadPin, INPUT);        // Get ready to measure voltage.
  digitalWrite(chargePin, HIGH);      // Send voltage pulse to cap under test.
  int val = analogRead(voltReadPin);  // Read voltage at point between 'C1' and C-test.

      //Clear everything for next measurement
  digitalWrite(chargePin, LOW);       // Remove voltage from caps.
  pinMode(voltReadPin, OUTPUT);       // ?? - does this discharge C-test to ground??

  elapsedTime= millis() - startTime;  // Capture elapsed time out of curiosity.
  digitalWrite(greenLedPin, LOW);     // For now let's use Green LED on only while CAP is being charged.

      //Calculate and store result
  float capacitance = (float)val * IN_CAP_TO_GND / (float)(MAX_ADC_VALUE - val);

  memcpy(pLoad->statusText, "Measurement", 11);
  pLoad->chargeTime = elapsedTime;
  memcpy(pLoad->units, "pF ", 3);
  pLoad->capacitance = capacitance;
  memcpy(pLoad->statusText, "pF 06/20 a ", 11);
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
