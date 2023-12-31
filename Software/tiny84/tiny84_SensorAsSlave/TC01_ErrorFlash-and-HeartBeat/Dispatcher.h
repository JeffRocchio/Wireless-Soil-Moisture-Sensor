// Class: Dispatcher - Class Definition
//=================================================================================================

#ifndef Dispatcher_h
#define Dispatcher_h

#include "Arduino.h"

/************************************************************************************************
*    PURPOSE: Is the 'controller' in MVC perspective. Receives information and input and 
* determines how to direct processing. This class is key to creating non-blocking apps. In
* every iteration of the loop() function this class/object will evaluate the current state of
* in-flight activities (think of those as virtual threads), plus any new inputs from the
* outside world, and direct the firing off of appropriate new activities or the termination of
* completed activities. So the Dispatcher controls the execution and flow of all other
* activities.
*
*    USAGE:
*    1. Dispatcher object must be created (declared) as a global variable in the sketch.
*    2. You call the .begin() method to initiate the dispatcher. Typically you'll do this 
* in the sketch's setup() function.
*    3. You must call the .dispatch() method as part of the sktech's loop() function.
*
*    NOTE:
*    1. By design, this class is non-blocking.
*/


class Dispatcher {

  private:
    unsigned long _testErrorFrequency;
    unsigned long _errorStartTime;


  public:
    Dispatcher();
          /*      PURPOSE: Constructor. */
    void begin();
          /*    PURPOSE: Start sensor reading process. */
    void dispatch();
          /*    PURPOSE: Assesses changed states and new inputs and uses that 
           *  information to begin/ terminate /continue all other activities.
           *  Intended to be called once each loop() cycle. */

};
#endif
