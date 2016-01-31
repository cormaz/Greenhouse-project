/*Project Domotic Greenhouse*/

/*Autors: La Mura Antonio, Festa Umberto*/

/*Temperature and humidity control with DHT22 sensor*/
/*Slave reads humidity and temperature in greenhouse, then it calculates CRC-8 with Dallas/Maxim's formulas and finally it uses that to ensure that any information is lost*/
/*Slave sends datas to Master via Wireless at 2.4[GHz], with NFR24L01 sensor and it receives from Master new settings, new thresholds of temperature and humidity*/
/*Slave compares the sensor's values with thresholds and it turns on or turns off the HEATER or the HUMIDIFIER or the FAN*/

/*Master: Arduino Mega - Slave: Arduino UNO*/

/*Slave*/

#include <SPI.h>
#include <Mirf.h>
#include <nRF24L01.h>
#include <MirfHardwareSpiDriver.h>
#include <EEPROM.h>
#include <DHT22.h>
#include <avr/pgmspace.h>

//Define all PIN 
#define HUMIDIFIER  A4
#define HEATER     A5
#define DHT22_PIN   4
//Define motore
#define  IS_1  0
#define  IS_2  1
#define  IN_1  3
#define  IN_2  11
#define  INH_1 12
#define  INH_2 13

#define TCONST 100	//Delay Time between Steps

//Define DHT22
DHT22 myDHT22(DHT22_PIN);

//Special character "ARROW"
byte arrow[8] = {
  0,4,6,31,6,4,0,0};

//Variable to send
char temp[]="123";
char hum[]="456";
float hmin = 65.0;
float hmax = 85.0;
float tmin = 18;
float tmax = 25; 
int duty_motor = 0;
float humidity;
float temperature;

//Variable to send every 10 seconds
unsigned long previousMillis = 0; 
unsigned long interval = 10000; 

void setup(){
  Serial.begin(9600);

  //Initialize PIN (INPUT - OUTPUT)
  pinMode(HEATER, OUTPUT);
  digitalWrite(HEATER, LOW);
  pinMode(HUMIDIFIER, OUTPUT);
  digitalWrite(HUMIDIFIER, LOW);

  //Initialize communication
  Mirf.spi = &MirfHardwareSpi;
  Mirf.init();

  //Set the receiving address
  Mirf.setRADDR((byte *)"clie1");

  //Set the string's lenght to receive Configuro la lunghezza della stringa stringa da ricevere  
  Mirf.payload = 32;
  Mirf.config();

  //Set the PIN for the fan
  pinMode(IN_1,OUTPUT);
  pinMode(IN_2,OUTPUT);
  pinMode(INH_1,OUTPUT);
  pinMode(INH_2,OUTPUT);
  //Reset
  reset_ports();
  digitalWrite(INH_1,1);
  digitalWrite(INH_2,1);

  Serial.println("Beginning ... "); 
  delay(2000);
}

