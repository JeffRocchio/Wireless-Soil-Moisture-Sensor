// Class: ErrorFLash - Function Definitions
//=================================================================================================
/*    FOOTNOTES: Note that there are 'footnotes' at the bottom of this file that provide more
 *  detailed info and documentation that I didn't want to clutter up the code with; but which
 *  I am likely to want to remember when I come back to this in 6 months.  : ) 
 *   
 *      07/08/2023: First release. I believe I have it working as desired. Tho of course 
 *  real-world usage is likely to reveal subtle flaws to be fixed later.
 *
 */
//=================================================================================================


#include "Arduino.h"
#include "ErrorFlash.h"

//*************************************************************************************************


ErrorFlash::ErrorFlash(int pin) {
  _alertLength = 700;     //Set fixed values here in the object constructor.
  _flashLength = 250;
  _numFlashCycles = 2;
  _ledPin = pin;          // Set the pin LED is wired up to, as passed in from calling program.
  clean();                // Initilize into a no error condition.
} // END ErrorFlash (constructor method)


void ErrorFlash::begin() {
  pinMode(_ledPin, OUTPUT);
  digitalWrite(_ledPin, LOW);
  _ledOn = false;
} // END begin()


void ErrorFlash::setError(short int errorID) {
  _curErrID = errorID;
  _countRemaining = _curErrID * 2;
  _flashCyclesRemain = _numFlashCycles;
  _errPhase = 1;
  digitalWrite(_ledPin, HIGH);
  _ledOn = true;
  _phaseStartMillis = millis();
} // END setError()


void ErrorFlash::clear() {
  digitalWrite(_ledPin, LOW);
  clean();
} // END clear()


void ErrorFlash::update() {
  int timeoutLength;

  switch (_errPhase) {
    case 0: // No error, do nothing.
    case 3: // Done reporting error, but error condition still active; do nothing (LED should be lit, keep it lit).
      break;

    case 1: // We are in the start demarcation phase. When this phase is over simply set next phase based on cycles remaining.
      if(millis() > _phaseStartMillis+_alertLength) {
        if(!_flashCyclesRemain) {
          _errPhase = 3;
        } else {
          _errPhase = 2;
        }
       _phaseStartMillis = millis();
      }
      break;

    case 2: // Phase-2 -flashing the errID# and doing some logic work.
      if(millis() > _phaseStartMillis+_flashLength) {
        digitalWrite(_ledPin, !_ledOn);          // 1st, flip the LED on/off state.
        _ledOn = !_ledOn;
        _countRemaining--;                       // 2nd, decrement remaining count.
        if(!_countRemaining) {
          _flashCyclesRemain--;                  // Then IF we've finished a count-off, decrement that and go back to phase-1.
          _errPhase = 1;
          _countRemaining = _curErrID * 2;
        }
        _phaseStartMillis = millis();            // Finally, either way, restart count-down timer and exit.
      }
      break;

    default:                                     // Phases 0 & 3, we do nothing. In phase 0 LED should be off. In phase 3 it should be on steady.
      break;

  } // END switch statement

} // END update()

short int ErrorFlash::getErrorID() {
  return _curErrID;

} // END getErrorID()

bool ErrorFlash::isFlashing() {
  return (bool)_flashCyclesRemain;
} //END isFlashing()


void ErrorFlash::clean() {
  _curErrID = 0;
  _ledOn = false;
  _phaseStartMillis = 0;
  _countRemaining = 0;
  _flashCyclesRemain = 0;
  _errPhase = 0;
  _phaseStartMillis = 0;
} // END clean

/**************************************************************************************************
// FOOTNOTES
//*************************************************************************************************

/*   1. The visible count-out of the error number is the number of times the LED flashes to
  an ON state. So the error number flashing protocol is: LED starts out being ON for a
  longish time. Then it flashes OFF-ON the number of times of the error number. Then there is
  the same longish LED ON period between repeats of the count. Finally, when the count-out 
  fully completes the LED is ON steady until the error is cleared, "manually," by the calling
  program. So, e.g., for error #3: ON===OFF-ON=OFF-ON=OFF-ON===.....
*/

/*   2. The code logic in the switch statement in ErrorFlash::update() could definitely be
  challenging to follow. I was only able to work it out in my head by thinking like an
  oscilloscope - drawing out the 'square wave' of the LED ON/OFF cycle for an example of 
  error #3. And then against that diagram plotting out the phase transition points, 
  remaining count, and remaining cycle transitions and values. To make it fairly simple in 
  the code there is also a subtly that the "longish" on time that preceeds the error counting
  flashing includes one unit of the shorter 'flash' time so that the longer 'alert' on pulse
  prior to each flash count-out is really the sum of (_alertLength + _flashLength). Once that is
  your head it should be a bit easier to see how the logic is working. The 'oscilloscope' 
  diagram I used to work this out is at: 
  /Software/Documentation/ErrorFlash_PulseProtocol.odg
*/

