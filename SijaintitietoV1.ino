

#include "Arduino.h"
#include <Wire.h>
#include <Math.h>

#define PI 3.14159265359 

const uint8_t GPSAddress = 0x42;      // GPS I2C Address

//const double latDestRad = 1.134666018; // Kauppakeskus Valkea
//const double lonDestRad = 0.444584509; // Kauppakeskus Valkea
const double latDestRad = 1.13442141; // Teboil Kaukovainio
const double lonDestRad = 0.445300251; // Teboil Kaukovainio

double value, conv, lat, lon;


double dataTransfer(int8_t *data_buf, int8_t num)   // Data type converter：convert int8_t type to float
{                                           //*data_buf:int8_t data array ;num:float length
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


//  conv = floor(value/100) * (1 - 1/.6) + value/60;


double latitude()     // Latitude information
{
  int8_t lat[10] = { '0','0','0','0','0','0','0','0','0','0' };     // Store the latitude data
  rec_data(lat, 1 ,10);      // Receive the latitude data

  value = dataTransfer(lat, 5);
  conv = floor(value/100) * 4/-6.0 + value/60;
  
  Serial.print("Latitude:\traw = ");
  Serial.print(value, 8);
  Serial.print("\tconverted = ");
  Serial.print(conv, 8);
  
  return conv;
}


double longitude()     // Longitude information
{
  int8_t lon[11] = { '0','0','0','0','0','0','0','0','0','0','0' };   // Store longitude data
  rec_data(lon, 3, 11);     // Receive the longitude data

  value = dataTransfer(lon, 5);
  conv = floor(value/100) * 4/-6.0 + value/60;
  
  Serial.println();
  Serial.print("Longitude:\traw = ");
  Serial.print(value, 8);
  Serial.print("\tconverted = ");
  Serial.println(conv, 8);

  return conv;
}


void setup()
{
  Wire.begin();          // IIC Initialize
  Serial.begin(9600);
}


double laskeEtaisyys(double latRad, double lonRad, double latDestRad, double lonDestRad)
{
 // return 6378.8*acos((sin(latRad)*sin(latDestRad))+cos(latRad)*cos(latDestRad)*cos(lonDestRad - lonRad));
    const double two=2.0;
    return 6378.8*(two*asin(sqrt(square(sin((latRad-latDestRad)/two))+cos(latRad)*cos(latDestRad)*square(sin((lonDestRad-lonRad)/two)))));
}

void loop()
{
  while (1)
  {
    lat = latitude();
    double latRad = lat * PI / 180.0;
    lon = longitude();
    double lonRad = lon * PI / 180.0;
    double distance = laskeEtaisyys(latRad,lonRad,latDestRad,lonDestRad);
    //float distance = 6378.8*acos((sin(latRad)*sin(latDestRad))+cos(latRad)*cos(latDestRad)*cos(lonDestRad - lonRad));
    Serial.print("Distance to the destination is: ");
    Serial.print(distance,3);
    Serial.print(" km");
    Serial.println();

    //char buf[100];
    //sprintf(buf, "distance = %f km.", distance);
    //Serial.println(buf);
  }
}
