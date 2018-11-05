

uint8_t led = 5;


void setup()
{
  pinMode(led, OUTPUT);     
}


void loop()
{
  digitalWrite(led, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(567);               // wait for a second
  digitalWrite(led, LOW);    // turn the LED off by making the voltage LOW
  delay(567);               // wait for a second
}
