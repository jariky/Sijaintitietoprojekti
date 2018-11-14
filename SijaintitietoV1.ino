

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

char buf[90]; 
uint8_t input, naytolle[10], longitude2[10], latitude1[10], dest;
uint32_t timestamp;
double distance, value, conv, lat, lon, latRad, lonRad, oldLatRad, oldLonRad, diff;

const uint8_t GPSAddress = 0x42;      // GPS I2C Address
const char destText[4][30] = { "Kotkantie" , "Viehetie" , "Tirolintie" , "Santerinkuja" };
const double destCoords[4][4] =
{
  { 64.999488 , 25.512225   ,   1.134455078 , 0.445272326 },    // Koulu
  { 65.011574 , 25.472816   ,   1.134666018 , 0.444584509 },    // Valkea
  { 12.124355 , 12.234465   ,   1.123545 , 1.12355 },
  { 12.124355 , 12.234465   ,   1.123545 , 1.12355 }
};




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
  
  return conv;
}


double longitude()     // Longitude information
{
  int8_t lon[11] = { '0','0','0','0','0','0','0','0','0','0','0' };   // Store longitude data
  rec_data(lon, 3, 11);     // Receive the longitude data

  value = dataTransfer(lon, 5);
  conv = floor(value/100) * 4/-6.0 + value/60;
  
  return conv;
}


void setup()
{
  Wire.begin();          // IIC Initialize
  Serial.begin(9600);

  myScreen.begin();  
  myScreen.background(0,0,0); // clear the screen
  myScreen.stroke(255,255,255);

  // static text
  myScreen.text("Current location",0,0);
  myScreen.text("Sijaintitieto-",75,25);
  myScreen.text("projekti!!",75,35);
  myScreen.text("Distance to destination",0,60); 
   
  myScreen.setTextSize(1);    // increase font size for text in loop()
}


double laskeEtaisyys(double latRad, double lonRad, double latDestRad, double lonDestRad)
{
    return 6378.8 * ( 2.0 * asin(sqrt(square(sin((latRad-latDestRad)/2.0))+cos(latRad)*cos(latDestRad)*square(sin((lonDestRad-lonRad)/2.0)))) );
}


double rad(double X)
{
  return X * 3.14159265359 / 180.0;
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

  sprintf(buf, "Distance to %s: %d m, difference: %d cm.\n", destText[dest], uint16_t(1000 * distance), uint16_t(100000 * diff));
  Serial.println(buf);
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

    distance = laskeEtaisyys(latRad,lonRad,destCoords[dest][2],destCoords[dest][3]);
    diff = laskeEtaisyys(latRad, lonRad, oldLatRad, oldLonRad);
    // velocity = 1000 * diff / interval;

    printInfo();

    String lati = String(lat, 10);
    String longi = String(lon, 10);
    String lukema = String(distance, 3);
    lati.toCharArray(latitude1, 10);
    longi.toCharArray(longitude2, 10);
    lukema.toCharArray(naytolle, 10);

    myScreen.stroke(0,255,0);
    myScreen.text(latitude1,0,20);
    myScreen.text(longitude2,0,40);
    myScreen.text(naytolle,0,80);
    myScreen.text("km",55,80);
    delay(1000);
    myScreen.stroke(0,0,0);
    myScreen.text(latitude1,0,20);
    myScreen.text(longitude2,0,40);
    myScreen.text(naytolle,0,80);
    myScreen.text("km",55,80);

    checkSerial();    
    while (millis() - timestamp < interval);
  }
}

