/*
*   E-Ink Display / GxEPD Library Configuration
*   ===========================================
*/
#ifndef EPD_CONFIG_H
#define EPD_CONFIG_H

#include <Arduino.h>
#include <GxEPD.h>
#include <GxIO/GxIO_SPI/GxIO_SPI.h>
#include <GxIO/GxIO.h>
// BUGFIX: following include required to compile
// see https://community.platformio.org/t/adafruit-gfx-lib-will-not-build-any-more-pio-5/15776/7
#include <Adafruit_I2CDevice.h>

// ePaper Display pin definitions
// mapping suggestion from Waveshare SPI e-Paper to Wemos D1 mini
// BUSY -> D2, RST -> D4, DC -> D3, CS -> D1, CLK -> D5, DIN -> D7, GND -> GND, 3.3V -> 3.3V
// NOTE: do not use D8 for CS, because ESP won't boot! Using D1 instead
#define PBUSY D2
#define PRST D6
#define PDC D3
#define PCS D1
#define PCLK D5
#define PDIN D7

// Select used display through correct .h file
#define BIG
//#define SMALL
#ifdef BIG
#include <GxGDEW042T2/GxGDEW042T2.h>      // 4.2" b/w
#else
#include <GxGDEW0213Z16/GxGDEW0213Z16.h>  // 2.13" b/w/r
#define HAS_RED_COLOR
#endif

// Display Rotation 0 = Portrait
// Rotation 1 = Landscape
// inverse for 4.2" display!
#ifdef BIG
#define ROTATION 0
#else
#define ROTATION 1
#endif
extern uint16_t Rotation;

// Pixel Offset for first line
#ifdef BIG
#define X_OFFSET 0
#define Y_OFFSET 35
#else
#define X_OFFSET 0
#define Y_OFFSET 10
#endif

// Add Fonts
#ifdef BIG
//#include <Fonts/FreeMonoBold18pt7b.h>
//#include <Fonts/FreeMonoBold12pt7b.h>
#include <Fonts/FreeMonoBold24pt7b.h>
#else
#include <Fonts/FreeMonoBold9pt7b.h>
#endif

#endif //EPD_CONFIG_H