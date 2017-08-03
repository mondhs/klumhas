#include <SoftwareSerial.h>

SoftwareSerial masterSerial(5, 6); // RX, TX
String inputString = "";         // a string to hold incoming data
boolean stringComplete = false;  // whether the string is complete

void setup() {
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  //debug info
  //Serial.println("Master is prepared");
  
  // set the data rate for the SoftwareSerial port
  masterSerial.begin(9600);
}

void loop() {
  masterSerial.listen();
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
    //Serial.println(inputString);
    masterSerial.println(inputString);
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
      //readSlaveData();
      String params = masterSerial.readStringUntil('\n');
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



