/*   ATTiny84 Moisture Sensor Project
 *   -----------------------------------------------------------------------------------------------------
 *   
 *  Arduino IDE 'sketch' for the ATTiny84 MCU prototype capitance moisture sensor.
 *   
 *  This project is under local git version control, in folder:
 *  /home/jroc/Dropbox/projects/MoistureSensor/CapSensor
 *  Refer to git for version history and associated comments.
 *
 *      06/03/2023: Dispite the comment of 10/06/2022 about not using physical pin #6, due to the 
 *  pin blown out issue I had, I have noted that I am actually now using pin #6 for the voltage
 *  read operation - that is, analog pin A7 = physical pin#6. I did move to physical pin #2 for
 *  the discharge pin, but apparently using pin #6 for the ADC voltage read. Just noting this
 *  here as I am about to morph this code base into the method to read pF capacitance levels; and to
 *  get started with that I am intending to use the ATTiny part with blown pin #6 just in case I
 *  blow something else out I'll have at least damaged an already damaged part.
 *   
 *      03/03/2023: Changed resisterValue trying to get the ATTiny to be able to measure pico-farad 
 *  range.
 *  
 *      10/06/2022: Changed cap dischargePin to work around the original pin (physical pin #6) that I had
 *  blown out. See the Milestone #7 notes in Evernote, for the date 10/2 and 10/3. Also changed out the
 *  charging resistor for a higher value one so I could better watching the charging process.
 *  
 *      10/04/2022: Initial 'sketch' to measure a statically connected capacitor. Goal with this version is
 *  simply to get the program logic correct. I am not expected everything to work or be correct in this
 *  initial go at it.
 *  
 *  For the approach on capacitance measurement --: 
 *  RCTiming_capacitance_meter || Paul Badger 2008
 *  @ https://www.arduino.cc/en/Tutorial/Foundations/CapacitanceMeter
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
 * which he states to use the 'logical' port numbering schema - e.g., "PIN_PA#" or PIN_PB#."
 */

  /* Logical pin names representing connection of ATTiny84 to nRF24 chip */
#define CE_PIN PIN_PA2
#define CSN_PIN PIN_PA3

  /* LEDs to signal status. */
const int greenLedPin = PIN_PA1;  // the pin that a green led is wired to
const int errorLedPin = PIN_PB1;  // the pin that a red led is wired to

  /* For capacitance measurement */
    //A7 as the pin reference may no longer work. SpenceKonde core 2.0 has a note in it's 
    //FAQ which says: "Previously (prior to 2.0.0) numbers were treated as analog channel 
    //numbers. Now they are treated as the digital pin number"
    //However, I am currently using KpenceKonde core release 1.5.2, 
    //not 2.0 as of yet. So this is something to watch.
#define analogPin      A7         // Physical pin#6 - analog 'channel 7' for measuring capacitor voltage
#define chargePin      PIN_PB2    // Physical pin#5 - pin to charge the capacitor - connected to one end of the charging resistor
#define dischargePin   PIN_PB0    // Physical pin#2 - pin to discharge the capacitor
#define resistorValue  3000000.0F // 3 Meg Ohm. For accurate calculation must match actual charging resistor value


// ==== Global Variables
/*************************************************************************************************
 *    Admidittly being sloppy here as I experiment and learn, relative to relying heavily on
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
  digitalWrite(errorLedPin, HIGH);  // Turn LEDs on the verify operation and see where we are at this point
  digitalWrite(greenLedPin, HIGH);
  delay(3000);                      // Pause for humans to verify LEDs at this point.

    /*    Init capacitance measurement pins. Start with cap discharging. */
  pinMode(chargePin, INPUT);        // INPUT mode to turn off charging
  digitalWrite(chargePin, LOW);     // Not sure if this matters while in INPUT mode ??
  pinMode(dischargePin, OUTPUT);    // OUTPUT mode to discharge cap
  digitalWrite(dischargePin, LOW);  // LOW to shunt cap, through resistor, to ground for discharging

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
  capacitorStateMachine(&txPayload);                            // Manage and measure the capacitor.

} // data streaming loop



// ==== Capacitor Functions
/*************************************************************************************************
 *    I want to be able to see info as the capacitor is being charged and discharged. So I don't
 * want a capacitor measurement function to block the transmission loop.
 * Therefore, I am designing this function in an object-like manner - meaning that it will have
 * internal state so I can control what action to take based on it's current state, and 
 * immediately go back to the transmission loop so I can see on the RPi console what is going
 * on.
 */
