#include <Adafruit_NeoPixel.h>

#include "FastLED.h"


#define NO_OF_CLUSTERS 3
#define NUM_LEDS 25


int pirPins[NO_OF_CLUSTERS] = {A1, A2, A3};
const int ledPins[NO_OF_CLUSTERS] = {2, 3, 4};

int pirStates[NO_OF_CLUSTERS] = {LOW, LOW, LOW};
int pirVals[NO_OF_CLUSTERS] = {0, 0, 0};

// Define the array of leds
CRGB leds[NO_OF_CLUSTERS][NUM_LEDS];

//persistence affects the degree to which the "finer" noise is seen
float persistence = 0.25;
//octaves are the number of "layers" of noise that get computed
int octaves = 1;
float rnd = 0.0f;

int activeVariation = 10;
int calmVariation = 2;

bool isActive[NO_OF_CLUSTERS] = {false, false, false};
bool lastStateWasActive[NO_OF_CLUSTERS] = { false, false, false};

int currentBrightness[NO_OF_CLUSTERS][NUM_LEDS];

void setup() {

	Serial.begin(9600);

  for (int i = 0; i < NO_OF_CLUSTERS; i++) {
    pinMode(pirPins[i], INPUT);

    // Initialize calm values for all
    for (int j = 0; j < NUM_LEDS; j++) {
      currentBrightness[i][j] = random(1, 25);
    }
  }

  FastLED.addLeds<NEOPIXEL, 2>(leds[0], NUM_LEDS);
  FastLED.addLeds<NEOPIXEL, 3>(leds[1], NUM_LEDS);
  FastLED.addLeds<NEOPIXEL, 4>(leds[2], NUM_LEDS);
}

void loop() {
  rnd = float(millis())/100.0f;
	//readSensors();

	reactToActivity();

	//calculateLEDs();
}

void reactToActivity() {

  for(int i = 0; i < NO_OF_CLUSTERS; i++) {
    isActive[i] = digitalRead(pirPins[i]); // read input value

    if(isActive[i] && !lastStateWasActive[i]) {
      // Start active animation
      updateAnimation(i, true, true); // The cluster, setNewBrightness, isActive
    } else if (isActive[i] && lastStateWasActive[i]) { 
      // Continue active animation
      updateAnimation(i, false, true);
    } else if(!isActive[i] && lastStateWasActive[i]) {
      // Start calm animation
      updateAnimation(i, true, false);
    } else if (!isActive[i] && !lastStateWasActive[i]) {
      // Continue calm animation
      updateAnimation(i, false, false);
    }

    lastStateWasActive[i] = isActive[i];
   }

 
  /*
  for(int i = 0; i < NO_OF_CLUSTERS; i++) {
  	actualActivities[i] += (pirVals[i] == HIGH) ? 1 : -3;
  	// Make sure actualActivity is between maxActivity * activityDelay and 0
  	actualActivities[i] = constrain(actualActivities[i], 0, maxActivities[i] * activityDelays[i]);
  
  	int longtermActivity = actualActivities[i] / activityDelays[i];
    if(actualActivities[i] > 10 && lastChangedActivities[i] != 25) {
  		brightnessDistanceMax[i] = 40;
  		minBrightness[i] = 25;
  		maxBrightness[i] = 49;
  		lastChangedActivities[i] = 25;
  		wasBrightnessJustChanged[i] = true;
  	}
    // Calm mode
  	else if(actualActivities[i] < 10) {
  		brightnessDistanceMax[i] = 40;
  		lastChangedActivities[i] = 0;
  		minBrightness[i] = 1;
  		maxBrightness[i] = 25;
  		wasBrightnessJustChanged[i] = true;
  	}
  }
  */
}

void readSensors() {

  for(int i = 0; i < NO_OF_CLUSTERS; i++) {
    pirVals[i] = digitalRead(pirPins[i]); // read input value
  }
}


/*
 * Function to update the animation 
 * 
 * @param cluster - The cluster we're working with
 * @param setNewBrightness - Should we give the leds a new brightness (if we change mode)
 * @param isActive - True if we're to go to the active animation, false if calm animaition
 */

