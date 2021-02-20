/*  Firmware for the **49* (eg SS495) hall-sensor based filament diameter sensor.
    Reads analog value from the sensor and provides a mapped and filtered diameter reading over I2C (optional analog output)

    Built for ATTiny85

    Upload firmware via programmer, using internal 16 MHz clock on ATTiny

    Compact filament sensor hardware and PCB: [URL]

    Licensed CC-0 / Public Domain by Thomas Sanladerer
*/

#include <TinyWireS.h>

//pins as named on PCB
#define FAULT 4 //on-board LED and digital output pin
#define A_IN A3 //SS495 output and push-button input (for calibration procedure)
#define A_OUT 1 //analog (filtered PWM) output, voltage proportional to filament diameter

// This firmware does not support analog output. rev 1 boards are populated without the lowpass filter on A_OUT.

//I2C
#define I2C_ADDR 43 //iterate for additional sensors in same I2C bus

#define smooth 100 //intensity of digital lowpass filtering

float dia = 1.7;

void setup() {
  //setup pins
  pinMode(FAULT, OUTPUT);
  pinMode(A_IN, INPUT);
  pinMode(A_OUT, OUTPUT);

  //start I2C
  TinyWireS.begin(I2C_ADDR);
  TinyWireS.onRequest(requestISR);

  //blink to indicate sensor powered and ready
  digitalWrite(FAULT, HIGH);
  delay(50);
  digitalWrite(FAULT, LOW);
  delay(50);
}

void loop() {
  //get fresh reading
  short in = analogRead(A_IN);

  //lowpass filter
  dia += (convert2dia(in) - dia) / smooth;

  //light LED and pull up FAULT if sensor saturated, button pressed or diameter low
  if (in < 3 or dia < 1.5) {
    digitalWrite(FAULT, HIGH);
  }
  else {
    digitalWrite(FAULT, LOW);
  }
}

void requestISR() {
  //sends smoothed diameter reading in um as two bytes via I2C
  //first byte is upper 8 bits, second byte lower 8 bits
  long senddia = 1000 * dia;
  byte b1 = floor(senddia / 256);
  byte b2 = (senddia % 256);
  TinyWireS.write(b1);
  TinyWireS.write(b2);
  //optional: Send raw reading for calibration
}

float convert2dia(float in) {
  //converts an ADC reading to diameter
  //Inspired by Sprinter / Marlin thermistor reading
  byte numtemps = 6;
  const float table[numtemps][2] = {
    //{ADC reading in, diameter out}

    //REPLACE THESE WITH YOUR OWN READINGS AND DRILL BIT DIAMETERS
    
    { 0  , 3 },  // safety
    { 619  , 2.09 }, //2mm drill bit
    { 702  , 1.70 }, //1.7mm
    { 817  , 1.40 }, //1.4mm
    { 1000  , 1 }, // 1mm
    { 1023  , 0 } //safety
  };
  byte i;
  float out;
  for (i = 1; i < numtemps; i++)
  {
    //check if we've found the appropriate row
    if (table[i][0] > in)
    {
      float slope = ((float)table[i][1] - table[i - 1][1]) / ((float)table[i][0] - table[i - 1][0]);
      float indiff = ((float)in - table[i - 1][0]);
      float outdiff = slope * indiff;
      float out = outdiff + table[i - 1][1];
      return (out);
      break;
    }
  }
}

void calibrate(){
 /*  TODO: Self-calibration
  *  Press button, insert 1mm brill bit shaft, press to confirm reading
  *  Repeat for 1.5mm, 1.7mm, 2mm
  *  (optional: use interpolated value based on point between 1.5 and 2mm if 1.7mm if read with implausible value
  */
}
