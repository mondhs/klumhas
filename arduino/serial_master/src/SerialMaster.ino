#include <SoftwareSerial.h>
#include <SPI.h>
#include <avr/sleep.h>
#include <avr/power.h>
#include "nRF24L01.h"
#include "RF24.h"

SoftwareSerial masterSerial(5, 6); // RX, TX
String inputString = "";         // a string to hold incoming data
boolean stringComplete = false;  // whether the string is complete

/****************** User Config ***************************/

/* Hardware configuration: Set up nRF24L01 radio on SPI bus plus pins 7 & 8 */
RF24 radio(7,8);
const uint64_t pipes[2] = { 0xABCDABCD71LL, 0x544d52687CLL };   // Radio pipe addresses for the 2 nodes to communicate.

struct dataStruct{
  byte response;
  bool lightState;
  int temperature;
  // long vcc;
}myData;

String REQUEST_RFLAMP1_ON=String("RFSTATE?lamp1=on;");
String REQUEST_RFLAMP1_OFF=String("RFSTATE?lamp1=off;");

/**********************************************************/

void setup() {
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  //debug info
  //Serial.println("Master is prepared");

  // set the data rate for the SoftwareSerial port
  masterSerial.begin(9600);

  ////// RADIO ////
  radio.begin();
  radio.setAutoAck(1);                    // Ensure autoACK is enabled
  radio.enableAckPayload();               // Allow optional ack payloads
  radio.setRetries(0,15);                 // Smallest time between retries, max no. of retries
  radio.setPayloadSize(sizeof(myData));                // Here we are sending 1-byte payloads to test the call-response speed
  radio.openWritingPipe(pipes[1]);        // Both radios listen on the same pipes by default, and switch when writing
  radio.openReadingPipe(1,pipes[0]);
  radio.startListening();                 // Start listening

}

void loop() {
  while (Serial.available()) {
    // get the new byte:
    char inChar = (char)Serial.read();
    // add it to the inputString:
    inputString += inChar;
    // if the incoming character is a newline, set a flag
    // so the main loop can do something about it:
    if (inChar == '\n') {
      stringComplete = true;
    }
  }


  if (stringComplete) {
    inputString.trim();
    if(inputString == REQUEST_RFLAMP1_ON){
      myData.lightState = HIGH;
    }else if(inputString == REQUEST_RFLAMP1_OFF){
      myData.lightState = LOW;
    }else{
      //Serial.println(inputString);
      masterSerial.println(inputString);
    }
    // clear the string:
    inputString = "";
    stringComplete = false;
  }
  //masterSerial.println(String("DATA?"));
  //delay(2000);
  if (masterSerial.available()) {
    //Serial.println("Data from masterSerial port:");
    String command = masterSerial.readStringUntil('!');
    command.trim();

    if(command.length() == 0){
      //noop;
    }else if(command==String("DATA")){
      readSlaveData();
      String params = masterSerial.readStringUntil('\n');
      params = params + ";tempRF:" + myData.temperature;
      Serial.println(params);
    }else if(command==String("PONG")){
      Serial.println(command);
    }else if(command==String("STATE")){
      Serial.println(command);
      String params = masterSerial.readStringUntil('\n');
      Serial.println(params);
    }else{
      Serial.print("Unknown command: ");
      Serial.println(command);
      String params = masterSerial.readStringUntil('\n');
      Serial.println(params);
    }
    //debug info
    //Serial.println("----");
  }

  byte pipeNo;
  while (radio.available(&pipeNo)) {                             // Dump the payloads until we've gotten everything
    radio.read( &myData, sizeof(myData) );
    radio.writeAckPayload(pipeNo,&myData, sizeof(myData));
    printf("Got response %d, temp: %d\n\r",myData.response, myData.temperature);
  }

}


void readSlaveData(){
 String paramKey = masterSerial.readStringUntil(':');
 String paramVal = masterSerial.readStringUntil(';');
 Serial.println(paramKey + String("=") + paramVal );
 while(paramKey!=String("paramNo")){
   paramKey = masterSerial.readStringUntil(':');
   paramVal = masterSerial.readStringUntil(';');
   Serial.println(paramKey + String("=") + paramVal );
 }
}
