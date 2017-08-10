/*
  // March 2014 - TMRh20 - Updated along with High Speed RF24 Library fork
  // Parts derived from examples by J. Coliz <maniacbug@ymail.com>
*/
/**
 * Example for efficient call-response using ack-payloads
 *
 * This example continues to make use of all the normal functionality of the radios including
 * the auto-ack and auto-retry features, but allows ack-payloads to be written optionally as well.
 * This allows very fast call-response communication, with the responding radio never having to
 * switch out of Primary Receiver mode to send back a payload, but having the option to if wanting
 * to initiate communication instead of respond to a commmunication.
 */


#include <avr/sleep.h>
#include <avr/wdt.h>

#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"
#include "printf.h"


#include <OneWire.h>
// DS18S20 Temperature chip i/o
OneWire ds(5);  // on pin D5
#define MAX_DS1820_SENSORS 2
byte addr[MAX_DS1820_SENSORS][8];
short foundSensors = 0;


#define BOUNCE_DURATION 200   // define an appropriate bounce time in ms for your switches
volatile unsigned long bounceTime=0; // variable to hold ms count to debounce a pressed switch

#include <Wire.h>
#include <U8x8lib.h>
U8X8_SSD1306_128X64_NONAME_SW_I2C u8x8(/* clock=*/ SCL, /* data=*/ SDA, /* reset=*/ U8X8_PIN_NONE);
volatile uint8_t is_enable_screen_saver =1;



// Hardware configuration: Set up nRF24L01 radio on SPI bus plus pins 7 & 8
RF24 radio(7,8);

// Topology
const uint64_t pipes[2] = { 0xABCDABCD71LL, 0x544d52687CLL };              // Radio pipe addresses for the 2 nodes to communicate.


// A single byte to keep track of the data being sent back and forth
byte counter = 1;

typedef struct MyDataStruct{
  byte response;
  bool lightState;
  int temperature;
  // long vcc;
} MyDataStruct_t;
MyDataStruct_t myData;

///LIGHT CONTROL
const int lightControlPin = 9;

void setup(){
  pinMode(lightControlPin, OUTPUT);

  myData.response = counter;
  myData.lightState = LOW;
  myData.temperature = 0;
  Serial.begin(9600);
  printf_begin();
  Serial.print(F("\n\rRF24 client/\n\r"));

  u8x8.begin();
  u8x8.setFont(u8x8_font_chroma48medium8_r);
  u8x8.drawString(0,0,"Temp: ");
  u8x8.drawString(0,1,">    <    |    ");


  DDRD &= ~(1 << DDD2);     // Clear the PD2 pin
  // PD2 (PCINT0 pin) is now an input
  PORTD |= (1 << PORTD2);    // turn On the Pull-up
  // PD2 is now an input with pull-up enabled
  EICRA |= (1 << ISC00);    // set INT0 to trigger on ANY logic change
  EIMSK |= (1 << INT0);     // Turns on INT0
  sei();                    // turn on interrupts


  initDS_temperature();

  // Setup and configure rf radio

  radio.begin();
  radio.setPALevel(RF24_PA_MAX);
  radio.setAutoAck(1);                    // Ensure autoACK is enabled
  radio.enableAckPayload();               // Allow optional ack payloads
  radio.setRetries(0,15);                 // Smallest time between retries, max no. of retries
  radio.setPayloadSize(sizeof(MyDataStruct_t));                // Here we are sending 1-byte payloads to test the call-response speed
  radio.openWritingPipe(pipes[0]);
  radio.openReadingPipe(1,pipes[1]);

  radio.printDetails();                   // Dump the configuration of the rf unit for debugging
}

