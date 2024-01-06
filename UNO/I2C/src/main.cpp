#include <Arduino.h>
#include <Wire.h>

const uint8_t MAX_COLUMN = 21;
uint8_t textpos = 0;
char line_buffer[MAX_COLUMN + 1];

void reset_line()
{
  memset(line_buffer, 0, MAX_COLUMN + 1);
  line_buffer[0] = '>';
  line_buffer[1] = ' ';
  line_buffer[MAX_COLUMN] = 0;
  textpos = 2;
}

uint8_t I2C_device_addr = 0x3C;

void setup()
{
  reset_line();
  Serial.begin(115200);
  Serial.println("Welcome");
  Serial.print("> ");
}

// Possible Wire error code for endTransmission()
//   0 .. success
//   1 .. length to long for buffer
//   2 .. address send, NACK received
//   3 .. data send, NACK received
//   4 .. other twi error (lost bus arbitration, bus error, ..)
//   5 .. timeout

void command(const char *c)
{
  if (strcmp(c, "connect") == 0)
  {
    // Init I2C com
    Wire.begin(I2C_device_addr);
    Wire.setClock(400000L);

    // Check display type
    Wire.beginTransmission(I2C_device_addr);
    uint8_t rc = Wire.endTransmission();
    if (rc != 0)
    {
      Serial.print("I2C write errcode: ");
      Serial.println(rc);
    }
    uint8_t u = 0;
    uint8_t nb = Wire.requestFrom(I2C_device_addr, (uint8_t)1);
    if (nb != 1)
    {
      Serial.print("I2C read nb: ");
      Serial.println(nb);
    }
    else
    {
      u = Wire.read();
    }

    Serial.print("Screen status register: 0x");
    Serial.print(u, 16);
    uint8_t on = u & 0x40;
    uint8_t id = u & 0x3F;
    Serial.print(" Device ID: ");
    Serial.print(id, 2);
    if (on == 0)
    {
      Serial.println(" is ON");
    }
    else
    {
      Serial.println(" is OFF");
    }
    Wire.beginTransmission(I2C_device_addr);
    Wire.write(0x00); // command start D/C bit is 0 (0000 0000)
    rc = Wire.write(0xA4);
    rc = Wire.endTransmission(true);

    if (rc != 0)
    {
      Serial.print("I2C end: ");
      Serial.println(rc);
    }
  }
  else if (strcmp(c, "on") == 0)
  {
    Wire.beginTransmission(I2C_device_addr);
    Wire.write(0x00); // command start D/C bit is 0 (0000 0000)
    uint8_t rc = Wire.write(0xAF);
    rc = Wire.endTransmission(true);

    if (rc != 0)
    {
      Serial.print("I2C end: ");
      Serial.println(rc);
    }
  }
  else if (strcmp(c, "off") == 0)
  {
    Wire.beginTransmission(I2C_device_addr);
    Wire.write(0x00); // command start D/C bit is 0 (0000 0000)
    uint8_t rc = Wire.write(0xAE);
    rc = Wire.endTransmission(true);

    if (rc != 0)
    {
      Serial.print("I2C end: ");
      Serial.println(rc);
    }
  }
  else if (strcmp(c, "normal") == 0)
  {
    Wire.beginTransmission(I2C_device_addr);
    Wire.write(0x00); // command start D/C bit is 0 (0000 0000)
    uint8_t rc = Wire.write(0xA6);
    rc = Wire.endTransmission(true);

    if (rc != 0)
    {
      Serial.print("I2C end: ");
      Serial.println(rc);
    }
  }
  else if (strcmp(c, "invert") == 0)
  {
    Wire.beginTransmission(I2C_device_addr);
    Wire.write(0x00); // command start D/C bit is 0 (0000 0000)
    uint8_t rc = Wire.write(0xA7);
    rc = Wire.endTransmission(true);

    if (rc != 0)
    {
      Serial.print("I2C end: ");
      Serial.println(rc);
    }
  }
  else if (strcmp(c, "clear") == 0)
  {
    Wire.beginTransmission(I2C_device_addr);
    Wire.write(0x00); // command start D/C bit is 0 (0000 0000)
    uint8_t rc = Wire.write(0xA7);
    rc = Wire.endTransmission(true);

    if (rc != 0)
    {
      Serial.print("I2C end: ");
      Serial.println(rc);
    }
  }
}

void loop()
{
  bool escape = false;
  uint8_t readchar = 0;

  while (Serial.available())
  {
    readchar++;
    char c = Serial.read();
    if (c == '\n' || c == '\r')
    {
      if (strlen(line_buffer) > 2)
      {
        command(line_buffer + 2);
      }
      reset_line();
    }
    else if (c == 27)
    { // escape
      escape = true;
      Serial.println("Start escape");
    }
    else if (c == '\b' || c == 127)
    { // backspace or delete
      textpos--;
      if (textpos < 2)
      {
        textpos = 2;
      }
      line_buffer[textpos] = ' ';
    }
    else if (c > 31 && c < 127)
    { // only ASCII
      if (!escape)
      {
        line_buffer[textpos] = c;
        textpos++;
        if (textpos == MAX_COLUMN)
        {
          Serial.println("Buffer overflow. Clear buffer.");
          reset_line();
        }
      }
    }
  }
}
