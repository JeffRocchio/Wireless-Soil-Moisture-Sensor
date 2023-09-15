//Moisture Sensor Project - ATTiny84 Code
// TEST TEST - This sketch is an initial test case for proving that I can
// transmit data out from the nRF24 radio chip using my 'non-blocking'
// architectural design.

// Dispatcher checks for radio error condition and keeps retrying radio.begin()
// until it sees success in radio activation. It will use the errorFlash class
// report out radio errors.

// If we have an active radio connected then we simply transmit out a fake
// capacitor value once per second.

// My goal with this test case is only to prove my RadioComms class structure 
// design can work properly.

// Conditions for Test Case Completion:
//   - I see the transmissions over on the RPi.
//         SUCCESS on 9/15/2023. See RPi terminal screenshot TC03_TransmitPOC_terminalDisplay.jpg

#define VERSION "TC03_091523"

// ==== PULL IN REQUIRED LIBRARIES ===============================================================
  #include "HeartBeat.h"
  #include "ErrorFlash.h"
  #include "CapSensor.h"
  #include "RadioComms.h"
  #include "Dispatcher.h"

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
  ErrorFlash errorFlash(LED_ERROR);                       // Instantiate an error reporting object.
  HeartBeat heartBeat(LED_GREEN);                         // Instantiate a HeartBeat object.
  RadioComms radio(CE_PIN, CSN_PIN);                      // instantiate my nRF24L01 transceiver wrapper object.
  CapSensor capSensor(CAP_VOLTREAD_PIN, CAP_CHARGE_PIN);  // Instantiate a CapSensor object.
  Dispatcher dispatcher;                                  // Instantiate the Dispatcher object.

// END Declare Global Variables



// ==== SETUP PROCEDURE===========================================================================
void setup() {

  heartBeat.begin();                          // Start the heartbeat LED. Keep it lit for entire setup() process.
  errorFlash.begin();                         // Start the error reporting-out-by-flashing-LED process.
  capSensor.setup();
  if (!radio.setup()) errorFlash.setError(9); // If radio chip isn't there, set errorID 9.
  dispatcher.begin();

} // END setup()



// ==== Processor Running Loop
/*************************************************************************************************
 *    Using the 'non-blocking' classes/objects I've created, go into the standard Arduino 
 * infinitely running loop, continuously calling the various dispatch and update functions to
 * share processor time between the various tasks.
 */
void loop() {

  dispatcher.dispatch();
  heartBeat.update();
  errorFlash.update();

} // END loop()



//*************************************************************************************************
// FOOTNOTES
//*************************************************************************************************

   /*   1. When compiling you might see this warning: #warning "This is the CLOCKWISE pin 
      mapping - make sure you're using the pinout diagram with the pins in clockwise order." 
        As long as all pin references are in the SpenceKonde attiny core recommended form then ignore
      the warning. In fact, no matter which core variant you use you'll get a warning either way.
      Pin references should be of the form: "PIN_PA#" or PIN_PB#," for digital pins. For the 
      analog pins use the analog channel number as shown on the pinout diagram. E.g., use 
      'A7' for ADC7, physical pin #6; 'A0' for ADC0, physical pin 13. As long as you use this 
      pin referencing system the clock/counter-clock-wise pin ordering thing is irrelevant.
    */

   /*   2. Notes on return value from [bool report = radio.write(&txPayload, sizeof(txPayload));]
      FROM the RF24.h file, documentation on the return value of this function: Returns`true` 
      if the payload was delivered successfully and an acknowledgement (ACK packet) was received. 
      If auto-ack is disabled, then any attempt to transmit will also return true (even if the 
      payload was not received). Returns 'false` if the payload was sent but was not 
      acknowledged with an ACK packet. This condition can only be reported if the auto-ack 
      feature is on. 
    */