void loop(void) {

  myData.response = counter;
  myData.temperature=readFirstTemperature();
  // myData.vcc = readVcc();


    radio.powerUp();
    radio.stopListening();                                  // First, stop listening so we can talk.
    u8x8.setCursor(6, 0);
    u8x8.print(((float)myData.temperature)/100);
    u8x8.drawString(1,1,"    ");
    u8x8.setCursor(1, 1);
    u8x8.print(counter);
    u8x8.setCursor(2, 2);
    // u8x8.print(myData.vcc);

    printf("Now sending %d as payload. \n",counter);

    unsigned long time = micros();                          // Take the time, and send it.  This will block until complete


    if (!radio.write( &myData, sizeof(myData) )){
      Serial.println(F("failed."));
    }else{
      if(!radio.available()){
        Serial.println(F("Blank Payload Received."));
      }else{
        while(radio.available() ){
          unsigned long tim = micros();
          MyDataStruct_t responseData;
          radio.read( &responseData, sizeof(MyDataStruct_t) );
          unsigned long roundTrip = tim-time;

          printf("Got response %d, round-trip delay: %lu microseconds; led %d\n\r",responseData.response,roundTrip, responseData.lightState);

          u8x8.drawString(6,1,"    ");
          u8x8.setCursor(6, 1);
          u8x8.print(myData.response);

          u8x8.drawString(11,1,"    ");
          u8x8.setCursor(11, 1);
          u8x8.print(roundTrip);

          digitalWrite(lightControlPin, responseData.lightState);


        }
      }
    }
    counter++;
    // Try again later
    //delay(1000);

    ///////////////////////////////////////
    //deepsleep
    radio.powerDown();

    // disable ADC
    ADCSRA = 0;

    // clear various "reset" flags
    MCUSR = 0;
    // allow changes, disable reset
    WDTCSR = bit (WDCE) | bit (WDE);
    // set interrupt mode and an interval
    WDTCSR = bit (WDIE) | bit (WDP2) | bit (WDP1);    // set WDIE, and 1 second delay
    // WDTCSR = bit (WDIE) | bit (WDP3) | bit (WDP0);    // set WDIE, and 8 seconds delay
    wdt_reset();  // pat the dog

    set_sleep_mode (SLEEP_MODE_PWR_DOWN);
    noInterrupts ();           // timed sequence follows
    sleep_enable();

    // turn off brown-out enable in software
    MCUCR = bit (BODS) | bit (BODSE);
    MCUCR = bit (BODS);
    interrupts ();             // guarantees next instruction executed
    sleep_cpu ();

    // cancel sleep as a precaution
    sleep_disable();

}


//******************************************************
//returns the temperature from one DS18S20 in DEG Celsius

void initDS_temperature(){
  for (int thisSensor = 0; thisSensor < MAX_DS1820_SENSORS; thisSensor++) {
    if (ds.search(addr[thisSensor])){
      foundSensors++;
      //found one
      return;
    }else{
      ds.reset_search();
      delay(250);
      break;
    }
  }
}

/**
 * Read first temperature
 */
int readFirstTemperature(){
  for (short sensor=0;sensor<foundSensors;sensor++){
    float temperature = findTemperature(addr[sensor]);
    return (int)(temperature*100);
  }
  return 0;
}


float findTemperature(byte *addr){
  byte data[12];
  //byte addr[8];


  if ( OneWire::crc8( addr, 7) != addr[7]) {
    Serial.println("CRC is not valid!");
    return -1000;
  }

  if ( addr[0] != 0x10 && addr[0] != 0x28) {
    Serial.print("Device is not recognized as is not a DS18S20 family device.");
    return -1000;
  }

  ds.reset();
  ds.select(addr);
  ds.write(0x44,1); // start conversion, with parasite   on at the end

  ds.reset();
  ds.select(addr);
  ds.write(0xBE); // Read Scratchpad


  for (int i = 0; i < 9; i++) { // we need 9 bytes
    data[i] = ds.read();
  }

  ds.reset_search();

  byte HighByte = data[1];
  byte LowByte = data[0];

  float tempRead = ((HighByte << 8) | LowByte); //using two's compliment
  float TemperatureSum = tempRead / 16;
  return TemperatureSum;

}

/////////////// Button ////////////
ISR (INT0_vect){
  // this is the interrupt handler for button presses
  // it ignores presses that occur in intervals less then the bounce time
  if (abs(millis() - bounceTime) > BOUNCE_DURATION){
    if(is_enable_screen_saver == 0){
      is_enable_screen_saver = 1;
    }else{
      is_enable_screen_saver = 0;
    }
    u8x8.setPowerSave(is_enable_screen_saver);

    Serial.println("Button clicked!!!");
    // Your code here to handle new button press ?
    bounceTime = millis();  // set whatever bounce time in ms is appropriate
  }

}


////// watchdog

// watchdog interrupt
ISR (WDT_vect)
{
   wdt_disable();  // disable watchdog
}  // end of WDT_vect


long readVcc() {
  // Read 1.1V reference against AVcc
  // set the reference to Vcc and the measurement to the internal 1.1V reference
  #if defined(__AVR_ATmega32U4__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
    ADMUX = _BV(REFS0) | _BV(MUX4) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  #elif defined (__AVR_ATtiny24__) || defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__)
    ADMUX = _BV(MUX5) | _BV(MUX0);
  #elif defined (__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__)
    ADMUX = _BV(MUX3) | _BV(MUX2);
  #else
    ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  #endif

  delay(2); // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Start conversion
  while (bit_is_set(ADCSRA,ADSC)); // measuring

  uint8_t low  = ADCL; // must read ADCL first - it then locks ADCH
  uint8_t high = ADCH; // unlocks both

  long result = (high<<8) | low;

  result = 1125300L / result; // Calculate Vcc (in mV); 1125300 = 1.1*1023*1000
  return result; // Vcc in millivolts
}
