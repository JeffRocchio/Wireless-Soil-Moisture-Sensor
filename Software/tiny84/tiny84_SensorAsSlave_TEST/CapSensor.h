// Class: CapSensor - Class Definition
//=================================================================================================

#ifndef CapSensor_h
#define CapSensor_h

#include "Arduino.h"

/************************************************************************************************
*    PURPOSE: Manages the soil sensor capacitor.
*
*    USAGE:
*    1. CapSensor object must be created (declared) using the pin# reference that the sensor
*  wired up to.
*    2. You call the .begin() method to initiate the error LED cycle - do this first, even 
*  when no error condition yet exists. Typically you'll do this in the sketch's setup() 
*  function, but could be done anywhere.
*    3. You must call the .update() method as part of the sktech's loop() function for the 
*  error LED to be responsive to a reported error.
*    4. To report an error, call the setError() function.
*
*    NOTE:
*    1. By design, this class is non-blocking.
*/


#define IN_STRAY_CAP_TO_GND 24.48    // Stray capacitance, used as 'C1' in schematic. Adjust this value to calibrate the measurement.
#define MAX_ADC_VALUE 1023           // Fixed by the microprocessor model & specs. This for ATTiny84.

class CapSensor {

  private:
    int _chargePin;                   // Pin number 1st lead of cap is wired to.
    int _voltReadPin;                 // Pin number 2nd lead of cap is wired to.

    float _capacitance;                // Measured capacitance value.

  public:
    CapSensor(int pin);
          /*      PURPOSE: Constructor. 
       *  Used to set the pin#/reference that the sensing capacitor is wired up to. */
    void startReading();
          /*    PURPOSE: Start sensor reading process. */
    void readingAvailable();
          /*    PURPOSE: Serves two purposes - One, serves as the 'update' function for
          * an ongoing cap reading request. That is, it's going to cause a bit of work 
          * to be done on making a reading, then return control back to the loop() so
          * we don't 'block' the processor. Secondly, once a reading process has been
          * completed, and is now now available, this will return TRUE so that the
          * calling program will know it can get the reading data.. It will return FALSE
          * while the reading process is ongoing. To get a new reading, and not simply
          * return an old, prior, reading be sure to call makeReading() before testing
          * this. */
    void update();
          /*    PURPOSE: Determine if it is time to change the state of
           *  the error LED per the error report-out cycle. And if so, 
           *  does it. */

    void setError(short int errorID);
          /*    PURPOSE: Set an error to report out by blinking of 
           *  the error LED. */

    short int getErrorID();
          /*    PURPOSE: Can be used in the sktech to determine if an
           *  error condition is currently being reported out, and if so,
           *  obtain it's ID# (maybe for use, e.g., to transmit that info
           *  over to the sensor-master server (RPi).*/

  private:
    void clean();

};
#endif