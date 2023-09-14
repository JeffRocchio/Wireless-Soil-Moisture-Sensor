// Class: CapSensor - Function Definitions
//=================================================================================================
/*    FOOTNOTES: Note that there are 'footnotes' at the bottom of this file that provide more
 *  detailed info and documentation that I didn't want to clutter up the code with; but which
 *  I am likely to want to remember when I come back to this in 6 months.  : ) 
 *   
 *      07/08/2023: First draft. Goal with this 1st draft is just to test the ErrorFlash
 *  class by creating/clearing an error condition every 30 seconds or so.
 *
 */
//=================================================================================================

#include "Arduino.h"
#include "Dispatcher.h"
#include "ErrorFlash.h"
#include "CapSensor.h"
#include "RadioComms.h"

//*************************************************************************************************



// Dispatcher::Dispatcher() {
// }

void Dispatcher::begin() {
  // Nothing to do at the moment.
}

void Dispatcher::dispatch() {
  /* If we're flashing out an error condition, wait for that report-out to complete.
   * Once complete, clear the error. */
  if(errorFlash.getErrorID() == 9) { //TMP: A 9 error means no radio active.
    if(!errorFlash.isFlashing()) errorFlash.clear();
    return;
    }
  /* No radio connected - try again to connect/initilize the radio chip. */
  if(!_radioAvailable) {
    if(!radio.setup()) {
      _radioAvailable = false;
      errorFlash.setError(9);
    return;
    }
  }
  /*  for testing, simulate cap measurement on fixed intervals. */
  if(millis() > __lastCapMeasureMillis + __simInterCapMeasureTime) {
    _capacitorValue += 0.1;
    radio.setTxPayload(_capacitorValue);
    __lastCapMeasureMillis = millis();
  }
}


/**************************************************************************************************
// FOOTNOTES
//*************************************************************************************************

/*   1. This is not a 'library' class. The code in this class/object is very much an intimate 
  part of the moisture sensor application (sketch). As such it not generic, reusable, code.
*/

/*   2. Related to #1 above - this code assumes that other classes/objects used by the app are
  declared as global so that they are accessable here.
*/

