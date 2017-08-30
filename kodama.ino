#include <Adafruit_NeoPixel.h>

#include "FastLED.h"

#define NUM_LEDS 5

#define ECHOPIN 12
#define TRIGPIN 13
#define PIRPIN 12

#define DATA_PIN 3
#define CLOCK_PIN 13

// Define the array of leds
CRGB leds[NUM_LEDS];

int pirState = LOW; // we start, assuming no motion detected
int val = 0; // variable for reading the pin status

//persistence affects the degree to which the "finer" noise is seen
float persistence = 0.25;
//octaves are the number of "layers" of noise that get computed
int octaves = 1;

int actualActivity; // Previously cutoff
const int maxActivity = 100; 
int activityDelay = 5;

bool useUltrasound = false;

void setup() {
	delay(2000);
	Serial.begin(9600);

	if(useUltrasound) {
		pinMode(ECHOPIN, INPUT);
		pinMode(TRIGPIN, OUTPUT);
	}
	else {
		pinMode(PIRPIN, INPUT);
	}

	FastLED.addLeds<WS2811, DATA_PIN, RGB>(leds, NUM_LEDS);
	FastLED.clear();
}

void loop() {
	float rnd = float(millis())/100.0f;

	val = digitalRead(PIRPIN); // read input value

	// Get non-delayed activity level
	actualActivity += (val == HIGH) ? 1 : -1;
	// Make sure actualActivity is between maxActivity * activityDelay and 0
	actualActivity = constrain(actualActivity, 0, maxActivity * activityDelay);

	int delayedActivity = actualActivity / activityDelay;
	Serial.print("Activity: ");
	Serial.println(delayedActivity);

	// Source Ultrasound: http://www.instructables.com/id/Simple-Arduino-and-HC-SR04-Example/
	// Source PIR: https://learn.adafruit.com/pir-passive-infrared-proximity-motion-sensor?view=all
	if(useUltrasound) {
		Serial.println("All good");
		long duration, distance;

		digitalWrite(TRIGPIN, LOW);
		delayMicroseconds(2);
		digitalWrite(TRIGPIN, HIGH);
		delayMicroseconds(10);
		digitalWrite(TRIGPIN, LOW);

		duration = pulseIn(ECHOPIN, HIGH);
		distance = (duration / 2) / 29.1;

		Serial.print(distance);
		Serial.println(" cm");
		delay(500);
	}
	else {
		int pirVal = digitalRead(PIRPIN);
		Serial.println(val);
		delay(250);
	}

	Serial.println("------------LOOP START-------------");
	// Turn the LED on, then pause
	for(int i = 0; i < NUM_LEDS; i++) {
		Serial.println("------------LED IN-------------");
		// 128 are magic numbers that make contrast hit somewhere between 0 and 255 when multiplied by PerlinNoise(which returns a number between 0 and 1)
		float contrast = PerlinNoise2(i, rnd, persistence, octaves) * 128 + 128;
		Serial.print("Contrast: ");
		Serial.println(contrast);

		// aendr contrast så den passer til aktivitets-niveauet
		// Contrast must be larger than 0
		contrast = max(contrast - delayedActivity, 0);
		Serial.print("Contrast(max): ");
		Serial.println(contrast);
		// Slightly vary contrast
		contrast *= ((255 + delayedActivity) / (255 - delayedActivity));
		// Then make sure it's below 255
		contrast = min(contrast, 255);
		Serial.print("Contrast(min): ");
		Serial.println(contrast);
		// Contrast should be mapped to a number between 1 and double delayedActivity + 55
		// plus 55, since delayedActivity can be max 100, and 255 is max color value)
		contrast = map(contrast, 0, 255, 1, delayedActivity * 2 + 55);
		Serial.print("Contrast(map): ");
		Serial.println(contrast);

		byte r = contrast * (delayedActivity / maxActivity);
		Serial.print("RGB: ");
		Serial.print(r);
		byte g = contrast * ((maxActivity - delayedActivity) / maxActivity); 
		Serial.print(", ");
		Serial.print(g);
		byte b = constrain((contrast * (delayedActivity / maxActivity)), 0, 50);
		Serial.print(", ");
		Serial.println(b);

		leds[i].red = g;
		leds[i].green = g;
		leds[i].blue = g;
		FastLED.show();
		Serial.println("------------LED OUT-------------");
	}
	Serial.println("------------LOOP STOP-------------");
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