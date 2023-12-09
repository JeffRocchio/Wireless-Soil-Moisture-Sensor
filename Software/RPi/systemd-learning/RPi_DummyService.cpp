/*  systemd Learning
 *  -----------------------------------------------------------------------------------------------------
 *
 *  C++ program to create as simple a dummy service as I can thing of.
 *
 */
#define VERSION "12-09-2023 rel 01"

//#define LOG_FILEPATH "/home/dmyservice-log.txt"
#define LOG_FILEPATH "/home/jroc/Dropbox/projects/MoistureSensor/Software/RPi/systemd-learning/dmyservice-log.txt"
#define LOG_INTERVAL 10
//#define LOG_INTERVAL 60           // This is in seconds. 60=1 minute.
//#define LOG_INTERVAL 60 * 5     // Write an entry to the log every 5 minutes.

#include <ctime>       // time()
#include <iostream>    // cin, cout, endl
#include <iomanip>     // format manipulators for use with cout
#include <string>      // string, getline()
#include <sstream>     // For ostringstream object.
#include <cstring>     // strcpy()
#include <time.h>      // CLOCK_MONOTONIC_RAW, timespec, clock_gettime()
#include <fstream>     // For writing a log text file.
#include <unistd.h>    // For the sleep() function.


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
bool logData(string);



int main(int argc, char** argv) {
    ostringstream ossConsoleDisplay;
    string currTimeFormatted;
    string progName = argv[0];
    string consoleDisplay = "";
    time_t lastLog = time(0);
    time_t logInterval = LOG_INTERVAL;
    int k = 20;


    //   Post 'announcement' of running to the console/systemlog.
    ossConsoleDisplay << argv[0] << " [" << VERSION << "] " << "Started at: " << getCurrTimeFormatted();
    cout << ossConsoleDisplay.str();
    logData(ossConsoleDisplay.str());
    ossConsoleDisplay.str("");
    ossConsoleDisplay << " || Posting " << k << " entries every " << logInterval << " seconds.";
    cout << ossConsoleDisplay.str() << endl;
    logData(ossConsoleDisplay.str());
    ossConsoleDisplay.str("");

    // Enter infinite loop.
    while(k>0) {
        if(time(0) > lastLog + logInterval) {    // Time to write log entry?
            ossConsoleDisplay.str("");
            ossConsoleDisplay << "k=" << setw(3) << k << " Time: " << getCurrTimeFormatted() << " || Dummy log entry.";
            //cout << ossConsoleDisplay.str() << endl;
            logData(ossConsoleDisplay.str());
            lastLog = time(0);
            k--;
        } else {
            sleep(logInterval);
        }
    }

    //   Post completed 'announcement.'
    ossConsoleDisplay.str("");
    ossConsoleDisplay << "*** COMPLETED AT " << getCurrTimeFormatted();
    cout << ossConsoleDisplay.str() << endl;
    logData(ossConsoleDisplay.str());
    ossConsoleDisplay.str("");

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

bool logData(string strEntry) {

    // Open a file for appending sensor readings
    std::ofstream logFile;
    logFile.open(LOG_FILEPATH, std::ios::app);

    if (!logFile.is_open()) {
        std::cerr << "Error opening the log file: ." LOG_FILEPATH << std::endl;
        return false;
    }
    // Write a log entry
    logFile << strEntry;
    logFile << endl;

    // Close the file
    logFile.close();

    return true;
}

