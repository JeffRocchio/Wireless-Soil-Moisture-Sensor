/*  systemd Learning
 *  -----------------------------------------------------------------------------------------------------
 *
 *  C++ program to create as simple a dummy service as I can thing of.
 *
 */
#define VERSION "12-07-2023 rel 01"

//#define LOG_FILEPATH "/home/dmyservice-log.txt"
#define LOG_FILEPATH "/home/jroc/Dropbox/projects/systemd-learning/dmyservice-log.txt"
#define LOG_INTERVAL 10
//#define LOG_INTERVAL 60           // This is in seconds. 60=1 minute.
//#define LOG_INTERVAL 60 * 5     // Write an entry to the log every 5 minutes.

#include <ctime>       // time()
#include <iostream>    // cin, cout, endl
#include <iomanip>     // format manipulators for use with cout
#include <string>      // string, getline()
#include <cstring>     // strcpy()
#include <time.h>      // CLOCK_MONOTONIC_RAW, timespec, clock_gettime()
#include <fstream>     // For writing a log text file.


# define TIMEBUFFERSIZE 80

using namespace std;

/* =============================================================================
   Gloabal Variable Declarations
   =============================================================================
*/

    /* Custom defined timer for doing something
    */
struct timespec startTimer, endTimer;

char currTimeFormatted[TIMEBUFFERSIZE];


/* =============================================================================
   Function prototypes
   =============================================================================
*/

string getCurrTimeFormatted();
bool logData();



int main(int argc, char** argv) {
    string currTimeFormatted;
    string progName = argv[0];
    time_t lastLog = time(0);
    time_t logInterval = LOG_INTERVAL;
    int k = 20;


    // Send something to the terminal just to see what happens.
    cout << argv[0] << " [" << VERSION << "]" << endl;
    cout << endl<< "Current Time: " << getCurrTimeFormatted() << endl;
    cout << "We will post a log entry every " << logInterval << " seconds." << endl;
    cout << "We will post a total of " << k << " entries." << endl;
    cout << endl;

    // Enter infinite loop.
    while(k>0) {
        if(time(0) > lastLog + logInterval) {    // Time to write log entry?
            logData();
            cout << "k=" << setw(3) << k << " Time: " << getCurrTimeFormatted() << " || Post a log entry now." << endl;
            lastLog = time(0);
            k--;
        }
    }

    return 0;

} // Main()



/* =============================================================================
   Local Functions
   =============================================================================
*/

string getCurrTimeFormatted() {
    string formattedTime;

    // Get the current time, in a pretty string format.
    time_t now = time(0);
    tm* localTime = localtime(&now);
    char buffer[80];
    strftime(buffer,80, "%a %R %F", localTime);
    formattedTime = buffer;
    return formattedTime;
}

bool logData() {
    string currTime;

    // Open a file for appending sensor readings
    std::ofstream logFile;
    logFile.open(LOG_FILEPATH, std::ios::app);

    if (!logFile.is_open()) {
        std::cerr << "Error opening the log file." << std::endl;
        return false;
    }

    // Get the current time, in a pretty string format.
    currTime = getCurrTimeFormatted();

    // Write a log entry
    cout << currTime << "Writing a Log Entry to file.";
    logFile << currTime << ": A test log entry from running service.";
    logFile << endl;

    // Close the file
    logFile.close();

    return true;
}

