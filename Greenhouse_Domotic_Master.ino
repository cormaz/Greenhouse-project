/*Project Domotic Greenhouse*/

/*Autors: La Mura Antonio, Festa Umberto*/

/*Temperature and humidity control with DHT22 sensor*/
/*Master communicates with webserver via WiFi, using CC3000 shield. It sends datas which are received from Slave, to DataBase. (You can see how it works on www.cormaz.altervista.org)*/
/*Master receives datas from Slave via Wireless at 2.4[GHz], with NFR24L01 sensor and if it is necessary, it can send new thresholds of temperature and humidity*/

/*Master: Arduino Mega - Slave: Arduino UNO*/

/*Master*/


#include <Adafruit_CC3000.h>
#include <SPI.h>
#include "utility/debug.h"
#include "utility/socket.h"
#include <Ethernet.h>
#include <Mirf.h>
#include <nRF24L01.h>
#include <MirfHardwareSpiDriver.h>
#include <EEPROM.h>

// These are the interrupt and control pins
#define ADAFRUIT_CC3000_IRQ   3  // MUST be an interrupt pin!
// These can be any two pins
#define ADAFRUIT_CC3000_VBAT  5
#define ADAFRUIT_CC3000_CS    10
// Use hardware SPI for the remaining pins
// On an UNO, SCK = 13, MISO = 12, and MOSI = 11
Adafruit_CC3000 cc3000 = Adafruit_CC3000(ADAFRUIT_CC3000_CS, ADAFRUIT_CC3000_IRQ, ADAFRUIT_CC3000_VBAT,
SPI_CLOCK_DIVIDER); // you can change this clock speed

#define WLAN_SSID       "your_line"           // cannot be longer than 32 characters!
#define WLAN_PASS       "your_password"
// Security can be WLAN_SEC_UNSEC, WLAN_SEC_WEP, WLAN_SEC_WPA or WLAN_SEC_WPA2
#define WLAN_SECURITY   WLAN_SEC_WPA2

#define LISTEN_PORT           7    // What TCP port to listen on for connections.  The echo protocol uses port 7.

Adafruit_CC3000_Server echoServer(LISTEN_PORT);

//Variable reset millis()
extern unsigned long timer0_millis;

byte subnet[] = { 
  255,255,255,0};
byte gateway[] = { 
  192,168,0,1};
const char server[] = "www.yoursite.com";  //Insert your website

Adafruit_CC3000_Client client;

const unsigned long
  connectTimeout  = 15L * 1000L; // Max time to wait for server connection

//Sends this variable to client
double hmin = 65.0;
double hmax = 85.0;
float tmin = 3.0;
float tmax = 45.0; 

//Variable to check the correct datas received
double hminr = 65.0;
double hmaxr = 85.0;
float tminr = 3.0;
float tmaxr = 45.0;

char toSend[32] = {
  '\0'};

void setup(void)
{
  Serial.begin(115200);
  Serial.println(F("Hello, CC3000!\n")); 
  
  /* Initializes the module */
  Serial.println(F("\nInitializing..."));
  if (!cc3000.begin())
  {
    Serial.println(F("Couldn't begin()! Check your wiring?"));
    while(1);
  }

  Serial.print(F("\nAttempting to connect to ")); 
  Serial.println(WLAN_SSID);
  if (!cc3000.connectToAP(WLAN_SSID, WLAN_PASS, WLAN_SECURITY)) {
    Serial.println(F("Failed!"));
    while(1);
  }

  Serial.println(F("Connected!"));

  Serial.println(F("Request DHCP"));
  while (!cc3000.checkDHCP())
  {
    delay(100); // ToDo: Insert a DHCP timeout!
  }  

  /* Display the IP address DNS, Gateway, etc. */
  while (! displayConnectionDetails()) {
    delay(1000);
  }

  /*********************************************************/
  /* You can safely remove this to save some flash memory! */
  /*********************************************************/
  Serial.println(F("\r\nNOTE: This sketch may cause problems with other sketches"));
  Serial.println(F("since the .disconnect() function is never called, so the"));
  Serial.println(F("AP may refuse connection requests from the CC3000 until a"));
  Serial.println(F("timeout period passes.  This is normal behaviour since"));
  Serial.println(F("there isn't an obvious moment to disconnect with a server.\r\n"));

  //Starts listening for connections

  Serial.println(F("Listening for connections..."));

  uint32_t ipAddress, netmask, gateway, dhcpserv, dnsserv;

  if(!cc3000.getIPAddress(&ipAddress, &netmask, &gateway, &dhcpserv, &dnsserv))
  {
    Serial.println(F("Unable to retrieve the IP Address!\r\n"));
  }
  else
  {
    Serial.println(F("IP Address obtained!\r\n"));
    uint8_t macAddres[6];
    Serial.println(ipAddress);
    if(!cc3000.getMacAddress(macAddres))
    {
      Serial.println(F("Unable to retrieve MAC Address!\r\n"));
    }
    else{
      Serial.println(F("MAC Address obtained!\r\n"));
           
    }
  }
  
  //Initializes communication
  Mirf.spi = &MirfHardwareSpi;
  Mirf.init();

  //Sets the receiving address
  Mirf.setRADDR((byte *)"server");
  //Sets the string lenght to be received
  Mirf.payload = 32;
  Mirf.config();

  Serial.println("Beginning ... "); 
  delay(2000);

}

