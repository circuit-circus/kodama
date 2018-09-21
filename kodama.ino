#include "FastLED.h"

#define NUM_LEDS 25
#define COLOR_ORDER RGB

const int pirPin = 4;
const int ledPin = 3;

// Define the array of leds
CRGB leds[NUM_LEDS];

// This is updated every loop to increase the calculated noise
float millisPerFrame = 0.0f;
// Change this to increase the update speed of the calculated noise
// increasing this, decreases the speed of the noise 
const float noiseSpeed = 3.0f;

// What are the min and max brightnesses overall
const int minBrightness = 0;
const int maxBrightness = 255;
// What are the min and max brightnesses in calm mode
const int calmMin = 40;
const int calmMax = 80;
// What are the min and max brightnesses in active mode
const int activeMin = 80;
const int activeMax = 255;

// Keeps track of the PIR activity
bool isActive = false;
bool lastStateWasActive = false;

// Variables used for lerping
// What is the LED's present brightness
int currentBrightness[NUM_LEDS];
// What is the LED's future brightness
int newBrightness[NUM_LEDS];
// Should this LED lerp towards newBrightness
bool shouldLerpLED[NUM_LEDS];

// A variable used when fading saturation after change in activity
int hueVariation = 0;

// In order to show the LEDs as active for a while after change in activity, we monitor a timer
long keepActiveTimer = 0;
const long keepActiveTimerLength = 2000;

// Helper variables to easily prototype different hues and saturations
const int baseHue = 30;
const int activeHue = 40;
const int baseSat = 180;

// Helper variables to easily prototype different hues and saturations
const int maxVariation = 10;

void setup() {

  pinMode(pirPin, INPUT);

  // Initialize random calm values for all
  for(int i = 0; i < NUM_LEDS; i++) {
    currentBrightness[i] = random(1, 25);
    newBrightness[i] = currentBrightness[i];
    shouldLerpLED[i] = false;
  }

  FastLED.addLeds<WS2812B, ledPin, COLOR_ORDER>(leds, NUM_LEDS);
}

void loop() {
  // Change the divisor to change the speed of the noise changes
  millisPerFrame = float(millis() / noiseSpeed);

	reactToActivity();
}

/*
 * Reads the PIR sensor and calls the correct function depending on current and past activity
 * 
 */
void reactToActivity() {

  isActive = digitalRead(pirPin); // read input value

  if(isActive && !lastStateWasActive) {
    // Start active animation
    updateAnimation(true, true); // The cluster, setNewBrightness, isActive
  }
  else if (isActive && lastStateWasActive) { 
    // Continue active animation
    updateAnimation(false, true);
  }
  else if(!isActive && lastStateWasActive) {
    // Start calm animation
    updateAnimation(true, false);
  }
  else if (!isActive && !lastStateWasActive) {
    // Continue calm animation
    updateAnimation(false, false);
  }

  lastStateWasActive = isActive;
}

/*
 * Function to update the animation 
 * 
 * @param setNewBrightness - Should we give the leds a new brightness (if we change mode)
 * @param isActive - True if we're to go to the active animation, false if calm animaition
 */

void updateAnimation(boolean setNewBrightness, boolean isActiveAnimation) {
  int theDelay = 0;

  if(millis() < keepActiveTimer + keepActiveTimerLength) {
    hueVariation = abs(baseHue - activeHue);
  }
  else {
    hueVariation = LinearInterpolate(hueVariation, 0, 0.001);
  }

  for (int i = 0; i < NUM_LEDS; i++) {
    // Use FastLED's builtin noise function 
    // Multiply i by 1000 to make sure that each LED has different behavior
    uint8_t contrast = inoise8(i * 1000, millisPerFrame);

    // Set the newBrightness based on perlin noise and activity
    if(isActiveAnimation) {
      contrast = map(contrast, minBrightness, maxBrightness, activeMin, activeMax);
      newBrightness[i] = contrast;
    }
    else {
      contrast = map(contrast, minBrightness, maxBrightness, calmMin, calmMax);
      newBrightness[i] = contrast;
    }

    // If we have recently seen change in action, do something with colors
    if(setNewBrightness) {
      shouldLerpLED[i] = true;
      if(isActiveAnimation) {
        theDelay = random(5, 15);
        keepActiveTimer = millis();
      }
    }

    // Lerp the LED brightness
    if(shouldLerpLED[i]) {
      if(isActiveAnimation) {
        currentBrightness[i] = LinearInterpolate(currentBrightness[i], newBrightness[i], 0.5);
      }
      else {
        currentBrightness[i] = LinearInterpolate(currentBrightness[i], newBrightness[i], 0.1);
      }
      currentBrightness[i] = constrain(currentBrightness[i], 5, 255);

      // until we reach the desired brightness
      if(abs(newBrightness[i] - currentBrightness[i]) < 3) {
        shouldLerpLED[i] = false;
      }
    }
    // Else just make sure to constrain it and set it to the noise
    else {
      newBrightness[i] = constrain(newBrightness[i], 5, 255);
      currentBrightness[i] = newBrightness[i];
    }

    // Update the LEDs
    updateLEDs(i, currentBrightness[i], hueVariation, theDelay);
  }
}

/*
 * Updates an individual LED based on given parameters
 * 
 * @param pixel - The index of the LED to change
 * @param theBrightness - The brightness to apply to the LED
 * @param variation - A variation that changes the saturation of the LED
 * @param theDelay - A delay that can be added after this LED for added effect
 */

void updateLEDs(int pixel, int theBrightness, int variation, int theDelay) {
  /*if(variation > 0) {
    leds[pixel] = CHSV(activeHue, baseSat, theBrightness);
  }
  else {
    leds[pixel] = CHSV(baseHue, baseSat, theBrightness);
  }*/
  leds[pixel] = CHSV(baseHue + variation, baseSat, theBrightness);
  FastLED.show();

  delay(theDelay);
}

// Helper functions
/*
 * Interpolate between two variables, a & b, by x
 * 
 * @param a - The variable to interpolate from
 * @param b - The variable to interpolate to
 * @param x - The variable that controls how much to interpolate by
 */
float LinearInterpolate(float a, float b, float x) {
  return(a * (1 - x) + b * x);
}
