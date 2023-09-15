// Class: CapSensor - Function Definitions
//=================================================================================================
/*    FOOTNOTES: Note that there are 'footnotes' at the bottom of this file that provide more
 *  detailed info and documentation that I didn't want to clutter up the code with; but which
 *  I am likely to want to remember when I come back to this in 6 months.  : ) 
 *   
 *      09/15/2023: This version is for test case TC03_TransmitPOC (1st test of RadioComms class). 
 * SUCCESS. See screenshot TC03_TransmitPOC_terminalDisplay.jpg.
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
  /* What follows is a series of tests that determine which action to take. That is, 
   * each test determines a 'dispatch' action. The intent here is that each test
   * stand on it's own as a test-action pair; and that we return immediately from 
   * each of those in order to be non-blocking. */


  /* CHECK ERROR CONDITION IN PROGRESS ---
   *    If we're flashing out an error condition don't initiate
   * any new actions until the  error report-out is complete.
   *    Once complete, clear the error, which on subsequent dispatch calls
   * will then fall through this test to allow other actions to be taken. */
  if(errorFlash.getErrorID() > 0) {
    if(!errorFlash.isFlashing()) errorFlash.clear();
    return;
    }

  /* IS THERE A RADIO THERE? ---
   *    Check to see if we have an active/alive radio chip connected
   * and initilized. If not, to connect * initilize the radio chip. */
  if(!_radioAvailable) {
    if(!radio.setup()) {
      errorFlash.setError(9);           // 9 error means no radio active.
      return;
    }
    _radioAvailable = true;
    return;
  }

  /* CAPACITANCE MEASUREMENT ---
   *   For testing, simulate cap measurement on fixed intervals. 
   *   I will note that by doing a return in this block I am making
   * a choice that radio Rx/Tx actions are to be suspended until we 
   * complete a cap measurement. This choice is based on my intention 
   * to take a measurement on a pace which is order of magnititude 
   * slower that the speed of a single Tx/Rx event. And that I don't 
   * want to have 'partial' measurements overlapping with in-flight 
   * data transmissions. */
  if(millis() > __lastCapMeasureMillis + __simInterCapMeasureTime) {
    _capacitorValue += 0.1;
    radio.setTxPayload(_capacitorValue);
    __lastCapMeasureMillis = millis();
    return;
  }

  /* When we fall through to here everything is 'nominal' and we can give a CPU slice to
   * the radio object for it to do a chunk of Tx/Rx pending work (if any is in process).  */
  errorFlash.setError(radio.update());

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

