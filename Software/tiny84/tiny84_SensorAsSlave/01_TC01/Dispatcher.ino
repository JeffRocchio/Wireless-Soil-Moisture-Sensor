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

//*************************************************************************************************



Dispatcher::Dispatcher() {
      /*      PURPOSE: Constructor. */
  _testErrorFrequency = 500;
  _errorStartTime = 0;

}


void Dispatcher::begin() {
      /*    PURPOSE: Start the Dispatcher process. */

  _testErrorFrequency = 15000;
  _errorStartTime = 0;

}

void Dispatcher::dispatch() {
        /*    PURPOSE: Assesses changed states and new inputs and uses that 
        *  information to begin/ terminate /continue all other activities.
        *  Intended to be called once each loop() cycle. */
  short int errorNum;

  if(!errorFlash.isFlashing()) {
    if(millis() > _errorStartTime + _testErrorFrequency) {
      errorNum = errorFlash.getErrorID() + 1;
      if (errorNum > 10) errorNum = 3;
      errorFlash.setError(errorNum);
      _errorStartTime = millis();
    }

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

