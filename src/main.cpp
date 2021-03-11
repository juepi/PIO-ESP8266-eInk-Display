/*
 * ESP8266 E-Ink Display
 * ======================
 * Displays Data on EPD fetched through MQTT
 * 
 * Includes useful functions like
 * - DeepSleep
 * - Read VCC
 * - MQTT
 * - OTA Updates (ATTN: requires MQTT!)
 * 
 * If you use DeepSleep, make sure to connect pin 16 (D0) to RESET, or your ESP will never wake up!
 * Also keep in mind that you can DeepSleep for ~ 1 hour max (hardware limitation)!
 * ATTENTION: Keep in mind that it takes quite a while after the sketch has booted until we receive messages from all subscribed topics!
 * In my case it takes up to 27 seconds (see WAIT_MILLIS_FOR_TOPICS_AFTER_BOOT).
 * To get OTA update working on windows + Arduino-IDE, you need to install python and python.exe needs to be in %PATH%
 * First flash needs to be wired of course. Afterwards Arduino IDE needs to be restarted if you cannot find
 * the ESP OTA-port in the IDE (also MQTT ota_topic needs to be set to "on" to be able to flash OTA).
 * Keep in mind that you'll need a reliable power source for OTA updates, 2x AA batteries might not work.
 * If you brick your ESP during OTA update, you can probably revive it by flashing it wired.
 */

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <PubSubClient.h>
#include "hardware-config.h"
#include "generic-config.h"
#include "wifi-config.h"
#include "mqtt-ota-config.h"
#include "common-functions.h"
#include "setup-functions.h"
#include "epd-config.h"
#include "user-config.h"

// Setup WiFi instance
WiFiClient WIFI_CLTNAME;

// Setup PubSub Client instance
PubSubClient mqttClt(MQTT_BROKER, 1883, MqttCallback, WIFI_CLTNAME);

// Setup GxEPD instance
GxIO_Class io(SPI, PCS, PDC, PRST);
GxEPD_Class display(io, PRST, PBUSY);

/*
/ Functions
*/

// Function to subscribe to MQTT topics
bool MqttSubscribe(const char *Topic)
{
  if (mqttClt.subscribe(Topic))
  {
    DEBUG_PRINTLN("Subscribed to " + String(Topic));
    SubscribedTopics++;
    mqttClt.loop();
    return true;
  }
  else
  {
    DEBUG_PRINTLN("Failed to subscribe to " + String(Topic));
    delay(100);
    return false;
  }
}

// Function to connect to MQTT Broker and subscribe to Topics
bool ConnectToBroker()
{
  // Reset subscribed/received Topics counters
  SubscribedTopics = 0;
  ReceivedTopics = 0;
  bool RetVal = false;
  int ConnAttempt = 0;
  // Try to connect x times, then return error
  while (ConnAttempt < MAXCONNATTEMPTS)
  {
    DEBUG_PRINT("Connecting to MQTT broker..");
    // Attempt to connect
    if (mqttClt.connect(MQTT_CLTNAME))
    {
      DEBUG_PRINTLN("connected");
      RetVal = true;

      // Subscribe to Topics
#ifdef OTA_UPDATE
      MqttSubscribe(ota_topic);
      MqttSubscribe(otaInProgress_topic);
#endif //OTA_UPDATE
      MqttSubscribe(INDOOR_TEMP_TOP);
      MqttSubscribe(INDOOR_RH_TOP);
      MqttSubscribe(OUTDOOR_RH_TOP);
      MqttSubscribe(OUTDOOR_TEMP_TOP);
      break;
    }
    else
    {
      DEBUG_PRINTLN("failed, rc=" + String(mqttClt.state()));
      DEBUG_PRINTLN("Sleeping 2 seconds..");
      delay(2000);
      ConnAttempt++;
    }
  }
  return RetVal;
}

/*
 * Setup
 * ========================================================================
 */