void updateAnimation(int cluster, boolean setNewBrightness, boolean isActiveAnimation) {
  float red = 0;
  int theDelay = 0;
  for (int i = 0; i < NUM_LEDS; i++) {
    int newBrightness;
    
    if(setNewBrightness) {
      if(isActiveAnimation) {
        newBrightness = random(40, 80);
        red = 50;
        theDelay = random(5, 15);
        // Maybe set color to something different here
      } else {
        newBrightness = random(1, 25);
      }
    } else {
      // Use perlin noise to step up or down
      float contrast = PerlinNoise2(i, rnd, persistence, octaves);
      // newBrightness = int(currentBrightness[cluster][i] * contrast);

      /*
      if(isActiveAnimation) {
        if(cluster == 1) {
          Serial.println("ACtive ongoing");
        }
        newBrightness = map(contrast, -0.5, 0.5, currentBrightness[cluster][i] - activeVariation, currentBrightness[cluster][i] + activeVariation);
      } else {
        //newBrightness = map(contrast, -0.5, 0.5, currentBrightness[cluster][i] - calmVariation, currentBrightness[cluster][i] + calmVariation);
        newBrightness = random(10, 25);
        //newBrightness = random(currentBrightness[cluster][i] - 5, currentBrightness[cluster][i] + 5);
      }

      */


      // Alternatively, just random
      
      if(isActiveAnimation) {
        newBrightness = random(currentBrightness[cluster][i] - activeVariation, currentBrightness[cluster][i] + activeVariation); 
      } else {
        newBrightness = random(currentBrightness[cluster][i] - calmVariation, currentBrightness[cluster][i] + calmVariation); 
      }
      
    }

    newBrightness = constrain(newBrightness, 1, 255);


    currentBrightness[cluster][i] = newBrightness;

 
    updateLEDs(cluster, i, currentBrightness[cluster][i], red, theDelay);
  }
}


/*
void calculateLEDs() {
  for(int k = 0; k < NO_OF_CLUSTERS; k++) {
  	// Turn the LED on, then pause
  	for(int i = 0; i < NUM_LEDS; i++) {
  		float contrast;
  		// The goal is reached, when it is 0
  		if(brightnessGoals[k][i] == 0) {
  			// A number between -1 and 1
  			contrast = PerlinNoise2(i, rnd, persistence, octaves);
  			// Set the goal to contrast multiplied by max distance
  			brightnessGoals[k][i] = contrast * brightnessDistanceMax[i];

        if(shouldJumpBrightness[k]) {
          shouldJumpBrightness[k] = false;
        }
  		}
  
  		int brightnessChange = brightnessGoals[k][i] < 0 ? -1 : 1;
      // If we are in high mode, multiply jump
      brightnessChange *= minBrightness[k] == 25 ? 10 : 1;
  		int brightness = 0;
  
  		if(wasBrightnessJustChanged[k]) { 
        brightness = random(minBrightness[k], maxBrightness[k]);
        updateLEDs(k, i, brightness);
  			wasBrightnessJustChanged[k] = false;
  		}
    
  		// Clamp brightness 
  		brightness = constrain(leds[k][i].red + brightnessChange, minBrightness[k], maxBrightness[k]);
  
  		updateLEDs(k, i, brightness);
  
  		// Make the goal move one closer to 0
  		brightnessGoals[k][i] += brightnessGoals[k][i] < 0 ? 1 : -1;
  	}
  }
}
*/

void updateLEDs(int cluster, int pixel, int theBrightness, int red, int theDelay) {
  //float redMultiplier = 1;
  // float blueMultiplier = 1;
  // float greenMultiplier = 1;

  // leds[cluster][pixel].red = brightness * redMultiplier;
  // leds[cluster][pixel].green = brightness * blueMultiplier;
  // leds[cluster][pixel].blue = brightness * greenMultiplier;

	leds[cluster][pixel].red = theBrightness + red;
	leds[cluster][pixel].green = theBrightness;
	leds[cluster][pixel].blue = theBrightness;
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
