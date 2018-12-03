

#include <TFT.h>

#define TFTrst 8
#define TFTdc 9
#define TFTcs 10

TFT Screen = TFT(TFTcs, TFTdc, TFTrst);





void drawTransfer()
{
  Screen.fillScreen(0);
  Screen.stroke(255, 255, 0);
  Screen.text("UPLOADING...", 48, 60);
}


void drawStaticText()
{
  Screen.fillScreen(0);

  Screen.stroke(255,255,255);
  Screen.text("Current location", 0, 0);
  Screen.text("Distance to", 0, 25);
  Screen.text("Speed", 0, 50);
  Screen.text("Temperature", 0, 75);
  Screen.text("Counter", 0, 100);

  Screen.stroke(0, 255, 0);  
  Screen.text("km", 35, 37);
  Screen.text("km/h", 30, 62);
  Screen.text("C", 30, 87);
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
  draw(lat, 10, 0, 12);
  draw(lon, 10, 65, 12);
  Screen.fillRect(74, 25, 65, 7, 0);
  Screen.text(destText[dest], 74, 25);
  draw(dist, 6, 0, 37);
  draw(vel, 4, 0, 62);
  draw(temp, 5, 0, 87);
  draw(counter, 4, 0, 112);
}