void loop(void)
{
  unsigned long t;
  String strURL;
  unsigned long ip;
  
  int i = 0;
  double temp;
  double hum;
  char name[6];
  byte data[Mirf.payload];
  char threshold [32];
  byte crc;

  while(!Mirf.dataReady()){
    //Waiting for some information
  }
  Mirf.getData(data);

  Serial.print("RAW DATA: ");
  Serial.println((char*)data);
  char* command = strtok((char *)data, ";");
  int count = 0;
  while (command != 0){

    //Splits the information
    switch (count){
    case 0:
      snprintf(name, sizeof(name), "%s", command);
      break;

    case 1:
      temp = atof(command)/10;
      break;

    case 2:
      hum = atof(command)/10;
      break;
    }

    command = strtok(0, ";");
    count++;
  }
  Serial.print("Package received from ");
  Serial.print(name);
  Serial.print("  T: ");
  Serial.print(temp, 1);
  Serial.print(" H: ");
  Serial.println(hum, 1);

  delay(20);

  float humax = 0;
  float humin = 0; 
  float tumax = 0;
  float tumin = 0;   

  if(Serial.read() == 'k'){

    Serial.println("Insert humidity max");
    while (Serial.available() == 0);
    humax = Serial.parseFloat();
    Serial.flush();

    Serial.println("Insert humidity min");
    while (Serial.available() == 0);
    humin = Serial.parseFloat();
    Serial.flush();

    Serial.println("Insert temperature max");
    while (Serial.available() == 0);
    tumax = Serial.parseFloat();
    Serial.flush();

    Serial.println("Insert temperature min");
    while (Serial.available() == 0);
    tumin = Serial.parseFloat();

    Serial.print("Written: ");
    Serial.println(tumin);
    Serial.flush();

    delay(100);

    //CRC-8, creates byte's array
    /********************************************************************************/
    byte bhmax = (byte)humax;
    byte bhmin = (byte)humin;
    byte btmax = (byte)tumax;
    byte btmin = (byte)tumin;

    byte crc32_str[4] = {
      bhmax, bhmin, btmax, btmin        };

    crc = CRC8(crc32_str);
    Serial.println("CRC: "); 
    Serial.println(crc);
    /********************************************************************************/

    humax = humax * 10;
    humin = humin * 10;
    tumax = tumax * 10;
    tumin = tumin * 10;

    Mirf.setTADDR((byte *)name);

    snprintf(threshold, sizeof(toSend), "%s;%d;%d;%d;%d;%d", name, (int)humax, (int)humin, (int)tumax, (int)tumin, (int)crc);
    Serial.print("Thresholds: ");
    Serial.println(threshold);
    Mirf.send((byte *)threshold);

    //Timeout, stop trasmission
    while(Mirf.isSending()){
    }
    Serial.println("Finished sending");
    delay(10);
    t = millis();
    while(!Mirf.dataReady()){
      if ( ( millis() - t ) > 1000 ) {
        Serial.println("Timeout Setting parametri");
        resetMillis();
        return;
      }
    }
    delay(10);
  }
  
  strURL = "GET /address.php?name=";
  strURL += 'name';
  strURL += "&parameter1=";
  strURL += 'temp';
  strURL += "&parameter2=";
  strURL += 'hum';
  
  //Hostname lookup
  Serial.print(F("Locating Web server..."));
  cc3000.getHostByName((char *)server, &ip);
  
  Serial.print("IP: ");
  cc3000.printIPdotsRev(ip);
  Serial.println("");
  
  //Connect to numeric IP
  Serial.print(F("OK\r\nConnecting to server..."));
  t = millis();
  
  do {
    client = cc3000.connectTCP(ip, 80);
  } while((!client.connected()) && ((millis() - t) < connectTimeout));
      resetMillis();

  if (client.connected()) {
    Serial.println("Loop connected");
    
    client.print(strURL);
    client.println(" HTTP/1.1");
    client.println("Host: Insert your host");  //Insert your host
    client.println("Content-Type: application/x-www-form-urlencoded");
    client.println("Connection: close");
    client.println();

    Serial.print(strURL);
    Serial.println(" HTTP/1.1");
    Serial.println("Host: Insert your host");  //Insert your host
    Serial.println("Content-Type: application/x-www-form-urlencoded");
    Serial.println("Connection: close");
    Serial.println();

  }
}

/**************************************************************************/
/*!
   Tries to read the IP address and other connection details
 */
/**************************************************************************/
bool displayConnectionDetails(void)
{
  uint32_t ipAddress, netmask, gateway, dhcpserv, dnsserv;

  if(!cc3000.getIPAddress(&ipAddress, &netmask, &gateway, &dhcpserv, &dnsserv))
  {
    Serial.println(F("Unable to retrieve the IP Address!\r\n"));
    return false;
  }
  else
  {
    Serial.print(F("\nIP Addr: ")); 
    cc3000.printIPdotsRev(ipAddress);
    Serial.print(F("\nNetmask: ")); 
    cc3000.printIPdotsRev(netmask);
    Serial.print(F("\nGateway: ")); 
    cc3000.printIPdotsRev(gateway);
    Serial.print(F("\nDHCPsrv: ")); 
    cc3000.printIPdotsRev(dhcpserv);
    Serial.print(F("\nDNSserv: ")); 
    cc3000.printIPdotsRev(dnsserv);
    Serial.println();
    return true;
  }
}

//CRC-8 - algorithm based on Dallas/Maxim's formulas
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

//Reset millis()
void resetMillis() {
  cli();
  timer0_millis = 0;
  sei();
}
