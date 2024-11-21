#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unity.h>

// Basic type definitions
typedef uint8_t byte;

// Mock NeoPixel definitions
#define NEO_RGB 0x06
#define NEO_GRB 0x52
#define NEO_KHZ800 0x0000

// Effect mode definitions
#define FX_MODE_MULTI_COMET 0
#define FX_MODE_NOISEMETER 1

// Mock audio variables
float fftResult[16];
float FFT_MajorPeak = 0;
float FFT_Magnitude = 0;
float sampleAvg = 0;
int samplePeak = 0;
int soundSquelch = 10;

// Mock time
unsigned long _millis = 0;
unsigned long millis() { return _millis; }

// Utility functions
int random(int min, int max) { return min + rand() % (max - min + 1); }

int random(int max) { return rand() % (max + 1); }

float constrain(float value, float min, float max) {
  if (value < min) return min;
  if (value > max) return max;
  return value;
}

int map(int value, int fromLow, int fromHigh, int toLow, int toHigh) {
  return (value - fromLow) * (toHigh - toLow) / (fromHigh - fromLow) + toLow;
}

// Mock LED Strip class
class MockStrip {
 private:
  struct Pixel {
    uint32_t color;
    uint8_t brightness;
  };

  Pixel pixels[30];
  uint8_t brightness;
  uint8_t currentMode;
  uint8_t speed;
  uint8_t intensity;
  uint16_t numLeds;

 public:
  MockStrip()
      : brightness(128),
        currentMode(0),
        speed(128),
        intensity(128),
        numLeds(30) {
    for (int i = 0; i < 30; i++) {
      pixels[i] = {0, 0};
    }
  }

  void init() {
    for (int i = 0; i < numLeds; i++) {
      pixels[i].color = 0;
      pixels[i].brightness = 0;
    }
  }

  void setBrightness(uint8_t b) { brightness = b; }
  void setMode(uint8_t m) { currentMode = m; }
  void setSpeed(uint8_t s) { speed = s; }
  void setIntensity(uint8_t i) { intensity = i; }
  uint16_t getLength() { return numLeds; }

  uint32_t getPixelColor(uint16_t i) {
    if (i >= numLeds) return 0;
    return pixels[i].color;
  }

  void setPixelColor(uint16_t i, uint32_t c) {
    if (i < numLeds) {
      pixels[i].color = c;
      pixels[i].brightness = brightness;
    }
  }

  void service() {
    switch (currentMode) {
      case FX_MODE_MULTI_COMET:
        serviceMultiComet();
        break;
      case FX_MODE_NOISEMETER:
        serviceNoisemeter();
        break;
    }
  }

 private:
  void serviceMultiComet() {
    // Simulate comet effect
    for (int i = 0; i < numLeds; i++) {
      if (pixels[i].color > 0) {
        pixels[i].color = (pixels[i].color * 240) >> 8;
      }
    }

    // Create new comet based on bass
    float bassLevel = 0;
    for (int i = 0; i < 4; i++) {
      bassLevel += fftResult[i];
    }
    bassLevel /= 4.0f;

    if (bassLevel > soundSquelch * 2) {
      int pos = random(numLeds);
      uint32_t color = 0xFF0000;  // Red comet
      setPixelColor(pos, color);
    }
  }

  void serviceNoisemeter() {
    static float minLevel = 0;
    static float maxLevel = 1;

    minLevel = minLevel * 0.9 + sampleAvg * 0.1;
    maxLevel = maxLevel * 0.9 + sampleAvg * 0.1;

    float level = (sampleAvg - minLevel) / (maxLevel - minLevel);
    level = constrain(level, 0.0f, 1.0f);

    int activePixels = level * numLeds;

    for (int i = 0; i < numLeds; i++) {
      setPixelColor(i, i < activePixels ? 0x00FF00 : 0);
    }
  }
};

// Global mock strip instance
MockStrip strip;

// Mock audio data generators
void mockQuietAudio() {
  for (int i = 0; i < 16; i++) {
    fftResult[i] = random(0, 10);
  }
  sampleAvg = random(0, 20);
  FFT_MajorPeak = 0;
  FFT_Magnitude = 0;
  samplePeak = 0;
}

void mockBassyAudio() {
  for (int i = 0; i < 4; i++) {
    fftResult[i] = random(180, 255);
  }
  for (int i = 4; i < 16; i++) {
    fftResult[i] = random(0, 64);
  }
  sampleAvg = random(128, 255);
  FFT_MajorPeak = random(60, 120);
  FFT_Magnitude = random(128, 255);
  samplePeak = 1;
}

// Test functions
void setUp(void) {
  strip.init();
  mockQuietAudio();
  _millis = 0;
}

void test_multi_comet_bass_response(void) {
  strip.setMode(FX_MODE_MULTI_COMET);
  strip.setIntensity(128);

  // Test with quiet audio
  mockQuietAudio();
  strip.service();

  int quietPixels = 0;
  for (int i = 0; i < strip.getLength(); i++) {
    if (strip.getPixelColor(i) != 0) quietPixels++;
  }

  // Test with bass heavy audio
  mockBassyAudio();
  strip.service();

  int bassPixels = 0;
  for (int i = 0; i < strip.getLength(); i++) {
    if (strip.getPixelColor(i) != 0) bassPixels++;
  }

  TEST_ASSERT_GREATER_THAN(quietPixels, bassPixels);
}

void test_noisemeter_response(void) {
  strip.setMode(FX_MODE_NOISEMETER);
  strip.setIntensity(128);

  // Test with quiet audio
  mockQuietAudio();
  strip.service();

  int quietPixels = 0;
  for (int i = 0; i < strip.getLength(); i++) {
    if (strip.getPixelColor(i) != 0) quietPixels++;
  }

  // Test with loud audio
  sampleAvg = 200;
  strip.service();

  int loudPixels = 0;
  for (int i = 0; i < strip.getLength(); i++) {
    if (strip.getPixelColor(i) != 0) loudPixels++;
  }

  TEST_ASSERT_GREATER_THAN(quietPixels, loudPixels);
}

int main(void) {
  UNITY_BEGIN();

  RUN_TEST(test_multi_comet_bass_response);
  RUN_TEST(test_noisemeter_response);

  UNITY_END();
  return 0;
}