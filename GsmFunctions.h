

#define GPSAddress 0x42      // GPS I2C Address


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


void receiveData(int8_t *buff, int8_t num1, int8_t num2)   // buff = Received data array, num1 = Number of commas, num2 = Length of the array
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
