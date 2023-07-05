//Moisture Sensor Project - ATTiny84 Code

#define VERSION "SEN_070523"
/*    DESCRIPTION: Arduino sketch to make the ATTiny84 MCU serve as a Slave sensor, with 
 * a Raspberry Pi as the Master - i.e., sensor server.
 *   
 *      07/05/2023: First attempt to code up reasonable sensor behavior per the Design Notes in
 *  milestone #12.
 *
 */

// ==== PULL IN REQUIRED LIBRARIES ===============================================================
  #include <SPI.h>
  #include "RF24.h"
  #include "HeartBeat.h"

// ==== PROTOTYPES FOR CLASSES AND FUNCTIONS DEFINED IN THIS SOURCE FILE =========================
// END Prototypes



// ==== PIN REFERENCES. (For ATTiny84 Pin Reference guidance see footnote #1 at bottom of this file.)

    /* Hard-wired LEDs */
  #define LED_GREEN PIN_PA1  // Physical pin #12 The pin that a green led is wired to
  #define LED_ERROR PIN_PB2  // Physical pin #5. The pin that a red led is wired to

    /* Hard-wired pins of nRF24 chip to the ATTiny84 */
  #define CE_PIN PIN_PA2
  #define CSN_PIN PIN_PA3

    /* Hard-wired pins for capacitance measurement. */
  #define CAP_VOLTREAD_PIN A0        // Physical pin#13 - analog 'channel 0' for measuring voltage
  #define CAP_CHARGE_PIN   PIN_PB1   // Physical pin#3 - pin to charge the capacitor - connected to one end of the charging resistor

// END Pin References

// DECLARE GLOBAL VARIABLES =======================================================================

    /* Global Classes (objects) */
  HeartBeat heartBeat(LED_GREEN);  // Instantiate a HeartBeat object.
  RF24 radio(CE_PIN, CSN_PIN);     // instantiate an object for the nRF24L01 transceiver.

    /* Global variables needed For capacitance measurement */
  const float IN_STRAY_CAP_TO_GND = 24.48;  // Stray capacitance, used as 'C1' in schematic. 
  const float IN_EXTRA_CAP_TO_GND = 0.0;    // Extra capacitance can be added to measure higher values.
  const float IN_CAP_TO_GND  = IN_STRAY_CAP_TO_GND + IN_EXTRA_CAP_TO_GND;
  const int MAX_ADC_VALUE = 1023;

  /* Other Global Variables */
    /*    Being sloppy here as I experiment and learn, relative to relying heavily on
    * globals. Although, my mindset clearly comes from desktop/server coding. Perhaps efficient
    * coding on an MCU is paradigmatically different vis-a-vis globals? */

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

// END Declare Global Variables



