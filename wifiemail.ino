#include <ESP8266WiFi.h>
#include "Gsender.h"
#include <OneWire.h>
#include <DallasTemperature.h>


#pragma region Globals
const char* ssid = "";                           // WIFI network name
const char* password = "";                       // WIFI network password
uint8_t connection_state = 0;                    // Connected to WIFI or not
uint16_t reconnect_interval = 10000;             // If not connected wait time to try again
float alertMedium = -18;
float alertHight = -12;
// Data wire is plugged into port 4 on the Arduino or ESP32

#define ONE_WIRE_BUS D4// PIN D2 ON ESP8266
#define TEMPERATURE_PRECISION 10

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);

// Tableaux contenant l'adresse de chaque sonde OneWire | arrays to hold device addresses
DeviceAddress insideThermometer = { 0x28,  0xEA,  0x20,  0x77,  0x91,  0x6,  0x2,  0xBB };
DeviceAddress outsideThermometer = { 0x28,  0xF4,  0xBC,  0x26,  0x0,  0x0,  0x80,  0x2B };
#pragma endregion Globals

uint8_t WiFiConnect(const char* nSSID = nullptr, const char* nPassword = nullptr)
{
    static uint16_t attempt = 0;
    Serial.print("Connecting to ");
    if(nSSID) {
        WiFi.begin(nSSID, nPassword);  
        Serial.println(nSSID);
    } else {
        WiFi.begin(ssid, password);
        Serial.println(ssid);
    }

    uint8_t i = 0;
    while(WiFi.status()!= WL_CONNECTED && i++ < 50)
    {
        delay(200);
        Serial.print(".");
    }
    ++attempt;
    Serial.println("");
    if(i == 51) {
        Serial.print("Connection: TIMEOUT on attempt: ");
        Serial.println(attempt);
        if(attempt % 2 == 0)
            Serial.println("Check if access point available or SSID and Password\r\n");
        return false;
    }
    Serial.println("Connection: ESTABLISHED");
    Serial.print("Got IP address: ");
    Serial.println(WiFi.localIP());
    return true;
}

void Awaits()
{
    uint32_t ts = millis();
    while(!connection_state)
    {
        delay(50);
        if(millis() > (ts + reconnect_interval) && !connection_state){
            connection_state = WiFiConnect();
            ts = millis();
        }
    }
}

void sendMail(float tempC)
{
  String mystring;
  
  mystring = String(tempC);

  if(tempC > alertHight)
  {
    Gsender *gsender = Gsender::Instance();    // Getting pointer to class instance
    String subject = "Temperature du congelateur hight alert";   
    if(gsender->Subject(subject)->Send("nicolas@darder.fr", mystring)) {
          Serial.println("Message send.");
    } else {
          Serial.print("Error sending message: ");
          Serial.println(gsender->getError());
    } 
  }
  else if(tempC > alertMedium)
  {
    Gsender *gsender = Gsender::Instance();    // Getting pointer to class instance
    String subject = "Temperature du congelateur medium alert";   
    if(gsender->Subject(subject)->Send("nicolas@darder.fr", mystring)) {
          Serial.println("Message send.");
    } else {
          Serial.print("Error sending message: ");
          Serial.println(gsender->getError());
    } 
  }
  
}

void printTemperature(String label, DeviceAddress deviceAddress){
  float tempC = sensors.getTempC(deviceAddress);
  Serial.print(label);
  if (tempC == -127.00) {
    Serial.print("Error getting temperature");
  } else {
    Serial.print(" Temp C: ");
    Serial.print(tempC);
    Serial.print(" Temp F: ");
    Serial.println(DallasTemperature::toFahrenheit(tempC));
     
    sendMail(tempC);
  }  
}

void setup()
{
    Serial.begin(115200);
    connection_state = WiFiConnect();
    if(!connection_state)  // if not connected to WIFI
        Awaits();          // constantly trying to connect    
    
    // Start up the library
  sensors.begin();

  // locate devices on the bus
  Serial.print("Locating devices...");
  Serial.print("Found ");
  Serial.print(sensors.getDeviceCount(), DEC);
  Serial.println(" devices.");

  // report parasite power requirements
  Serial.print("Parasite power is: "); 
  if (sensors.isParasitePowerMode()) Serial.println("ON");
  else Serial.println("OFF");

  // Vérifie sir les capteurs sont connectés | check and report if sensors are conneted 
  if (!sensors.getAddress(insideThermometer, 0)) Serial.println("Unable to find address for Device 0"); 
  //if (!sensors.getAddress(outsideThermometer, 1)) Serial.println("Unable to find address for Device 1"); 

  // set the resolution to 9 bit per device
  sensors.setResolution(insideThermometer, TEMPERATURE_PRECISION);
  //sensors.setResolution(outsideThermometer, TEMPERATURE_PRECISION);

  // On vérifie que le capteur st correctement configuré | Check that ensor is correctly configured
  Serial.print("Device 0 Resolution: ");
  
}

void loop(){
  Serial.print(sensors.getResolution(insideThermometer), DEC); 
  Serial.println();
  Serial.print("Requesting temperatures...");
  sensors.requestTemperatures();
  Serial.println("DONE");

  // print the device information
  printTemperature("Inside : ", insideThermometer);
  delay(60000);
}
