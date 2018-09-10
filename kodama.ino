#include <Adafruit_NeoPixel.h>

#include "FastLED.h"

#define NUM_LEDS 25
#define COLOR_ORDER RGB

int pirPin = 4;
const int ledPin = 3;

int pirState = LOW;
int pirVal = 0;

// Define the array of leds
CRGB leds[NUM_LEDS];

//persistence affects the degree to which the "finer" noise is seen
float persistence = 0.25;
//octaves are the number of "layers" of noise that get computed
int octaves = 1;
float rnd = 0.0f;

int activeVariation = 10;
int calmVariation = 2;

bool isActive = false;
bool lastStateWasActive = false;

int currentBrightness[NUM_LEDS];

void setup() {

  pinMode(pirPin, INPUT);

  // Initialize calm values for all
  for (int i = 0; i < NUM_LEDS; i++) {
    currentBrightness[i] = random(1, 25);
  }

  FastLED.addLeds<WS2812B, ledPin, COLOR_ORDER>(leds, NUM_LEDS);
}

void loop() {
  rnd = float(millis())/100.0f;

	reactToActivity();
}

void reactToActivity() {

  isActive = digitalRead(pirPin); // read input value

  if(isActive && !lastStateWasActive) {
    // Start active animation
    updateAnimation(true, true); // The cluster, setNewBrightness, isActive
  } else if (isActive && lastStateWasActive) { 
    // Continue active animation
    updateAnimation(false, true);
  } else if(!isActive && lastStateWasActive) {
    // Start calm animation
    updateAnimation(true, false);
  } else if (!isActive && !lastStateWasActive) {
    // Continue calm animation
    updateAnimation(false, false);
  }

  lastStateWasActive = isActive;
}

void readSensors() {
  pirVal = digitalRead(pirPin); // read input value
}


/*
 * Function to update the animation 
 * 
 * @param cluster - The cluster we're working with
 * @param setNewBrightness - Should we give the leds a new brightness (if we change mode)
 * @param isActive - True if we're to go to the active animation, false if calm animaition
 */

void updateAnimation(boolean setNewBrightness, boolean isActiveAnimation) {
  float green = 0;
  int theDelay = 0;
  for (int i = 0; i < NUM_LEDS; i++) {
    int newBrightness;
    
    if(setNewBrightness) {
      if(isActiveAnimation) {
        newBrightness = random(40, 80);
        green = 50;
        theDelay = random(5, 15);
        // Maybe set color to something different here
      } else {
        newBrightness = random(1, 25);
      }
    } else {
      // Use perlin noise to step up or down
      float contrast = PerlinNoise2(i, rnd, persistence, octaves);
      // Alternatively, just random
      
      if(isActiveAnimation) {
        newBrightness = random(currentBrightness[i] - activeVariation, currentBrightness[i] + activeVariation); 
      } else {
        newBrightness = random(currentBrightness[i] - calmVariation, currentBrightness[i] + calmVariation); 
      }
      
    }

    newBrightness = constrain(newBrightness, 1, 255);


    currentBrightness[i] = newBrightness;

 
    updateLEDs(i, currentBrightness[i], green, theDelay);
  }
}

void updateLEDs(int pixel, int theBrightness, int green, int theDelay) {
	leds[pixel].red = theBrightness;
	leds[pixel].green = theBrightness + green;
	leds[pixel].blue = theBrightness;
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
