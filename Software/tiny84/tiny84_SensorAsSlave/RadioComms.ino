// Class: RadioComms - Function Definitions
//=================================================================================================
/*    FOOTNOTES: Note that there are 'footnotes' at the bottom of this file that provide more
 *  detailed info and documentation that I didn't want to clutter up the code with; but which
 *  I am likely to want to remember when I come back to this in 6 months.
 *
 *      09/20/2023: Sending ATTiny millis() in the 'chargeTime' field; for ref over on the RPi.
 * Added statement to clear any existing error upon successful Tx/Rx-ACK back.
 *
 *      09/18/2023: Fixed missing _rxPayloadAvailable = true; in Phase-2 logic.
 *
 */
//=================================================================================================


#include "Arduino.h"
#include "RadioComms.h"

RadioComms::RadioComms(int cePin, int csnPin) : _cePin(cePin), 
                                                _csnPin(csnPin),
                                                _radioChip(cePin, csnPin)
                                                {
      /* Initializing the RF24 _radioChip object
       * instance using a member initializer
       * list in order to properly have the 
       * compilier call the RF24 constructor 
       * function with the pin params. */
  _rxAckPayload.command = 0;
} // END RadioComms (constructor method)



bool RadioComms::setup() {
  bool result = true;

  result = _radioChip.begin();            // Instantiate the nRF24L01 transceiver.
  if(result) {
    _radioChip.setPALevel(RF24_PA_LOW);                 // RF24_PA_MAX is default.
    _radioChip.enableDynamicPayloads();                 // To use ACK payloads, we need to enable dynamic payload lengths for all nodes.
    _radioChip.enableAckPayload();                      // Enable for all nodes so we can get ACK payloads back from the RPi.
    _radioChip.openWritingPipe((const uint8_t *)_addressMaster);         // Load the 'masters' address into the transmit pipe.
    _radioChip.openReadingPipe(1, (const uint8_t *)_addressSelf);        // Load 'self' address into receiving pipe.
    _radioChip.stopListening();                         // Put radio in transmit mode.
    _radioAvail = true;                                 // All appears to be well, so mark the radio available for use.
    _phase = 0;                                         // Set to operational phase 0.
  }
  return(result);

} // END setup()


void RadioComms::setTxPayload(float fCap) {

  /* To get us started with testing our concept out, let's just dummy up some data to transmit. */
  _txPayload.capacitance = fCap;                     // calculated capacitance
  _txPayload.chargeTime = millis();                  // Send CPU current time just for reference.
  memcpy(_txPayload.units, "pF ", 3);                // capacitance units
  memcpy(_txPayload.statusText, "testing", 7);

  _rxPayloadAvailable = false;                       // Make sure we 'reset' from any prior Tx cycle.

  _lastMillis = 0;                                   // No delay to begin work on phase-1.
  _phase = 1;
}


short int RadioComms::update() {
  short int iErr = 0;

  switch(_phase) {
    case 1:                         // Phase-1: Transmit TxPayload.
      if(millis()>(_lastMillis+_txWaitDelay)) {
        bool report = _radioChip.write(&_txPayload, sizeof(_txPayload)); 
        if(report) {
          _txPayload.ctSuccess++;
          _phase = 2;
          iErr = 0;                                                // Success - clear any prior error.
        } else {
          _txPayload.ctErrors++;
          iErr = 2;                                                 // Call this error #2. Stay in phase-1 for a retry.
        }
        _lastMillis = millis();                                     // In effect, restart the txWaitDelay.
      }
      break;

    case 2:                         // Phase-2: Retreive ACK payload...
      if(millis()>(_lastMillis+_ackWaitDelay)) {       // ...but only if hard-coded inter-measurement 'rest time' has elapsed.
        uint8_t pipe;                                               // Tmp variable to hold pipe # with the ack payload.
        if (_radioChip.available(&pipe)) {                          // Do we have an ACK payload (with auto-ack on, we def should).
          _radioChip.read(&_rxAckPayload, sizeof(_rxAckPayload));   // get incoming ACK payload.
          _rxPayloadAvailable = true;
        } else {                                                    // No ACK payload....
          iErr = 3;                                                 // Call this error #3, and...
          _txPayload.ctErrors++;                                    // ...increment the errors tracking counter and...
          _phase = 1;                                               // ...go back to Phase-1 to retry whole Tx/Rx-ACK process.
        }
        _phase = 0;
        _lastMillis = millis();                                     // In effect, restart the txWaitDelay.
      }
      break;

    default:                        // Phase-0: Do nothing.
      _lastMillis = millis(); // <-- This doesn't really have any practical effect given the current process flow.
      break;
  }
  return(iErr);
}


bool RadioComms::ackAvailable() {
  return(_rxPayloadAvailable);
}


RadioComms::RxPayloadStruct* RadioComms::getAckPayload() {
  return(&_rxAckPayload);
}




/**************************************************************************************************
// FOOTNOTES
//*************************************************************************************************

/*   1.   
*/

/*   2. Also reference documentation in: /Software/Documentation/RadioComms_ConverseProtocol.odg.
*/


