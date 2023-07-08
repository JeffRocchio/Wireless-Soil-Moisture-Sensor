// Class: CapSensor - Function Definitions
//=================================================================================================

#include "Arduino.h"
#include "CapSensor.h"

//*************************************************************************************************


CapSensor::CapSensor(int pin) {
      /*      PURPOSE: Constructor. 
       *  Used to set the pin#/reference that the soil capacitor sensor is wired up to. */

  _alertLength = 700;     //Set fixed values here in the object constructor.
  _flashLength = 250;
  _numFlashCycles = 2;

  _ledPin = pin;          // Set the pin LED is wired up to, as passed in from calling program.
  
  clean();                // Initilize into a no error condition.

} // END ErrorFlash (constructor method)




void capacitorMeasurement(TxPayloadStruct * pLoad) {
  unsigned long startTime = millis();
  unsigned long elapsedTime;

  pinMode(CAP_VOLTREAD_PIN, INPUT);        // Get ready to measure voltage.
  digitalWrite(CAP_CHARGE_PIN, HIGH);      // Send voltage pulse to cap under test.
  int val = analogRead(CAP_VOLTREAD_PIN);  // Read voltage at point between 'C1' and C-test.

      //Clear everything for next measurement
  digitalWrite(CAP_CHARGE_PIN, LOW);       // Remove voltage from caps.
  pinMode(CAP_VOLTREAD_PIN, OUTPUT);       // ?? - does this discharge C-test to ground??

  elapsedTime= millis() - startTime;       // Capture elapsed time out of curiosity.

                                           //Calculate and store result
  float capacitance = (float)val * IN_CAP_TO_GND / (float)(MAX_ADC_VALUE - val);

  //memcpy(pLoad->statusText, "Measurement", 11);
  pLoad->chargeTime = elapsedTime;
  memcpy(pLoad->units, "pF ", 3);
  pLoad->capacitance = capacitance;
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

/*   2. 
*/


