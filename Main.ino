

#include <TFT.h>
#include <SPI.h>
#include "Arduino.h"
#include <Wire.h>
#include <Math.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "GsmFunctions.h"


template<class T> inline Print &operator <<(Print &obj, T arg)
{
  obj.print(arg);
  return obj;
} 


#define PF(x) pgm_read_float(x)
#define RST 8
#define DC 9
#define CS 10
#define interval 4000

TFT Screen = TFT(CS, DC, RST);
OneWire OW(2);                               // Setup a oneWire instance to communicate with any OneWire devices, connecting data wire into pin 2
DallasTemperature sensors(&OW);              // Pass our oneWire reference to Dallas Temperature

// Huom: Yritä saada muuttujia PROGMEMiin
uint8_t dest;
float lon, lat;

const char destText[4][15] = { "Kotkantie" , "Viehetie" , "Tirolintie" , "Santerinkuja" };
const PROGMEM float destPos[4][2] =
{
  64.999488 , 25.512225  ,    // Kotkantie
  65.046135 , 25.483199  ,    // Viehetie
  65.034385 , 25.462756  ,    // Tirolintie       
  64.893637 , 25.564052       // Santerinkuja
};





float trueGPS(float x)
{
  return (x - floor(x/100) * 40.0) / 60;
}


float latitude()
{
  int8_t lat[10] = { '0','0','0','0','0','0','0','0','0','0' };     // Store the latitude data
  receiveData(lat, 1, 10);
  return trueGPS( dataTransfer(lat, 5) );
}


float longitude()
{
  int8_t lon[11] = { '0','0','0','0','0','0','0','0','0','0','0' };   // Store longitude data
  receiveData(lon, 3, 11);
  return trueGPS( dataTransfer(lon, 5) );
}


void setup()
{
  Wire.begin();
  Serial.begin(9600);
  sensors.begin();

  Screen.begin();
  Screen.fillScreen(0);
  Screen.stroke(255,255,255);
  
  Screen.text("Current location", 0, 0);
  Screen.text("Distance to", 0, 30);
  Screen.text("Speed", 0, 60);
  Screen.text("Temperature", 0, 90);

  Screen.stroke(0,255,0);  
}


float laskeEtaisyys(float lat1, float lon1, float lat2, float lon2)
{
  return 6378.8 * ( 2.0 * asin(sqrt(square(sin(.5*(lat1-lat2)))+cos(lat1)*cos(lat2)*square(sin(.5*(lon2-lon1))))) );
}


void checkSerial()
{
  if ( Serial.available() )
  {
    uint8_t input = Serial.parseInt();
    dest = constrain(input, 0, 3);
  }
}


void draw(float value, uint8_t precision, uint8_t X, uint8_t Y)
{
  uint8_t displayStr[10];
  Screen.fillRect(X, Y, 5*(precision+1), 7, 0);
  String(value, precision).toCharArray(displayStr, precision);
  Screen.text(displayStr, X, Y);
}


void updateScreen(float dist, float vel, float temp)
{
  draw(lat, 10, 0, 15);
  draw(lon, 10, 65, 15);
  draw(dist, 6, 0, 45);
  Screen.text("km", 35, 45);
  Screen.fillRect(74, 30, 65, 7, 0);
  Screen.text(destText[dest], 74, 30);
  draw(vel, 5, 0, 75);
  Screen.text("km/h", 40, 75);
  draw(temp, 5, 0, 105);
  Screen.text("C", 40, 105);
}


float rad(float X)
{
  return X * .017453292519;         // return X * pi / 180.0;
}


void loop()
{  
  while (1)
  {
    static float oldLon = lon;
    static float oldLat = lat;
    lon = longitude();
    lat = latitude();

    float dist = laskeEtaisyys( rad(lat), rad(lon), rad(PF(&destPos[dest][0])), rad(PF(&destPos[dest][1])) );
    float vel = 3.6 * laskeEtaisyys( rad(lat), rad(lon), rad(oldLat), rad(oldLon) ) / interval;

    sensors.requestTemperatures();
    float temp = sensors.getTempCByIndex(0);

    Serial  <<  "Current position: "  <<  String(lat, 5)  <<  " , "  <<  String(lon, 5)  <<  "\nDistance to "  <<  destText[dest]  <<  ": "  <<  dist  <<  " km.\n";
    updateScreen(dist, vel, temp);
    checkSerial();

    while ( millis() % interval > 5 );
  }
}