void capacitorStateMachine(TxPayloadStruct * pLoad) {
  static int state = 0;
  uint32_t iVolts;

  switch(state) {

   case 0 :                                             // Cap has been discharged and we can being measurement.
      memcpy(pLoad->statusText, "Charging...", 11);
      iVolts = analogRead(analogPin);
      pLoad->capacitance = (float)iVolts;                       // Read volts on pin #6, as converted by ATTiny into integer relative value
      memcpy(pLoad->units, "Vlt", 3);
      pinMode(dischargePin, INPUT);                             // INPUT mode to switch discharge path off
      pinMode(chargePin, OUTPUT);                               // OUTPUT mode to switch charging current path on
      digitalWrite(chargePin, HIGH);                            // HIGH to apply 3.3 volts down charging path to capacitor
      startTime = millis();                                     // Start charging time counter
      state=1;                                                  // Transition to state #1: "Charging in Process"
      pLoad->chargeTime = state;
      digitalWrite(greenLedPin, HIGH);
      break;
  
   case 1 :                                             // Cap is in process of charging, or has reached charge point.
      iVolts = analogRead(analogPin);
      pLoad->capacitance = (float)iVolts;                       // Read volts on pin #6, as converted by ATTiny into integer relative value
      if(analogRead(analogPin) > 648) {                         // 647 is 63.2% of 1023, which corresponds to full-scale voltage
        elapsedTime= millis() - startTime;
        digitalWrite(greenLedPin, LOW);                         // For now let's use Green LED on only while CAP is being charged.
        microFarads = ((float)elapsedTime/resistorValue)*1000;  // Convert milliseconds to seconds ( 10^-3 ) and Farads to microFarads ( 10^6 ),  net 10^3 (1000)
        memcpy(pLoad->statusText, "Measurement", 11);
        pLoad->chargeTime = elapsedTime;                        // Time it took for capacitor to charge.
        memcpy(pLoad->units, "mFD", 3);
        pLoad->capacitance = microFarads;
        if (microFarads <= 1) {
                /*  if value is smaller than one microFarad, 
                    convert to nanoFarads (10^-9 Farad). */
          nanoFarads = microFarads * 1000.0;                    // multiply by 1000 to convert to nanoFarads (10^-9 Farads)
          memcpy(pLoad->units, "nFD", 3);
          pLoad->capacitance = nanoFarads;
        }
        state=2;                                               // Reached charge point, transition to next state
      }
      break;

   case 2 :                                             // Cap is charged, initiate discharge cycle.
      delay(3000);                                              // Introduce a delay to see the cap measurement over on the RPi console
      iVolts = analogRead(analogPin);
      pLoad->capacitance = (float)iVolts;
      memcpy(pLoad->statusText, "Discharging", 11);
      memcpy(pLoad->units, "Vlt", 3);
      digitalWrite(chargePin, LOW);                             // LOW to shut off power to charging path
      pinMode(chargePin, INPUT);                                // INPUT mode to switch charging current path off
      pinMode(dischargePin, OUTPUT);                            // OUTPUT mode to switch discharge path on
      digitalWrite(dischargePin, LOW);                          // LOW to shunt cap to ground through resistor
      state=3;
      pLoad->chargeTime = state;
      digitalWrite(greenLedPin, LOW);
      break;
  
   case 3 :                                             // Cap is in process of discharging, or has reached full discharge.
      iVolts = analogRead(analogPin);
      pLoad->capacitance = (float)iVolts;                       // Read volts on pin #6, as converted by ATTiny into integer relative value
      if(iVolts < 10) {                                        // Fully discharged should be a value of 0, tho noise may make a liar of this assumption.
        state=0;                                                // Fully discharged, transition back to state #0: "Begin Charging"
      }
      pLoad->chargeTime = state;
      break;   
   
   default :                                            // If we get here something has gone wrong as there is no intended path to this code.
      iVolts = analogRead(analogPin);
      pLoad->capacitance = (float)iVolts;
      pLoad->chargeTime = 9999;
      digitalWrite(errorLedPin, HIGH);
      memcpy(pLoad->statusText, "ERR in Case", 11);
   }  
} // end capacitorStateMachine()



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
