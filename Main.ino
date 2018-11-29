

#include <TFT.h>
#include <SPI.h>
#include <Wire.h>
#include <Math.h>
#include <OneWire.h>
#include "Arduino.h"
#include <SoftwareSerial.h>
#include <DallasTemperature.h>
#include "GpsFunctions.h"

#define PF(x)                               pgm_read_float(x)       // Read a float from program memory
#define rad(x)                              x * .017453292519       // x * pi / 180;
#define getDist(lon1, lat1, lon2, lat2)     12757.6 * asin(  sqrt( square(sin(.5*(lat1-lat2))) + cos(lat1)*cos(lat2)*square(sin(.5*(lon2-lon1))) )  )
#define trueGPS(x)                          (x - floor(x/100) * 40.0) / 60

#define rxPin 2     // Wire this to TX pin of modem
#define txPin 3     // Wire this to RX pin of modem
#define TFTrst 8
#define TFTdc 9
#define TFTcs 10
#define interval 5000
#define recSize 85

TFT Screen = TFT(TFTcs, TFTdc, TFTrst);
OneWire OW(4);                              // Setup a oneWire instance to communicate with any OneWire devices, connecting data wire into pin 4
DallasTemperature Sensors(&OW);             // Pass our oneWire reference to Dallas Temperature
SoftwareSerial modem(rxPin, txPin);         // We'll use a software serial interface to connect to modem

uint8_t dest, counter, recTemp[recSize];
uint16_t recLon[recSize], recLat[recSize];
float lon, lat;
String response;

const char destText[4][15] = { "Kotkantie" , "Viehetie" , "Tirolintie" , "Santerinkuja" };
const PROGMEM float destLat[4] = { 64.999488 , 65.046135 , 65.034385 , 64.893637 };
const PROGMEM float destLon[4] = { 25.512225 , 25.483199 , 25.462756 , 25.564052 };


template<class T> inline Print &operator <<(Print &obj, T arg)
{
  obj.print(arg);
  return obj;
}





bool listenModem(uint32_t timeOut = 50, bool echo = 0)
{
  uint32_t startTime = millis();

  while (millis() - startTime < timeOut)
    while ( modem.available() )
    {
      response = modem.readStringUntil('\n');

      if (echo)
        Serial  <<  "Modem: "  <<  response  <<  "\n";

      if (response.indexOf("ERROR") > -1)
        Serial  <<  "\nError found!";
    } 
}


void out(String cmd, String check = "OK", uint16_t timeout = 10000)
{
  delay(500);
  modem  <<  cmd  << "\n";
  response = "";
  uint32_t startTime = millis();


  while (response.indexOf(check) < 0  &&  millis() - startTime < timeout)
    listenModem();
}


float getLat()
{
  int8_t tempLat[10] = { '0','0','0','0','0','0','0','0','0','0' };     // Store the latitude data
  receiveData(tempLat, 1, 10);
  return trueGPS( dataTransfer(tempLat, 5) );
}


float getLon()
{
  int8_t tempLon[11] = { '0','0','0','0','0','0','0','0','0','0','0' };   // Store longitude data
  receiveData(tempLon, 3, 11);
  return trueGPS( dataTransfer(tempLon, 5) );
}


void setup()
{
  Wire.begin();
  Serial.begin(38400);
  modem.begin(38400);        // Change this to the baudrate used by modem
  delay(1000);              // Let the module self-initialize

  out(F("AT+QICSGP=1,\"internet\""));

  Sensors.begin();

  Screen.begin();
  Screen.fillScreen(0);

  Screen.stroke(255,255,255);
  Screen.text("Current location", 0, 0);
  Screen.text("Distance to", 0, 30);
  Screen.text("Speed", 0, 60);
  Screen.text("Temperature", 0, 90);

  Screen.stroke(0, 255, 0);  
  Screen.text("km", 35, 45);
  Screen.text("km/h", 40, 75);
  Screen.text("C", 40, 105);
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
  Screen.fillRect(74, 30, 65, 7, 0);
  Screen.text(destText[dest], 74, 30);
  draw(vel, 5, 0, 75);
  draw(temp, 5, 0, 105);
}


void createPayload()
{
  uint8_t index = 0;
  uint16_t value = 0;

  modem  <<  F("POST /~t7kyja01/Add.php HTTP/1.1\r\nHost: 193.167.100.74\r\nContent-Type: application/x-www-form-urlencoded\r\nContent-Length: 1500\r\n\r\nM=");

  while (index < recSize + 1)
  {
    if (index)
      modem  <<  ":";

    for (uint8_t i=0 ; i<3 ; i++)
    {
      switch(i)
      {
        case 0:   value = recLon[index];      break;
        case 1:   value = recLat[index];      break;
        case 2:   value = recTemp[index];     break;  
      }

      modem  <<  value;

      if (i < 2)
        modem  <<  ',';
    }

    if (!counter)
      return;      

    counter--;
    index++;
  }

  Serial  <<  "\nIndex: "  <<  index;
}


void upload()
{
  Serial  <<  "\n\nBeginning transfers ("  <<  counter + 1  <<  " entries total)...";

  uint32_t sendStartTime = millis();
  out(F("AT+QIOPEN=\"TCP\",\"193.167.100.74\",80"), F("CONNECT"));   // Start a TCP connection, port 80
  out(F("AT+QISEND"), F(">"));
  createPayload();
  modem  <<  "\x1A";
  out(F("AT+QICLOSE"));

  Serial  <<  "\nTransfer completed in "  <<  millis() - sendStartTime  <<  " ms.\n";
}


void loop()
{
  uint32_t timestamp = millis();

  float oldLon = lon;
  float oldLat = lat;
  lon = getLon();     // 25.45
  lat = getLat();     // 65.00

  float dist = getDist( rad(lon), rad(lat), rad(PF(&destLon[dest])), rad(PF(&destLat[dest])) );
  float vel = 3.6 * getDist( rad(lon), rad(lat), rad(oldLon), rad(oldLat) ) / interval;

  Sensors.requestTemperatures();
  float temp = Sensors.getTempCByIndex(0);

  //printUTC();

  Serial  <<  "\n#"  <<  counter  <<  "\tCurrent position: "  <<  String(lat, 5)  <<  " , "  <<  String(lon, 5)  <<  "     Distance to "  <<  destText[dest]  <<  ": "  <<  dist  <<  " km     Temperature: "  <<  temp  <<  " C";
  updateScreen(dist, vel, temp);

  recLon [counter] = constrain( 354130.304389406 * (lon - 25.385982), 0, 65535 );
  recLat [counter] = constrain( 338224.213210034 * (lat - 64.889301), 0, 65535 );
  recTemp[counter] = constrain( 4 * (temp + 31.75), 0, 255 );

  uint8_t input = 0;

  if ( Serial.available() )
    input = Serial.parseInt();


  if (input  ||  counter == recSize - 1)
    upload();
  else
    counter++;


  listenModem(1000, 1);
  while ( millis() - timestamp < interval );
}

