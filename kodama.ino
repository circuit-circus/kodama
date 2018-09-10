#include "FastLED.h"

#define NUM_LEDS 25
#define COLOR_ORDER RGB

const int pirPin = 4;
const int ledPin = 3;

// Define the array of leds
CRGB leds[NUM_LEDS];

//persistence affects the degree to which the "finer" noise is seen
const float persistence = 0.75;
//octaves are the number of "layers" of noise that get computed
const int octaves = 1;
float millisPerFrame = 0.0f;
const float minPerlin = -0.4f;
const float maxPerlin = 0.4f;
const float calmMin = 20.0f;
const float calmMax = 50.0f;
const float activeMin = 50.0f;
const float activeMax = 255.0f;

bool isActive = false;
bool lastStateWasActive = false;

int currentBrightness[NUM_LEDS];
int newBrightness[NUM_LEDS];
int pixelVariation[NUM_LEDS];
bool shouldLerpLED[NUM_LEDS];

void setup() {

  pinMode(pirPin, INPUT);
  // Serial.begin(9600);

  // Initialize calm values for all
  for(int i = 0; i < NUM_LEDS; i++) {
    currentBrightness[i] = random(1, 25);
    newBrightness[i] = currentBrightness[i];
    shouldLerpLED[i] = false;
    pixelVariation[i] = random(-10, 10);
  }

  FastLED.addLeds<WS2812B, ledPin, COLOR_ORDER>(leds, NUM_LEDS);
}

void loop() {
  millisPerFrame = float(millis())/1000.0f;

	reactToActivity();
}

void reactToActivity() {

  isActive = digitalRead(pirPin); // read input value

  // Serial.println(isActive);

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
  int variation = 0;
  int theDelay = 0;
  // Use perlin noise to step up or down

  for (int i = 0; i < NUM_LEDS; i++) {

    float contrast = PerlinNoise2(i, millisPerFrame, persistence, octaves);
    // Set the newBrightness based on perlin noise and activity
    if(isActiveAnimation) {
      contrast = mapfloat(contrast, minPerlin, maxPerlin, activeMin, activeMax);
      newBrightness[i] = contrast;
    }
    else {
      contrast = mapfloat(contrast, minPerlin, maxPerlin, calmMin, calmMax);
      newBrightness[i] = contrast;
    }

    // If we have recently seen change in action, do something with colors
    if(setNewBrightness) {
      shouldLerpLED[i] = true;
      if(isActiveAnimation) {
        variation = 20;
        theDelay = random(5, 15);
      }
    }

    // Lerp the LED brightness
    if(shouldLerpLED[i]) {
      currentBrightness[i] = LinearInterpolate(currentBrightness[i], newBrightness[i], 0.5);
      currentBrightness[i] = constrain(currentBrightness[i], 5, 255);

      // until we reach the desired brightness
      if(currentBrightness[i] == newBrightness[i]) {
        shouldLerpLED[i] = false;
      }
    }
    // Else just make sure to constrain it
    else {
      newBrightness[i] = constrain(newBrightness[i], 5, 255);
      currentBrightness[i] = newBrightness[i];
    }

    // Update the LEDs
    updateLEDs(i, currentBrightness[i], variation, theDelay);
  }
}

void updateLEDs(int pixel, int theBrightness, int variation, int theDelay) {
  // int newGreen = constrain(theBrightness + variation, 0, 255);
  // leds[pixel].red = theBrightness;
	// leds[pixel].green = newGreen;
	// leds[pixel].blue = theBrightness;
  leds[pixel] = CHSV(113 + pixelVariation[pixel] + variation, 120, theBrightness);
	FastLED.show();

  delay(theDelay);
}

// Helper functions
// using the algorithm from http://freespace.virgin.net/hugo.elias/models/m_perlin.html
// thanks to hugo elias
float Noise2(float x, float y) {
  long noise;
  noise = x + y * 57;
  noise = (noise << 13) ^ noise;
  return ( 1.0 - ( long(noise * (noise * noise * 15731L + 789221L) + 1376312589L) & 0x7fffffff) / 1073741824.0);
}

float SmoothNoise2(float x, float y) {
  float corners, sides, center;
  corners = ( Noise2(x-1, y-1)+Noise2(x+1, y-1)+Noise2(x-1, y+1)+Noise2(x+1, y+1) ) / 16;
  sides   = ( Noise2(x-1, y)  +Noise2(x+1, y)  +Noise2(x, y-1)  +Noise2(x, y+1) ) /  8;
  center  =  Noise2(x, y) / 4;
  return (corners + sides + center);
}

float InterpolatedNoise2(float x, float y) {
  float v1,v2,v3,v4,i1,i2,fractionX,fractionY;
  long longX,longY;

  longX = long(x);
  fractionX = x - longX;

  longY = long(y);
  fractionY = y - longY;

  v1 = SmoothNoise2(longX, longY);
  v2 = SmoothNoise2(longX + 1, longY);
  v3 = SmoothNoise2(longX, longY + 1);
  v4 = SmoothNoise2(longX + 1, longY + 1);

  i1 = Interpolate(v1 , v2 , fractionX);
  i2 = Interpolate(v3 , v4 , fractionX);

  return(Interpolate(i1 , i2 , fractionY));
}

float Interpolate(float a, float b, float x) {
  //cosine interpolations
  return(CosineInterpolate(a, b, x));
}

float LinearInterpolate(float a, float b, float x) {
  return(a*(1-x) + b*x);
}

float CosineInterpolate(float a, float b, float x) {
  float ft = x * 3.1415927;
  float f = (1 - cos(ft)) * .5;

  return(a*(1-f) + b*f);
}

float PerlinNoise2(float x, float y, float persistance, int octaves) {
  float frequency, amplitude;
  float total = 0.0;

  for (int i = 0; i <= octaves - 1; i++)
  {
    frequency = pow(2,i);
    amplitude = pow(persistence,i);

    total = total + InterpolatedNoise2(x * frequency, y * frequency) * amplitude;
  }

  return(total);
}

float mapfloat(float x, float in_min, float in_max, float out_min, float out_max) {
 return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}