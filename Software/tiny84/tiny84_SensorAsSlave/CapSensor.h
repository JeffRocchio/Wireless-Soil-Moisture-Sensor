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


#define IN_STRAY_CAP_TO_GND 24.48     // Stray capacitance, used as 'C1' in schematic. Adjust this value to calibrate the measurement.
#define MAX_ADC_VALUE 1023            // Fixed by the microprocessor model & specs. This for ATTiny84.
#define NUM_READINGS_TO_AVERAGE 10    // How many readings to take and compute a 'final' average reading for.
#define INTER_MEASUREMENT_DELAY 300   // Minimum number of milliseconds to wait between successive cap readings to accumulate an average.

class CapSensor {

  private:
    int _chargePin;                   // Pin number 1st lead of cap is wired to.
    int _voltReadPin;                 // Pin number 2nd lead of cap is wired to.
    short int _measurePhase;          // Were we are in the measure protocol 0:No Measurement. 1:Pluse, Read & Clear. 2: Calculate. 3: Take Average. 4: Measurement Available.
    float _capacitance;               // Measured capacitance value.
    bool _readingAvailable;           // Will be TRUE if a sensor capacitance reading has completed.
    short int _ReadingsRemain;        // The number of readings remaining in the total number we're averaging over.
    float _capAccumulator;            // Will accumulate multiple cap readings so we can take an average for the final value.
    unsigned long _nextMeasureMillis; // Wait until at least this time to take another reading.
    int _tempVolts;                   // Stores voltage reading between Phase-1 and Phase-2.


  public:

    CapSensor(int chargePin, int voltReadPin);
          /*    PURPOSE: Constructor. 
       *  Used to set the pin#/reference that the sensing capacitor is wired up to. */

    void setup();
          /*    PURPOSE: Initilize the sensor to enable readings. */

    void initiateSensorReading();
          /*    PURPOSE: Change state of the object such that we begin to make a series of 
           *  capacitance readings, which we will average out when done into a 'final' 
           *  reading of the sensor's capacitance value. */

    bool readingAvailable();
          /*    PURPOSE: Serves two purposes - One, serves as the 'update' function for
          * an ongoing cap reading request. That is, it's going to cause a bit of work 
          * to be done on making a reading, then return control back to the loop() so
          * we don't 'block' the processor. Secondly, once a reading process has been
          * completed, and is now now available, this will return TRUE so that the
          * calling program will know it can get the reading data.. It will return FALSE
          * while the reading process is ongoing. To get a new reading, and not simply
          * return an old, prior, reading be sure to call makeReading() before testing
          * this. For a somewhat graphical view of the logic of this function see:
          * /Software/Documentation/CapSensor_MeasurementProtocol.odg. */

  private:
    void pulseAndReadVolts();

};
#endif