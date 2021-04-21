/*  Firmware for the **49* (eg SS495) hall-sensor based filament diameter sensor.
    Reads analog value from the sensor and provides a mapped and filtered diameter reading over I2C (optional analog output)

    Built for ATTiny85

    Upload firmware via programmer, using internal 16 MHz clock on ATTiny

    Compact filament sensor hardware and PCB: [URL]

    Licensed CC-0 / Public Domain by Thomas Sanladerer

    EEPROM Update and Value Set over I2C by Michael Doppler (midopple)
    Standalone Calibration from midopple
*/

#include <TinyWireS.h>
#include <EEPROM.h>

//pins as named on PCB
#define FAULT_IO_LED 4 //on-board LED and digital output pin
#define A_IN A3 //SS495 output and push-button input (for calibration procedure)
#define A_OUT 1 //analog (filtered PWM) output, voltage proportional to filament diameter


//I2C
#define I2C_ADDR 43 //iterate for additional sensors in same I2C bus
//#define I2C_ADDR 44 //iterate for additional sensors in same I2C bus

#define smooth 100.0 //intensity of digital lowpass filtering

#define numtemps 6    //Tablecount 

#define VERSION_I2C 12  //I2C Protokoll Version
#define MAIN_VERSION 3

#define EE_CHKSUM_ADR  8

//I2C Commands from Host
#define I2C_CMD_VAL 0
#define I2C_CMD_RAWVAL 2
#define I2C_CMD_MEANVAL 5
#define I2C_CMD_OVERRIDE_DAC 7
#define I2C_CMD_VER 121
#define I2C_CMD_TABLE 131
#define I2C_CMD_SET_TAB 132


float dia = 1.7;
uint16_t  raw_ad_in = 0; //RAW ADC Value for IIC Transmit

uint8_t  I2C_akt_cmd = 0;

//Values for ADC Mean sampling 
uint16_t ADC_mean_min = 0;
uint16_t ADC_mean_max = 0;
uint8_t ADC_mean_cnt = 0;

//Variables for EEPROM handling
uint16_t ee_address = 0;
uint8_t  ee_value;

//Variables for Testing an Calibration DAC Output
uint8_t DAC_override_active = 0;
uint8_t DAC_override_value = 0;
uint16_t DAC_min_out_voltage = 1437;  //DAC out at PWM 0 --> 1,437 V
uint16_t DAC_max_out_voltage = 2156;  //DAC out at PWM 255 --> 2,156 V

int16_t dia_table[numtemps][2] = {
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
  pinMode(FAULT_IO_LED, OUTPUT);
  pinMode(A_IN, INPUT);
  pinMode(A_OUT, OUTPUT);

  //start I2C
  TinyWireS.begin(I2C_ADDR);
  TinyWireS.onRequest(requestISR);
  TinyWireS.onReceive(receiveISR);

  //blink to indicate sensor powered and ready
  digitalWrite(FAULT_IO_LED, HIGH);
  delay(400);
  digitalWrite(FAULT_IO_LED, LOW);
  delay(400);

  //Check EEPROM Chksum and when OK read Table from EEPROM, 
  //if CHKSUM is not OK write standard Values to EEPROM
  if(read_eeprom_chksum())
    read_eeprom_tab();
  else
    write_eeprom_tab();

  //blink to indicate sensor read EEPROM
  digitalWrite(FAULT_IO_LED, HIGH);
  delay(400);
  digitalWrite(FAULT_IO_LED, LOW);
  delay(400);

  //Check at startup if the Button is pressed to start Calibrazion
  check_for_calibrate();
  
}

