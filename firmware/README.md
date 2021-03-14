# Inline Filament Diameter Estimator, Lowcost (InFiDEL) - Firmware

<p xmlns:dct="http://purl.org/dc/terms/" xmlns:vcard="http://www.w3.org/2001/vcard-rdf/3.0#">
  <a rel="license"
     href="http://creativecommons.org/publicdomain/zero/1.0/">
    <img src="https://licensebuttons.net/p/zero/1.0/80x15.png" style="border-style: none;" alt="CC0" />
  </a>
  Originally created by Thomas Sanladerer
</p>

## Files
| File | Note |
| ------ | ------ |
| calibration.ino | First Testsoftware by Thomas Sanladerer, it send the ADC raw Value over I2C, add some Drill diameters to the Sensor and note the ADC Value, put the Values to the Code driver.ino and comile it new |
| driver.ino | Code for the Sensor, it use a fix Table to convert the ADC to Diameter, send 2 byte over I2C no Analog out |
| host-example.ino | Read over I2C the 2 byte of Diameter and Display it over the Uart, Test the Sensor |
| Host_ee_prog.ino | Code for the Host Arduino, communicate with the Sensor in two way an have a simple Uart Console to to the calibration and Sensor testing |
| Infidel_release_ee.ino | Firmware for the Sensor, Table in EE-Prom, Set Values over I2C |

## Setup and Programming 

### You need
- Arduino > V1.8.8
- TinyWireS LIbary from https://github.com/nadavmatalon/TinyWireS
- ATtiny85 Board for Arduino from https://github.com/damellis/attiny
- ISP Programmer like Arduino over ISP --> https://www.arduino.cc/en/Tutorial/BuiltInExamples/ArduinoISP
 
Load the Code, wire the ISP Programm to the ISP Port on the Sensor and Programm it
Set the Attiny85
Tools --> Prozessor: Attiny85
Tools --> Clock: Internal 1 Mhz
Tools --> Burn Bootloader to sett the Fuses

Sketch --> Upload with Programmer

After Programming the Led flashes 2 times

## Wire to the Hostboard

![Alt text](host_to_sensor_arduino.PNG?raw=true "Wire Disgramm")

Connect the the Sensor to the Hostboard like Arduino Uno or Mega

Programm the Host with Host_ee_prog.ino and start the Console with 19200 baud

## Console

At starting this Data are shown
```sh
Version: 1.11
Table [ADC] [DIA in um]:
00: 0001 / 2999
01: 0617 / 2092
02: 0722 / 1711
03: 0816 / 1401
04: 0999 / 1001
05: 1022 / 0001
Command Input (0 - val / 1 - RAW val / 2 - Version / 3 - Table / 4 - Set Tabel Val):
```

| Commands | Note |  Output |
| ------ | ------ |  ------ |
| 0 | Read the Diameter value  |  Diameter [mm]: 2.242 |
| 1 | Read the Diameter + raw ADC Value | Diameter [mm] / [ADC]: 2.242 / RAW: 515 |
| 2 | Read the Version | Version: 1.11 |
| 3 | Read the Diameter Table | Table [idx] [ADC] [DIA in um] |
| 4 | Set the Value in the Table | Input values for Table [IDX],[ADC],[DIA um] like (1,619,2090) |
| 5 | Ongoing reading the ADC rae Value, stop when the command 5 is send one more time |
| h | show the Commandlist |

## Calibration

Start with the bigger Drill like 2 mm, put it in the Sensor and read the raw ADC value with Command "2"
```sh
Diameter [mm] / [ADC]: 2.12 / RAW: 503
```
note the ADC Value and use the Command "4" 
The Console show 

```sh
Input values for Table [IDX],[ADC],[DIA um] like (1,619,2090)
Input: 
```

Input this String --> 1,503,2000
Means, Tabel Index 1 (Comnand "3"), ADC Val 503, Diameter 3

Repeat this for the next two Diameter (1,7mm, 1,4 mm) and write the Values to the Sensor

At the End check the Settings with Command 3

```sh
Table [ADC] [DIA in um]:
00: 0001 / 2999
01: 0617 / 2092
02: 0722 / 1711
03: 0816 / 1401
04: 0999 / 1001
05: 1022 / 0001
```

The Values are stored in the Epromm and will load from the Epromm at the next Power up

 If you Programm the Sensor with a new Firmware over the ISP the EEPROM will delete 
 and the Sensor start with standard Settings
 
 
 ## Analog Output
 
 The Sensor send an Analog Signal to Pin 5 [OUT]
 
 The range go from 1,42 VDC to 2,14 VDC 
 The Voltage ist the Diameter, 1,73V means 1,73mm
 
 
 ## Fault Pin
 
 The Fault Pin is high wehn the Diameter is bigger then 3mm and Smaler 1,5mm
 it show the the Sensor ist out of the working range.
 
 
 
 


