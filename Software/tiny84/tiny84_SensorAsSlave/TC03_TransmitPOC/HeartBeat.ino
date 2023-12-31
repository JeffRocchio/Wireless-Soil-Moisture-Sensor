// Class: HeartBeat - Function Definitions
//=================================================================================================

#include "Arduino.h"
#include "HeartBeat.h"

 /************************************************************************************************
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

HeartBeat::HeartBeat(int pin) {
  _onLength = 700;
  _offLength = 350;
  _ledPin = pin;
  _ledOn = false;
  _phaseStartMillis = 0;
}

void HeartBeat::begin() {
  pinMode(_ledPin, OUTPUT);
  digitalWrite(_ledPin, HIGH);
  _ledOn = true;
  _phaseStartMillis = millis();
}

void HeartBeat::stop() {
  digitalWrite(_ledPin, LOW);
  _ledOn = false;
  _phaseStartMillis = 0;
}

void HeartBeat::update() {
  int timeoutLength;
  if (_ledOn) { timeoutLength = _onLength; } else { timeoutLength = _offLength; }
    if(millis() > _phaseStartMillis+timeoutLength) {
      /* Time to toggle the LED's on/off state.*/
      digitalWrite(_ledPin, !_ledOn);
      _ledOn = !_ledOn;
      _phaseStartMillis = millis();
    }
}


