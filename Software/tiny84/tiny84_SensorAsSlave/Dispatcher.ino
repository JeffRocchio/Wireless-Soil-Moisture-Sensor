// Class: CapSensor - Function Definitions
//=================================================================================================
/*    FOOTNOTES: Note that there are 'footnotes' at the bottom of this file that provide more
 *  detailed info and documentation that I didn't want to clutter up the code with; but which
 *  I am likely to want to remember when I come back to this in 6 months.  : ) 
 *   
 *      09/24/2023: Put in logic for handling commands back from master. This includeds
 * redefining the RadioComms::RxPayloadStruct struct. 
 * 
 *      09/20/2023: Changed the cap measure / TX cycle to occur once every 10 mins to make 
 * this release suitable to put in a real plant pot and keep an eye on the readings every 
 * so often in a real-world setting.

 *      09/19/2023: With this version I believe I am now back to where I was at Milestone #11; 
 *  but now in a 'non-blocking' mode with solid heartbeat and error reporting LED functions
 *  implemented.
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
    if(!errorFlash.isFlashing()) errorFlash.clear(); // Don't do this here. I clear any pending errors in the RadioComms object.
    return;
    }

  /* IS THERE A RADIO THERE? ---
   *    Check to see if we have an active/alive radio chip connected
   * and initilized. If not, try initilize the radio chip. */
  if(!_radioAvailable) {
    if(!radio.setup()) {
      errorFlash.setError(9);           // 9 error means no radio active.
      return;
    }
    _radioAvailable = true;             // If we get here then the radio chip got activated.
    return;
  }

  /* PROCEED WITH READ-TX-RESPOND CYCLE ---
   *    No error reporting in flight, and we have an active radio.
   * Proceed with the normal cycle of wake; sensor reading; transmission;
   * and ACK command response. */
  switch (_phase) {
    case 0:  // Sleeping (sort of)
      if(millis() > (_capReadingStartTime + _capReadingInterval)) _phase = 1;
      break;

    case 1: // Initiate sensor reading.
      _capReadingStartTime = millis();
      capSensor.initiateSensorReading();
      _phase = 2;
      break;

    case 2: // Wait for and fetch sensor reading, then initiate transmission.
      if(capSensor.readingAvailable()) {   // This gives a slice of CPU time to CapSensor object.
        radio.setTxPayload(capSensor.getCapacitance());
        _phase = 3;
      }
      break;

    case 3: // Wait for and fetch ACK payload.
      // Note that it is therotically possible to get stuck in an infinite loop here...
      if(radio.ackAvailable()) {
        _ackPayloadPtr = radio.getAckPayload();
        _phase = 4;
      }
      break;

    case 4: // Handle master's command back to me.
      // Master command handler here....
      // For now tho, do a pseudo sleep state.
      switch(_ackPayloadPtr->command) {
        case 1: // Command ID 01 | Sleep for uliCmdData milliseconds.
          _capReadingInterval = _ackPayloadPtr->uliCmdData;
          _capReadingStartTime = millis();
          _phase = 0;
          break;

        default: // No command back. Sleep for last sleep time interval.
          _phase = 0;
          break;
      }
  }

  /* ONGOING RADIO TRANSMISSION ---
  /*    Give a CPU slice to the radio object for it to do a 
   * chunk of Tx/Rx pending work (if any is in process).  */
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