void setup()
{
// start serial port and digital Outputs
#ifdef SERIAL_OUT
  Serial.begin(BAUD_RATE);
#endif
  DEBUG_PRINTLN();
  DEBUG_PRINTLN(FIRMWARE_NAME);
  DEBUG_PRINTLN(FIRMWARE_VERSION);
#ifdef ONBOARD_LED
  pinMode(LED, OUTPUT);
  digitalWrite(LED, LEDOFF);
#endif

  // Startup WiFi
  wifi_setup();

  // Setup MQTT Connection to broker and subscribe to topics
  if (ConnectToBroker())
  {
    // New connection to broker, fetch topics
    // ATTN: will run endlessly if subscribed topics
    // does not have retained messages and no one posts a message
    DEBUG_PRINT("Waiting for topics..");
    while (ReceivedTopics < SubscribedTopics)
    {
      DEBUG_PRINT(".");
      mqttClt.loop();
#ifdef ONBOARD_LED
      ToggleLed(LED, 100, 2);
#else
      delay(100);
#endif
    }
    DEBUG_PRINTLN("");
    DEBUG_PRINTLN("All topics received.");
  }
  else
  {
    DEBUG_PRINTLN("3 connection attempts to broker failed!");
    DEBUG_PRINTLN("");
#ifdef ONBOARD_LED
    ToggleLed(LED, 100, 40);
#endif
#ifdef DEEP_SLEEP
    ESP.deepSleep(DS_DURATION_MIN * 60000000);
    delay(3000);
#else
    ESP.reset();
#endif
    delay(1000);
  }

  // Setup OTA-Updates
#ifdef OTA_UPDATE
  ota_setup();
#endif

  // Initialize e-ink display
  display.init();

#ifdef ONBOARD_LED
  // Signal setup finished
  ToggleLed(LED, 200, 6);
#endif
}

/*
 * Main Loop
 * ========================================================================
 */
