

uint8_t led = 18;


void setup()
{
  pinMode(led, OUTPUT);     
}


void loop()
{
  digitalWrite(led, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(1000);               // wait for a second
  digitalWrite(led, LOW);    // turn the LED off by making the voltage LOW
  delay(10);               // wait for a second
  
}