void loop() {
  
  //Varibale for Analog Out Calculation
  int16_t aout_val;
  
  //get fresh reading
  int16_t in = analogRead(A_IN);
  

  //Store ADC Value for IIC Transmit
  raw_ad_in = in;
  
  //lowpass filter
  dia += (((float)convert2dia(in)/1000.0) - dia) / smooth;

  //Calculate Voltage for Analog Out --> Volate = Diameter --> 1,73 V = 1,73 mm
  int16_t help_dia_int = (int16_t)(dia*1000);
  aout_val = map(help_dia_int, DAC_min_out_voltage, DAC_max_out_voltage, 0, 255);
  aout_val = constrain(aout_val, 0, 255);
  
  //Write Value to Analog Out
  if(DAC_override_active)
    analogWrite(A_OUT, (uint8_t)(DAC_override_value));
  else
    analogWrite(A_OUT, (uint8_t)(aout_val));

  //light LED and pull up FAULT_IO_LED if sensor saturated, button pressed or diameter low
  if (in < 3 or dia < 1.5) {
    digitalWrite(FAULT_IO_LED, HIGH);
  }
  else {
    digitalWrite(FAULT_IO_LED, LOW);
  }
}


//Send Date over I2C to HOST
void requestISR() {
  
  //sends smoothed diameter reading in um as two bytes via I2C
  //first byte is upper 8 bits, second byte lower 8 bits
  int32_t senddia = 1000 * dia;
  int16_t ADC_help_val;
  uint8_t b1,b2,b3,b4,b5,b6;
  uint8_t *tab_ptr;

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
    //Messure Meanvalue for Calibration
    case I2C_CMD_MEANVAL:
      ADC_help_val = sample_AD_cal_val(100);
      b1 = floor(ADC_help_val / 256);
      b2 = (ADC_help_val % 256);
      b3 = floor(ADC_mean_min / 256);
      b4 = (ADC_mean_min % 256);
      b5 = floor(ADC_mean_max / 256);
      b6 = (ADC_mean_max % 256);

      TinyWireS.write(b1);
      TinyWireS.write(b2);
      TinyWireS.write(b3);
      TinyWireS.write(b4);
      TinyWireS.write(b5);
      TinyWireS.write(b6);
      TinyWireS.write(ADC_mean_cnt);
   break;
    //Send version 2 byte
    case I2C_CMD_VER:
      TinyWireS.write(MAIN_VERSION);
      TinyWireS.write(VERSION_I2C);
    break;
    //Response that DAC Override is active
    case I2C_CMD_OVERRIDE_DAC:
       TinyWireS.write(DAC_override_active);
    break;
    // Send Table 24 byte + 4byte DAC cal. Values
    case I2C_CMD_TABLE:
      tab_ptr = (byte*)&dia_table[0][0];
      for(uint8_t cnt_i=0;cnt_i < (numtemps*4);cnt_i++){
         TinyWireS.write(*tab_ptr++);
      }

      b1 = floor(DAC_min_out_voltage / 256);
      b2 = (DAC_min_out_voltage % 256);
      b3 = floor(DAC_max_out_voltage / 256);
      b4 = (DAC_max_out_voltage % 256);

      TinyWireS.write(b1);
      TinyWireS.write(b2);
      TinyWireS.write(b3);
      TinyWireS.write(b4);
    break;
    case I2C_CMD_SET_TAB:
       TinyWireS.write(1);
    break;
  }
}

