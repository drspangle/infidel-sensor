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
| calibration.ino | First test firmware by Thomas Sanladerer, it sends the ADC raw value over I2C, insert some drills of known diameters into the sensor and note the ADC value, then put the values into driver.ino and re compile |
| driver.ino | Code for the sensor, it uses a hardcoded table to convert the ADC to Diameter, and sends 2 bytes over I2C -- no analog out |
| host-example.ino | Reads  the 2 bytes of diameter over I2C, and displays it over the UART, used to test the sensor |
| Host_ee_prog.ino | Code for the Host Arduino, uses bidirectional communication with the sensor and has a simple UART Console to do the calibration and sensor testing |
| Infidel_release_ee.ino | Firmware for the sensor, table in EEPROM, sets values over I2C |

## Setup and Programming 

### Requirements
- Arduino > V1.8.8
- TinyWireS LIbary from https://github.com/nadavmatalon/TinyWireS
- ATtiny85 Board for Arduino from https://github.com/damellis/attiny
- ISP Programmer like Arduino over ISP --> https://www.arduino.cc/en/Tutorial/BuiltInExamples/ArduinoISP
 
Load the code, wire the ISP program to the ISP port on the sensor and program it.

Remember to set the processor to Attiny85.

Tools --> Processor: Attiny85

Tools --> Clock: Internal 1 MHz

Tools --> Burn bootloader to set the fuses


Sketch --> Upload with Programmer

After programming, the LED should flash two times.

## Wiring to the Host Board

![Alt text](host_to_sensor_arduino.PNG?raw=true "Wire Diagram")

Connect the the sensor to the host board, such as an Arduino Uno or Mega.

Program the Host with Host_ee_prog.ino and start the console with 19200 baudrate.

## Console

On start, the following is shown:
```sh
Version: 1.11
Table [ADC] [DIA in um]:
00: 0001 / 2999
01: 0617 / 2092
02: 0722 / 1711
03: 0816 / 1401
04: 0999 / 1001
05: 1022 / 0001
Command Input (0 - val / 1 - RAW val / 2 - Version / 3 - Table / 4 - Set Table Val / 5 - Ongoing raw read):
```

| Commands | Note |  Output |
| ------ | ------ |  ------ |
| 0 | Read the Diameter value  |  Diameter [mm]: 2.242 |
| 1 | Read the Diameter + raw ADC Value | Diameter [mm] / [ADC]: 2.242 / RAW: 515 |
| 2 | Read the Version | Version: 1.11 |
| 3 | Read the Diameter Table | Table [idx] [ADC] [DIA in um] |
| 4 | Set the Value in the Table | Input values for Table [IDX],[ADC],[DIA um] like (1,619,2090) |
| 5 | Ongoing reading the ADC raw Value, stop when the command 5 is sent one more time | 
| h | Show the command list |

## Calibration

Start with the bigger shaft of known diameter (e.g., 2 mm), insert it into the sensor and read the raw ADC value with command "2".
```sh
Diameter [mm] / [ADC]: 2.12 / RAW: 503
```
Note the ADC value and use the command "4".
The console should show:

```sh
Input values for Table [IDX],[ADC],[DIA um] like (1,619,2090)
Input: 
```

Input this string: `1,503,2000`
Means, Table Index 1 (Command "3"), ADC Val 503, Diameter 3

Repeat this for the next two Ddiameter (1,7mm, 1,4 mm) and write the values to the sensor.

At the end check the settings with Command "3".

```sh
Table [ADC] [DIA in um]:
00: 0001 / 2999
01: 0617 / 2092
02: 0722 / 1711
03: 0816 / 1401
04: 0999 / 1001
05: 1022 / 0001
```

The values are stored in the EEPROM and will load from the EEPROM at the next power up.

If you program the sensor with a new firmware over the ISP the EEPROM will be erased and the sensor will start with default settings.
 
## Analog Output
 
The sensor sends an analog signal to Pin 5 [OUT].

The range goes from 1.42 VDC to 2.14 VDC .
The voltage is the analog for the diameter: 1.73V is equal to 1.73mm diameter.


## Fault Pin

The fault pin is high when the diameter is bigger than 3mm and smaller than 1.5mm.
This indicates that the sensor is outside of the normal working range.
 
 
 
 