void loop(){
  unsigned long time = millis();
  unsigned long currentMillis = millis();
  DHT22_ERROR_t errorCode;
  int m = 0;
  int i = 0;
  char name[] ="clie1";
  humidity = myDHT22.getHumidity()*10;
  temperature = myDHT22.getTemperatureC()*10;
  char Response[32];
  char toSend[32];
  errorCode = myDHT22.readData();
  byte data[Mirf.payload];
  byte crc;
  int crc_ric;

  //Check sensor humidity and temperature DHT22 errors
  switch(errorCode)
  {
  case DHT_ERROR_NONE:
    char buf[128];
    sprintf(buf, "Integer-only reading: Temperature %hi.%01hi C, Humidity %i.%01i %% RH",
    myDHT22.getTemperatureCInt()/10, abs(myDHT22.getTemperatureCInt()%10),
    myDHT22.getHumidityInt()/10, myDHT22.getHumidityInt()%10);
    break;
  case DHT_ERROR_CHECKSUM:
    break;
  case DHT_BUS_HUNG:
    break;
  case DHT_ERROR_NOT_PRESENT:
    break;
  case DHT_ERROR_ACK_TOO_LONG:
    break;
  case DHT_ERROR_SYNC_TIMEOUT:
    break;
  case DHT_ERROR_DATA_TIMEOUT:
    break;
  case DHT_ERROR_TOOQUICK:
    break;
  }
  if(humidity != myDHT22.getHumidity()*10 || temperature != myDHT22.getTemperatureC()*10){
    Serial.print("T: ");
    Serial.print(myDHT22.getTemperatureC(), 1);
    Serial.print("C");
    Serial.print(" H: ");
    Serial.print(myDHT22.getHumidity(), 1);
    Serial.println("%");
  }

  //Send the parameters every 10 seconds
  if(currentMillis - previousMillis > interval) {
    previousMillis = currentMillis;
    Mirf.setTADDR((byte *)"server");
    snprintf(toSend, sizeof(toSend), "%s;%d;%d", name, (int)humidity, (int)temperature);  
    Serial.println(toSend);
    Mirf.send((byte *)toSend);

    //Timeout, stop the trasmission
    while(Mirf.isSending()){
    }
    Serial.println("Finished sending");
    delay(10);

    while(!Mirf.dataReady()){
      if ( ( millis() - time ) > 1000 ) {
        return;
      }
    }
  }

  while(!Mirf.dataReady()){
    if ( ( millis() - time ) > 1000 ) {
      return;
    }
  }

  Mirf.getData(data);
  Serial.print("RAW DATA: ");
  Serial.println((char*)data);
  char* command = strtok((char *)data, ";");
  int count = 0;
  while (command != 0){

    //Split the information
    switch (count){
    case 0:
      snprintf(name, sizeof(name), "%s", command);
      break;

    case 1:
      hmax = atof(command)/10; //atof(char* ) change type char* in double
      break;

    case 2:
      hmin = atof(command)/10;
      break;

    case 3:
      tmax = atof(command)/10;
      break;

    case 4:
      tmin = atof(command)/10;
      break;

    case 5:
      crc_ric = atoi(command);
      break;
    }
    command = strtok(0, ";");
    count++;
  }
  Serial.print("Answer: ");
  Serial.print(name);
  Serial.print(";");
  Serial.print(hmax, 1);
  Serial.print(";");
  Serial.print(hmin, 1);
  Serial.print(";");
  Serial.print(tmax, 1);
  Serial.print(";");
  Serial.println(tmin, 1);
  Serial.print("CRC received: ");
  Serial.println(crc_ric);

  //calculate CRC-8, and I obtain the byte's array
  /********************************************************************************/
  byte bhmax = (byte)hmax;
  byte bhmin = (byte)hmin;
  byte btmax = (byte)tmax;
  byte btmin = (byte)tmin;

  byte crc32_str[4] = {
    bhmax, bhmin, btmax, btmin    };

  crc = CRC8(crc32_str);
  Serial.println("CRC: "); 
  Serial.println(crc);
  /********************************************************************************/

  //Check datas receive
  /********************************************************************************/
  if((byte)crc_ric == crc){
    Serial.println("CHECK DATA OK!");
    Mirf.setTADDR((byte *)name);
    Mirf.send((byte *)"OK");
  } 
  else {
    Serial.println("CHECK DATA ERROR!");
    Mirf.setTADDR((byte *)name);
    Mirf.send((byte *)"Error");
  }
  /********************************************************************************/

  Mirf.getData(data);

  if(data == (byte *)"OK"){
    //Save thresholds in EEPROM
    EEPROM_writeDouble(0, tmax);
    EEPROM_writeDouble(4, tmin);
    EEPROM_writeInt(8, hmax);
    EEPROM_writeInt(10, hmin);
  }

  //Manage the HHUMIDIFIER, HEATER and fan according to the sensor's value
  /**************************************************************************************/
  //HEATER
  if(myDHT22.getTemperatureC() >= tmax){
    digitalWrite(HEATER, HIGH);
  }
  if(myDHT22.getTemperatureC() <= tmin+1){
    digitalWrite(HEATER, LOW);
  }

  //HUMIDIFIER
  if((int)myDHT22.getHumidity() >= hmax){
    digitalWrite(HUMIDIFIER, HIGH);
  }
  if((int)myDHT22.getHumidity() <= hmin+1){
    digitalWrite(HUMIDIFIER, LOW);
  }

  //Fan, Brushless motor
  if(myDHT22.getTemperatureC() >= tmax+4){
    //Rotation of the motor according to with temperature, duty -> 0 per t = tmax+4 and duty -> 100 per t > tmax+10
    //Rotazione del motore al variare della temperatura, duty -> 0 per t = tmax+4 e duty -> 100 per t > tmax+10
    duty_motor = map(i , tmax+4, tmax+10, 0, 100);
    if(tmax > tmax+10){
      duty_motor = 100;
    }
    analogWrite(IN_2, duty_motor);
    delay(TCONST);
  }
  if(myDHT22.getTemperatureC() <= (tmax+tmin)/2){
    reset_ports();
    //Rotation of the motor according to with temperature, duty -> 0 per t = tmax+4 and duty -> 255 per t > tmax+10
    //Rotazione del motore al variare della temperatura, duty -> 0 per t = tmax+4 e duty -> 255 per t > tmax+10
    duty_motor = 0;
    analogWrite(IN_2, duty_motor);
    delay(TCONST);
  }

  /**************************************************************************************/

  delay(1000);
}

//Save double in EEPROM
void EEPROM_writeDouble(int ee, double value){

  byte* p = (byte*)(void*)&value;
  for (int i = 0; i < sizeof(value); i++)
    EEPROM.write(ee++, *p++);
}

//Save int in EEPROM
void EEPROM_writeInt(int ee, int value){

  byte* p = (byte*)(void*)&value;
  for (int i = 0; i < sizeof(value); i++)
    EEPROM.write(ee++, *p++);
}

//Read double to EEPROM
double EEPROM_readDouble(int ee){

  double value = 0.0;
  byte* p = (byte*)(void*)&value;
  for (int i = 0; i < 4; i++)
    *p++ = EEPROM.read(ee++);
  return value;
}

//Read int to EEPROM
int EEPROM_readInt(int ee){

  int value = 0;
  byte* p = (byte*)(void*)&value;
  for (int i = 0; i < 2; i++)
    *p++ = EEPROM.read(ee++);
  return value;
}

//Reset input
void reset_ports()
{
  digitalWrite(IN_1,0);
  digitalWrite(IN_2,0);
}

//Algorithm for CRC-8 based on Dallas/Maxim's farmulas
byte CRC8(const byte *data) {
  byte crc = 0x00;

  while (*data) {
    byte extract = *data++;
    for (byte tempI = 8; tempI; tempI--) {
      byte sum = (crc ^ extract) & 0x01;
      crc >>= 1;
      if (sum) {
        crc ^= 0x8C;
      }
      extract >>= 1;
    }
  }
  return crc;
}