//Receive Command from Host
void receiveISR(uint8_t num_bytes) {

  while(TinyWireS.available())
  {
    uint8_t b1,b2,b3,b4;
    uint8_t i2c_read_cmd = TinyWireS.read();
   
    switch(i2c_read_cmd){
      case I2C_CMD_VAL:
        I2C_akt_cmd = I2C_CMD_VAL;
        DAC_override_active = 0;
      break;

      case I2C_CMD_RAWVAL:
        I2C_akt_cmd = I2C_CMD_RAWVAL;
        DAC_override_active = 0;
      break;

      case I2C_CMD_MEANVAL:
        I2C_akt_cmd = I2C_CMD_MEANVAL;
        DAC_override_active = 0;
      break;

      case I2C_CMD_VER:
        I2C_akt_cmd = I2C_CMD_VER;
        DAC_override_active = 0;
      break;

      case I2C_CMD_TABLE:
        I2C_akt_cmd = I2C_CMD_TABLE;
        DAC_override_active = 0;
      break;

      case I2C_CMD_OVERRIDE_DAC:

        I2C_akt_cmd = I2C_CMD_OVERRIDE_DAC;
        
        b1 = TinyWireS.read();
        b2 = TinyWireS.read();

        if(b1 > 0) 
          DAC_override_active = 1;
        else
          DAC_override_active = 0;

        DAC_override_value = b2;

      break;
      case I2C_CMD_SET_TAB:
        
        I2C_akt_cmd = I2C_CMD_SET_TAB;
        
        uint8_t idx = TinyWireS.read();
        b1 = TinyWireS.read();
        b2 = TinyWireS.read();
        b3 = TinyWireS.read();
        b4 = TinyWireS.read();

        if(idx >= 0 && idx < 6){
          int16_t adc_val_ee = (int16_t)b1 * 256 + b2;
          int16_t dia_val_ee = (int16_t)b3 * 256 + b4;
          dia_table[idx][0] = adc_val_ee;
          dia_table[idx][1] = dia_val_ee;
        }

        //Set the cal Values for Analog Out, min and max Value
        if(idx == 9){
          DAC_min_out_voltage = (uint16_t)b1 * 256 + b2;
          DAC_max_out_voltage = (uint16_t)b3 * 256 + b4;
        }
        
        write_eeprom_tab();
      
      break;
      
    }
  }
}

//Convert AD Value to Diameter
int16_t convert2dia(int16_t in) {
  
  //converts an ADC reading to diameter
  //Inspired by Sprinter / Marlin thermistor reading
 
  
  uint8_t i;
  
  for (i = 1; i < numtemps; i++)
  {
    //check if we've found the appropriate row
    if (dia_table[i][0] > in)
    {
      float slope = ((float)dia_table[i][1] - dia_table[i - 1][1]) / ((float)dia_table[i][0] - dia_table[i - 1][0]);
      float indiff = ((float)in - dia_table[i - 1][0]);
      float outdiff = slope * indiff;
      int16_t out = (int16_t)outdiff + dia_table[i - 1][1];
      return (out);
      break;
    }
  }
}


//read Values for Table from EEPROM (24 byte)
void read_eeprom_tab(){
  uint8_t *tab_ptr;
  uint8_t b1,b2,b3,b4;

  ee_address = 10;

  tab_ptr = (uint8_t*)&dia_table[0][0];
  for(uint8_t cnt_i=0;cnt_i < (numtemps*4);cnt_i++){
    ee_value = EEPROM.read(ee_address);
    ee_address++;
    *tab_ptr = ee_value;
    tab_ptr++;
  }

  //Read the Calibartion Values for DAC
  b1 = EEPROM.read(ee_address++);
  b2 = EEPROM.read(ee_address++);
  b3 = EEPROM.read(ee_address++);
  b4 = EEPROM.read(ee_address++);

  DAC_min_out_voltage = (uint16_t)b1 * 256 + b2;
  DAC_max_out_voltage = (uint16_t)b3 * 256 + b4;

}

//Calculate Chksum from EEPROM
uint8_t  read_eeprom_chksum(){
  uint8_t ee_chksum = 0;
  uint8_t ee_check = 0;
  
  ee_check = EEPROM.read(EE_CHKSUM_ADR);
  ee_address = 10;

  for(uint8_t cnt_i=0;cnt_i < (numtemps*4);cnt_i++){
    ee_value = EEPROM.read(ee_address);
    ee_chksum ^= ee_value;
    ee_address++;
  }

  //Read the 4 byte for DAC values
  for(uint8_t cnt_i=0;cnt_i < 4;cnt_i++){
    ee_value = EEPROM.read(ee_address);
    ee_chksum ^= ee_value;
    ee_address++;
  }

  if(ee_check == ee_chksum)
    return(true);
  else
    return(false);
}

