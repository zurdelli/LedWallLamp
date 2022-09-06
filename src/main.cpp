// #include <avr/sleep.h>
// #include <EEPROM.h>

#include <Arduino.h>
#include <ESP8266WiFi.h> 
#include <WiFiClient.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266mDNS.h>
#include <PubSubClient.h>  // for MQTT connection
#include <FastLED.h> // Libreria de leds
#include <PinButton.h> // Libreria para gestionar el boton
#include <ArduinoOTA.h> // Libreria para gestionar subir codigo por internet
#include "ESP8266_Utils_OTA.hpp" // Libreria para gestionar subir codigo por internet
#include "FS.h" // SPIFFS
#include <ESPAsyncWebServer.h>
#include <ESPAsyncWiFiManager.h>
#include "server.hpp"

const char* mqttServer = "192.168.1.18";
const char* inTopic = "ledwallin";
const char* outTopic = "ledwallout";
char msg[75];
long lastMsg = 0;
int value = 0;
String dString = "";
WiFiClient espClient;
PubSubClient client(espClient);

AsyncWebServer server(80);
DNSServer dns;
AsyncWiFiManager wifiManager(&server,&dns);

bool ledOn = false;

// define the LEDs
#define LED_PIN 6
#define WAKE_UP_PIN 4
#define NUM_LEDS 32
#define BRIGHTNESS 160 //maximum brightness
#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB
struct CRGB leds[NUM_LEDS];
#define UPDATES_PER_SECOND 100

#include "solid_color_mode.h"
#include "palette_mode.h"
#include "effect_mode.h"

PinButton FunctionButton(WAKE_UP_PIN);
int setMode = 0;

void changeState(int signal){
  if (signal) ledOn = true;
  else ledOn = false;
}

void singleClick() {
  switch (setMode) {
  case 0:
    colorCounter++;
    break;
  case 1:
    paletteCounter++;
    break;
  case 2:
    nextPattern();
  default:
    break;
  }

  if (colorCounter > 17) colorCounter = 0;
  if (paletteCounter > 11) paletteCounter = 0;
}

void reconnect() {  //connect to MQTT with a client ID, subscribe & publish to corresponding topics
  // loop until reconnected
  while (!client.connected()) {
    if (client.connect("ledWallClient")) {
      client.publish(outTopic, "Hello world, I'm ledWallClient");
      client.subscribe(inTopic);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(4000);
    }
  }
}


// handle message arrived from MQTT and do the real actions depending on the command
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  // debugging message at serial monitor
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  /**
   * Here payloads the message.
   * R = Reset
   * 1 = Solid color
   * 2 = Palette
   * 3 = Effects
   * 4 = Next
   * 5 = Turn on
   * 6 = Turn off
   */
  char received = (char)payload[0];
  switch (received)
  {
  case 'R':
    snprintf(msg, 75, "Resetting Wifi");
    wifiManager.resetSettings();
    break;
  case '1':
    setMode = 0;
    snprintf(msg, 75, "Solid");
    break;
  case '2':
    setMode = 1;
    snprintf(msg, 75, "Palette");
    break;
  case '3':
    setMode = 2;
    snprintf(msg, 75, "Effect");
    break;
  case '4':
    singleClick();
    snprintf(msg, 75, "Next");
    break;
  case '5':
    changeState(1);
    snprintf(msg, 75, "Turn On");
    break;
  case '6':
    changeState(0);
    snprintf(msg, 75, "Turn Off");
    break;
  default:
    snprintf(msg, 75, "Unknown command: %c, do nothing!", (char)payload[0]);
    break;
  }
  client.publish(outTopic, msg);
}


void initServer(){
  server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");

   server.on("/LED", HTTP_POST, [](AsyncWebServerRequest *request) {
      ledOn = !ledOn;
      request->redirect("/");
   });

   server.onNotFound([](AsyncWebServerRequest *request) {
      request->send(400, "text/plain", "Not found");
   });

   server.begin();
   Serial.println("HTTP server started");
}

void handleMqtt(){
  // if not connected to mqtt server, keep trying to reconnect
  if (!client.connected()) {
    reconnect();
  }
  client.loop();  // wait for message packet to come & periodically ping the server
  // to show that ESP8266 is alive, publish a message every 2 seconds to the MQTT broker
  long now = millis();
  if (now - lastMsg > 2000) {
    lastMsg = now;
    ++value;
    snprintf(msg, 75, "Hello world #%ld", value);
    Serial.print("Publish message: ");
    Serial.println(msg);
    client.publish(outTopic, msg);
  }
}


void setup() {
  
  delay(1000); // power-up safety delay

  // MQTT
  //client.setServer(mqttServer, 1883); // prepare for MQTT connection
  //client.setCallback(mqttCallback);

  //InitOTA(); // Inicia para subir a traves de internet
  
  // SPIFFS
  //SPIFFS.begin();

  //wifiManager.autoConnect("ZurimuriAP");
  //wifiManager.setTimeout(120);


  //initServer();

  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection( TypicalLEDStrip );
  FastLED.setBrightness(BRIGHTNESS);
  FastLED.clear();
}

void loop() {
  FunctionButton.update();
  //ArduinoOTA.handle();
  
  //handleMqtt();

  if (FunctionButton.isSingleClick()) singleClick();
  if (FunctionButton.isDoubleClick()) {
    setMode++;
    if (setMode > 3) setMode = 0;
  } 
  
  if (FunctionButton.isLongClick()) ledOn = !ledOn;


  if (ledOn) {
    switch (setMode){
    case 0:
      if (colorCounter % 2 == 0) {
        float breath = (exp(sin(millis() / 2000.0 * PI)) - 0.36787944) * 108.0;
        FastLED.setBrightness(breath);
      } else {FastLED.setBrightness(BRIGHTNESS); }
      ChangeColorPeriodically();
      break;
    case 1:
      FastLED.setBrightness(BRIGHTNESS);
      ChangePalettePeriodically();
      static uint8_t startIndex = 0;
      startIndex = startIndex + 1;
      FillLEDsFromPaletteColors(startIndex);
      break;
    case 2:
      gPatterns[gCurrentPatternNumber]();
      break;
    case 3:
      fill_solid( leds, NUM_LEDS, CHSV(0, 0, 192));
    default:
      break;
    }
    
    FastLED.show();
    FastLED.delay(2000 / UPDATES_PER_SECOND);
    EVERY_N_MILLISECONDS( 20 ) {
      gHue++;  // slowly cycle the "base color" through the rainbow
    }
  } else {
    FastLED.clear();
    FastLED.show();
  }

}
