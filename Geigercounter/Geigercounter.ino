/**
 * 
 * Geigercounter 0.9b
 * by ShuzzDE
 * 
 * This sketch uses a HC-SR04 and a speaker connected to an arduino to simulate a Geiger Counter.
 * It is intended to be used on end-time LARP cons.
 * 
 * If you press a button, the klick rate will increase.
 * If you get closer to an object, the click rate will increase.
 * 
 * All these things are tuneable in the code, check the defines!
 * 
 * 
 */


#include <NewPing.h>


#ifdef ARDUINO_AVR_DIGISPARK 
// W A R N I N G
// This is my idea on how to connect the circuitry to a digispark.
// If you do it differently, adjust the values...
#define TRIGGER_PIN 0
#define ECHO_PIN 2
#define LED_PIN 1
#define SPEAKER_PIN 1
#define BUTTON_PIN 3
#define ANALOG_PIN_FOR_RANDOM_SEED 5
#define BUTTON_PULLUP LOW  // Set this to HIGH when not using Pin3 on Digispark.
#undef DEBUG
#elif ARDUINO_AVR_NANO
// Used Sepp's schematic to define the pin values.
// However, the HC-SR04 isn't in the picture, yet.
// Edit these before you use this code on Arduino Nano!!!
// Code won't compile otherwise when setting board to Arduino Nano.
#define TRIGGER_PIN ???
#define ECHO_PIN ???
#define LED_PIN 8
#define SPEAKER_PIN 10
#define BUTTON_PIN 9
#define ANALOG_PIN_FOR_RANDOM_SEED A1
#define BUTTON_PULLUP HIGH
#undef DEBUG
#else
// Pin definitions for development Board (Arduino UNO in my case)
#define TRIGGER_PIN 3
#define ECHO_PIN 4
#define LED_PIN 10
#define SPEAKER_PIN 10
#define BUTTON_PIN 12
#define ANALOG_PIN_FOR_RANDOM_SEED A1
#define BUTTON_PULLUP HIGH
#define DEBUG   // when DEBUG is defined, code for serial output will be compiled.
#endif

/*
 * Handy little trick to avoid preprocessor statements everywhere for debug code:
 * Serial.begin, Serial.print and Serial.println are wrapped in "virtual" functions.
 * When Debugging is disabled, the calls are simply replaced with nothing.
 * Makes the code much cleaner.
 * Hey, I just learned this and I think it's great! ;-P
 */
#ifdef DEBUG
#define debugSetup() Serial.begin(115200)
#define debugPrint(n) Serial.print(n)
#define debugPrintln(n) Serial.println(n)
#else
#define debugSetup()
#define debugPrint(n)
#define debugPrintln(n)
#endif



/*
 * Distance to object
 * At maximum distance, you will have the minimum tock rate
 * At minimum distance, you'll get the maximum tock rate
 */
#define MAX_DISTANCE_CM 80   // Maximum distance acknowledged
#define MIN_DISTANCE_CM 10   // Minimum distance to object acknowledged

/*
 * tock rates.
 * Higher values mean faster tocks.
 */
#define LOW_RATE 0.6    // Rate for button NOT pressed
#define HIGH_RATE 5.0   // Rate for button pressed (aka. "OMG we have radioactivity"-mode)

/*
 * Rate of tock rate change over the distance.
 * Lower values mean tocks will get faster at minimum distance compared to maximum distance
 */
#define MIN_DIST_PCT_LOW   0.5    // Rate change for button NOT pressed
#define MIN_DIST_PCT_HIGH  0.1    // Rate change for button pressed

/**
 * Amount of time for which LED and speaker will be turned on.
 * 1ms seems to be working well to produce nice tocks.
 */
#define TOCK_LENGTH 1      // Duration for doing speaker tock and LED blink

NewPing sonar(TRIGGER_PIN, ECHO_PIN, MAX_DISTANCE_CM);

/*
 * Initialize the random generator the RIGHT way.
 * Function will do 16 A/D conversions on a floating analog pin.
 * The lowest bit will usually flap rather randomly, therefore we
 * store the lowest bit of every reading in a 16bit long variable
 * and use that to initalize the randomSeed();
 */
