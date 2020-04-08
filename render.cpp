#include <Bela.h>
#include <cmath>
#include <thread>
#include <chrono>
#include <libraries/PulseIn/PulseIn.h>

/*** POTENTIOMETER TEST VARIABLES ***/
float gPhase;
float gPhase2;
float gInverseSampleRate;
int gAudioFramesPerAnalogFrame = 0;

// Set the analog channels to read from
int gSensorInputFrequency = 1;
int gSensorInputAmplitude = 2;
int gSensorInputFrequency2 = 3;
int gSensorInputAmplitude2 = 4;


/*** LED TEST VARIABLES ***/
float gInterval = 0.5; // how often to toggle the LED (in seconds)
int gCount = 0; //counts elapsed samples
bool gStatus = false;


/*** ULTRASONIC TEST VARIABLES ***/
int gTriggerInterval = 2646; // how often to send out a trigger. 2646 samples are 60ms 
int gMinPulseLength = 7; //to avoid spurious readings
float gRescale = 58; // taken from the datasheet

// Ultrasonic 1
PulseIn pulseIn1;
unsigned int gTriggerDigitalOutPin1 = 0; //channel to be connected to the module's TRIGGER pin
unsigned int gEchoDigitalInPin1 = 1; //channel to be connected to the modules's ECHO pin (digital pin 1)
int gTriggerCount1 = 0;
int gPrintfCount1 = 0;

// Ultrasonic 2
PulseIn pulseIn2;
unsigned int gTriggerDigitalOutPin2 = 2; //channel to be connected to the module's TRIGGER pin
unsigned int gEchoDigitalInPin2 = 3; //channel to be connected to the modules's ECHO pin (digital pin 1)
int gTriggerCount2 = 0;
int gPrintfCount2 = 0;

bool setup(BelaContext *context, void *userData)
{
	/*** POTENTIOMETER TEST SETUP ***/
	// Check if analog channels are enabled
	if(context->analogFrames == 0 || context->analogFrames > context->audioFrames) {
		rt_printf("Error: this example needs analog enabled, with 4 or 8 channels\n");
		return false;
	}

	// Useful calculations
	if(context->analogFrames)
		gAudioFramesPerAnalogFrame = context->audioFrames / context->analogFrames;
	gInverseSampleRate = 1.0 / context->audioSampleRate;
	gPhase = 0.0;
	gPhase2 = 0.0;

	
	/*** LED TEST SETUP ***/
	for(int pin = 3; pin < 16; pin++) {
		pinMode(context, 0, pin, OUTPUT); // Set gOutputPin as output
	}
	
	for(int pin = 3; pin < 16; pin++) {
		if(pin != 3) {
			digitalWrite(context, 0, (pin-1), 0);
		}
		digitalWrite(context, 0, pin, 1);
    	std::this_thread::sleep_for(std::chrono::milliseconds(300));
	}
	digitalWrite(context, 0, 15, 0);
	
	
	/*** ULTRASONIC TEST SETUP ***/
	pinMode(context, 0, gTriggerDigitalOutPin1, OUTPUT); // sending to TRIGGER PIN
	pinMode(context, 0, gEchoDigitalInPin1, INPUT); // receiving from ECHO PIN
	pulseIn1.setup(context, gEchoDigitalInPin1, HIGH); //detect HIGH pulses on this pin
	pinMode(context, 0, gTriggerDigitalOutPin2, OUTPUT); // sending to TRIGGER PIN
	pinMode(context, 0, gEchoDigitalInPin2, INPUT); // receiving from ECHO PIN
	pulseIn2.setup(context, gEchoDigitalInPin2, HIGH); //detect HIGH pulses on this pin
	
	
	return true;
}

