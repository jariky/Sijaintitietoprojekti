
/*
  Huom:
    Optimoi muuttujien kokoja
    Yritä saada muuttujia PROGMEMiin
*/


#include <TFT.h>
#include <SPI.h>
#include "Arduino.h"
#include <Wire.h>
#include <Math.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#define CS 10
#define DC 9
#define RESET 8 
#define interval 4000

TFT Screen = TFT(CS, DC, RESET);
OneWire OW(2);                               // Setup a oneWire instance to communicate with any OneWire devices, connecting data wire into pin 2
DallasTemperature sensors(&OW);              // Pass our oneWire reference to Dallas Temperature

char temp, buf[70];
uint8_t vel, dist, dest, input, displayStr[10];
float diff, lat, lon, oldLat, oldLon;

const uint8_t GPSAddress = 0x42;      // GPS I2C Address
const char destText[4][15] = { "Kotkantie" , "Viehetie" , "Tirolintie" , "Santerinkuja" };
const float destPos[4][4] =
{
  { 64.999488 , 25.512225 },    // Kotkantie
  { 65.046135 , 25.483199 },    // Viehetie
  { 65.034385 , 25.462756 },    // Tirolintie       
  { 64.893637 , 25.564052 }     // Santerinkuja
};




float dataTransfer(int8_t *data_buf, int8_t num)   // Data type converter：convert int8_t type to float. *data_buf = int8_t data array, num = float length
{
  float T = 0;
  uint8_t i,j;

  i = data_buf[0] == '-'  ?  1  :  0;
  
  while (data_buf[i] != '.')    // The date in the array is converted to an integer and accumulative
    T = T * 10 + (data_buf[i++] - 0x30);
  for (j=0 ; j<num ; j++)
    T = T * 10 + (data_buf[++i] - 0x30);    
  for (j=0 ; j<num ; j++)       // The date will converted integer transform into a floating point number
    T /= 10;

  if (data_buf[0] == '-')       // Negative case
    T *= -1;                    // Converted to a negative number    

  return T;
}


void rec_init()   // Initial GPS
{
  Wire.beginTransmission(GPSAddress);
  Wire.write(0xff);   // To send data address      
  Wire.endTransmission(); 

  Wire.beginTransmission(GPSAddress);
  Wire.requestFrom(GPSAddress, 10);    // Require 10 bytes read from the GPS device
}


int8_t ID()       // Receive the statement ID
{
  int8_t i = 0;
  int8_t valueI[7] = { '$','G','P','G','G','A',',' };        // To receive the ID content of GPS statements
  int8_t buff[7] = { '0','0','0','0','0','0','0' };

  while (1)
  {
    rec_init();                      // Receive data initialization

    while ( Wire.available() )
    { 
      buff[i] = Wire.read();         // Receive serial data  

      if (buff[i] == valueI[i])      // Contrast the correct ID
      {
        if (++i == 7)
        {
          Wire.endTransmission();    // End of receiving
          return 1;                  // Receiving returns 1
        }
      }
      else
        i=0;
    }

    Wire.endTransmission();          // End receiving
  }
}


void rec_data(int8_t *buff, int8_t num1, int8_t num2)   // Receive data function. buff = Received data array, num1 = Number of commas, num2 = Length of the array
{
  int8_t i=0, count=0;

  if ( ID() )
    while (1)
    {
      rec_init();

      while ( Wire.available() )   
      { 
        buff[i] = Wire.read();

        if (count != num1)
        {  
          if (buff[i] == ',')
            count++;
        }

        else if (++i == num2)
          {
            Wire.endTransmission();
            return;
          }
      }

      Wire.endTransmission();
    }
}


float trueGPS(float x)
{
  return (x - floor(x/100) * 40.0) / 60;
}


float latitude()     // Latitude information
{
  int8_t lat[10] = { '0','0','0','0','0','0','0','0','0','0' };     // Store the latitude data
  rec_data(lat, 1, 10);      // Receive the latitude data
  return trueGPS( dataTransfer(lat, 5) );
}


float longitude()     // Longitude information
{
  int8_t lon[11] = { '0','0','0','0','0','0','0','0','0','0','0' };   // Store longitude data
  rec_data(lon, 3, 11);     // Receive the longitude data
  return trueGPS( dataTransfer(lon, 5) );
}


void setup()
{
  Wire.begin();               // IIC Initialize
  Serial.begin(9600);
  sensors.begin();            // Start up the library

  Screen.begin();  
  Screen.background(0,0,0);   // Clear screen
  Screen.stroke(255,255,255);

  // static text
  Screen.text("Current location", 0, 0);
  Screen.text("Distance to", 0, 30);
  Screen.text("Speed", 0, 60); 
  Screen.text("Temperature", 0, 90); 
  Screen.setTextSize(1);    // Increase font size for text in loop()
}


float laskeEtaisyys(float lat1, float lon1, float lat2, float lon2)
{
  return 6378.8 * ( 2.0 * asin(sqrt(square(sin(.5*(lat1-lat2)))+cos(lat1)*cos(lat2)*square(sin(.5*(lon2-lon1))))) );
}


void checkSerial()
{
  if ( Serial.available() )
  {
    input = Serial.parseInt();
    
    if (input > -1  &&  input < 4)
      dest = input; 
  }
}


void draw(double value, uint8_t precision, uint8_t X, uint8_t Y)
{
  String(value, precision).toCharArray(displayStr, precision);
  Screen.text(displayStr, X, Y);
}


void drawVariables(uint8_t R, uint8_t G, uint8_t B)
{
  Screen.stroke(R,G,B);

  draw(lat, 10, 0, 15);
  draw(lon, 10, 65, 15);
  draw(.1 * dist, 6, 0, 45);              // Saved distance is 10x the real value
  Screen.text("km", 35, 45);
  Screen.text(destText[dest], 74, 30);
  draw(.5 * vel, 5, 0, 75);               // Saved velocity is 2x the real value
  Screen.text("km/h", 40, 75);
  draw(.25 * temp, 5, 0, 105);            // Saved temperature is 4x the real value
  Screen.text("C", 40, 105);  
}


void printInfo()
{
  Serial.print("Current position: ");
  Serial.print(lat, 5);
  Serial.print(" , ");
  Serial.println(lon, 5);

  sprintf(buf, "Distance to %s: %d m, difference: %d cm.\n", destText[dest], uint16_t(1000 * dist), uint16_t(100000 * diff));
  Serial.println(buf);

  drawVariables(0, 255, 0);
  delay(500);
  drawVariables(0, 0, 0);
}


float rad(float X)
{
  return X * .017453292519;         // return X * pi / 180.0;
}


void loop()
{  
  while (1)
  {
    oldLat = lat;
    oldLon = lon;
    lat = latitude();
    lon = longitude();

    dist = 10 * laskeEtaisyys( rad(lat), rad(lon), rad(destPos[dest][0]), rad(destPos[dest][1]) ); // Save 10x the real distance
    diff = laskeEtaisyys( rad(lat), rad(lon), rad(oldLat), rad(oldLon) );
    vel = 7.2 * diff / interval;           // vel = 2 * 3.6  * diff / interval;         Save 2x the real velocity

    sensors.requestTemperatures();               // Issues a global temperature request to all devices on the bus
    temp = 4 * sensors.getTempCByIndex(0);           // Saving 4x the real temperature

    printInfo();
    checkSerial();

    while ( millis() % interval > 5 );
  }
}