void loop()
{
  // Check connection to MQTT broker and update topics
  if (!mqttClt.connected())
  {
    if (ConnectToBroker())
    {
      // New connection to broker, fetch topics
      // ATTN: will run endlessly if subscribed topics
      // does not have retained messages and no one posts a message
      DEBUG_PRINT("Waiting for topics..");
      while (ReceivedTopics < SubscribedTopics)
      {
        DEBUG_PRINT(".");
        mqttClt.loop();
#ifdef ONBOARD_LED
        ToggleLed(LED, 100, 2);
#else
        delay(100);
#endif
      }
      DEBUG_PRINTLN("");
      DEBUG_PRINTLN("All topics received.");
    }
    else
    {
      DEBUG_PRINTLN("Unable to connect to MQTT broker.");
      display.setRotation(Rotation);
      display.fillScreen(GxEPD_BLACK);
      display.setCursor(0, 150);
      display.setTextColor(GxEPD_WHITE);
      display.setFont(&FreeMonoBold24pt7b);
      display.println(" No MQTT Broker !");
      display.update();

#ifdef ONBOARD_LED
      ToggleLed(LED, 100, 40);
#endif
#ifdef DEEP_SLEEP
      ESP.deepSleep(DS_DURATION_MIN * 60000000);
      delay(3000);
#else
      ESP.reset();
#endif
    }
  }
  else
  {
    // update subscribed topics
    mqttClt.loop();
  }

#ifdef OTA_UPDATE
  // If OTA Firmware Update is requested,
  // only loop through OTA function until finished (or reset by MQTT)
  if (OTAupdate)
  {
    if (OtaInProgress && !OtaIPsetBySketch)
    {
      DEBUG_PRINTLN("OTA firmware update successful, resuming normal operation..");
      mqttClt.publish(otaStatus_topic, String(UPDATEOK).c_str(), true);
      mqttClt.publish(ota_topic, String("off").c_str(), true);
      mqttClt.publish(otaInProgress_topic, String("off").c_str(), true);
      OTAupdate = false;
      OtaInProgress = false;
      OtaIPsetBySketch = true;
      SentOtaIPtrue = false;
      SentUpdateRequested = false;
      return;
    }
    if (!SentUpdateRequested)
    {
      mqttClt.publish(otaStatus_topic, String(UPDATEREQ).c_str(), true);
      SentUpdateRequested = true;
      delay(100);
    }
    DEBUG_PRINTLN("OTA firmware update requested, waiting for upload..");
    //set MQTT reminder that OTA update was executed
    if (!SentOtaIPtrue)
    {
      DEBUG_PRINTLN("Setting MQTT OTA-update reminder flag on broker..");
      mqttClt.publish(otaInProgress_topic, String("on").c_str(), true);
      OtaInProgress = true;
      SentOtaIPtrue = true;
      OtaIPsetBySketch = true;
      delay(100);
    }
    //call OTA function to receive upload
    ArduinoOTA.handle();
#ifdef ONBOARD_LED
    // Signal OTA update requested
    ToggleLed(LED, 100, 10);
#else
    delay(1000);
#endif
    if (!DisplayShowsOTA)
    {
      display.setRotation(Rotation);
      display.fillScreen(GxEPD_BLACK);
      display.setCursor(0, 150);
      display.setTextColor(GxEPD_WHITE);
      display.setFont(&FreeMonoBold24pt7b);
      display.println(" OTA Update !");
      display.update();
      DisplayShowsOTA = true;
    }
    return;
  }
  else
  {
    if (SentUpdateRequested)
    {
      DEBUG_PRINTLN("OTA firmware update cancelled by MQTT, resuming normal operation..");
      mqttClt.publish(otaStatus_topic, String(UPDATECANC).c_str(), true);
      mqttClt.publish(otaInProgress_topic, String("off").c_str(), true);
      OtaInProgress = false;
      OtaIPsetBySketch = true;
      SentOtaIPtrue = false;
      SentUpdateRequested = false;
    }
  }
#endif //OTA_UPDATE

  // START STUFF YOU WANT TO RUN HERE!
  // ============================================

  display.setRotation(Rotation);
  //display.fillScreen(GxEPD_WHITE);
  display.setCursor(X_OFFSET, Y_OFFSET);
  display.setTextColor(GxEPD_BLACK);
#ifdef BIG
  display.setFont(&FreeMonoBold24pt7b);
#else
  display.setFont(&FreeMonoBold9pt7b);
#endif
  display.println(" Temperaturen");
  display.print("Wohnz.: ");
#ifdef HAS_RED_COLOR
  display.setTextColor(GxEPD_RED);
#endif
  display.println(String(InTemp, 1) + "C");
  display.setTextColor(GxEPD_BLACK);
  display.print("Aussen: ");
#ifdef HAS_RED_COLOR
  display.setTextColor(GxEPD_RED);
#endif
  display.println(String(OutTemp, 1) + "C");
#ifdef HAS_RED_COLOR
  display.setTextColor(GxEPD_RED);
#endif
  display.setTextColor(GxEPD_BLACK);

  display.setCursor(0, 195);
  display.println("  rel. Luftf.");
  display.print("Wohnz.: ");
#ifdef HAS_RED_COLOR
  display.setTextColor(GxEPD_RED);
#endif
  display.println(String(InRH, 0) + "%");
  display.print("Aussen: ");
#ifdef HAS_RED_COLOR
  display.setTextColor(GxEPD_RED);
#endif
  display.println(String(OutRH, 0) + "%");
  display.setTextColor(GxEPD_BLACK);
  DEBUG_PRINTLN("Starting display update..");
  display.update();
  DEBUG_PRINTLN("Display update done.");

  // Read VCC and publish to MQTT
  delay(300);
  VCC = ((float)ESP.getVcc()) / VCCCORRDIV;
  mqttClt.publish(vcc_topic, String(VCC).c_str(), true);
  delay(100);

#ifdef DEEP_SLEEP
  // disconnect WiFi and go to sleep
  DEBUG_PRINTLN("Good night for " + String(DS_DURATION_MIN) + " minutes.");
  WiFi.disconnect();
  ESP.deepSleep(DS_DURATION_MIN * 60000000);
#else
  DEBUG_PRINTLN("Loop finished, DeepSleep disabled. Restarting in 5 seconds.");
#endif
  //ATTN: Sketch continues to run for a short time after initiating DeepSleep, so pause here
  delay(5000);
}