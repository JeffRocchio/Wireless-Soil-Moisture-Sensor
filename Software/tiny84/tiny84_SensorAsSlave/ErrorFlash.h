// Class: ErrorFLash - Class Definition
//=================================================================================================

#ifndef ErrorFlash_h
#define ErrorFlash_h

#include "Arduino.h"

 /************************************************************************************************
 * 
 *    PURPOSE: Provide a means to report error conditions to a user via an LED.
 *
 *    USAGE:
 *    1. Error object must be created (declared) using the pin# reference that the error LED is 
 *  wired to. I.e., Error error(4).
 *    2. You must call the .begin() method to initiate the error LED cycle - do this first, even 
 *  when no error condition yet exists. Typically you'll do this in the sketch's setup() 
 *  function, but could be done anywhere.
 *    3. You must call the .update() method as part of the sktech's loop() function for the 
 *  error LED to be responsive to a reported error.
 *    4. To report an error, call the setError() function.
 *
 *    NOTE:
 *    1. By design, error reporting is non-blocking. Two implications to this. One, the calling
 *  program will need to manage what other activities the MCU can be performing while the error
 *  condition exists and is being reported out via the flashing error LED. And, two, if a 2nd
 *  error arises before the current error is finished being reported out, the calling program
 *  will need to decide on what to do. I.e., block until the current error reporting is done,
 *  or stop current error and start reporting the new error, or maybe ignore the new error
 *  (if, say, it isn't critical).
 */
class ErrorFlash {

  private:
    int _ledPin;                     // Pin number the heartbeat LED is wired up to.
    int _alertLength;                // number of milliseconds to visibly demark the start/end of each flash cycle.
    int _flashLength;                // Number of milliseconds to flash LED to 'count out' the errorID#.
    short int _numFlashCycles;       // Number of times to repeat the error flashing count-out process.
    short int _flashCyclesRemain;    // Number of count-out cycles still remaining to be completed.
    short int _curErrID;                   // IF > 0, ID# of error currently being reported out.
    short int _countRemaining;       // Holds how many flashes remain to complete the error count-out process.
    short int _errPhase;             // Were we are in the LED blinking/reporting cycle. 0:No Error. 1:Start Demarcation. 2: Flashing Count. 3: End Demarcation. 4: LED on steady, no blinking.
    unsigned long _phaseStartMillis; // Milliseconds on the MCU clock at start of current phase of the blink cycle.
    bool _ledOn;                     // TRUE = on, FALSE = off.

  public:

      //      PURPOSE: Constructor.
      /*  Used to set the pin#/reference that the LED is wired up to. */
    ErrorFlash(int pin);

      /*    PURPOSE: Start the error reporting process going for a new error. */
    void begin();

      /*    PURPOSE: Set an error to report out by blinking of 
       *  the error LED. */
    void setError(short int errorID);

      /*    PURPOSE: Stop any in-process error reporting process
       *  and clear the error condition. */
    void clear();

      /*    PURPOSE: Determine if it is time to toggle the LED on/off;
       *  if so, do so. AND also update what phase of the reporting
       *  cycle we are in; including potentially ending the report. */
    void update();

          /*    PURPOSE: Can be used in the sktech to obtain the 
           *  current error number. Will return 0 if no error. BUT
           *  note that I don't automatically clear the error after
           *  flashing out it's number. The error info will stay in
           *  the object's data fields until a new error is initiated,
           *  or explicitly cleared by the using application. */
    short int getErrorID();

          /*    PURPOSE: Use to test if an error is in processes of being
           *  counted-out. Returns TRUE if so, FALSE if the current error
           * has finished being reported out. */
    bool isFlashing();

  private:

    void clean();

};
#endif
