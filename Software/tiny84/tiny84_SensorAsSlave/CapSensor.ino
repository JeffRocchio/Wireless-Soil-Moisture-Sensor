// Class: CapSensor - Function Definitions
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
  _readingAvailable = false;
  _ReadingsRemain = NUM_READINGS_TO_AVERAGE;  // Establish number of measurements to average over.
  _measurePhase = 1;                          // Start the process by moving into Phase-1 of the reading protocol.  
}


bool CapSensor::readingAvailable() {
          /*    PURPOSE: See longish description in capSensor.h. */

  switch(_measurePhase) {
    case 1:                                   // Phase-1: Pulse, Read & Clear.
      pulseAndReadVolts();
      _measurePhase =2;
      break;

    case 2:                                   // Phase-2: Calculate Capacitance.
      if(millis()>_nextMeasureMillis) {       // But only if hard-coded inter-measurement 'rest time' has elapsed.
        _capAccumulator +=  (float)(_tempVolts * IN_STRAY_CAP_TO_GND) / (float)(MAX_ADC_VALUE - _tempVolts);
        _ReadingsRemain--;
        _nextMeasureMillis = millis() + INTER_MEASUREMENT_DELAY;
        if(!_ReadingsRemain) _measurePhase = 3;
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

/*   2. Also reference documentation in: /Software/Documentation/CapSensor_MeasurementProtocol.odg.
*/