//write Values from Table to EEPROM (24 byte)
void write_eeprom_tab(){
  byte *tab_ptr;
  uint8_t ee_chksum = 0;
  uint8_t DAC_val_ee[4];

  ee_address = 10;
  
  tab_ptr = (byte*)&dia_table[0][0];
  
  for(byte cnt_i=0;cnt_i < (numtemps*4);cnt_i++){
    ee_value = (*tab_ptr++);
    ee_chksum ^= ee_value;
    EEPROM.write(ee_address, ee_value);
    ee_address++;
  }

  DAC_val_ee[0] = floor(DAC_min_out_voltage / 256);
  DAC_val_ee[1] = (DAC_min_out_voltage % 256);
  DAC_val_ee[2] = floor(DAC_max_out_voltage / 256);
  DAC_val_ee[3] = (DAC_max_out_voltage % 256);
  
  for(byte cnt_i=0;cnt_i < 4;cnt_i++){
    ee_value = DAC_val_ee[cnt_i];
    ee_chksum ^= ee_value;
    EEPROM.write(ee_address, ee_value);
    ee_address++;
  }
  
  EEPROM.write(EE_CHKSUM_ADR, ee_chksum);      //Check XOR Value
}


/********************************************************************
* **** Function for Standalone Calibrating                    *******
*********************************************************************/

//Check at Startup for Calibrate-mode
void check_for_calibrate(){
  
  //get fresh reading
  int16_t in = 0;
  in = analogRead(A_IN);

  if(in < 5){   //Button pressed
    //read 5 times more to filter
    for(uint8_t cnt_i = 0;cnt_i < 5;cnt_i++){
      in += analogRead(A_IN);
      delay(50);
    }

    if(in < 10)
      calibrate();
  }

}


//Flash the Led with the given count
void flash_led(uint8_t count){

  for(uint8_t cnt_i = 0;cnt_i < count;cnt_i++){
    digitalWrite(FAULT_IO_LED, HIGH);
    delay(120);
    digitalWrite(FAULT_IO_LED, LOW);
    delay(120);
  }
}

//Wait for press and relase the button
void wait_for_button_press(){
  int16_t in = 0;

  in = analogRead(A_IN);
  
  while(in > 20){
    in = analogRead(A_IN);
    digitalWrite(FAULT_IO_LED, HIGH);delay(80);digitalWrite(FAULT_IO_LED, LOW);delay(80);
 }
 
 while(in < 20){
   in = analogRead(A_IN);
   delay(10);
 }
}

//Sample AD Values with ppeak rejection for calibration
// When OK return the Meanvalue / when not OK Return 0
uint16_t sample_AD_cal_val(uint8_t count){
  int16_t  in = 0;
  uint32_t ADC_sum_samples = 0;
  int16_t  return_ADC_val = 0;
  int16_t  ADC_range_val_high = 0;
  int16_t  ADC_range_val_low = 0;
  uint8_t  sample_count = 0;

  ADC_mean_min = 1024;
  ADC_mean_max = 0;

  //First 10 Sample to get Mean Value
  for(uint8_t cnt_i = 0;cnt_i < 10;cnt_i++){
    in += analogRead(A_IN);
    delay(50);
  }

  //ADC Values should in the Range from +/- 10%, --> Check for Peaks
  ADC_range_val_high = (in / 9);
  ADC_range_val_low = (in / 11);

  digitalWrite(FAULT_IO_LED, HIGH);

  for(uint8_t cnt_i = 0;cnt_i < count;cnt_i++){
    in = analogRead(A_IN);
    //Filtert out Peak Values 
    if(in < ADC_range_val_high && in > ADC_range_val_low){
      sample_count++;
      ADC_sum_samples += in;

      if(in < ADC_mean_min) ADC_mean_min = in;
      if(in > ADC_mean_max) ADC_mean_max = in;
      
      delay(50);
    }
  }

  digitalWrite(FAULT_IO_LED, LOW);

  if(sample_count > (count / 2)){
    return_ADC_val = (int16_t)(ADC_sum_samples / sample_count);

    ADC_mean_cnt = sample_count;
    
    return(return_ADC_val);
  }
  else{
    return(0);
  }
  
}

