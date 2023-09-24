// Class: CapSensor - Function Definitions
//=================================================================================================
/*    FOOTNOTES: Note that there are 'footnotes' at the bottom of this file that provide more
 *  detailed info and documentation that I didn't want to clutter up the code with; but which
 *  I am likely to want to remember when I come back to this in 6 months.
 *
 *      09/18/2023: Fixed failure to clear _capAccumulator on new reading initiation. Also
 * implemented what should be a fix for the 'inf' capacitance readings, which I believe 
 * were caused by a logic flow gap which left _tempVolts =0, causing a divide by zero 
 * error.
 *
 *      09/17/2023: Added function to retreive the measured value of the sensor's capacitance.
 *
 */
//=================================================================================================


#include "Arduino.h"
#include "CapSensor.h"

//*************************************************************************************************


CapSensor::CapSensor(int chargePin, int voltReadPin) {
      /*      PURPOSE: Constructor. 
       *  Used to set the pin#/reference that the soil capacitor sensor is wired up to. */

  _chargePin = chargePin;                     // Set the pins the sensor leads are wired up to.
  _voltReadPin = voltReadPin; 

  _capacitance = 0;
  _readingAvailable = false;
  _measurePhase = 0;

} // END CapSensor (constructor method)


void CapSensor::setup() {
          /*    PURPOSE: Initilize the sensor to enable readings. */
  pinMode(_chargePin, OUTPUT);
  digitalWrite(_chargePin, LOW);              // Start with 0 volts on charge pin.
  pinMode(_voltReadPin, OUTPUT);
  digitalWrite(_voltReadPin, LOW);            // Start with the analog pin low, 0 volts.

}


void CapSensor::initiateSensorReading() {
          /*    PURPOSE: Change state of the object such that we begin to make a series of 
           *  capacitance readings, which we will average out when done into a 'final' 
           *  reading of the sensor's capacitance value. */

  _capacitance = 0;                           // Clear out prior reading.
  _capAccumulator = 0;
  _readingAvailable = false;
  _ReadingsRemain = NUM_READINGS_TO_AVERAGE;  // Establish number of measurements to average over.
  _nextMeasureMillis = 0;
  _measurePhase = 1;                          // Start the process by moving into Phase-1 of the reading protocol.  
}


bool CapSensor::readingAvailable() {
          /*    PURPOSE: See longish description in capSensor.h. */

  switch(_measurePhase) {
    case 1:                                   // Phase-1: Pulse, Read & Clear.
      if(millis()>_nextMeasureMillis) {       // But only if hard-coded inter-measurement 'rest time' has elapsed.
        pulseAndReadVolts();
        _measurePhase = 2;
      }
      break;

    case 2:                                   // Phase-2: Calculate Capacitance.
      _capAccumulator +=  (float)(_tempVolts * IN_STRAY_CAP_TO_GND) / (float)(MAX_ADC_VALUE - _tempVolts);
      _ReadingsRemain--;
      _nextMeasureMillis = millis() + INTER_MEASUREMENT_DELAY;
      if(_ReadingsRemain) {
        _measurePhase = 1;
      } else {
        _measurePhase = 3;
      }
      break;

    case 3:                                   // Phase-3: Take Average.
      _capacitance = _capAccumulator / NUM_READINGS_TO_AVERAGE;
      _readingAvailable = true;
      _measurePhase = 4;
      break;

    default:                                  // Phase-4: Measurement Available. Don't take any action.
      break;
  }

  return(_readingAvailable);
}


float CapSensor::getCapacitance() {
          /*    PURPOSE: Obtain last read value of the sensor capacitance. */
  return(_capacitance);
}

void CapSensor::pulseAndReadVolts() {
          /*    PURPOSE: Private function. Performs the Phase-1 step of taking one measurement of
           *  the voltage between 'C1' and C-test -> i.e., the sensor's capacitance. */

  pinMode(_voltReadPin, INPUT);         // Get ready to measure voltage.
  digitalWrite(_chargePin, HIGH);       // Send voltage pulse to cap under test.
  _tempVolts = analogRead(_voltReadPin);   // Read voltage at point between 'C1' and C-test.
                                        // -- Clear everything for next measurement --
  digitalWrite(_chargePin, LOW);        // Remove voltage from caps and short C-test to ground.
  pinMode(_voltReadPin, OUTPUT);        // Block C-test & 'C1' short-circut path through voltRead pin in prep for next charge cycle.
}



/**************************************************************************************************
// FOOTNOTES
//*************************************************************************************************

/*   1. PLEASE NOTE: This class will only handle and measure pico-farad level of capacitance.
  The approach to pF scale capacitance measurement is the one described and explained by
  Jonathan Nethercott in a web post back in 2014. I have taken a pdf 'snapshot' of the web page 
  where I got this from. See the file:
  /Reference-and-Instructions/Pico-farad-cap-measurement-with-Arduino.pdf   
*/

/*   2. There is a possible error condition that goes untrapped. In readingAvailable(), case 2:
  if for some reason _tempVolts = 0 then we have a divide by zero error condition. In such a 
  case I believe the resultant value of _capAccumulator will = 'inf' (infinity), which will 
  then also flow into _capacitance. In theory this should never happen as there is always some
  stray capacitance in the physical circut, so the pulseAndReadVolts() function should always
  return some voltage. However, in my testing of a 'real world' version of Dispatcher.ino I
  did get 'inf' as the resultant cap reading. I believe I have fixed this by fixing a logic error. 
  But if you see 'inf' over on the RPi side this may be the cause.
*/

/*   3. The below line of code, which calculates the capacitance value given the voltage the
  cap achieved when pulsed with a Pin-High event, consumes 10% of the ATTiny84's program
  memory space. Yikes. As of this writing I am at 71% code memory consumed and not fully done
  with implementing all desired features. Should this become an issue and I need that 10% back
  I could just send over to the RPi the sum of the 10 _tempVoltage readings and let the Pi
  make the caluclations. OR, really, we don't care about actual capacitance value since it is
  simply a function of the _tempVoltage value. We're trying to correlate to soil moisture; so
  really we only need the _tempVoltage data at the RPi and can use that raw data to work out
  moisture factors. And if we did this we also avoid the potential of the untrapped error
  condition of #2 above.
     _capAccumulator +=  (float)(_tempVolts * IN_STRAY_CAP_TO_GND) / (float)(MAX_ADC_VALUE - _tempVolts);
*/

/*   3. Also reference documentation in: /Software/Documentation/CapSensor_MeasurementProtocol.odg.
*/

