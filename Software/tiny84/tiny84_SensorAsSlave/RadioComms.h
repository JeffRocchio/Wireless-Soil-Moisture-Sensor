// Class: RadioComms - Class Definition
//=================================================================================================

#ifndef RadioComms_h
#define RadioComms_h

#include "Arduino.h"
#include <SPI.h>
#include "RF24.h"


/************************************************************************************************
*
*    PURPOSE: Manages conversational communications between the ATTiny84 and the Raspberry Pi
* using the nRF24 radio chip and associated nRF24 Arduino library.
*
*    USAGE:
*    1. RadioComms object must be created (declared) using the pin# reference that the sensor
* is wired up to.
*    2. You call the .begin() method to initiate the nRF24 radio. Typically you'll do this 
* in the sketch's setup() function.
*    3. You must call the .update() method as part of the sktech's loop() function for the 
* radio conversation protocol to operate and do it's thing. It is operating in a non-blocking
* manner, so it is doing bits and pieces of comms work on each pass of the loop(); trying
* hard not to keep an exclusive hold over the processor so that other operations can run
* in between comms activities.
*
*    NOTE:
*    1. By design, this class is non-blocking.
*/


class RadioComms {

  private:
    RF24 _radioChip;                        // The nRF24 radio object (defined in the RF25.h library).
    int _cePin;                             // 'Chip Enable.' CE pin nRF24 is wired to.
    int _csnPin;                            // 'Chip Select Not.' SPI chip select pin nFR24 is wired to.
    const char * _addressMaster = "1Node";  // Address of the my master. I send messages to this address.
    const char * _addressSelf = "2Node";    // The address of 'me' - I receive messages addressed with this.
    unsigned long _txWaitDelay = 30;        // How long to wait between repeated transmission retries.
    unsigned long _ackWaitDelay = 30;       // How long to wait for an ACK response after a transmission.
    unsigned long _lastMillis;              // Timestamp of when we last had a slice of the CPU to do work.
    short int _phase = 0;                   // Comms phase we are in.
    bool _rxPayloadAvailable;               // We have a received payload.
    bool _radioAvail = false;               // True if the radio is up and running.
    struct TxPayloadStruct {                // struct to accumulate txPayload data.
      float capacitance;
      uint32_t chargeTime;            // Time it took for capacitor to charge.
      uint32_t ctSuccess;             // count of success Tx attempts tiny84 has seen since boot
      uint32_t ctErrors;              // count of Tx errors tiny84 saw since last successful transmit
      char units[4];                  // nFD, mFD, FD
      char statusText[12];            // For use in debugging. Be sure there is space for a NULL terminating char
    };
    TxPayloadStruct _txPayload;
    struct RxPayloadStruct {                // struct for incoming ACK response from Master.
      char message[11];               // Incoming message up to 10 chrs+Null.
      uint8_t counter;
    };
    RxPayloadStruct _rxAckPayload;

  public:

          /*    PURPOSE: Constructor. 
       *  Params are the pins the RF24 chip is wired up to.*/
    RadioComms(int chargePin, int voltReadPin);

          /*    PURPOSE: Initilize the radio. 
           *    RETURNS: True if radio chip is properly activated.
           *             False if something when wrong in activation.  */
    bool setup();


          /*    PURPOSE: Take next action in radio Tx/Rx, in non-blocking mode. 
           * This function is intended to be called once for every pass of the 
           * main processing loop. Based on current Tx/Rx phase it will take
           * the appropriate action. 
           *    RETURNS: Tx/Rx Phase we are in as a result of the update process. */
    short int update();


          /*    PURPOSE: Set the data to transmit in next transmit cycle. */
    void setTxPayload(float fCap);


    //void initiateSensorReading();
          /*    PURPOSE: Change state of the object such that we begin to make a series of 
           *  capacitance readings, which we will average out when done into a 'final' 
           *  reading of the sensor's capacitance value. */

    //bool readingAvailable();
          /*    PURPOSE: Serves two purposes - One, serves as the 'update' function for
          * an ongoing cap reading request. That is, it's going to cause a bit of work 
          * to be done on making a reading, then return control back to the loop() so
          * we don't 'block' the processor. Secondly, once a reading process has been
          * completed, and is now now available, this will return TRUE so that the
          * calling program will know it can get the reading data.. It will return FALSE
          * while the reading process is ongoing. To get a new reading, and not simply
          * return an old, prior, reading be sure to call makeReading() before testing
          * this. For a somewhat graphical view of the logic of this function see:
          * /Software/Documentation/RadioComms_MeasurementProtocol.odg. */

  //private:
    //void pulseAndReadVolts();

};
#endif