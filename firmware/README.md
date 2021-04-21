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
Infidel Sensor Programmer
Scanning...
I2C device found at address 43 
 
Version: 3.12
Table [ADC] [DIA in um]:
00: 0000 / 3000
01: 0619 / 2090
02: 0702 / 1700
03: 0817 / 1400
04: 1000 / 1000
05: 1023 / 0000
Table [DAC min Uout in uV] [DAC max Uout in uV]:
09: 1344 / 2017
Command Input 0 - val / 1 - RAW val / 2 - Version / 3 - Table / 4 - Set Tabel Val / 5 - Ongoing raw read / 6 - sample Mean ADC Val
Command Input 7 - DAC 0 PWW / 8 - DAC 255 PWM

```

| Commands | Note |  Output |
| ------ | ------ |  ------ |
| 0 | Read the Diameter value  |  Diameter [mm]: 2.242 |
| 1 | Read the Diameter + raw ADC Value | Diameter [mm] / [ADC]: 2.242 / RAW: 515 |
| 2 | Read the Version | Version: 1.11 |
| 3 | Read the Diameter Table | Table [idx] [ADC] [DIA in um] |
| 4 | Set the Value in the Table | Input values for Table [IDX],[ADC],[DIA um] like (1,619,2090) |
| 5 | Ongoing reading the ADC raw Value, stop when the command 5 is sent one more time | 
| 6 | Read Meanvalue from Sensor (100 Samples), Display Min / Max / Mean / cnt, used it for Calibration | ADC Mean: 704 / Min: 688 / Max: 713 / Cnt: 100 |
| 7 | Set DAC to PWM 0 --> for check Output Voltage at LOW |
| 8 | Set DAC to PWM 255 --> for check Output Voltage at HIGH |
| h | Show the command list |

## Calibration

Start with the bigger shaft of known diameter (e.g., 2 mm), insert it into the sensor and read the raw ADC value with command "6".
Command "6" determines the mean value over 100 measurements and removes the outliers

```sh
ADC Mean: 704 / Min: 688 / Max: 713 / Cnt: 100
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
 
## Calibration with Button (Standalone)

Press the Button at Powerup for 3 sec, if the Calibrationmode start the LED flashes 10 times
The sensor sends an analog signal to Pin 5 [OUT].

* Step 1, Led Flash 1 Times
	*   Insert Drill with 1,4mm
	*   Wait a short Time, 1-2 sec
	*   Press the Button for 1 sec
	*   The Led light for 2 sec, the Sensor is getting 100 Samples from the ADC 
	*   If the messure is Ok the Led flash fast
	*   Remove the drill an press the Button
*   Step 2 Led flashes 2 Times (1,7mm Drill)
    *   Insert the Drill 1,7mm and repeat Step 1
*   Step 3, Led flash 3 times (2mm Drill)
	*   Insert the Drill with 2mm and repeat Step 1

The Calibration is done

## Analog Output
 
The sensor sends an analog signal to Pin 5 [OUT].

The range goes from 1.42 VDC to 2.14 VDC .
The voltage is the analog for the diameter: 1.73V is equal to 1.73mm diameter.

## Calibrate the Analog Output

Connect a Multimeter to GND and OUT.
The Analog Output depend on the VCC Voltage, so make the Calibration when the Sensor is connected to the Printerboard
and not to the unstable USB Port.

* Set with the command "7" the PWM to LOW, meassure the Voltage on Analog OUT and note it (like 1,344 V)
* Set with the command "8" the PWM to HIGH, meassure the Voltage on Analog OUT and note it (like 2,017 V)

Set with the command "4" the table Value for Index 9 (Calibration Values for DAC)
IDX 9 then LOW Voltage and the HIGH Voltage --> like: 9,1344,2017

Check the table with command "3".
The Values are stored in the EEPROM for the next Start

## Fault Pin

The fault pin is high when the diameter is bigger than 3mm and smaller than 1.5mm.
This indicates that the sensor is outside of the normal working range.
 
 
 
 


