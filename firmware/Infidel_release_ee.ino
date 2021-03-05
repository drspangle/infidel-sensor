/*  Firmware for the **49* (eg SS495) hall-sensor based filament diameter sensor.
    Reads analog value from the sensor and provides a mapped and filtered diameter reading over I2C (optional analog output)

    Built for ATTiny85

    Upload firmware via programmer, using internal 16 MHz clock on ATTiny

    Compact filament sensor hardware and PCB: [URL]

    Licensed CC-0 / Public Domain by Thomas Sanladerer

    EEPROM Update and Value Set over I2C by Michael Doppler (midopple)
*/

#include <TinyWireS.h>
#include <EEPROM.h>

//pins as named on PCB
#define FAULT 4 //on-board LED and digital output pin
#define A_IN A3 //SS495 output and push-button input (for calibration procedure)
#define A_OUT 1 //analog (filtered PWM) output, voltage proportional to filament diameter


//I2C
#define I2C_ADDR 43 //iterate for additional sensors in same I2C bus
//#define I2C_ADDR 44 //iterate for additional sensors in same I2C bus

#define smooth 100.0 //intensity of digital lowpass filtering
#define numtemps 6    //Tablecount 

#define VERSION_I2C 11  //I2C Protokoll Version

#define I2C_CMD_VAL 0
#define I2C_CMD_RAWVAL 2
#define I2C_CMD_VER 121
#define I2C_CMD_TABLE 131
#define I2C_CMD_SET_TAB 132


float dia = 1.7;
unsigned int raw_ad_in = 0; //RAW ADC Value for IIC Transmit
unsigned char I2C_akt_cmd = 0;

//Variables for EEPROM handling
int ee_address = 0;
byte ee_value;

short dia_table[numtemps][2] = {
  //{ADC reading in, diameter out [um]}
  //Init Values for the first start
  //After init Values are read from EEPROM
 
  { 0     , 3000 },  // safety
  { 619   , 2090 }, //2mm drill bit
  { 702   , 1700 }, //1.7mm
  { 817   , 1400 }, //1.4mm
  { 1000  , 1000 }, // 1mm
  { 1023  , 0000 } //safety
};



void setup() {
  //setup pins
  pinMode(FAULT, OUTPUT);
  pinMode(A_IN, INPUT);
  pinMode(A_OUT, OUTPUT);

  //start I2C
  TinyWireS.begin(I2C_ADDR);
  TinyWireS.onRequest(requestISR);
  TinyWireS.onReceive(receiveISR);

  //blink to indicate sensor powered and ready
  digitalWrite(FAULT, HIGH);
  delay(50);
  digitalWrite(FAULT, LOW);
  delay(50);

  //Read Table from EEPROM
  byte ee_check = EEPROM.read(8);
  if(ee_check = 0x2d)
    read_eeprom_tab();
  else
    write_eeprom_tab();

  //blink to indicate sensor powered and ready
  digitalWrite(FAULT, HIGH);
  delay(50);
  digitalWrite(FAULT, LOW);
  delay(50);
  
}

void loop() {
  
  int aout_val;
  
  //get fresh reading
  short in = analogRead(A_IN);
  

  //Store ADC Value for IIC Transmit
  raw_ad_in = in;
  
  //lowpass filter
  dia += (((float)convert2dia(in)/1000.0) - dia) / smooth;

  //Calculate Voltage for Analog Out --> Volate = Diameter --> 1,73 V = 1,73 mm
  int help_dia_int = dia*1000;
  aout_val = map(help_dia_int, 1420 , 2140, 0, 255);
  
  //Wreite Value to Analog Out
  analogWrite(A_OUT, (unsigned byte)(aout_val));

  //light LED and pull up FAULT if sensor saturated, button pressed or diameter low
  if (in < 3 or dia < 1.5) {
    digitalWrite(FAULT, HIGH);
  }
  else {
    digitalWrite(FAULT, LOW);
  }
}


