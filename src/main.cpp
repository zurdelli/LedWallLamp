#include <Arduino.h>
#include <FastLED.h> // Libreria de leds
#include <PinButton.h> // Libreria para gestionar el boton

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
    break;
  case 3:
    FastLED.setBrightness(BRIGHTNESS + 30 );
    if (FastLED.getBrightness() > 250) FastLED.setBrightness(100);
  default:
    break;
  }

  if (colorCounter > 17) colorCounter = 0;
  if (paletteCounter > 11) paletteCounter = 0;
}


void setup() {
  
  delay(1000); // power-up safety delay

  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection( TypicalLEDStrip );
  FastLED.setBrightness(BRIGHTNESS);
  FastLED.clear();
}

void loop() {
  FunctionButton.update();

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
