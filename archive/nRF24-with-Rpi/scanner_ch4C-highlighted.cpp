/*
 * To Copy over to the PI:
 * scp /home/jroc/Dropbox/projects/MoistureSensor/nRF24-with-Rpi/scanner_ch4C-highlighted.cpp pi@192.168.1.118:/home/rf24libs/RF24/examples_linux/scanner_ch4C-highlighted.cpp
 *
 *
 Copyright (C) 2011 J. Coliz <maniacbug@ymail.com>

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 version 2 as published by the Free Software Foundation.

06/17/2022 : Jeff Rocchio
                Confirmed this is the version that I fixed for that 1-character offset issue.
                I copied this over to the Pi and reran cd /build  || make to compile it.
                I then ran it and Confirmed it works correctly.

04/27/2022 : Jeff Rocchio
                Modified to use formatting strings for simplier logic

04/22/2022 : Jeff Rocchio
                Modified to highlight channel 4C so I can focus
                on seeing my ATTiny84 driven nRF24 module is
                active and transmitting.


 03/17/2013 : Charles-Henri Hallard (http://hallard.me)
              Modified to use with Arduipi board http://hallard.me/arduipi
                          Changed to use modified bcm2835 and RF24 library

 */

/**
 * Channel scanner
 *
 * Example to detect interference on the various channels available.
 * This is a good diagnostic tool to check whether you're picking a
 * good channel for your application.
 *
 * Inspired by cpixip.
 * See http://arduino.cc/forum/index.php/topic,54795.0.html
 */

#include <cstdlib>
#include <iostream>
#include <RF24/RF24.h>

using namespace std;

/****************** Linux ***********************/
// Radio CE Pin, CSN Pin, SPI Speed
// CE Pin uses GPIO number with BCM and SPIDEV drivers, other platforms use their own pin numbering
// CS Pin addresses the SPI bus number at /dev/spidev<a>.<b>
// ie: RF24 radio(<ce_pin>, <a>*10+<b>); spidev1.0 is 10, spidev1.1 is 11 etc..

// Generic:
RF24 radio(22, 0);
/****************** Linux (BBB,x86,etc) ***********************/
// See http://nRF24.github.io/RF24/pages.html for more information on usage
// See http://iotdk.intel.com/docs/master/mraa/ for more information on MRAA
// See https://www.kernel.org/doc/Documentation/spi/spidev for more information on SPIDEV

// Channel of interest - channel to highlight
int highlight_channel = 0x4C; // = 76

// To hold string representation of channel hit-counts
char crCts[] = "-";
// printf stings for normal and highlighted
char chNorm[] = "%s";
char chHiLight[] = "| %s |";
char formatStr[sizeof(chHiLight)];

// Channel info
const uint8_t num_channels = 126;
uint8_t values[num_channels];

const int num_reps = 100;
int reset_array = 0;

int main(int argc, char** argv)
{
    // Print preamble

    // print example's name
    printf("%s", argv[0]);

    //
    // Setup and configure rf radio
    //
    radio.begin();

    radio.setAutoAck(false);

    // Get into standby mode
    radio.startListening();
    radio.stopListening();


    radio.printDetails();

    // Print out header. In two lines, list each
    // channel number, high digit, then low digit
    printf("\n");
    printf("%s \n", "Each line below will show how many times a carrier was detected, in 100 tries, on each of the 127 channels");
    printf("%s \n", "First two lines are a header, showing each of the 127 channel numbers, in HEX");
    printf("\n");

    char buf[] = "00";
    int i = 0;
    while (i < num_channels) {
        strcpy(formatStr, chNorm);
        if (i == highlight_channel) strcpy(formatStr, chHiLight);
        sprintf(buf, "%X", i >> 4); // Convert int i to text and put that in char buf[].
        printf(formatStr, buf);
        ++i;
    }
    printf("\n");
    i = 0;
    while (i < num_channels) {
        strcpy(formatStr, chNorm);
        if (i == highlight_channel) strcpy(formatStr, chHiLight);
        sprintf(buf, "%X", i & 0xf);
        printf(formatStr, buf);
        ++i;
    }
    printf("\n");
    printf("\n");

    // forever loop - scan all channels, 100 times / line, and show how many carrier hits we detected per channel.
    while (1) {
        // Clear measurement values
        memset(values, 0, sizeof(values));

        // Scan all channels num_reps times
        int rep_counter = num_reps;
        while (rep_counter--) {

            int i = num_channels;
            while (i--) {

                // Select this channel
                radio.setChannel(i);

                // Listen for a little
                radio.startListening();
                delayMicroseconds(128);
                radio.stopListening();

                // Did we get a carrier?
                if (radio.testCarrier()) {
                    ++values[i];
                }
            }
        }

        // Print out channel carrier detect counts, tho clamped to a single hex digit so might be undercounted.
        i = 0;
        while (i < num_channels) {
            strcpy(formatStr, chNorm);  // Set or restore the formatting string to non-highlight.
            char crCts[] = "-";
            if (i == highlight_channel) strcpy(formatStr, chHiLight);
            if (values[i]) {  // At least 1 count of a carrier hit in the 100 tries.
                sprintf(crCts, "%X", min(0xf, (values[i] & 0xf)));
            }
            else crCts[0] = '-';
            printf(formatStr, crCts); // ORIGINAL: printf(formatStr, min(0xf, (values[i] & 0xf)));
            strcpy(formatStr, chNorm);  // Restore the formatting string to non-highlight.
            ++i;
        }
        printf("\n");
    }

    return 0;
}

// vim:ai:cin:sts=2 sw=2 ft=cpp

