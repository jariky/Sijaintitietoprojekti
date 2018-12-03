

#include <SPI.h>
#include <Wire.h>
#include <Math.h>
#include <OneWire.h>
#include <SoftwareSerial.h>
#include <DallasTemperature.h>
#include "Arduino.h"

#define PF(x)                               pgm_read_float(x)       // Read a float from program memory
#define rad(x)                              x * .017453292519       // x * pi / 180;
#define getDist(lon1, lat1, lon2, lat2)     12757.6 * asin(  sqrt( square(sin(.5*(lat1-lat2))) + cos(lat1)*cos(lat2)*square(sin(.5*(lon2-lon1))) )  )

#define rxPin 2     // Wire this to TX pin of modem
#define txPin 3     // Wire this to RX pin of modem
#define tempPin 4
#define ledPin 5
#define switchPin 6
#define recSize 25

uint8_t dest, counter, recTemp[recSize];
uint16_t interval = 5000, recLon[recSize], recLat[recSize];
float lon, lat;
String response;

const char destText[4][15] = { "Kotkantie" , "Viehetie" , "Tirolintie" , "Santerinkuja" };
const PROGMEM float destLat[4] = { 64.999488 , 65.046135 , 65.034385 , 64.893637 };
const PROGMEM float destLon[4] = { 25.512225 , 25.483199 , 25.462756 , 25.564052 };

#include "GpsFunctions.h"
#include "ScreenFunctions.h"


OneWire OW(tempPin);                        // Setup a oneWire instance to communicate with any OneWire devices, connecting data wire into pin 4
DallasTemperature Sensors(&OW);             // Pass our oneWire reference to Dallas Temperature
SoftwareSerial modem(rxPin, txPin);         // We'll use a software serial interface to connect to modem


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

      if (response.indexOf("Dest = ") > -1)
      {
        dest = constrain(response[7] - 48, 0, 3);
        response.remove(0, 16);
        interval = constrain(1000 * response.toInt(), 3000, 16000);
      }
    }
}


void out(String cmd, String check = "OK", uint16_t timeout = 10000)
{
  delay(95);
  modem  <<  cmd  << "\n";
  response = "";
  uint32_t startTime = millis();


  while (response.indexOf(check) < 0  &&  millis() - startTime < timeout)
    listenModem();
}


void setup()
{
  Wire.begin();
  Serial.begin(38400);
  modem.begin(38400);         // Change this to the baudrate used by modem
  delay(1000);                // Let the module self-initialize

  out(F("AT+QICSGP=1,\"internet\""));
  pinMode(ledPin, OUTPUT);

  Sensors.begin();

  Screen.begin();
  drawStaticText();
}


void sendPayload()
{
  uint8_t row = 0;
  uint16_t value = 0;

  modem  <<  F("POST /~t7kyja01/Add.php HTTP/1.1\r\nHost: 193.167.100.74\r\nContent-Type: application/x-www-form-urlencoded\r\nContent-Length: 1500\r\n\r\nM=");

  while (1)
  {
    if (row)
      modem  <<  ":";

    for (uint8_t col=0 ; col<3 ; col++)
    {
      if (col)
        modem  <<  ',';

      switch(col)
      {
        case 0:   value = recLon[row];      break;
        case 1:   value = recLat[row];      break;
        case 2:   value = recTemp[row];     break;  
      }

      modem  <<  value;
    }

    if (!counter)
      return;      

    counter--;
    row++;
  }
}


void updateControls()
{
  String outData = "http://www.students.oamk.fi/~t7kyja01/ControlView.php";

  Serial  <<  "\nUpdating controls...";

  out("AT+QHTTPURL=" + String(outData.length()) + ",30", "CONNECT");
  out(outData);
  out("AT+QHTTPGET=30");
  out("AT+QHTTPREAD=30");

  Serial  <<  "\nCurrent destination is "  <<  dest  <<" ("  <<  destText[dest]  <<  "), current interval is "  <<  interval  <<  ".\n";
}


void upload()
{
  counter--;
  drawTransfer();
  digitalWrite(ledPin, 1);
  Serial  <<  "\n\nBeginning transfers ("  <<  counter + 1  <<  " entries total)...";

  uint32_t sendStartTime = millis();
  out(F("AT+QIOPEN=\"TCP\",\"193.167.100.74\",80"), F("CONNECT"));   // Start a TCP connection, port 80
  out(F("AT+QISEND"), F(">"));
  sendPayload();
  modem  <<  "\x1A";
  out(F("AT+QICLOSE"));

  Serial  <<  "\nTransfer completed in "  <<  millis() - sendStartTime  <<  " ms.";
  updateControls();
  digitalWrite(ledPin, 0);
  drawStaticText();
}


void loop()
{
  uint32_t timestamp = millis();

  if ( digitalRead(switchPin) )
  {  
    float oldLon = lon;
    float oldLat = lat;
    lon = getLon();     // 25.45
    lat = getLat();     // 65.00
  
    float dist = getDist( rad(lon), rad(lat), rad(PF(&destLon[dest])), rad(PF(&destLat[dest])) );
    float vel = 3.6 * getDist( rad(lon), rad(lat), rad(oldLon), rad(oldLat) ) / interval;
  
    Sensors.requestTemperatures();
    float temp = Sensors.getTempCByIndex(0);
  
    Serial  <<  "\n#"  <<  counter  <<  "\tCurrent position: "  <<  String(lat, 5)  <<  " , "  <<  String(lon, 5)  <<  "     Distance to "  <<  destText[dest]  <<  ": "  <<  dist  <<  " km     Temperature: "  <<  temp  <<  " C";
    updateScreen(dist, vel, temp);
  
    recLon [counter] = constrain( 354130.304389406 * (lon - 25.385982), 0, 65535 );
    recLat [counter] = constrain( 338224.213210034 * (lat - 64.889301), 0, 65535 );
    recTemp[counter] = constrain( 4 * (temp + 31.75), 0, 255 );
  
    if (++counter == recSize)
      upload();
  }

  else if (counter)
    upload();

  else
    Serial  <<  "\nWaiting...";

  listenModem(500, 1);

  while ( millis() - timestamp < interval );
}

