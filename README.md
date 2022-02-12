# Pomodoro

This is a tomato-shaped timer that counts down from 25 minutes.

## Hardware

* ATmega328p or compatible (I used ATmega8 due to supply chain issues)
* USB Micro-B power
* Four-digit 7-segment multiplexed display with colon LEDs
* Reset button
* Wheel potentiometer to adjust brightness
* Extra GPIO is exposed on a header
* Designed to be mostly assembled by JLCPCB

## Software

* Hard-coded 25 minute countdown
* Flashes 00:00 for one minute after countdown finishes
* Full shutdown after count is complete to save power
* Brightness adjustment goes down "off" to reduce visual distraction. At the start of the countdown, the display will start on and fade down to the selected brightness after 10 seconds. The flashing at the end of the countdown ignores the brightness setting.
