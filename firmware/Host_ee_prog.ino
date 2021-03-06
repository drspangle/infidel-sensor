/*  Firmware for the Host for the SS495) hall-sensor based filament diameter sensor.
    Reads and send I2C and schow Values, 

    Built for Arduino Uno oder Mega

    Licensed CC-0 / Public Domain by Thomas Sanladerer

    by Michael Doppler (midopple)
*/


#include <Wire.h>

#define numtemps 6

#define SERIAL_BUFFER_SIZE 32

//I2C addresses for Infidel sensor
#define INFIDELADD 43
//#define INFIDELADD 44 //iterate for additional sensors in same I2C bus

#define I2C_CMD_VAL 0
#define I2C_CMD_RAWVAL 2
#define I2C_CMD_VER 121
#define I2C_CMD_TABLE 131
#define I2C_CMD_SET_TAB 132

float infidelin = 0;
float infidelin_raw = 0;

short dia_table[numtemps][2];

void setup() {
  Wire.begin();        // join i2c bus (address optional for host)
  Serial.begin(19200);  // start serial for output

  read_version();
  read_table();
  print_table();

  Serial.println(F("Command Input (0 - val / 1 - RAW val / 2 - Version / 3 - Table / 4 - Set Tabel Val / 5 - Ongoing raw read):"));
  
}

void loop() {

  static uint8_t ongoing_read = 0;
  static uint8_t ongoing_cnt = 0;

  if (Serial.available()) {
    byte inByte_uart = Serial.read();
    switch(inByte_uart){
      case '0':
        readInfidel();
        Serial.print(F("Diameter [mm]: "));
        Serial.println(infidelin, 3);
      break;
      case '1':
        readInfidel_raw();
        Serial.print(F("Diameter [mm] / [ADC]: "));
        Serial.print(infidelin, 3);
        Serial.print(F(" / RAW: "));
        Serial.println(infidelin_raw,0);
      break;
      case '2':
        read_version();
      break;
      case '3':
        read_table();
        print_table();
      break;
      case '4':
        inByte_uart = Serial.read(); //read linefeed from buffer
        get_value_from_uart();
      break;
      case '5':
        if(ongoing_read == 0)
          ongoing_read = 1;
        else
          ongoing_read = 0;
      break;
      case 10:
      break;
      case 13:
      break;
      default:
        Serial.println(F("Error wrong command"));
      break;
    }
  }
  else if(ongoing_read == 1){
    if(ongoing_cnt < 50){
      ongoing_cnt++;
    }
    else{
      ongoing_cnt = 0;
      readInfidel_raw();
      Serial.print(F("Diameter [mm] / [ADC]: "));
      Serial.print(infidelin, 3);
      Serial.print(F(" / RAW: "));
      Serial.println(infidelin_raw,0);
    }
  }
  
  delay(10);
}

void readInfidel() {

  Wire.beginTransmission(INFIDELADD); // transmit to device #44 (0x2c)
  Wire.write(I2C_CMD_VAL);             // sends value byte  
  Wire.endTransmission();     // stop transmitting
  
  Wire.requestFrom(INFIDELADD, 2);
  byte b1 = Wire.read();
  byte b2 = Wire.read();
  infidelin = (((float) b1) * 256 + b2) / 1000;
}

void readInfidel_raw() {
  Wire.beginTransmission(INFIDELADD); // transmit to device #44 (0x2c)
  Wire.write(I2C_CMD_RAWVAL);             // sends value byte  
  Wire.endTransmission();     // stop transmitting
  
  Wire.requestFrom(INFIDELADD, 4);
  byte b1 = Wire.read();
  byte b2 = Wire.read();
  byte b3 = Wire.read();
  byte b4 = Wire.read();
  infidelin = (((float) b1) * 256 + b2) / 1000;
  infidelin_raw = (((short) b3) * 256 + b4);
}