// ==== SETUP PROCEDURE===========================================================================
void setup() {

  heartBeat.begin();                        // Start the heartbeat LED. Keep it lit for entire setup() process.

  pinMode(LED_ERROR, OUTPUT);


    /*    Init capacitance measurement pins. */
  pinMode(CAP_CHARGE_PIN, OUTPUT);
  digitalWrite(CAP_CHARGE_PIN, LOW);       // Start with 0 volts on charge pin.
  pinMode(CAP_VOLTREAD_PIN, OUTPUT);
  digitalWrite(CAP_VOLTREAD_PIN, LOW);     // Start with the analog pin low, 0 volts.

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
 */
void loop() {
  unsigned long start_timer = micros();                         // time the Tx cycle - obtain start time
  bool report = radio.write(&txPayload, sizeof(txPayload));     // attempt transmit - obtaining 'success' or 'timeout' result
                                                                /*    FROM the RF24.h file, documentation on the return value of 
                                                                 *  this function: Returns`true` if the payload was delivered 
                                                                 *  successfully and an acknowledgement (ACK packet) was received. 
                                                                 *  If auto-ack is disabled, then any attempt to transmit will 
                                                                 *  also return true (even if the payload was not received).
                                                                 *  Returns 'false` if the payload was sent but was not 
                                                                 *  acknowledged with an ACK packet. This condition can only 
                                                                 *  be reported if the auto-ack featureis on. */

  unsigned long end_timer = micros();                           // time the Tx cycle - obtain end time




  if (report) {               // IF 'report' is true than nRF24 chip sent the payload out & got an ack back
                                        /* report is TRUE: meaning the nRF24 chip sent the payload out & got an ack back. 
                                         * Since we have full success, clear any prior error conditions. */
      digitalWrite(LED_ERROR, LOW);
      memcpy(txPayload.statusText, "Success    ", 11);       // clear error status text
      txPayload.ctSuccess = txPayload.ctSuccess + 1;         // we have a Tx/ack success, increment the success tracking counter.

        /* See if we got a receipt ACK payload back from the receiver (this would be an auto-ack payload).
         * Declare variable to hold the pipe number that received the acknowledgement payload. */
      uint8_t pipe;
      if (radio.available(&pipe)) {                          // ACK if-then block. Test for ack payload, which will also return the pipe number that received it.
        radio.read(&rxAckPayload, sizeof(rxAckPayload));     // get incoming ACK payload. Tho doing nothing with it on this side of the conversation.
        delay(20);                                           // brief pause, then fall through to bottom of loop and keep transmitting.
      } else {
                            /*  In principle we should never get here. 'report' is TRUE, so the NRF24 is saying it has received 
                             *  an ACK. BUT, maybe there can be cases where we do can ACK back, yet no data in the ACK, so no 
                             *  payload? If that condition is possible, then this block traps it. */
        errorLED (2);                                        // Call this error #2.
        memcpy(txPayload.statusText, "ERROR 02   ", 11);     // Set text to send back to RPi to show error.
        txPayload.ctErrors++;                                // we have a Tx/ack failure, increment the errors tracking counter.
      }

  } else {
                            /*  NRF24 chip reports a transmission failure - almost certainly due to not getting an ACK back from 
                             *  the RPi. Report, then fall through and keep trying. */
    /* errorLED (3); <- Let's try skipping the delay this reporting introduces and see what happens */ // Call this error #3.
    digitalWrite(LED_ERROR, HIGH);                           // Instead just show LED red to signal an error
    memcpy(txPayload.statusText, "ERROR 03   ", 11);
    txPayload.ctErrors = txPayload.ctErrors + 1;             // we have a Tx/ack failure, increment the errors tracking counter.
  } // if(report) -i.e., bottom of the transmit success/fail IF-ELSE block


  delay(500);                                                // Introduce a bit of delay each loop cycle, tho this will mess with timing measurement, so in the end this must go away.
  capacitorMeasurement(&txPayload);                          // Manage and measure the capacitor.

  heartBeat.update();

} // END loop()



// ==== Capacitor Functions

/*************************************************************************************************
 *    Measure capacitance using Jon's approach. Put the measurement data into the payload
 * structure and return. 
 *    The capacitor under test is connected between CAP_CHARGE_PIN and CAP_VOLTREAD_PIN.
 */
void capacitorMeasurement(TxPayloadStruct * pLoad) {
  unsigned long startTime = millis();
  unsigned long elapsedTime;

  pinMode(CAP_VOLTREAD_PIN, INPUT);        // Get ready to measure voltage.
  digitalWrite(CAP_CHARGE_PIN, HIGH);      // Send voltage pulse to cap under test.
  int val = analogRead(CAP_VOLTREAD_PIN);  // Read voltage at point between 'C1' and C-test.

      //Clear everything for next measurement
  digitalWrite(CAP_CHARGE_PIN, LOW);       // Remove voltage from caps.
  pinMode(CAP_VOLTREAD_PIN, OUTPUT);       // ?? - does this discharge C-test to ground??

  elapsedTime= millis() - startTime;       // Capture elapsed time out of curiosity.

                                           //Calculate and store result
  float capacitance = (float)val * IN_CAP_TO_GND / (float)(MAX_ADC_VALUE - val);

  //memcpy(pLoad->statusText, "Measurement", 11);
  pLoad->chargeTime = elapsedTime;
  memcpy(pLoad->units, "pF ", 3);
  pLoad->capacitance = capacitance;
  //memcpy(pLoad->statusText, "07-04      ", 11);
  memcpy(pLoad->statusText, VERSION, size_t(VERSION));
}


void errorLED (int errorNo) {
  digitalWrite(LED_ERROR, HIGH);          // Show RED on for a bit before counting out the error number.
  delay(1500);
  digitalWrite(LED_ERROR, LOW);
  for (int i=errorNo; i>0; i--) {               // Flash LED to count the error number.
    delay(500);
    digitalWrite(LED_ERROR, HIGH);
    delay(500);
    digitalWrite(LED_ERROR, LOW);
  }
  delay(500);
  digitalWrite(LED_ERROR, HIGH);          // Show RED on for a bit before counting out the error number.
} // errorLED


//*************************************************************************************************
// FOOTNOTES
//*************************************************************************************************

   /*   1. When compiling you might see this warning: #warning "This is the CLOCKWISE pin 
      mapping - make sure you're using the pinout diagram with the pins in clockwise order" <- As 
      long as all pin references are in the SpenceKonde attiny core recommended form then ignore
      the warning. In fact, no matter which core variant you use you'll get a warning either way.
      Pin references should be of the form: "PIN_PA#" or PIN_PB#," for digital pins. For the 
      analog pins use the analog channel number as shown on the pinout diagram. E.g., use 
      'A7' for ADC7, physical pin #6; 'A0' for ADC0, physical pin 13. 
   */

