#include <Adafruit_NeoPixel.h>

#include "FastLED.h"


//#define PIRPIN A5
//#define LEDPIN 7

#define NO_OF_CLUSTERS 5
#define NUM_LEDS 25

int pirPins[NO_OF_CLUSTERS] = {A1, A2, A3, A4, A5};
const int ledPins[NO_OF_CLUSTERS] = {2, 3, 4, 5, 6};

int pirStates[NO_OF_CLUSTERS] = {LOW, LOW, LOW, LOW, LOW};
int pirVals[NO_OF_CLUSTERS] = {0, 0, 0, 0, 0};

// Define the array of leds
CRGB leds[NO_OF_CLUSTERS][NUM_LEDS];

// Set a brightness level goal for each LED to reach
int brightnessGoals[NO_OF_CLUSTERS][NUM_LEDS];
// What should the maximum distance to new brightness goal be?
int brightnessDistanceMax[NO_OF_CLUSTERS] = {20, 20, 20, 20, 20};
int shouldJumpBrightness[NO_OF_CLUSTERS] = {false, false, false, false, false};
int wasBrightnessJustChanged[NO_OF_CLUSTERS] = {false, false, false, false, false};

int minBrightness[NO_OF_CLUSTERS] = {25, 25, 25, 25, 25};
int maxBrightness[NO_OF_CLUSTERS] = {75, 75, 75, 75, 75};

//int pirState = LOW; // we start, assuming no motion detected
//int val = 0; // variable for reading the pin status

//persistence affects the degree to which the "finer" noise is seen
float persistence = 0.25;
//octaves are the number of "layers" of noise that get computed
int octaves = 1;
float rnd = 0.0f;

int actualActivities[NO_OF_CLUSTERS] = {0, 0, 0, 0, 0};
int lastChangedActivities[NO_OF_CLUSTERS] = {150, 150, 150, 150, 150};
int maxActivities[NO_OF_CLUSTERS] = {100, 100, 100, 100, 100};
int activityDelays[NO_OF_CLUSTERS] = {5, 5, 5, 5, 5};


bool isDebugging = false;

void setup() {
	delay(2000);
	Serial.begin(9600);

  for (int i = 0; i < NO_OF_CLUSTERS; i++) {
    pinMode(pirPins[i], INPUT);

    for(int j = 0; j < NUM_LEDS; j++) {
      brightnessGoals[i][j] = 0;
    }
  }

  FastLED.addLeds<NEOPIXEL, 2>(leds[0], NUM_LEDS);
  FastLED.addLeds<NEOPIXEL, 3>(leds[1], NUM_LEDS);
  FastLED.addLeds<NEOPIXEL, 4>(leds[2], NUM_LEDS);
  FastLED.addLeds<NEOPIXEL, 5>(leds[3], NUM_LEDS);
  FastLED.addLeds<NEOPIXEL, 6>(leds[4], NUM_LEDS);
}

void loop() {
	rnd = float(millis())/100.0f;

	readSensors();

	reactToActivity();

	calculateLEDs();
}

void reactToActivity() {
	// Get non-delayed activity level

  for(int i = 0; i < NO_OF_CLUSTERS; i++) {
  	actualActivities[i] += (pirVals[i] == HIGH) ? 1 : -1;
  	// Make sure actualActivity is between maxActivity * activityDelay and 0
  	actualActivities[i] = constrain(actualActivities[i], 0, maxActivities[i] * activityDelays[i]);
  
  	int longtermActivity = actualActivities[i] / activityDelays[i];
  	debugInt("longtermActivity: ", longtermActivity, false);
  	//debugInt(char("actualActivity for " + char(i) + ": "), actualActivities[i], false);
  
  	if(actualActivities[i] > 150 && lastChangedActivities[i] != 150) {
  		brightnessDistanceMax[i] = 35;
  		minBrightness[i] = 0;
  		maxBrightness[i] = 0;
  		lastChangedActivities[i] = 150;
  		wasBrightnessJustChanged[i] = true;
  	}
  	else if(actualActivities[i] > 0 && lastChangedActivities[i] != 25) {
  		brightnessDistanceMax[i] = 20;
  		minBrightness[i] = 150;
  		maxBrightness[i] = 225;
  		lastChangedActivities[i] = 25;
  		wasBrightnessJustChanged[i] = true;
  	}
  	else if(lastChangedActivities[i] != 0) {
  		brightnessDistanceMax[i] = 40;
  		lastChangedActivities[i] = 0;
  		minBrightness[i] = 0;
  		maxBrightness[i] = 60;
  		wasBrightnessJustChanged[i] = true;
  	}
  }
}

