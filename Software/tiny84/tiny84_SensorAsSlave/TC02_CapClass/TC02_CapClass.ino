//Moisture Sensor Project - ATTiny84 Code
// TEST TEST - This sketch is the test case for the pF capacitance measurement class.

// Dispatcher simply initiates a new measurement request after about a 30 second
// delay from the prior measurement. HeartBeat should be running continuously in
// the background.

// NOTE: In this test case I am using the ErrorFlash class to flash
// out an 'error-3' after each measurement has completed, just as way to signal
// that we have a successful measurement since in this test case we are not
// using the radio to send data over to a Raspberry Pi; thus we can't otherwise
// see anything.

#define VERSION "TC02_071823"

// ==== PULL IN REQUIRED LIBRARIES ===============================================================
  #include "HeartBeat.h"
  #include "ErrorFlash.h"
  #include "CapSensor.h"
  #include "Dispatcher.h"

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
  Dispatcher dispatcher;                                  // Instantiate a Dispatcher object.
  CapSensor capSensor(CAP_VOLTREAD_PIN, CAP_CHARGE_PIN);  // Instantiate a CapSensor object.

// END Declare Global Variables



// ==== SETUP PROCEDURE===========================================================================
void setup() {

  heartBeat.begin();                        // Start the heartbeat LED. Keep it lit for entire setup() process.
  errorFlash.begin();                       // Start the error reporting-out-by-flashing-LED process.
  dispatcher.begin();
  capSensor.setup();

} // END setup()

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


