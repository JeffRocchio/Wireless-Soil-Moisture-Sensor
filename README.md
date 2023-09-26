# Description

Wireless soil moisture sensor using capacitance as the sensing mechanism.

This is a DIY project for use in my own garden, and if it works out, my neighboorhood's community garden.

For the initial proof of concept I am making just one sensor device, which wirelessly sends its sensor data over to a Raspberry Pi for processing and display onto a terminal session (which I SSH into from my desktop PC). If the proof of concept works then I would have the Raspberry Pi post the data to a web page.

This project uses an ATTiny84 MCU and two NRF24 radio modules along with the Raspberry Pi.

The folder ProjectLog contains detailed notes on all the trials, tribulations, and successes as I go. Those log entries also contain key references - such as the Arduino core used for the ATTiny84 and the github repo for the NRF24 libraries.


# Status

**9/26/2023**: I am working on *Milestone #12*: Redesigning the ATTiny84 and Raspberry Pi software to be 'Event-Driven,' 'Non-Blocking,' and conversational. I am close to completing this. I just worked out what was causing Issue #1. So I now need to finalize the fixup for that issue, then implement the ability to handle commands on the ATTiny side.

**July 2023**: I created a protocol of sorts for myself for managing test cases in the Arduino environment. This is documented in /home/`Software/tiny84/Readme-TESTcases.md`.
