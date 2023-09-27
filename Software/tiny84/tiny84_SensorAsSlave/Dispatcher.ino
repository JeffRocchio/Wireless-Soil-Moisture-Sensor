// Class: CapSensor - Function Definitions
//=================================================================================================
/*    FOOTNOTES: Note that there are 'footnotes' at the bottom of this file that provide more
 *  detailed info and documentation that I didn't want to clutter up the code with; but which
 *  I am likely to want to remember when I come back to this in 6 months.  : ) 
 *   
 * 09/27/2023: 
 *    > Changed the sensor-read/Transmit cycle to once every 15 minutes.
 *    > This is my 1st version for 'beta testing' in a live flower pot.
 *
 * 09/25/2023: 
 *    > Created logic structure in dispatch() for the goals in Milestone #12.
 *      As of this update I am successfully reading capacitance, transmitting over to the 
        RPi, the RPi is displaying results on the terminal. The ATTiny is saying it is 
        getting ack packets back from the RPi - tho I haven't yet 'proven' that fact. 
 *      I made a commit on branch Issue-1 at this point: WIP Basic Read/Tx/Ack-back working.
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
      if(radio.ackAvailable()) {
        _ackPayloadPtr = radio.getAckPayload();
        _phase = 4;
      }
      break;

    case 4: // Handle master's command back to me.
      // Master command handler here....
      // For now tho, do a pseudo sleep state.
      _phase = 0;
      break;
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

