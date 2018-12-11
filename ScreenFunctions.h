

#include <TFT.h>

#define TFTrst 8
#define TFTdc 9
#define TFTcs 10

TFT Screen = TFT(TFTcs, TFTdc, TFTrst);





void writeStatus(String str, uint8_t R = 255, uint8_t G = 255, uint8_t B = 255)
{
  Screen.fillScreen(0);
  Screen.stroke(R, G, B);
  uint8_t x = 80 - 3 * str.length();
  uint8_t displayStr[str.length() + 1];
  str.toCharArray(displayStr, str.length() + 1);
  Screen.text(displayStr, x, 60);
}


void draw(float value, uint8_t precision, uint8_t X, uint8_t Y)
{
  uint8_t displayStr[10];
  Screen.fillRect(X, Y, 5 * (precision + 1), 7, 0);
  String(value, precision).toCharArray(displayStr, precision);
  Screen.text(displayStr, X, Y);
}


void draw(String str, uint8_t X, uint8_t Y, uint8_t extraBlanking = 0)
{
  uint8_t displayStr[str.length() + 1];
  Screen.fillRect(X, Y, 5 * (str.length() + 1) + extraBlanking, 7, 0);
  str.toCharArray(displayStr, str.length() + 1);
  Screen.text(displayStr, X, Y);
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


void updateScreen(float dist, float vel, float temp)
{
  draw(lat, 10, 0, 12);
  draw(lon, 10, 65, 12);
  draw(destText[dest], 74, 25);
  draw(dist, 6, 0, 37);
  draw(vel, 4, 0, 62);
  draw(temp, 5, 0, 87);
  draw(counter, 4, 0, 112);
}