void initRandom(){
  uint16_t seed = 0;
  int i = 0;
  for(i = 0; i < 16; i++){
    seed <<= 1;  // Shift everything one to the left
    seed += analogRead(ANALOG_PIN_FOR_RANDOM_SEED) & 1;   // Read analog value, mask out lowest bit and add the result.
  }
  randomSeed(seed);
  debugPrint("Analog Seed: ");
  debugPrintln(seed);
}


void setup() {

  debugSetup();  // Initializes Serial interface if DEBUG is defined.
  initRandom();  // Init the randomSeed

  // TRIGGER and ECHO pins are setup by NewPing lib, we don't care about them.

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  pinMode(BUTTON_PIN, INPUT);

  digitalWrite(BUTTON_PIN, BUTTON_PULLUP);


}


void tock(){
#if LED_PIN != SPEAKER_PIN  // if LED and SPEAKER pins differ, turn on speaker
  digitalWrite(SPEAKER_PIN, HIGH);
#endif
  digitalWrite(LED_PIN, HIGH);
  delay(TOCK_LENGTH);
  digitalWrite(LED_PIN, LOW);
#if LED_PIN != SPEAKER_PIN  // if LED and SPEAKER pins differ, turn off speaker
  digitalWrite(SPEAKER_PIN, LOW);
#endif
}

uint16_t nextDelay(long distance){

  // IDEA: Maybe we should also run the distance through some sort of log() function
  // in order to make the change more noticeable/realistic when we get closer.
  float distPct = ((float)distance - MIN_DISTANCE_CM) / (float)(MAX_DISTANCE_CM + 1);

  if(digitalRead(BUTTON_PIN)){  // No debouncing necessary here, we can just use the momentary reading.
    distPct = MIN_DIST_PCT_LOW + ((1.0 - MIN_DIST_PCT_LOW) * distPct);
  } else {
    distPct = MIN_DIST_PCT_HIGH + ((1.0 - MIN_DIST_PCT_HIGH) * distPct);
  }

  float nt = -log(((float)( random(9500) + 250 )/ 10001.0)) / digitalRead(BUTTON_PIN)?LOW_RATE:HIGH_RATE ;

  uint16_t tockDelay = 1000 * nt * (distPct);

  debugPrint(distance);
  debugPrint("   ");
  debugPrint(distPct);
  debugPrint("   ");
  debugPrint(nt);
  debugPrint("   ");
  debugPrint(tockDelay);
//  debugPrint("ms   ");
//  debugPrint(minDelay);
//  debugPrint("ms   ");
//  debugPrint(maxDelay);
  debugPrintln("ms");

  return tockDelay;

}


long getPing(){
  uint16_t distance = sonar.ping_cm(MAX_DISTANCE_CM);

  // Normalize reading
  // Sonar sensors occasionally have strange, wrong readings.
  // If we read anything out of our range, just take min or max instead.
  if(distance == 0){  // 0 means the ping "timed out", i.e. we are beyond max distance
    distance = MAX_DISTANCE_CM;
  } else if(distance < MIN_DISTANCE_CM){ // Readings below min_distance are cut off.
    distance = MIN_DISTANCE_CM;
  }

  return distance;
}


void loop() {


  long distance = getPing();
  long tockDelay = nextDelay(distance);  

  long ts = millis();

  /*
   * This while loop replaces the delay() that I used at first.
   * Problem was that if you hit a very long delay and then moved the sensor closer to an object,
   * the long delay would have to pass before the new "rate" would take hold, resulting in unrealistic behaviour.
   * 
   * This way, when you get closer to an object, the tick rate will increase. Feels more realistic to me.
   * 
   * However, this means that longer delays have a tendency to be artificially shortened due to ever-changing readings.
   * I think it's not really noticeable, though...
   */
  while(millis()-ts < tockDelay){
    long newDist = getPing();
    long diff = distance > newDist ? distance - newDist : newDist - distance;
    if(diff > ((MAX_DISTANCE_CM - MIN_DISTANCE_CM)/10)){
      debugPrint("! ");
      tockDelay = nextDelay(newDist);
      distance = newDist;
    } 
    if(millis() - ts + 50 < tockDelay){   // Wait for delay in 50ms steps.
      delay(50);  
    } else {
      long myDelay = tockDelay - (millis() - ts);
      delay(myDelay < 0 ? 1 : myDelay);
    }
  }

  tock(); // Finally, ladies and gents: The TOCK

}
