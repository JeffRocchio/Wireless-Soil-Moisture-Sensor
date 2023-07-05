// Class: ErrorFLash - Function Definitions
//=================================================================================================

#include "Arduino.h"
#include "ErrorFlash.h"

//*************************************************************************************************


ErrorFlash::ErrorFlash(int pin) {
      /*      PURPOSE: Constructor. 
       *  Used to set the pin#/reference that the LED is wired up to. */

  _alertLength = 700;     //Set default values here in the object constructor.
  _flashLength = 250;
  _numFlashCycles = 2;

  _ledPin = pin;          // Set the pin LED is wired up to, as passed in from calling program.
  
  clean();                // Initilize into a no error condition.

} // END ErrorFlash (constructor method)


void ErrorFlash::begin() {
      /*    PURPOSE: Start the error reporting process going - with presumption that
       *  no error condition currently exists. */
  
  pinMode(_ledPin, OUTPUT);
  digitalWrite(_ledPin, LOW);
  _ledOn = false;

} // END begin()


void setError(short int errorID) {
      /*    PURPOSE: Set an error to report out by blinking of 
       *  the error LED. */
  
  _curErrID = errorID;
  _countRemaining = _curErrID * 2;
  _flashCyclesRemain = _numFlashCycles;
  _errPhase = 1;
  digitalWrite(_ledPin, HIGH);
  _ledOn = true;
  _phaseStartMillis = millis();

} // END setError()


void ErrorFlash::clear() {
      /*    PURPOSE: Stop any in-process error reporting process
       *  and clear the error condition. */
  
  digitalWrite(_ledPin, LOW);
  clean();

} // END clear()


void ErrorFlash::update() {
      /*    PURPOSE: Determine if it is time to toggle the LED on/off;
       *  if so, do so. AND also update what phase of the reporting
       *  cycle we are in; including potentially ending the report. */
  
  int timeoutLength;

  switch _errPhase {
    case 0: // No error, do nothing.
    case 3: // Done reporting error, but error condition still active; do nothing (LED should be lit, keep it lit).
      break;

    case 1: // We are in the start demarcation phase.
      if(millis() > _phaseStartMillis+_alertLength {
        digitalWrite(_ledPin, LOW);
        _ledOn = false;
        _errPhase = 2;
       _phaseStartMillis = millis();
      }
      break;

    case 2: // We are flashing the errID#.
      if(millis() > _phaseStartMillis+_flashLength {
        digitalWrite(_ledPin, !_ledOn);                 // 1st, flip the LED on/off state.
        _ledOn = !_ledOn;
        _countRemaining--;                              // 2nd, decrement remaining count.
        if(!_countRemaining) {
          _flashCyclesRemain--;                         // Then IF we've finished a count-off, decrement that.
          if(!_flashCyclesRemain) {
            errPhase = 3;                               // And IF we've also finished all the count-offs, move to steady LED on.
          }
          else {
            errPhase = 1;                               // IF we haven't finished all count-offs, start a new count-off cycle.
            _countRemaining = curErrID * 2;
          }
        }
      _phaseStartMillis = millis();                   // Finally, either way, restart count-down timer and exit.
      }
      break;

    default:                                          // Phases 0 & 3, we do nothing. In phase 0 LED should be off. In phase 3 it should be on steady.
      break;

  } // END switch statement

} // END update()


void ErrorFlash::clean() {
      /*    PURPOSE: Sets state to 'no error' condition. */
  
  _curErrID = 0;
  _ledOn = false;
  _phaseStartMillis = 0;
  _countRemaining = 0;
  _flashCyclesRemain = 0;
  _errPhase = 0;
  _phaseStartMillis = 0;

} // END clean