/********************************************************************
* **** Calibrate Sensor with Button / Standalone Calibration *******
*********************************************************************/

#define CAL_STATE_START         0
#define CAL_STATE_DIA_1         1
#define CAL_STATE_WAIT_DIA_1    2
#define CAL_STATE_DIA_2         3
#define CAL_STATE_WAIT_DIA_2    4
#define CAL_STATE_DIA_3         5
#define CAL_STATE_WAIT_DIA_3    6
#define CAL_STATE_END           10

void calibrate(){
  
 /*  Self-calibration
  *  
  *  Press button, insert 1mm brill bit shaft, press to confirm reading
  *  Repeat for 1.4mm, 1.7mm, 2mm
  *  (optional: use interpolated value based on point between 1.5 and 2mm if 1.7mm if read with implausible value
  */

  uint8_t calibrate_state = CAL_STATE_START;
  uint16_t cal_sample_AD_val = 0;
  int16_t in = 0;

  //LED flash 10 times to show that calibrate is start
  flash_led(10);

  while(calibrate_state < CAL_STATE_END){


    //dia_table[numtemps][2] = {
    switch(calibrate_state){
      case CAL_STATE_START:
        in = analogRead(A_IN);
        if(in > 100)
          calibrate_state = CAL_STATE_WAIT_DIA_1;
      break;
      case CAL_STATE_WAIT_DIA_1:
        in = analogRead(A_IN);
        flash_led(1);delay(500);
        if(in < 20) calibrate_state = CAL_STATE_DIA_1;
      break;
      case CAL_STATE_DIA_1:
        in = analogRead(A_IN);
        while(in < 50){
          in = analogRead(A_IN);
        }
        
        //Sample Valuse für AD to DIA Table --> 100 Samples
        cal_sample_AD_val = sample_AD_cal_val(100);

        wait_for_button_press();
        
        //ADC Value for 1.4mm
        if(cal_sample_AD_val > 0) dia_table[3][0] = cal_sample_AD_val;
        calibrate_state = CAL_STATE_WAIT_DIA_2;
      break;
      case CAL_STATE_WAIT_DIA_2:
        in = analogRead(A_IN);
        flash_led(2);delay(500);
        if(in < 20) calibrate_state = CAL_STATE_DIA_2;
      break;
      case CAL_STATE_DIA_2:
        in = analogRead(A_IN);
        while(in < 50){
          in = analogRead(A_IN);
        }
        
        //Sample Valuse für AD to DIA Table --> 100 Samples
        cal_sample_AD_val = sample_AD_cal_val(100);

        wait_for_button_press();
        
        //ADC Value for 1.7mm
        if(cal_sample_AD_val > 0) dia_table[2][0] = cal_sample_AD_val;
        calibrate_state = CAL_STATE_WAIT_DIA_3;
      break;
      case CAL_STATE_WAIT_DIA_3:
        in = analogRead(A_IN);
        flash_led(3);delay(500);
        if(in < 20) calibrate_state = CAL_STATE_DIA_3;
      break;
      case CAL_STATE_DIA_3:
        in = analogRead(A_IN);
        while(in < 50){
          in = analogRead(A_IN);
        }
        
        //Sample Valuse für AD to DIA Table --> 100 Samples
        cal_sample_AD_val = sample_AD_cal_val(100);

        wait_for_button_press();
        
        //ADC Value for 2.0mm
        if(cal_sample_AD_val > 0) dia_table[1][0] = cal_sample_AD_val;
        write_eeprom_tab();
        calibrate_state = CAL_STATE_END;
      break;
      default:
      break;
    }
    delay(40);
  }

  
}