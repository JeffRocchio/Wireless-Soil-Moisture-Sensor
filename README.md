# Description

Wireless soil moisture sensor using capacitance as the sensing mechanism.

This is a DIY project for use in my own garden, and if it works out, my neighboorhood's community garden.

For the initial proof of concept I am making just one sensor device, which wirelessly sends its sensor data over to a Raspberry Pi for processing and display onto a terminal session (which I SSH into from my desktop PC). If the proof of concept works then I would have the Raspberry Pi post the data to a web page.

This project uses an ATTiny84 MCU and two NRF24 radio modules along with the Raspberry Pi.

The folder ProjectLog contains detailed notes on all the trials, tribulations, and successes as I go. Those log entries also contain key references - such as the Arduino core used for the ATTiny84 and the github repo for the NRF24 libraries.


# Status

**9/27/2023**: I have nearly completed work on *[Milestone #12](https://github.com/JeffRocchio/Wireless-Soil-Moisture-Sensor/blob/main/ProjectLog/MS12_a_Notes_Conversational-Software.pdf)*: Redesigning the ATTiny84 and Raspberry Pi software to be 'Event-Driven,' 'Non-Blocking,' and conversational. The only missing piece is the ability of the ATTiny to handle a command coming back from the RPi in one of it's ack packets. What I have done tho is implement a once every two hours logging function on the RPi so that I have a text file in the home directory of the RPi that I can CAT or TAIL via SSH to see the history of sensor readings. With this in place I will now deploy the sensor into an actual plant pot in my house and just see how it goes for awhile. My main interest at this point is to see if this capacitance approach to sensing soil moisture levels can actually be effective.

**July 2023**: I created a protocol of sorts for myself for managing test cases in the Arduino environment. This is documented in /home/`Software/tiny84/Readme-TESTcases.md`.
