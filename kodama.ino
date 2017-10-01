#include <Adafruit_NeoPixel.h>

#include "FastLED.h"

#define NUM_LEDS 50

#define PIRPIN D3
#define LEDPIN D4

// Define the array of leds
CRGB leds[NUM_LEDS];

// Set a brightness level goal for each LED to reach
int brightnessGoals[NUM_LEDS];
// What should the maximum distance to new brightness goal be?
int brightnessDistanceMax = 20;
int shouldJumpBrightness = false;
int wasBrightnessJustChanged = false;

int minBrightness = 25;
int maxBrightness = 75;

int pirState = LOW; // we start, assuming no motion detected
int val = 0; // variable for reading the pin status

//persistence affects the degree to which the "finer" noise is seen
float persistence = 0.25;
//octaves are the number of "layers" of noise that get computed
int octaves = 1;
float rnd = 0.0f;

int actualActivity = 0; // Previously cutoff
int lastChangedActivity = 150;
const int maxActivity = 100; 
int activityDelay = 5;

bool isDebugging = false;

void setup() {
	delay(2000);
	Serial.begin(9600);

	pinMode(PIRPIN, INPUT);

	FastLED.addLeds<WS2811, LEDPIN, RGB>(leds, NUM_LEDS);
	FastLED.clear();

	for(int i = 0; i < NUM_LEDS; i++) {
		brightnessGoals[i] = 0;
	}
}

void loop() {
	rnd = float(millis())/100.0f;

	readSensors();

	reactToActivity();

	calculateLEDs();
}

void reactToActivity() {
	// Get non-delayed activity level
	actualActivity += (val == HIGH) ? 1 : -1;
	// Make sure actualActivity is between maxActivity * activityDelay and 0
	actualActivity = constrain(actualActivity, 0, maxActivity * activityDelay);

	int longtermActivity = actualActivity / activityDelay;
	debugInt("longtermActivity: ", longtermActivity, false);
	debugInt("actualActivity: ", actualActivity, false);

	if(actualActivity > 150 && lastChangedActivity != 150) {
		brightnessDistanceMax = 35;
		minBrightness = 0;
		maxBrightness = 0;
		lastChangedActivity = 150;
		wasBrightnessJustChanged = true;
	}
	else if(actualActivity > 0 && lastChangedActivity != 25) {
		brightnessDistanceMax = 20;
		minBrightness = 150;
		maxBrightness = 225;
		lastChangedActivity = 25;
		wasBrightnessJustChanged = true;
	}
	else if(lastChangedActivity != 0) {
		brightnessDistanceMax = 40;
		lastChangedActivity = 0;
		minBrightness = 0;
		maxBrightness = 60;
		wasBrightnessJustChanged = true;
	}
}

void readSensors() {
	val = digitalRead(PIRPIN); // read input value
  Serial.println(val);
	debugInt("PIRVAL: ", val, false);
}

void calculateLEDs() {
	debugString("------------LOOP START-------------", false);
	// Turn the LED on, then pause
	for(int i = 0; i < NUM_LEDS; i++) {
		debugString("------------LED IN-------------", false);
		float contrast;
		// The goal is reached, when it is 0
		if(brightnessGoals[i] == 0) {
			// A number between -1 and 1
			contrast = PerlinNoise2(i, rnd, persistence, octaves);
			debugFloat("Contrast: ", contrast, false);
			// Set the goal to contrast multiplied by max distance
			brightnessGoals[i] = contrast * brightnessDistanceMax;

			if(i == 0) {
				debugInt("New goal: ", brightnessGoals[i], false);
			}
		}

		int brightnessChange = brightnessGoals[i] < 0 ? -1 : 1;
		int brightness = 0;

		if(wasBrightnessJustChanged) {
			debugInt("Minbright - ledsred: ", minBrightness - leds[i].red, true);
			for(int j = leds[i].red; j < minBrightness - leds[i].red; j++) {
				debugInt("j: ", j, false);
				brightness = j;
				updateLEDs(i, brightness);
			}
			wasBrightnessJustChanged = false;
		}

		// Clamp brightness 
		brightness = constrain(leds[i].red + brightnessChange, minBrightness, maxBrightness);

		if(i == 0) {
			debugInt("Goal: ", brightnessGoals[i], false);
			debugFloat("Brightness: ", float(brightness), false);
		}
    debugFloat("Brightness: ", float(brightness), false);
		updateLEDs(i, brightness);

		// Make the goal move one closer to 0
		brightnessGoals[i] += brightnessGoals[i] < 0 ? 1 : -1;

		debugString("------------LED OUT-------------", false);
	}
	debugString("------------LOOP STOP-------------", false);
}

void updateLEDs(int LEDId, int brightness) {
	leds[LEDId].red = brightness;
	leds[LEDId].green = brightness;
	leds[LEDId].blue = brightness;
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
