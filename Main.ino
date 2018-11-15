
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

#define CS   10
#define DC   9
#define RESET  8 
#define interval 5000

TFT myScreen = TFT(CS, DC, RESET);

char buf[70];
uint8_t dest, input, distTFT[10], lonTFT[10], latTFT[10], velTFT[10];
uint32_t timestamp;
double vel, dist, diff, lat, lon, latRad, lonRad, oldLatRad, oldLonRad;

const char destText[4][15] = { "Kotkantie" , "Viehetie" , "Tirolintie" , "Santerinkuja" };
const uint8_t GPSAddress = 0x42;      // GPS I2C Address
const double destCoords[4][4] =
{
  { 64.999488 , 25.512225   ,   1.134455078 , 0.445272326 },    // Koulu
  { 65.046135 , 25.483199   ,   1.135269221 , 0.444765726 },    // Viehetie
  { 65.034385 , 25.462756   ,   1.135064145 , 0.444408929 },    // Tirolintie       
  { 64.893637 , 25.564052   ,   1.132600000 , 0.446177000 }     // Santerinkuja
};




double dataTransfer(int8_t *data_buf, int8_t num)   // Data type converter：convert int8_t type to float. *data_buf = int8_t data array, num = float length
{
  double temp = 0;
  uint8_t i,j;

  i = data_buf[0] == '-'  ?  1  :  0;
  
  while (data_buf[i] != '.')   // The date in the array is converted to an integer and accumulative
    temp = temp * 10 + (data_buf[i++] - 0x30);
  for (j=0 ; j<num ; j++)
    temp = temp * 10 + (data_buf[++i] - 0x30);    
  for (j=0 ; j<num ; j++)    // The date will converted integer transform into a floating point number
    temp /= 10;

  if (data_buf[0] == '-') // Negative case
    temp *= -1;           // Converted to a negative number    

  return temp;
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
    rec_init();       // Receive data initialization    
    while ( Wire.available() )
    { 
      buff[i] = Wire.read();         // Receive serial data  

      if (buff[i] == valueI[i])      // Contrast the correct ID
      {
        if (++i == 7)
        {
          Wire.endTransmission();   // End of receiving
          return 1;                 // Receiving returns 1
        }
      }
      else
        i=0;
    }

    Wire.endTransmission();         // End receiving
  }
}


void rec_data(int8_t *buff, int8_t num1, int8_t num2)   // Receive data function
{                        //*buff：Receive data array；num1：Number of commas ；num2：The   length of the array
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


double trueGPS(double x)
{
  return (x - floor(x/100) * 40.0) / 60;
}


double latitude()     // Latitude information
{
  int8_t lat[10] = { '0','0','0','0','0','0','0','0','0','0' };     // Store the latitude data
  rec_data(lat, 1, 10);      // Receive the latitude data
  return trueGPS( dataTransfer(lat, 5) );
}


double longitude()     // Longitude information
{
  int8_t lon[11] = { '0','0','0','0','0','0','0','0','0','0','0' };   // Store longitude data
  rec_data(lon, 3, 11);     // Receive the longitude data
  return trueGPS( dataTransfer(lon, 5) );
}


void setup()
{
  Wire.begin();          // IIC Initialize
  Serial.begin(9600);

  myScreen.begin();  
  myScreen.background(0,0,0);   // Clear screen
  myScreen.stroke(255,255,255);

  // static text
  myScreen.text("Current location",0,0);
  myScreen.text("Distance to",0,30);
  myScreen.text("Speed",0,60); 
  myScreen.text("Temperature",0,90); 
  myScreen.setTextSize(1);    // Increase font size for text in loop()
}


double laskeEtaisyys(double latRad, double lonRad, double latDestRad, double lonDestRad)
{
    return 6378.8 * ( 2.0 * asin(sqrt(square(sin((latRad-latDestRad)/2.0))+cos(latRad)*cos(latDestRad)*square(sin((lonDestRad-lonRad)/2.0)))) );
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


void printInfo()
{
  Serial.print("Current position: ");
  Serial.print(lat, 5);
  Serial.print(" , ");
  Serial.println(lon, 5);

  sprintf(buf, "Distance to %s: %d m, difference: %d cm.\n", destText[dest], uint16_t(1000 * dist), uint16_t(100000 * diff));
  Serial.println(buf);

  String(lat, 10).toCharArray(latTFT, 10);
  String(lon, 10).toCharArray(lonTFT, 10);
  String(dist, 3).toCharArray(distTFT, 10);
  String(vel, 2).toCharArray(velTFT, 10);

/*
  String latStr = String(lat, 10);
  String lonStr = String(lon, 10);
  String distStr = String(dist, 3);
  String velStr = String(vel, 2);
  
  latStr.toCharArray(latTFT, 10);
  lonStr.toCharArray(lonTFT, 10);
  distStr.toCharArray(distTFT, 10);
  velStr.toCharArray(velTFT, 10);
*/

  myScreen.stroke(0,255,0);
  myScreen.text(latTFT,0,15);
  myScreen.text(lonTFT,65,15);
  myScreen.text(distTFT,0,45);
  myScreen.text("km",35,45);
  myScreen.text(destText[dest],74,30);
  myScreen.text(velTFT,0,75);
  myScreen.text("m/s",40,75); 

  delay(4500);

  myScreen.stroke(0,0,0);
  myScreen.text(latTFT,0,15);
  myScreen.text(lonTFT,65,15);
  myScreen.text(distTFT,0,45);
  myScreen.text("km",35,45);
  myScreen.text(destText[dest],74,30);
  myScreen.text(velTFT,0,75);
  myScreen.text("m/s",40,75);
}


double rad(double X)
{
  return X * 3.14159265359 / 180.0;
}


void loop()
{  
  while (1)
  {
    timestamp = millis();

    lat = latitude();
    lon = longitude();
    oldLatRad = latRad;
    oldLonRad = lonRad;
    latRad = rad(lat);
    lonRad = rad(lon);

    dist = laskeEtaisyys(latRad, lonRad, destCoords[dest][2], destCoords[dest][3]);
    diff = laskeEtaisyys(latRad, lonRad, oldLatRad, oldLonRad);
    vel = 1000 * diff / interval;

    printInfo();



    checkSerial();
    while (millis() - timestamp < interval);
  }
}

