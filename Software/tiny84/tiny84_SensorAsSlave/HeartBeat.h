// Class: HeartBeat - Definition
//=================================================================================================

#ifndef HeartBeat_h
#define HeartBeat_h

#include "Arduino.h"

 /************************************************************************************************
 *
 *    PURPOSE: Blink green LED to show that the MCU is running through the main loop. That is, it 
 *  is UP and running. Even if there is a current error condition, my goal here is to indicate 
 *  that the MCU is still processing and responsive...or not. Note that for the fun of it I have
 *  incorporated the ability to have a different duty cycle for the LED's on state vs it's off
 *  time; thus the 'onLength' and 'offLength' data fields.
 *
 *    USAGE --
 *    1. Heartbeat object must be created (declared) using the pin# reference that the LED is wired
 *  to. I.e., Heartbeat heartBeat(4).
 *    2. You call the .begin() method to begin the LED blinking cycle. Typically you'll do this in
 *  the sketch's setup() function, but could be done anywhere.
 *    3. You must call the .update() method in the sktech's loop() function for the LED to toggle
 * on/off on it's timed blink pattern.
 */
class HeartBeat {
  private:
    int _ledPin;                     // Pin number the heartbeat LED is wired up to.
    int _onLength;                   // number of milliseconds to keep LED light on per blink cycle.
    int _offLength;                  // Number of milliseconds to keep LED off per blink cycle.
    unsigned long _phaseStartMillis; // Milliseconds on the MCU clock at start of current phase of the blink cycle.
    bool _ledOn;                     // TRUE = on, FALSE = off.

  public:

          /*      PURPOSE: Constructor. 
       *  Used to set the pin#/reference that the LED is wired up to. */
    HeartBeat(int pin);

          /*    PURPOSE: Start the heartbeat process going. */
    void begin();

          /*    PURPOSE: stop the heartbeat process. 
            May be useful if you need to use the heartbeat LED for some
            other purpose temporarily. */
    void stop();

          /*    PURPOSE: Determine if it is time to toggle the heartbeat LED on/off;
          if so, do so. */
    void update();
};
#endif
