// Class: RadioComms - Function Definitions
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
  _txPayload.capacitance = fCap;                        // calculated capacitance
  _txPayload.chargeTime = 0;                         // Time it took for capacitor to charge.
  memcpy(_txPayload.units, "---", 3);                // capacitance units
  _txPayload.ctSuccess = 0;                          // running count of no-error transmissions
  _txPayload.ctErrors = 0;                           // running count of transmissions errors
  memcpy(_txPayload.statusText, "testing", 7);

  _phase = 1;

}


short int RadioComms::update() {

  switch(_phase) {
    case 1:                                   // Phase-1: Transmit TxPayload.
      if(millis()>(_lastMillis+_txWaitDelay)) {
        bool report = _radioChip.write(&_txPayload, sizeof(_txPayload)); 
        if(report) {
          _phase = 2;
        }
      }
      _lastMillis = millis();
      break;

    case 2:                                   // Phase-2: Retreive, ACK payload.
      if(millis()>(_lastMillis+_ackWaitDelay)) {       // But only if hard-coded inter-measurement 'rest time' has elapsed.
        _phase = 0;
      }
      _lastMillis = millis();
      break;

    default:                                  // Phase-0: Do nothing.
      _lastMillis = millis();
      break;
  }
  return(_phase);
}



/**************************************************************************************************
// FOOTNOTES
//*************************************************************************************************

/*   1.   
*/

/*   2. Also reference documentation in: /Software/Documentation/RadioComms_ConverseProtocol.odg.
*/


