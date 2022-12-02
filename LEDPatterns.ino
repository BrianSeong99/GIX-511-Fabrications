#include <FastLED.h>

#define LED_PIN     5
#define LED_POWER 13  
#define NUM_LEDS    72
#define BRIGHTNESS  255
#define LED_TYPE    WS2811
#define COLOR_ORDER GRB
CRGB leds[NUM_LEDS];
uint8_t mode = 5;

uint8_t updates_per_second[6] = {
  100, 100, 1, 1, 1, 100,
}; // access via `mode`

extern const uint8_t ArrowPattern[6*12] = {
  1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
  1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
  1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1,
};

extern const CRGB ArrowPalette[2] = {
  CRGB::Green, CRGB::Black,
};

extern const uint8_t HorizontalHeartPattern[2][6*12] = {
  {
    1, 1, 1, 1, 0, 1, 0, 1, 1, 1, 1, 1,
    1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 1, 1,
    1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1,
    1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 1, 1, 
    1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1,
  },
  {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 0, 1, 0, 1, 1, 1, 1, 1,
    1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 1, 1,
    1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 
    1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  }
};

extern const uint8_t VerticalHeartPattern[2][6*12] = {
  {
    1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1,
    1, 1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 1, 
    1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 1, 1,
    1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 1, 1,
    1, 1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 1,
    1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1,
  },
  {
    1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 
    1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1,
  }
};

extern const CRGB HeartPalette[2] = {
  CRGB::Red, CRGB::Black,
};

extern const uint8_t GIXPattern[6*12] = {
  0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 1, 0,
  0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 0, 
  0, 1, 1, 1, 1, 1, 0, 1, 1, 1, 0, 1,
  0, 1, 0, 0, 1, 1, 0, 1, 1, 1, 0, 1,
  0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 0,
  0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 1, 0,
};

extern const CRGB GIXPalette[2] = {
  CRGB::Yellow, CRGB::Black,
};

void setup() {
    delay( 3000 ); // power-up safety delay
    FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection( TypicalLEDStrip );
    FastLED.setBrightness(  BRIGHTNESS );

    pinMode(LED_POWER,OUTPUT); 
    digitalWrite(LED_POWER,HIGH);
}

void loop() {    
    static uint8_t motionIndex = 0;
    motionIndex = motionIndex + 1; /* motion speed */
    PaletteFillIn(motionIndex);
        
    FastLED.show();
    FastLED.delay(1000 / updates_per_second[mode]);
}

uint8_t GetLEDIndex(uint8_t row, uint8_t col) {
  return row * 12 + col;
}

// There are several different palettes of colors demonstrated here.
//
// FastLED provides several 'preset' palettes: RainbowColors_p, RainbowStripeColors_p,
// OceanColors_p, CloudColors_p, LavaColors_p, ForestColors_p, and PartyColors_p.
//
// Additionally, you can manually define your own color palettes, or you can write
// code that creates color palettes on the fly.  All are shown here.

void PaletteFillIn(uint8_t motionIndex)
{
  // uint8_t secondHand = (millis() / 1000) % 80;
  // static uint8_t lastSecond = 99;
  if (mode == 0) // LED light Strips
  {
    for( int i = 0; i < NUM_LEDS; i++) {
      leds[i] = ColorFromPalette( RainbowStripeColors_p, motionIndex, BRIGHTNESS, LINEARBLEND);
      motionIndex += 3;
    }
  }
  else if (mode == 1) // raindrop
  {

  }
  else if (mode == 2) // arrow
  {
    uint8_t currentOffset = motionIndex % 12;
    for (uint8_t i = 0; i < 6; i ++) {
      for (uint8_t j = 0; j < 12; j ++) {
        if (i % 2 == 0) {
          leds[GetLEDIndex(i, j)] = ArrowPalette[ArrowPattern[GetLEDIndex(i, (j+currentOffset) % 12)]];
        } else {
          leds[GetLEDIndex(i, 11-j)] = ArrowPalette[ArrowPattern[GetLEDIndex(i, (j+currentOffset) % 12)]];
        }
      }
    }        
  }
  else if(mode == 3) // heart
  {        
    uint8_t currentOffset = motionIndex % 2;
    for (uint8_t i = 0; i < 6; i ++) {
      for (uint8_t j = 0; j < 12; j ++) {
        if (i % 2 == 0) {
          leds[GetLEDIndex(i, j)] = HeartPalette[HorizontalHeartPattern[currentOffset][GetLEDIndex(i, j)]];
        } else {
          leds[GetLEDIndex(i, 11-j)] = HeartPalette[HorizontalHeartPattern[currentOffset][GetLEDIndex(i, j)]];
        }
      }
    }
  }
  else if (mode == 4) // GIX Yellow
  {
    for (uint8_t i = 0; i < 6; i ++) {
      for (uint8_t j = 0; j < 12; j ++) {
        if (i % 2 == 0) {
          leds[GetLEDIndex(5-i, 11-j)] = GIXPalette[GIXPattern[GetLEDIndex(i, j)]];
        } else {
          leds[GetLEDIndex(5-i, j)] = GIXPalette[GIXPattern[GetLEDIndex(i, j)]];
        }
      }
    } 
  }
  else if (mode == 5) // GIX colors
  {
    for (uint8_t i = 0; i < 6; i ++) {
      for (uint8_t j = 0; j < 12; j ++) {
        if (i % 2 == 0) {
          leds[GetLEDIndex(5-i, 11-j)] = GIXPattern[GetLEDIndex(i, j)] == 0 ? ColorFromPalette( RainbowStripeColors_p, motionIndex, BRIGHTNESS, LINEARBLEND) : CRGB::Black;
        } else {
          leds[GetLEDIndex(5-i, j)] = GIXPattern[GetLEDIndex(i, j)] == 0 ? ColorFromPalette( RainbowStripeColors_p, motionIndex, BRIGHTNESS, LINEARBLEND) : CRGB::Black;
        }
        motionIndex += 3;
      }
    } 
  }
}