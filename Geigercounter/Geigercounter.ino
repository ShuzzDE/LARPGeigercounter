/**
 * 
 * Geigercounter 0.95b
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
 * Uses the NewPing library by Tim Eckel for 
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
#define BUTTON_PULLUP LOW  // External pullup
#undef DEBUG   // Disable Debugging
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
#define BUTTON_PULLUP LOW  // External pullup, change to HIGH when removing ext.PU resistor
#undef DEBUG   // Disable Debugging
#else
// Pin definitions for development Board (Arduino UNO in my case)
#define TRIGGER_PIN 3
#define ECHO_PIN 4
#define LED_PIN 10
#define SPEAKER_PIN 10
#define BUTTON_PIN 12
#define ANALOG_PIN_FOR_RANDOM_SEED A1
#define BUTTON_PULLUP HIGH  // Internal pullup
#define DEBUG   // Enable Debugging
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
 * 
 * tock rates will fall off exponentially between minimum and maximum distances.
 * This means that you'll get the defined LOW and HIGH rates at minimum distance only.
 * Increase the distance and the tock rates will decrease exponentially.
 * 
 * Basically you can use this to tune the "sensitivity" of your device.
 * 
 * Keep in mind: 
 * If MAX_DISTANCE_CM == 4 * MIN_DISTANCE_CM 
 * then TOCK_RATE@MAX_DIST == 1/16 * TOCK_RATE@MIN_DIST
 */
#define MAX_DISTANCE_CM 80   // Maximum distance acknowledged
#define MIN_DISTANCE_CM 20   // Minimum distance to object

/*
 * Counts per Minute
 * Higher values mean faster tocks.
 * 
 * Example:
 * A rate of 100 CPM means you'll get an average of 100 tocks per minute AT MINIMUM DISTANCE.
 * Note that cpm will fall off exponentially with increased distance.
 */
#define LOW_CPM 100.0d    // Rate for button NOT pressed
#define HIGH_CPM 1800.0d   // Rate for button pressed (aka. "OMG we have radioactivity"-mode)


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

/**
 * Setup routine. The usual, pins, pullups, ...
 */
void setup() {

  debugSetup();  // Initializes Serial interface if DEBUG is defined.
  initRandom();  // Init the randomSeed

  // TRIGGER and ECHO pins are setup by NewPing lib, we don't care about them.

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  pinMode(BUTTON_PIN, INPUT);
  digitalWrite(BUTTON_PIN, BUTTON_PULLUP);


}

/**
 * Generate a single tock on the speaker and flash the LED.
 * Speaker and LED will be turned on for TOCK_LENGTH ms.
 */
void tock(){
  digitalWrite(SPEAKER_PIN, HIGH);
  digitalWrite(LED_PIN, HIGH);
  delay(TOCK_LENGTH);
  digitalWrite(LED_PIN, LOW);
  digitalWrite(SPEAKER_PIN, LOW);
}



/**
 * Calculate a random, poisson-distributed delay until the next tock event.
 * 
 * Result is based on measured distance as well as configured CPM rates.
 * The state of the button is included.
 */
uint16_t nextDelay(long distance){

  // Calculate exponential falloff.
  float distPct = pow((float)MIN_DISTANCE_CM/(float)(distance/2.0 + MIN_DISTANCE_CM/2.0),2.0);

  // Count per minute decided by Button (Hi/Lo) multiplied by distance falloff.
  double cpm = (digitalRead(BUTTON_PIN)?LOW_CPM:HIGH_CPM) * distPct;

  // Calculate poisson distributed random number for current cpm.
  // random number is cut a bit short of 0 and 1 in order to reduce extreme outliers.
  // TODO: This might need a bit more tweaking.
  // Result is a fraction of a minute since parameter is count per minute.
  float nextTock = -log(((float)( random(9500) + 250 )/ 10001.0)) / cpm ;

  // nextTock is multiplied by 60000ms in order to get waiting time.
  uint16_t tockDelay = 60000 * nextTock;

  debugPrint(distance);
  debugPrint("   ");
  debugPrint(distPct);
  debugPrint("   ");
  debugPrint(cpm);
  debugPrint("   ");
  debugPrint(nextTock);
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

  // Get reading from sensor.
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
   * Keep checking the distance while waiting for the next tock.
   * Reason is we don't want to keep delay()'ing when the device closes in on an object.
   */
  while(millis()-ts < tockDelay){
    long newDist = getPing();
    long diff = distance > newDist ? distance - newDist : newDist - distance;
    if(diff > ((MAX_DISTANCE_CM - MIN_DISTANCE_CM)/10)){ // Difference bigger than 10% of max distance interval?
      debugPrint("! ");  // Output ! so we can see the value was taken due to changes in distance in debug output.
      tockDelay = nextDelay(newDist);
      distance = newDist;
    }
     
    if(millis() - ts + 50 < tockDelay){   // Wait for delay in 50ms steps.
      delay(50);  
    } else {
      long myDelay = tockDelay - (millis() - ts);
      delay(myDelay < 0 ? 0 : myDelay);
    }
  }

  tock(); // Finally, ladies and gents: The TOCK

}
