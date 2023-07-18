// Class: CapSensor - Function Definitions
//=================================================================================================
/*    FOOTNOTES: Note that there are 'footnotes' at the bottom of this file that provide more
 *  detailed info and documentation that I didn't want to clutter up the code with; but which
 *  I am likely to want to remember when I come back to this in 6 months.  : ) 
 *   
 *      07/18/2023: This instance is specific and unique to the TC02_CapClass test case. Be
 *  careful NOT to copy this back up to the parent directory once work on this test case has 
 *  completed.
 *      SUCCESS: Test for non-blocking Sensor Cap Measurement was successful. Tho note that in
 *  this particular test I'm not getting much data back, just the 3-flash on the red LED to
 *  show me that the capSensor class is returning what it thinks is a valid, average,
 *  capacitor reading. And the processor HeartBeat blinks with no hiccups. So all does appear
 *  to be fine per the conditions set up in this particular test case. So I shall close out
 *  this test case now.
 */
//=================================================================================================

#include "Arduino.h"
#include "Dispatcher.h"
#include "ErrorFlash.h"
#include "CapSensor.h"

//*************************************************************************************************



Dispatcher::Dispatcher() {
      /*      PURPOSE: Constructor. */
  _capReadingFrequency = 30000;                                     // 30,000 milliseconds = 30 seconds.
  _capReadingStartTime = 0;
  _testErrorFrequency = 1500;
  _errorStartTime = 0;

}


void Dispatcher::begin() {
      /*    PURPOSE: Start the dispatcher. */

  _testErrorFrequency = 15000;
  _errorStartTime = 0;

}

void Dispatcher::dispatch() {
        /*    PURPOSE: Assesses changed states and new inputs and uses that 
        *  information to begin/ terminate /continue all other activities.
        *  Intended to be called once each loop() cycle. */

  if(millis() > (_capReadingStartTime + _capReadingFrequency)) {    // Time to initiate a new sensor reading?
    _capReadingStartTime = millis();                                // Yes.
    capSensor.initiateSensorReading();                              // So fire it off.
  } 
  else {                                                            // No. So keep tickling the reading cycle, and handle case where we already have a reading result in hand.
    if(capSensor.readingAvailable()) {                              // This is how we're calling readingAvailable() every loop() cycle to continue any in-process measurement.
      if(millis() > (_errorStartTime + _testErrorFrequency)) {      // Use 'error' reporting to show we have a completed cap reading since we aren't transmitting to RPi in this test case.
        errorFlash.setError(3);                                     // And be sure to only restart the 'error' reporting signal only after enough time has past for it to have reported out the last setError() call.
        _errorStartTime = millis();
      }
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