void readSensors() {

  for(int i = 0; i < NO_OF_CLUSTERS; i++) {
    pirVals[i] = digitalRead(pirPins[i]); // read input value
    Serial.println(pirVals[i]);
    //debugInt(char("PIRVAL " + char(i) + ": "), pirVals[i], false);
  }
}

void calculateLEDs() {
  for(int k = 0; k < NO_OF_CLUSTERS; k++) {
  	debugString("------------LOOP START-------------", false);
  	// Turn the LED on, then pause
  	for(int i = 0; i < NUM_LEDS; i++) {
  		debugString("------------LED IN-------------", false);
  		float contrast;
  		// The goal is reached, when it is 0
  		if(brightnessGoals[k][i] == 0) {
  			// A number between -1 and 1
  			contrast = PerlinNoise2(i, rnd, persistence, octaves);
  			debugFloat("Contrast: ", contrast, false);
  			// Set the goal to contrast multiplied by max distance
  			brightnessGoals[k][i] = contrast * brightnessDistanceMax[i];
  
  			if(i == 0) {
  				debugInt("New goal: ", brightnessGoals[k][i], false);
  			}
  		}
  
  		int brightnessChange = brightnessGoals[k][i] < 0 ? -1 : 1;
  		int brightness = 0;
  
  		if(wasBrightnessJustChanged[k]) {
  			debugInt("Minbright - ledsred: ", minBrightness[k] - leds[k][i].red, true);
  			for(int j = leds[k][i].red; j < minBrightness[k] - leds[k][i].red; j++) {
  				debugInt("j: ", j, false);
  				brightness = j;
  				updateLEDs(k, i, brightness);
  			}
  			wasBrightnessJustChanged[k] = false;
  		}
  
  		// Clamp brightness 
  		brightness = constrain(leds[k][i].red + brightnessChange, minBrightness[k], maxBrightness[k]);
  
  		if(i == 0) {
  			debugInt("Goal: ", brightnessGoals[i], false);
  			debugFloat("Brightness: ", float(brightness), false);
  		}
      debugFloat("Brightness: ", float(brightness), false);
  		updateLEDs(k, i, brightness);
  
  		// Make the goal move one closer to 0
  		brightnessGoals[k][i] += brightnessGoals[k][i] < 0 ? 1 : -1;
  
  		debugString("------------LED OUT-------------", false);
  	}
  	debugString("------------LOOP STOP-------------", false);
  }
}

void updateLEDs(int cluster, int pixel, int brightness) {
	leds[cluster][pixel].red = brightness;
	leds[cluster][pixel].green = brightness;
	leds[cluster][pixel].blue = brightness;
	FastLED.show();
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

void debugLong(char text[], long value, bool override) {
	if(isDebugging || override) {
		Serial.print(text);
		Serial.println(value);
	}
}

void debugInt(char text[], int value, bool override) {
	if(isDebugging || override) {
		Serial.print(text);
		Serial.println(value);
	}
}

void debugString(char text[], bool override) {
	if(isDebugging || override) {
		Serial.println(text);
	}
}

void debugFloat(char text[], float value, bool override) {
	if(isDebugging || override) {
		Serial.print(text);
		Serial.println(value);
	}
}
