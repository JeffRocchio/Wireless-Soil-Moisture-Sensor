// Class: RadioComms - Function Definitions
//=================================================================================================
/*    FOOTNOTES: Note that there are 'footnotes' at the bottom of this file that provide more
 *  detailed info and documentation that I didn't want to clutter up the code with; but which
 *  I am likely to want to remember when I come back to this in 6 months.
 *
 * 09/26/2023:
 *    > Changed field chargeTime to sensorTime in TxPayloadStruct.
 *    > Added code to populate sensorTime at each transmit cycle start.
 *    > THIS CHANGE TRIGGERED THE ERROR OF ISSUE-1 --->> 
 *        Redefined the AckPayloadStruct struct to support the intended command/response 
 *        protocol specified in the log entry for milestone #12. Tho at this point have not
 *        implemented doing anything with that data coming back from the RPi.
 *          I changed that struct to be only 2 fields, both uint32_t, and with that
 *          change it works. So it's something to do with byte alignment/boundaries
 *          in the struct definitions. I am assuming that misalignment causes overflows, 
 *          memory leaks, or other types of corruptions.
 *
 * 09/18/2023:
 *    > Fixed missing _rxPayloadAvailable = true; in Phase-2 logic.
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
  _txPayload.sensorTime = millis();                  // Load current CPU time to payload.
  memcpy(_txPayload.units, "---", 3);                // capacitance units
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
        } else {
          _txPayload.ctErrors++;
          iErr = 2;                                                 // Call this error #2. Stay in phase-1 for a retry.
        }
        _lastMillis = millis();                                     // In effect, restart the txWaitDelay.
      }
      break;

    case 2:                         // Phase-2: Retreive, ACK payload...
      if(millis()>(_lastMillis+_ackWaitDelay)) {       // ...but only if hard-coded inter-measurement 'rest time' has elapsed.
        uint8_t pipe;                                               // Tmp variable to hold pipe # with the ack payload.
        if (_radioChip.available(&pipe)) {                          // Do we have an ACK payload (with auto-ack on, we def should).
          _radioChip.read(&_rxAckPayload, sizeof(_rxAckPayload));   // get incoming ACK payload.
          _rxPayloadAvailable = true;
        } else {                                                    // No ACK payload....
          iErr = 3;                                                 // Call this error #3, and...
          _txPayload.ctErrors++;                                    // Increment the errors tracking counter.
        }
        _phase = 0;
        _lastMillis = millis();                                     // In effect, restart the txWaitDelay.
      }
      break;

    default:                        // Phase-0: Do nothing.
      _lastMillis = millis();
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