void render(BelaContext *context, void *userData)
{
	/*** POTENTIOMETER TEST ***/
	float frequency = 440.0;
	float amplitude = 0.8;
	float frequency2 = 440.0;
	float amplitude2 = 0.8;

	for(unsigned int n = 0; n < context->audioFrames; n++) {
		if(gAudioFramesPerAnalogFrame && !(n % gAudioFramesPerAnalogFrame)) {
			// read analog inputs and update frequency and amplitude
			// Depending on the sampling rate of the analog inputs, this will
			// happen every audio frame (if it is 44100)
			// or every two audio frames (if it is 22050)
			frequency = map(analogRead(context, n/gAudioFramesPerAnalogFrame, gSensorInputFrequency), 0, 1, 100, 1000);
			amplitude = analogRead(context, n/gAudioFramesPerAnalogFrame, gSensorInputAmplitude);
			frequency2 = map(analogRead(context, n/gAudioFramesPerAnalogFrame, gSensorInputFrequency2), 0, 1, 100, 1000);
			amplitude2 = analogRead(context, n/gAudioFramesPerAnalogFrame, gSensorInputAmplitude2);
		}
		
		float wave = amplitude * sinf(gPhase);
		float wave2 = amplitude2 * sinf(gPhase2);

		// Update and wrap phase of sine tone
		gPhase += 2.0f * (float)M_PI * frequency * gInverseSampleRate;
		if(gPhase > M_PI)
			gPhase -= 2.0f * (float)M_PI;
		
		// Update and wrap phase of sine tone
		gPhase2 += 2.0f * (float)M_PI * frequency2 * gInverseSampleRate;
		if(gPhase2 > M_PI)
			gPhase2 -= 2.0f * (float)M_PI;
			
		
		/*** MIC TEST ***/
		float mic = audioRead(context, n, 0);

		audioWrite(context, n, 0, (wave + mic));
		audioWrite(context, n, 1, (wave2 + mic));
	}
	
	for(unsigned int n = 0; n < context->digitalFrames; n++){
		/*** LED TEST ***/
		if(gCount == (int)(context->digitalSampleRate * gInterval)){ //if enough samples have elapsed
			gCount = 0; //reset the counter
			//toggle the status
			if(gStatus == 0)
				gStatus = 1;
			else
				gStatus = 0;
			for(int pin = 3; pin < 16; pin++) {
				digitalWrite(context, n, pin, gStatus); //write the status to the LED (gOutputPin)
			}
		}
		gCount++;
		
		
		/*** ULTRASONIC TEST ***/
		// Ultrasonic 1
		gTriggerCount1++;
		if(gTriggerCount1 == gTriggerInterval){
			gTriggerCount1 = 0;
			digitalWriteOnce(context, n / 2, gTriggerDigitalOutPin1, HIGH); //write the status to the trig pin
		} else {
			digitalWriteOnce(context, n / 2, gTriggerDigitalOutPin1, LOW); //write the status to the trig pin
		}
		int pulseLength1 = pulseIn1.hasPulsed(context, n); // will return the pulse duration(in samples) if a pulse just ended 
		float duration1 = 1e6 * pulseLength1 / context->digitalSampleRate; // pulse duration in microseconds
		static float distance1 = 0;
		if(pulseLength1 >= gMinPulseLength){
			// rescaling according to the datasheet
			distance1 = duration1 / gRescale;
			rt_printf("US1: pulseLength: %d, distance: %fcm\n", pulseLength1, distance1);
		}
		
		// Ultrasonic 2
		gTriggerCount2++;
		if(gTriggerCount2 == gTriggerInterval){
			gTriggerCount2 = 0;
			digitalWriteOnce(context, n / 2, gTriggerDigitalOutPin2, HIGH); //write the status to the trig pin
		} else {
			digitalWriteOnce(context, n / 2, gTriggerDigitalOutPin2, LOW); //write the status to the trig pin
		}
		int pulseLength2 = pulseIn2.hasPulsed(context, n); // will return the pulse duration(in samples) if a pulse just ended 
		float duration2 = 1e6 * pulseLength2 / context->digitalSampleRate; // pulse duration in microseconds
		static float distance2 = 0;
		if(pulseLength2 >= gMinPulseLength){
			// rescaling according to the datasheet
			distance2 = duration2 / gRescale;
			rt_printf("US2: pulseLength: %d, distance: %fcm\n", pulseLength2, distance2);
		}
	}
}

void cleanup(BelaContext *context, void *userData)
{
	
}
