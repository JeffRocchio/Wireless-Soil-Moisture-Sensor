
/*  7/8/2022: This is my Milestone #6: Measure capacitance on the ATTiny84, and transmit that info over 
 *  to the nRF24 hooked up to the Raspberry Pi. The "receiver" code on the RPi side is MS-06_CapDataReceive_04.cpp.
 *  
 *  I could not get this to work at all. The RPi side was picking up no transmissions at all. And Scanner also
 *  didn't seen any RF signale in the air for the channel this was supposedly transmitting on. So I rolled back
 *  to CapMeasureAndTransmit_02 - dummy data only - and confirmed that version's transmissions were being picked
 *  up by my MS-06_CapDataReceive_04.cpp program on the RPi side.
 *  
 *  So I have rolled back to CapMeasureAndTransmit_02 and will start all over again - tho this time taking it in
 *  smaller increments, not advancing until each increment is shown to be working. So the new starting point is 
 *  MS-06_CapMeasureAndTx_04.
 *  
 * ==================================================================================================================
 *  
 *  This sketch is for the ATtiny84.
 *  
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
  float capacitance;
  uint32_t chargeTime;            // Time it took for capacitor to charge.
  char units[4];                  // nFD, mFD, FD
  char statusText[12];            // For use in debugging.
};
TxPayloadStruct txPayload;

// Make a data structure to store the ACK payload
struct RxPayloadStruct {
  uint8_t counter;
  char message[12];               // Incoming message up to 11 chrs+Null.
};
RxPayloadStruct rxAckPayload;



void setup() {

    /* Init LED signal pins. */
  pinMode(greenLedPin, OUTPUT);
  pinMode(errorLedPin, OUTPUT);
    /* Confirm operation of LEDs */
  digitalWrite(errorLedPin, HIGH);
  digitalWrite(greenLedPin, HIGH);
  delay(1500);                        // Stop to see LED signals at this point.
  digitalWrite(errorLedPin, LOW);     // Clear the error LED.
  digitalWrite(greenLedPin, LOW);     // For now let's use Green LED on only while CAP is being charged.

    /*  Init capacitance measurement pins. */
  pinMode(chargePin, OUTPUT);       // set chargePin to output
  digitalWrite(chargePin, LOW);     // Set chargePin LOW

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
  txPayload.chargeTime = 0;                       // Time it took for capacitor to charge.
  memcpy(txPayload.units, "---", 3);
  txPayload.capacitance = 0;
  radio.stopListening();                          // put radio in TX mode


} // END setup()

  /*    OK, we've setup the ATTiny84 and the radio chips. Now go into an infinite loop
   * continuously transmitting data, seeking a valid acknowlegement of receipt; then repeat....
   *    I removed the else portion of code pertaining to receive nodes as here
   * I am only allowing the ATTiny to be a data/sensor transmitter; with the RPi to be
   * the receiver. */
void loop() {
  digitalWrite(greenLedPin, LOW);

  //measureCap (&txPayload);
  
  unsigned long start_timer = micros();                         // start the timer
  bool report = radio.write(&txPayload, sizeof(txPayload));     // transmit & save the report
  unsigned long end_timer = micros();                           // end the timer

  if (report) { // IF report is true than nRF24 chip believes it sent the payload out.
        /* Since we have full success here, clear all error conditions. */
      digitalWrite(errorLedPin, LOW);

        /* See if we got a receipt ACK payload back from the receiver (this would be an auto-ack payload).
         * Declare variable to hold the pipe number that received the acknowledgement payload. */
      uint8_t pipe;
      if (radio.available(&pipe)) {                             // ACK if-then block. Test for ack payload, which will also return the pipe number that received it.
        radio.read(&rxAckPayload, sizeof(rxAckPayload));        // get incoming ACK payload. Tho doing nothing with it on this side of the conversation.
        delay(200);                                              // brief pause, then fall through to bottom of loop to keep measuring and transmitting.
      } else {
          /* Radio chip sent the tranmission out, but did not receive an ACK data payload back.
             Show this with the LED. But then fall through to bottom of loop and keep measuring
             and transmitting. */
        errorLED (2);               // Call this error #2.
        memcpy(txPayload.statusText, "ERROR 02 ", 9);           // Set text to send back to RPi to show error.
      } // ACK i.e., bottom of the test to see if we got an ACK back

  } else {
    /* Transmission failed or timed out. Code here to signal transmit fail. Like, e.g., turn a red LED on. 
     * Then we fall through and keep trying. */
    errorLED (3);                                               // Call this error #3.
    memcpy(txPayload.statusText, "ERROR 03 ", 9);               // Set text to send back to RPi to show error.
  } // if(report) -i.e., bottom of the transmit success/fail IF-ELSE block

} // data streaming loop


void measureCap (TxPayloadStruct * pLoad) {

  memcpy(pLoad->statusText, "Working...", 11);

  digitalWrite(greenLedPin, HIGH);  // For now let's use Green LED on only while CAP is being charged.


  digitalWrite(chargePin, HIGH);            // set chargePin HIGH and capacitor charging
  startTime = millis();
  while(analogRead(analogPin) < 648) {       // 647 is 63.2% of 1023, which corresponds to full-scale voltage
  }
  elapsedTime= millis() - startTime;
  digitalWrite(greenLedPin, LOW);  // For now let's use Green LED on only while CAP is being charged.


    /* convert milliseconds to seconds ( 10^-3 ) and Farads to 
       microFarads ( 10^6 ),  net 10^3 (1000) */
  microFarads = ((float)elapsedTime / resistorValue) * 1000;

  memcpy(pLoad->statusText, "Measurement", 11);
  pLoad->chargeTime = elapsedTime;       // Time it took for capacitor to charge.
  memcpy(pLoad->units, "mFD", 3);
  pLoad->capacitance = microFarads;

    /* if value is smaller than one microFarad, convert to nanoFarads (10^-9 Farad).
       This is  a workaround because Serial.print will not print floats */
  if (microFarads <= 1) {
    nanoFarads = microFarads * 1000.0;      // multiply by 1000 to convert to nanoFarads (10^-9 Farads)
    memcpy(pLoad->units, "nFD", 3);
    pLoad->capacitance = nanoFarads;
  }

    /* Dicharge the capacitor  */
  digitalWrite(chargePin, LOW);             // set charge pin to  LOW
  pinMode(dischargePin, OUTPUT);            // set discharge pin to output
  digitalWrite(dischargePin, LOW);          // set discharge pin LOW
  while(analogRead(analogPin) > 0) {         // wait until capacitor is completely discharged
  }
  pinMode(dischargePin, INPUT);             // set discharge pin back to input

} // measureCap



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
