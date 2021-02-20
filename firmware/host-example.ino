#include <Wire.h>

//I2C addresses for Infidel sensor
#define INFIDELADD 43

float infidelin = 0;

void setup() {
  Wire.begin();        // join i2c bus (address optional for host)
  Serial.begin(9600);  // start serial for output
}

void loop() {
  readInfidel();
  Serial.println(infidelin, 3);
  delay(1000);
}

void readInfidel() {
  Wire.requestFrom(INFIDELADD, 2);
  byte b1 = Wire.read();
  byte b2 = Wire.read();
  infidelin = (((float) b1) * 256 + b2) / 1000;
}