//Send Date over I2C to HOST
void requestISR() {
  
  //sends smoothed diameter reading in um as two bytes via I2C
  //first byte is upper 8 bits, second byte lower 8 bits
  long senddia = 1000 * dia;
  byte b1,b2,b3,b4;
  byte *tab_ptr;

  switch(I2C_akt_cmd){
    //Send diameter 2 byte
    case I2C_CMD_VAL:
      b1 = floor(senddia / 256);
      b2 = (senddia % 256);
      
      TinyWireS.write(b1);
      TinyWireS.write(b2);
    break;
    //Send Diameter + RAW ADC for Calibrate 4 byte
    case I2C_CMD_RAWVAL:
      b1 = floor(senddia / 256);
      b2 = (senddia % 256);
      
      b3 = floor(raw_ad_in / 256);
      b4 = (raw_ad_in % 256);
      
      //Send Diameter
      TinyWireS.write(b1);
      TinyWireS.write(b2);
      //Send raw reading for calibration
      TinyWireS.write(b3);
      TinyWireS.write(b4);
    break;
    //Send version 2 byte
    case I2C_CMD_VER:
      TinyWireS.write(1);
      TinyWireS.write(VERSION_I2C);
    break;
    // Send Table 24 byte
    case I2C_CMD_TABLE:
    tab_ptr = (byte*)&dia_table[0][0];
    for(byte cnt_i=0;cnt_i < (numtemps*4);cnt_i++){
       TinyWireS.write(*tab_ptr++);
    }
    break;
  }
}

//Receive Command from Host
void receiveISR(uint8_t num_bytes) {

  while(TinyWireS.available())
  {
    byte b1,b2,b3,b4;
    byte i2c_read_cmd = TinyWireS.read();
   
    switch(i2c_read_cmd){
      case I2C_CMD_VAL:
        I2C_akt_cmd = I2C_CMD_VAL;
      break;

      case I2C_CMD_RAWVAL:
        I2C_akt_cmd = I2C_CMD_RAWVAL;
      break;

      case I2C_CMD_VER:
        I2C_akt_cmd = I2C_CMD_VER;
      break;

      case I2C_CMD_TABLE:
        I2C_akt_cmd = I2C_CMD_TABLE;
      break;

      case I2C_CMD_SET_TAB:
        byte idx = TinyWireS.read();
        b1 = TinyWireS.read();
        b2 = TinyWireS.read();
        b3 = TinyWireS.read();
        b4 = TinyWireS.read();

        if(idx >= 0 && idx < 6){
          short adc_val_ee = (short)b1 * 256 + b2;
          short dia_val_ee = (short)b3 * 256 + b4;
          dia_table[idx][0] = adc_val_ee;
          dia_table[idx][1] = dia_val_ee;
        }
        write_eeprom_tab();
      
      break;
      
    }
  }
}


short convert2dia(short in) {
  
  //converts an ADC reading to diameter
  //Inspired by Sprinter / Marlin thermistor reading
 
  
  byte i;
  
  for (i = 1; i < numtemps; i++)
  {
    //check if we've found the appropriate row
    if (dia_table[i][0] > in)
    {
      float slope = ((float)dia_table[i][1] - dia_table[i - 1][1]) / ((float)dia_table[i][0] - dia_table[i - 1][0]);
      float indiff = ((float)in - dia_table[i - 1][0]);
      float outdiff = slope * indiff;
      short out = (short)outdiff + dia_table[i - 1][1];
      return (out);
      break;
    }
  }
}


//read Values for Table from EEPROM (24 byte)
void read_eeprom_tab(){
  byte *tab_ptr;

  byte ee_check = 0;

  ee_check = EEPROM.read(8);

  if(ee_check == 0x2d){
    ee_address = 10;

    tab_ptr = (byte*)&dia_table[0][0];
    for(byte cnt_i=0;cnt_i < (numtemps*4);cnt_i++){
      ee_value = EEPROM.read(ee_address);
      ee_address++;
      *tab_ptr = ee_value;
      tab_ptr++;
    }
  }
}

//write Values from Table to EEPROM (24 byte)
void write_eeprom_tab(){
  byte *tab_ptr;

  ee_address = 10;
  
  tab_ptr = (byte*)&dia_table[0][0];
  for(byte cnt_i=0;cnt_i < (numtemps*4);cnt_i++){
       ee_value = (*tab_ptr++);
       EEPROM.write(ee_address, ee_value);
       ee_address++;
    }
  EEPROM.write(8, 0x2d);      //Check Value
}


void calibrate(){
 /*  TODO: Self-calibration
  *  Press button, insert 1mm brill bit shaft, press to confirm reading
  *  Repeat for 1.5mm, 1.7mm, 2mm
  *  (optional: use interpolated value based on point between 1.5 and 2mm if 1.7mm if read with implausible value
  */
}
