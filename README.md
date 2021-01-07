# Introduction 
Sketch to show data fetched from an MQTT broker (Topics) on E-Ink display. Based on [PIO-ESP8266-Template](https://github.com/juepi/PIO-ESP8266-Template).

## Local Requirements
A (local) MQTT broker is mandatory for OTA-Flashing and receiving data to be displayed.  

## Generic Requirements
Please see [PIO-ESP8266-Template](https://github.com/juepi/PIO-ESP8266-Template) for general requirements and usage of this sketch.

## Accessing the E-Ink Display
[GxEPD](https://github.com/ZinggJM/GxEPD) library used to access e-ink display. I'm currently using a waveshare 2.13" B/W/R display (Type GDEW0213Z16).
Note that the default pinout used to connect the display to the ESP has been slightly modified for the used Wemos D1 Mini board.