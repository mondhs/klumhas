#include <SoftwareSerial.h>
#include <OneWire.h>


#define RELAY1  4
#define LIGHT_SENSOR 1
#define LEVEL_SENSOR 3


// DS18S20 Temperature chip i/o
OneWire ds(2);  // on pin 2


#define MAX_DS1820_SENSORS 10
byte addr[MAX_DS1820_SENSORS][8];
short foundSensors = 0;
String command = String();
SoftwareSerial slaveSerial(10, 11); // RX, TX

String RESPONSE_WAITING=String("SLAVE_WAITING!");
String REQUEST_DATA=String("DATA?");
String REQUEST_PING=String("PING?");
String RESPONSE_PONG=String("PONG!");
String REQUEST_LAMP1_ON=String("STATE?lamp1=on;");
String RESPONSE_LAMP1_ON=String("STATE!lamp1=on;");
String REQUEST_LAMP1_OFF=String("STATE?lamp1=off;");
String RESPONSE_LAMP1_OFF=String("STATE!lamp1=off;");
String REQUEST_LAMP2_ON=String("STATE?lamp2=on;");
String RESPONSE_LAMP2_ON=String("STATE!lamp2=on;");
String REQUEST_LAMP2_OFF=String("STATE?lamp2=off;");
String RESPONSE_LAMP2_OFF=String("STATE!lamp2=off;");

void setup() {
  // set the data rate for the SoftwareSerial port
  for (int thisSensor = 0; thisSensor < MAX_DS1820_SENSORS; thisSensor++) {
    if (ds.search(addr[thisSensor])){
      foundSensors++;
    }
    else{
      ds.reset_search();
      delay(250);
      break;
    }
  }
  pinMode(RELAY1, OUTPUT);
  slaveSerial.begin(9600);
  //slaveSerial.setTimeout(1000);
  slaveSerial.println(RESPONSE_WAITING);
  //debug info
  //Serial.begin(9600); Serial.print("Slave ready");
}



void loop() {
  //debug info
  //Serial.println("Slave Loop1");
  
  if (slaveSerial.available() > 0) {
    String command = slaveSerial.readStringUntil('\n');
    command.trim();
    
    //debug info
    //Serial.print("Command recieved: ");Serial.println(command);

    if (command == REQUEST_DATA){
      sendData();
    }else if(command == REQUEST_PING){
      slaveSerial.println(RESPONSE_PONG);
    }else if(command == REQUEST_LAMP1_ON){
      changeLightState(RELAY1, HIGH);
      slaveSerial.println(RESPONSE_LAMP1_ON);
    }else if(command == REQUEST_LAMP1_OFF){
      changeLightState(RELAY1, LOW);
      slaveSerial.println(RESPONSE_LAMP1_OFF);
    }else if(command == REQUEST_LAMP2_ON){
      slaveSerial.println(RESPONSE_LAMP2_ON);
    }else if(command == REQUEST_LAMP2_OFF){
      slaveSerial.println(RESPONSE_LAMP2_OFF);
    }else{
      String unknown=String("UNKOWN!");
      unknown = unknown+command;
      slaveSerial.println(unknown);
    }
    slaveSerial.flush();
  }
  delay(1000);
}

void changeLightState(int relayPin, int state){
  digitalWrite(relayPin, state);
}

void sendData(){
  String result = String("DATA!");
  for (short sensor=0;sensor<foundSensors;sensor++){
    float temperature = findTemperature(addr[sensor]);
    result += toStr(String("temp")+String(sensor),temperature);
  }
  result += toStr("light",readLight());
  result += toStr("waterLevel",readWaterLevel());
  result += toStr("paramNo", 2+foundSensors);//end
  slaveSerial.println(result);
  slaveSerial.flush();
}

String toStr(String key, float val){
  String result = key;
  result += String(":");
  result += val;
  result += ";";
  return result;
}

String toStr(String key, int val){
  String result = key;
  result += String(":");
  result += val;
  result += ";";
  return result;
}

int readLight(){
  return readAnalodSensor(LIGHT_SENSOR);
}

int readWaterLevel(){
  return readAnalodSensor(LEVEL_SENSOR);
}

int readAnalodSensor(int sensorPin){
    float lSensorCurr = 0;
  for (int i = 0; i < 10; i++){
    int iSensorCurr =analogRead(sensorPin);
    lSensorCurr += (float)iSensorCurr;
  }
  return (int)(lSensorCurr/10);
}


//returns the temperature from one DS18S20 in DEG Celsius
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
  ds.write(0x44,1); // start conversion, with parasite power on at the end

  byte present = ds.reset();
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