void read_version(){
  
  Wire.beginTransmission(INFIDELADD); // transmit to device #44 (0x2c)
  Wire.write(I2C_CMD_VER);             // sends value byte  
  Wire.endTransmission();     // stop transmitting

  delay(10);
  
  Wire.requestFrom(INFIDELADD, 2);

  byte b1 = Wire.read();
  byte b2 = Wire.read();

  Serial.print(F("Version: "));
  Serial.print(b1*1, DEC);
  Serial.print(F("."));
  Serial.println(b2*1,DEC);

  Wire.beginTransmission(INFIDELADD); // transmit to device #44 (0x2c)
  Wire.write(I2C_CMD_VAL);             // sends value byte  
  Wire.endTransmission();     // stop transmitting
}


void read_table(){

  byte *tab_ptr;
    
  Wire.beginTransmission(INFIDELADD); // transmit to device #44 (0x2c)
  Wire.write(I2C_CMD_TABLE);             // sends value byte  
  Wire.endTransmission();     // stop transmitting

  delay(10);
  
  Wire.requestFrom(INFIDELADD, 24);

  tab_ptr = (byte*)&dia_table[0][0];
  
  for(byte cnt_i = 0;cnt_i < (numtemps*4);cnt_i++){
    byte b1 = Wire.read();
    *tab_ptr = b1;
    tab_ptr++;
  }
  
  Wire.beginTransmission(INFIDELADD); // transmit to device #44 (0x2c)
  Wire.write(I2C_CMD_VAL);             // sends value byte  
  Wire.endTransmission();     // stop transmitting


}

void print_table(){

  Serial.println(F("Table [ADC] [DIA in um]:"));
  for(byte cnt_t=0;cnt_t < numtemps;cnt_t++){
    
    char s[20];
    sprintf(s,"%02u: %04u / %04u",cnt_t, dia_table[cnt_t][0], dia_table[cnt_t][1]); 
    Serial.println(s); 
  }
}


void get_value_from_uart(){

  Serial.println(F("Input values for Table [IDX],[ADC],[DIA um] like (1,619,2090)"));
  Serial.print(F("Input: "));

  byte uart_in = 0;
  byte index = 0;
  char serialBuffer[SERIAL_BUFFER_SIZE];

  index = 0;
  
  while(uart_in != '\n'){
    if(Serial.available()){
      uart_in = Serial.read();
      Serial.write(uart_in);    //ECHO ??
      if(uart_in >= 32 && index < SERIAL_BUFFER_SIZE - 1)
      {
        serialBuffer[index++] = uart_in;
      }
    }    
  }

  serialBuffer[index] = '\0';

  byte idx      = atoi(strtok(serialBuffer, ","));
  short ADC_val = atoi(strtok(NULL, ","));
  short dia_val = atoi(strtok(NULL, ","));
  byte b1,b2,b3,b4;

  Serial.print(F("read: "));
  Serial.print(idx, DEC);
  Serial.print(F(" / "));
  Serial.print(ADC_val,DEC);
  Serial.print(F(" / "));
  Serial.println(dia_val,DEC);

  if(idx >= 0 && idx < numtemps){
    if(ADC_val >= 0 && ADC_val < 1024 && dia_val >= 0 && dia_val <= 3000)
    {
      b1 = floor(ADC_val / 256);
      b2 = (ADC_val % 256);
      b3 = floor(dia_val / 256);
      b4 = (dia_val % 256);
      
      Wire.beginTransmission(INFIDELADD); // transmit to device #44 (0x2c)
      Wire.write(I2C_CMD_SET_TAB);             // sends value byte  
      Wire.write(idx);             // sends value byte  
      Wire.write(b1);             // sends value byte  
      Wire.write(b2);             // sends value byte  
      Wire.write(b3);             // sends value byte  
      Wire.write(b4);             // sends value byte  
      Wire.endTransmission();     // stop transmitting

      Serial.println(F("Set Value for Table"));
    }
    else{
      Serial.println(F("Wrong Value for ADC or DIA"));
    }
  }
  else{
    Serial.println(F("Wrong Value for IDX 0-5"));
  }
  
}
  
