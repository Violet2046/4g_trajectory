                                                                                                                                      [AK09911]

                                           AK09911
3-axis Electronic Compass

  A 3-axis electronic compass IC with high sensitive Hall sensor technology.
  Best adapted to pedestrian city navigation use for cell phone and other portable appliance.

1. Features

  Functions:

  3-axis magnetometer device suitable for compass application
  Built-in A to D Converter for magnetometer data out
  14-bit data out for each 3-axis magnetic component

  Sensitivity: 0.6 µT/LSB (typ.)

  Serial interface



I2C bus interface
Standard, Fast and High-speed mode (up to 2.5 MHz) compliant with Philips I2C specification Ver.2.1

  Operation mode

  Power-down, Single measurement, Continuous measurement, Self-test and Fuse ROM access

  DRDY function for measurement data ready
  Magnetic sensor overflow monitor function
  Built-in oscillator for internal clock source
  Power on Reset circuit
  Self test function with internal magnetic source

  Operating temperatures:



                                                          -30˚C to +85˚C

  Operating supply voltage:

  Analog power supply
  Digital Interface supply

+2.4V to +3.6V
+1.65V to analog power supply voltage

  Current consumption:

  Power-down:
  Measurement:

3 µA (typ.)

  Average current consumption at 100 Hz repetition rate: 2.4 mA (typ.)

  Package:

  AK09911C      8-pin WL-CSP (BGA):

1.2 mm × 1.2 mm × 0.5 mm (typ.)

MS1526-E-01

                                            - 1 -

          2014/7

                                                                                                                                      [AK09911]

2. Overview

AK09911 is 3-axis electronic compass IC with high sensitive Hall sensor technology.
Small package of AK09911 incorporates magnetic sensors for detecting terrestrial magnetism in the X-axis, Y-axis, and
Z-axis, a sensor driving circuit, signal amplifier chain, and an arithmetic circuit for processing the signal from each sensor.
Self test function is also incorporated. From its compact foot print and thin package feature, it is suitable for map heading
up purpose in GPS-equipped cell phone to realize pedestrian navigation function.

AK09911 has the following features:

(1)  Silicon monolithic Hall-effect magnetic sensor with magnetic concentrator realizes 3-axis magnetometer on a
silicon chip. Analog circuit, digital logic, power block and interface block are also integrated on a chip.

(2)  Wide dynamic measurement range and high resolution with lower current consumption.

  Output data resolution:
  Measurement range:
  Average current at 100 Hz repetition rate:

14-bit (0.6 µT/LSB)
±4900 µT
2.4 mA (typ.)

(3)  Digital serial interface

  I2C bus interface to control AK09911 functions and to read out the measured data by external CPU. A

dedicated power supply for I2C bus interface can work in low-voltage apply as low as 1.65V.
(4)  DRDY register informs to system that measurement is end and set of data in registers are ready to be read.
(5)  Device is worked by on-chip oscillator so no external clock source is necessary.
(6)  Self test function with internal magnetic source to confirm magnetic sensor operation on end products.

MS1526-E-01

                                            - 2 -

          2014/7

                                                                                                                                      [AK09911]

3. Table of Contents

1. Features .............................................................................................................................. 1
2. Overview .............................................................................................................................. 2
3. Table of Contents ................................................................................................................. 3
4. Circuit Configration .............................................................................................................. 5
4.1. Block Diagram .............................................................................................................. 5
4.2. Block Function .............................................................................................................. 5
4.3. Pin Function .................................................................................................................. 6
5. Overall Characteristics ........................................................................................................ 7
5.1. Absolute Maximum Ratings .......................................................................................... 7
5.2. Recommended Operating Conditions .......................................................................... 7
5.3. Electrical Characteristics .............................................................................................. 7
5.3.1. DC Characteristics ................................................................................................. 7
5.3.2. AC Characteristics ................................................................................................. 8
5.3.3. Analog Circuit Characteristics ................................................................................ 9
5.3.4. I2C Bus Interface .................................................................................................. 10
6. Function Explanation ......................................................................................................... 13
6.1. Power States ............................................................................................................... 13
6.2. Reset Functions .......................................................................................................... 13
6.3. Operation Mode .......................................................................................................... 14
6.4. Description of Each Operation Mode ......................................................................... 15
6.4.1. Power-down Mode ............................................................................................... 15
6.4.2. Single Measurement Mode .................................................................................. 15
6.4.3. Continuous Measurement Mode 1, 2, 3 and 4 .................................................... 16
6.4.4. Self-test Mode ...................................................................................................... 19
6.4.5. Fuse ROM Access Mode ..................................................................................... 19
7. Serial Interface .................................................................................................................. 20
7.1. Data Transfer .............................................................................................................. 20
7.1.1. Change of Data .................................................................................................... 20
7.1.2. Start/Stop Condition ............................................................................................. 20
7.1.3. Acknowledge ........................................................................................................ 21
7.1.4. Slave Address ...................................................................................................... 21
7.2. WRITE Instruction ....................................................................................................... 22
7.3. READ Instruction ........................................................................................................ 23
7.3.1. One Byte READ ................................................................................................... 23
7.3.2. Multiple Byte READ ............................................................................................. 23
7.4. High-speed Mode (Hs-mode) ..................................................................................... 24
8. Registers ........................................................................................................................... 25
8.1. Description of Registers ............................................................................................. 25
8.2. Register Map .............................................................................................................. 26
8.3. Detailed of Description of Register ............................................................................. 27
8.3.1. WIA: Who I Am ..................................................................................................... 27
8.3.2. INFO: Information ................................................................................................ 27
8.3.3. ST1: Status 1 ........................................................................................................ 27
8.3.4. HXL to HZH: Measurement data .......................................................................... 28
8.3.5. TMPS: Dummy Register ...................................................................................... 28
8.3.6. ST2: Status 2 ........................................................................................................ 29
8.3.7. CNTL1: Dummy Register ..................................................................................... 29
8.3.8. CNTL2: Control 2 ................................................................................................. 29
8.3.9. CNTL3: Control 3 ................................................................................................. 30
8.3.10. TS1: Test ............................................................................................................ 30
8.3.11. ASAX, ASAY, ASAZ: Sensitivity Adjustment Values ........................................... 30
9. Example of Recommended External Connection ............................................................. 31
10. Package ........................................................................................................................... 32
10.1. Marking ..................................................................................................................... 32
10.2. Pin Assignment ......................................................................................................... 32
10.3. Outline Dimensions .................................................................................................. 33

MS1526-E-01

                                            - 3 -

          2014/7

                                                                                                                                      [AK09911]

10.4. Recommended Foot Print Pattern ............................................................................ 33
11. Relationsip between the Magnetic Field and Output Code ............................................. 34

MS1526-E-01

                                            - 4 -

          2014/7

                                                                                                                                      [AK09911]

4. Circuit Configration

4.1.   Block Diagram

3-axis
Hall
sensor

MUX

Chopper
    SW

Pre-
    AMP

Integrator＆ADC

HE-Drive

Magnetic source

VREF

OSC

Timing
Control

Interface Logic
& Register

SCL

SDA

RSTN

POR

FUSE ROM

TST

CAD

VSS

VDD

VID

4.2.   Block Function

Block

3-axis Hall sensor
MUX
Chopper SW
HE-Drive
Pre-AMP
Intergrator & ADC

OSC
POR
VREF
Interface Logic
&
Register

Timing Control

Magnetic Source
FUSE ROM

Function

Monolithic Hall elements.
Multiplexer for selecting Hall elements.
Performs chopping.
Magnetic sensor drive circuit for constant-current driving of sensor.
Fixed-gain differential amplifier used to amplify the magnetic sensor signal.
Integrates and amplifies pre-AMP output and performs analog-to-digital
conversion.
Generates an operating clock for sensor measurement.
Power On Reset circuit. Generates reset signal on rising edge of VDD.
Generates reference voltage and current.
Exchanges data with an external CPU.
I2C bus interface using two pins, namely, SCL and SDA. Standard, Fast and
High-speed modes are supported. The low-voltage specification can be supported
by applying 1.65V to the VID pin.
Generates a timing signal required for internal operation from a clock generated by
the OSC.
Generates magnetic field for self test of magnetic sensor.
Fuse for adjustment.

MS1526-E-01

                                            - 5 -

          2014/7

                                                                                                                                      [AK09911]

4.3.   Pin Function
Pin name

Pin No.

A1
A2

A3

B1
B3

C1
C2

C3

I/O

-
I

Power
supply
-
VDD

Type

Power
CMOS

VDD
CAD

TST

I/O

VDD

CMOS

VSS
SCL

VID
RSTN

-
I

-
I

SDA

I/O

-
VID

-
VID

VID

Power
CMOS

Power
CMOS

CMOS

Function

Positive power supply pin.
Slave address input pin.
Connect to VSS or VDD,
Test pin.
Pulled down by 100kΩ internal resister. Keep this pin
electrically non-connected.
Ground pin.
Control data clock input pin
Input: Schmidt trigger
Digital interface positive power supply pin.
Reset pin.
Resets registers by setting to “L”.
Control data input/output pin
Input: Schmidt trigger, Output: Open drain

MS1526-E-01

                                            - 6 -

          2014/7

                                                                                                                                      [AK09911]

5. Overall Characteristics

5.1.   Absolute Maximum Ratings

Vss=0V

Parameter
Power supply voltage
(Vdd, Vid)
Input voltage
Input current
Storage temperature

Symbol
V+

VIN
IIN
Tst

Min.
-0.3

-0.3
-
-40

Max.
+4.3

（V+）+0.3
±10
+125

Unit
V

V
mA
˚C

(Note 1)

If the device is used in conditions exceeding these values, the device may be destroyed. Normal operations are
not guaranteed in such exceeding conditions.

5.2.   Recommended Operating Conditions

Vss=0V

Parameter
Operating temperature
Power supply voltage

Remark

VDD pin voltage
VID pin voltage

Symbol
Ta
Vdd
Vid

Min.
-30
2.4
1.65

Typ.

3.0

Max.
+85
3.6
Vdd

Unit
˚C
V
V

5.3.   Electrical Characteristics
The following conditions apply unless otherwise noted:
Vdd=2.4V to 3.6V, Vid=1.65V to Vdd, Temperature range=-30˚C to 85˚C

5.3.1.   DC Characteristics

Parameter
High level input voltage 1

Symbol
VIH1

Low level input voltage 1

VIL1

High level input voltage 2
Low level input voltage 2
Input current 1

VIH2
VIL2
IIN1

Input current 2
Hysteresis input voltage
(Note 2)

Low level output voltage
(Note 3)

Current consumption
(Note 4)

IIN2
VHS

VOL

IDD1

IDD2

IDD3
IDD4

Pin
RSTN
SCL
SDA
RSTN
SCL
SDA
TST
CAD
RSTN
SCL
SDA
CAD
TST
SCL
SDA

SDA

VDD
VID

Condition

Min.
70%Vid
70%Vid

Typ.

Max.
Vid+0.3

Unit
V

-0.3

30%Vid

V

Vin=Vss or Vid

Vin=Vss or Vdd
Vin=Vdd
Vid≥2V
Vid<2V
IOL≤+3mA
Vid≥2V
IOL≤+3mA
Vid<2V
Power-down mode
Vdd=Vid=3.0V
When magnetic sensor
is driven
Self-test mode
(Note 5)

70%Vdd
-0.3
-10

-10

5%Vid
10%Vid

Vdd+0.3
30%Vdd
+10

V
V
µA

+10
100

µA
V

0.4

V

20%Vid

6

6

8
5

µA

mA

mA
µA

3

3

5
0.1

MS1526-E-01

                                            - 7 -

          2014/7

                                                                                                                                      [AK09911]

(Note 2)  Schmitt trigger input (reference value for design)
(Note 3)  Output is open-drain. Connect a pull-up resistor externally. Maximum capacitive load: 400pF (Capacitive load

of each bus line for I2C bus interface).

(Note 4)  Without any resistance load. It does not include the current consumed by external loads (pull-down resister, etc.).

(Note 5)

RSTN, SDA, SCL = Vid or 0V. CAD = Vdd or 0V.
(case 1) Vdd=ON, Vid=ON, RSTN pin = “L”. (case 2) Vdd=ON, Vid=OFF (0V), RSTN pin = “L”. (case 3)
Vdd=OFF (0V), Vid=ON.

5.3.2.   AC Characteristics

Parameter
Power supply rise time
(Note 6)
POR completion time
(Note 6)
Power supply turn off
voltage (Note 6)
Power supply turn on
interval (Note 6)

Pin
Symbol
PSUP  VDD
VID

PORT

SDV

VDD
VID
PSINT  VDD
VID

Wait time before mode
setting

Twat

Condition
Period of time that VDD (VID)
changes from 0.2V to Vdd (Vid).
Period of time after PSUP to
Power-down mode (Note 7)
Turn off voltage to enable POR to
restart (Note 7)
Period of time that voltage lower
than SDV needed to be kept to
enable POR to restart (Note 7)

Min.  Typ.  Max.  Unit
ms

50

100

0.2

µs

V

µs

µs

100

100

(Note 6)  Reference value for design.
(Note 7)  When POR circuit detects the rise of VDD/VID voltage, it resets internal circuits and initializes the registers.

After reset, AK09911 transits to Power-down mode.

Power-down mode

Power-down mode

VDD/(VID)

SDV

0V

PORT

PSUP

PSINT

Parameter
Reset input effective pulse width (“L”)

Symbol
tRSTL  RSTN

Pin

Condition

Min.
5

Typ.  Max.  Unit
µs

tRSTL

VIL1

MS1526-E-01

                                            - 8 -

          2014/7

                                                                                                                                      [AK09911]

5.3.3.   Analog Circuit Characteristics

Parameter
Measurement data output bit
Time for measurement
Magnetic sensor sensitivity (Note 8)
Magnetic sensor measurement range
(Note 9)
Magnetic sensor initial offset
(Note 10)

Symbol
DBIT
TSM
BSE
BRG

Condition

Single measurement mode
Tc = 25 ˚C
Tc = 25 ˚C

Min.
-

0.57
±4912

Typ.  Max.
14
7.2
0.6

-
8.5
0.63  µT/LSB

Unit
bit
ms

µT

Tc = 25 ˚C

-500

+500

LSB

(Note 8)  Value after sensitivity is adjusted using sensitivity fine adjustment data stored in Fuse ROM.
(Note 9)  Reference value for design
(Note 10)  Value of measurement data register on shipment without applying magnetic field on purpose.

MS1526-E-01

                                            - 9 -

          2014/7

                                                                                                                                      [AK09911]

5.3.4.   I2C Bus Interface
I2C bus interface is compliant with Standard mode, Fast mode and High-speed mode. Standard/Fast mode is selected
automatically by fSCL.

  Standard mode

        fSCL≤100kHz
Symbol
fSCL
tHIGH
tLOW
tR
tF
tHD:STA
tSU:STA
tHD:DAT
tSU:DAT
tSU:STO
tBUF

Parameter
SCL clock frequency
SCL clock “High” time
SCL clock “Low” time
SDA and SCL rise time
SDA and SCL fall time
Start Condition hold time
Start Condition setup time
SDA hold time (vs. SCL falling edge)
SDA setup time (vs. SCL rising edge)
Stop Condition setup time
Bus free time

Min.

Typ.

4.0
4.7

4.0
4.7
0
250
4.0
4.7

  Fast mode

100Hz≤fSCL≤400kHz

Symbol
fSCL
tHIGH
tLOW
tR
tF
tHD:STA
tSU:STA
tHD:DAT
tSU:DAT
tSU:STO
tBUF
tSP

Parameter
SCL clock frequency
SCL clock “High” time
SCL clock “Low” time
SDA and SCL rise time
SDA and SCL fall time
Start Condition hold time
Start Condition setup time
SDA hold time (vs. SCL falling edge)
SDA setup time (vs. SCL rising edge)
Stop Condition setup time
Bus free time
Noise suppression pulse width

Min.

Typ.

0.6
1.3

0.6
0.6
0
100
0.6
1.3

Max.
100

1.0
0.3

Max.
400

0.3
0.3

50

Unit
kHz
µs
µs
µs
µs
µs
µs
µs
ns
µs
µs

Unit
kHz
µs
µs
µs
µs
µs
µs
µs
ns
µs
µs
ns

[I2C bus interface timing]

1/fSCL

SCL

SDA

SCL

tBUF

tLOW

tR

tHIGH

tF

VIH1

VIL1

tSP

VIH1

VIL1

VIH1

VIL1

tHD:STA

tHD:DAT

tSU:DAT

tSU:STA

Stop  Start

Start

tSU:STO

Stop

MS1526-E-01

                                            - 10 -

          2014/7

                                                                                                                                      [AK09911]

  High-speed mode (Hs-mode)

  Cb≤100pF (Cb: load capacitance)

fSCLH≤2.5MHz
Symbol
fSCLH
tHIGH
tLOW
tR_CL

tR_CL1

tR_DA
tF_CL
tF_DA
tHD:STA
tSU:STA
tHD:DAT
tSU:DAT
tSU:STO
tSP

  Cb≤400pF

Parameter
SCLH clock frequency
SCLH clock “High” time
SCLH clock “Low” time
SCLH rise time
SCLH rise time after a repeated START
condition and after an acknowledge bit
SDAH rise time
SCLH fall time
SDAH fall time
Start Condition hold time
Start Condition setup time
SDAH hold time (vs. SCLH falling edge)
SDAH setup time (vs. SCLH rising edge)
Stop Condition setup time
Noise suppression pulse width

fSCLH≤1.7MHz
Symbol
fSCLH
tHIGH
tLOW
tR_CL

tR_CL1

tR_DA
tF_CL
tF_DA
tHD:STA
tSU:STA
tHD:DAT
tSU:DAT
tSU:STO
tSP

Parameter
SCLH clock frequency
SCLH clock “High” time
SCLH clock “Low” time
SCLH rise time
SCLH rise time after a repeated START
condition and after an acknowledge bit
SDAH rise time
SCLH fall time
SDAH fall time
Start Condition hold time
Start Condition setup time
SDAH hold time (vs. SCLH falling edge)
SDAH setup time (vs. SCLH rising edge)
Stop Condition setup time
Noise suppression pulse width

Min.

Typ.

Max.
2.5

110
220
10

10

10
-
-
160
160
0
10
160

Min.

Typ.

120
320
20

20

20
-
-
160
160
0
10
160

40

80

80
40
80

10

Max.
1.7

80

160

160
80
160

10

Unit
MHz
ns
ns
ns

ns

ns
ns
ns
ns
ns
ns
ns
ns
ns

Unit
MHz
ns
ns
ns

ns

ns
ns
ns
ns
ns
ns
ns
ns
ns

MS1526-E-01

                                            - 11 -

          2014/7

[I2C bus interface timing of Hs-mode]

                                                                                                                                      [AK09911]

1/fSCLH

SCLH

START

Tf_D

Tr_D

SDAH

VIH1

VIL1

START

STOP

tSU;STA

tHD;DAT

tHD;STA

tSU;DAT

tSU;STO

SCLH

tf_CL

tr_CL1

Tr_CL

Tr_CL1

tHIGH  tLOW

tHIGH

VIH1

VIL1

VIH1

VIL1

MS1526-E-01

                                            - 12 -

          2014/7

                                                                                                                                      [AK09911]

6. Function Explanation

6.1.   Power States
When VDD and VID are turned on from Vdd=OFF (0V) and Vid=OFF (0V), all registers in AK09911 are initialized by
POR circuit and AK09911 transits to Power-down mode.

All the states in the table below can be set, although the transition from state 2 to state 3 and the transition from state 3 to
state 2 are prohibited.

Table 6.1.  Power state

State
1

VDD
OFF (0V)

VID
OFF (0V)

Power state

OFF (0V).
It doesn’t affect external interface.Digital
input pins other than SCL and SDA pin
should be fixed to “L”(0V).

2

3

OFF (0V)

1.65V to 3.6V  OFF (0V)

2.4V to 3.6V

OFF (0V)

It doesn’t affect external interface.
OFF(0V)
It doesn’t affect external interface. Digital
input pins other than SCL and SDA pin
should be fixed to “L”(0V).

4

2.4V to 3.6V

1.65V to Vdd  ON

6.2.   Reset Functions
When the power state is ON, always keep Vid≤Vdd.
Power-on reset (POR) works until Vdd reaches to the operation effective voltage (about 1.1V: reference value for design)
on power-on sequence. After POR is deactivated, all registers are initialized and transits to Power-down mode.

When Vdd=2.4 to 3.6V, POR circuit and VID monitor circuit are active. When Vid=0V, AK09911 is in reset status and it
consumes the current of reset state (IDD4).

AK09911 has four types of reset;
(1)  Power on reset (POR)

When Vdd rise is detected, POR circuit operates, and AK09911 is reset.

(2)  VID monitor

When VID is turned OFF, AK09911 is reset.

(3)  Reset pin (RSTN)

AK09911 is reset by Reset pin. When Reset pin is not used, connect to VID.

(4)  Soft reset

AK09911 is reset by setting SRST bit. When AK09911 is reset, all registers are initialized and AK09911 transits to
Power-down mode.

MS1526-E-01

                                            - 13 -

          2014/7

                                                                                                                                      [AK09911]

6.3.   Operation Mode
AK09911 has following nine operation modes:

(1)  Power-down mode
(2)  Single measurement mode
(3)  Continuous measurement mode 1
(4)  Continuous measurement mode 2
(5)  Continuous measurement mode 3
(6)  Continuous measurement mode 4
(7)  Self-test mode
(8)  Fuse ROM access mode

By setting CNTL2 register MODE[4:0] bits, the operation set for each mode is started.
A transition from one mode to another is shown below.

Power-down

                        MODE[4:0]=“00001”
                                                MODE[4:0]=“00000”
                        Transits automatically

mode

                          MODE[4:0]=“00010”
                                                MODE[4:0]=“00000”

                        MODE[4:0]=“00100”
                                                MODE[4:0]=“00000”

                                                MODE[4:0]=“00110”
                        MODE[4:0]=“00000”

                        MODE[4:0]=“01000”
                                                MODE[4:0]=“00000”

Single measurement mode

Sensor is measured for one time and data is output.
Transits to Power-down mode automatically after
measurement ended.

Continuous measurement mode 1

Sensor is measured periodically in 10Hz.
Transits to Power-down mode by writing MODE[4:0]
= “00000”.

Continuous measurement mode 2

Sensor is measured periodically in 20Hz.
Transits to Power-down mode by writing
MODE[4:0]=“00000”.

Continuous measurement mode 3

Sensor is measured periodically in 50Hz.
Transits to Power-down mode by writing
MODE[4:0]=“00000”.

Continuous measurement mode 4

Sensor is measured periodically in 100Hz.
Transits to Power-down mode by writing
MODE[4:0]=“00000”.

                                                MODE[4:0]=“10000”
                          MODE[4:0]=“00000”
                                              Transits automatically

Self-test mode

Sensor is self-tested and the result is output. Transits
to Power-down mode automatically.

                                              MODE[4:0]=“11111”
  MODE[4:0]=“00000”

Fuse ROM access mode

Turn on the needed to read out Fuse ROM. Transits
to Power-down mode by writing
MODE[4:0]=“00000”.

Figure 6.1.  Operation mode

When power is turned ON, AK09911 is in Power-down mode. When a specified value is set to MODE[4:0], AK09911
transits to the specified mode and starts operation. When user wants to change operation mode, transit to Power-down mode
first and then transit to other modes. After Power-down mode is set, at least 100 µs (Twat) is needed before setting another
mode

MS1526-E-01

                                            - 14 -

          2014/7

                                                                                                                                      [AK09911]

6.4.   Description of Each Operation Mode

6.4.1.   Power-down Mode
Power to almost all internal circuits is turned off. All registers are accessible in Power-down mode. Data stored in
read/write registers are remained. They can be reset by soft reset.

6.4.2.   Single Measurement Mode
When Single measurement mode (MODE[4:0]=“00001”) is set, magnetic sensor measurement is started. After magnetic
sensor measurement and signal processing is finished, measurement magnetic data is stored to measurement data registers
(HXL to HZH), then AK09911 transits to Power-down mode automatically. On transition to Power-down mode,
MODE[4:0] turns to “00000”. At the same time, DRDY bit in ST1 register turnes to “1”. This is called “Data Ready”.
When any of measurement data register (HXL to TMPS) or ST2 register is read, DRDY bit turnes to “0”. It remains “1” on
transition from Power-down mode to another mode. (Figure 6.2. )
When sensor is measuring (Measurement period), measurement data registers (HXL to TMPS) keep the previous data.
Therefore, it is possible to read out data even in measurement period. Data read out in measurement period are previous
data.(Figure 6.3. )

Operation Mode:
Power-down

Single measuremnet
(1)

(2)

(3)

Measurement period

Measurement Data Register
Last Data

Measurement Data (1)

Data(2)

Data(3)

DRDY

Data read

Data(1)

Data(3)

Register Write

MODE[4:0]="00001"

MODE[4:0]="00001" MODE[4:0]="00001"

Figure 6.2.  Single measurement mode when data is read out of measurement period

Operation Mode:
Power-down

Single measuremnet
(1)

(2)

(3)

Measurement period

Measurement Data Register
Last Data

Measurement Data (1)

Data(3)

DRDY

Data read

Data(1)

Register Write

MODE[4:0]="00001"

MODE[4:0]="00001" MODE[4:0]="00001"

Figure 6.3.  Single measurement mode when data read started during measurement period

MS1526-E-01

                                            - 15 -

          2014/7

                                                                                                                                      [AK09911]

6.4.3.   Continuous Measurement Mode 1, 2, 3 and 4
When Continuous measurement mode 1 (MODE[4:0]=“00010”), 2 (MODE[4:0]=“00100”), 3 (MODE[4:0]=“00110”) or 4
(MODE[4:0]=“01000”) is set, magnetic sensor measurement is started periodically at 10 Hz, 20 Hz, 50 Hz or 100 Hz
respectively. After magnetic sensor measurement and signal processing is finished, measurement magnetic data is stored to
measurement data registers (HXL to HZH) and all circuits except for the minimum circuit required for counting cycle
length are turned off (PD). When the next measurement timing comes, AK09911 wakes up automatically from PD and
starts measurement again.
Continuous measurement mode ends when Power-down mode (MODE[4:0]=“00000”) is set. It repeats measurement until
Power-down mode is set.
When Continuous measurement mode 1 (MODE[4:0]=“00010”), 2 (MODE[4:0]=“00100”), 3 (MODE[4:0]=“00110”) or 4
(MODE[4:0]=“01000”) is set again while AK09911 is already in Continuous measurement mode, a new measurement starts.
ST1, ST2 and measurement data registers (HXL to TMPS) will not be initialized by this.

(N-1)th
PD

Nth
Measurement

PD

(N+1)th
Measurement

PD

10Hz,20Hz,50Hz or 100Hz

Figure 6.4.  Continuous measurement mode

6.4.3.2.   Data Ready
When measurement data is stored and ready to be read, DRDY bit in ST1 register turnes to “1”. This is called “Data Ready”.
When measurement is performed correctly, AK09911 becomes Data Ready on transition to PD after measurement.

6.4.3.3. Normal Read Sequence

(1)  Check Data Ready or not by polling DRDY bit of ST1 register

  DRDY: Shows Data Ready or not. Not when “0”, Data Ready when “1”.
  DOR: Shows if any data has been skipped before the current data or not. There are no skipped data when “0”,

there are skipped data when “1”.

(2)  Read measurement data

When any of measurement data register (HXL to TMPS) or ST2 register is read, AK09911 judges that data reading
is started. When data reading is started, DRDY bit and DOR bit turnes to “0”.

(3)  Read ST2 register (required)

  HOFL: Shows if magnetic sensor is overflowed or not. “0” means not overflowed, “1” means overflowed.
When ST2 register is read, AK09911 judges that data reading is finished. Stored measurement data is protected
during data reading and data is not updated. By reading ST2 register, this protection is released. It is required to read
ST2 register after data reading.

(N-1)th Nth

PD Measurement

PD

(N+1)th
Measurement

PD

Measurement Data Register
Nth
(N-1)th

DRDY

(N+1)th

Data read

ST1 Data(N)

ST2

ST1 Data(N+1)

ST2

Figure 6.5.  Normal read sequence

MS1526-E-01

                                            - 16 -

          2014/7

                                                                                                                                      [AK09911]

6.4.3.4. Data Read Start during Measurement
When sensor is measuring (Measurement period), measurement data registers (HXL to TMPS) keep the previous data.
Therefore, it is possible to read out data even in measurement period. If data is started to be read during measurement period,
previous data is read.

(N-1)th Nth

PD Measurement

PD

(N+1)th
Measurement

PD

Measurement Data Register
Nth
(N-1)th

DRDY

Data read

ST1 Data(N)

ST2

ST1 Data(N)

ST2

Figure 6.6.  Data read start during measurement

MS1526-E-01

                                            - 17 -

          2014/7

                                                                                                                                      [AK09911]

6.4.3.5.   Data Skip
When Nth data was not read before (N+1)th measurement ends, Data Ready remains until data is read. In this case, a set of
measurement data is skipped so that DOR bit turnes to “1”.
When data reading started after Nth measurement ended and did not finish reading before (N+1)th measurement ended, Nth
measurement data is protected to keep correct data. In this case, a set of measurement data is skipped and not stored so that
DOR bit turnes to “1”.
In both case, DOR bit turnes to “0” at the next start of data reading.

(N-1)th Nth

PD Measurement

PD

(N+1)th
Measurement

PD

Measurement Data Register
Nth
(N-1)th

(N+1)th

DRDY

DOR

Data read

ST1 Data(N+1)

ST2

Figure 6.7.  Data Skip: When data is not read

(N-1)th Nth

PD Measurement

PD

(N+1)th
Measurement

PD

(N+2)th
Measurement

PD

Measurement Data Register
(N-1)th

Nth

DRDY

DOR

Data register is protected
because data is being read

(N+2)th

Not data ready
because data is not updated

(N+1)th data is skipped

Data read

ST1 DataN

ST2

ST1 Data(N+2)

Figure 6.8.  Data Skip: When data read has not been finished before the next measurement end

6.4.3.6.   End Operation
Set Power-down mode (MODE[4:0]=“00000”) to end Continuous measurement mode.

MS1526-E-01

                                            - 18 -

          2014/7

                                                                                                                                      [AK09911]

6.4.3.7.   Magnetic Sensor Overflow
AK09911 has the limitation for measurement range that the sum of absolute values of each axis should be smaller than 4912
μT.

|X|+|Y|+|Z| < 4912 μT

When the magnetic field exceeded this limitation, data stored at measurement data are not correct. This is called Magnetic
Sensor Overflow.
When magnetic sensor overlow occurs, HOFL bit turns to “1”. When the next measurement starts, it returns to “0”.

6.4.4.   Self-test Mode
Self-test mode is used to check if the magnetic sensor is working normally.
When Self-test mode (MODE[4:0]=“10000”) is set, magnetic field is generated by the internal magnetic source and
magnetic sensor is measured. Measurement data is stored to measurement data registers (HXL to HZH), then AK09911
transits to Power-down mode automatically.
Data read sequence and functions of read-only registers in Self-test mode is the same as Single measurement mode.

6.4.4.1.   Self-test Sequence

(1)  Set Power-down mode. (MODE[4:0]=“00000”)
(2)  Set Self-test mode. (MODE[4:0]=“10000”)
(3)  Check Data Ready or not by polling DRDY bit of ST1 register

When Data Ready, proceed to the next step.

(4)  Read measurement data (HXL to HZH)

6.4.4.2.   Self-test Judgment
When measurement data read by the above sequence is in the range of following table after sensitivity adjustment (refer to
8.3.11), AK09911 is working normally.

Criteria

HX[15:0]
-30 ≤ HX ≤ +30

HY[15:0]
-30 ≤ HY ≤ +30

HZ[15:0]
-400 ≤ HZ ≤ -50

6.4.5.   Fuse ROM Access Mode
Fuse ROM access mode is used to read Fuse ROM data.  Sensitivity adjustment data for each axis is stored in fuse ROM.
Set Fuse ROM Access mode (MODE[4:0]=“11111”) before reading Fuse ROM data. When Fuse ROM Access mode is set,
circuits required for reading fuse ROM are turned on.
After reading fuse ROM data, set Power-down mode (MODE[4:0]=“00000”) before the transition to another mode.

MS1526-E-01

                                            - 19 -

          2014/7

                                                                                                                                      [AK09911]

7. Serial Interface

The I2C bus interface of AK09911 supports the Standard mode (100 kHz max.), the Fast mode (400 kHz max.) and the
High-speed mode (Hs-mode, 2.5 MHz max.).

7.1.   Data Transfer
To access AK09911 on the bus, generate a start condition first.
Next, transmit a one-byte slave address including a device address. At this time, AK09911 compares the slave address with
its own address. If these addresses match, AK09911 generates an acknowledgement, and then executes READ or WRITE
instruction. At the end of instruction execution, generate a stop condition.

7.1.1.   Change of Data
A change of data on the SDA line must be made during “Low” period of the clock on the SCL line. When the clock signal
on the SCL line is “High”, the state of the SDA line must be stable. (Data on the SDA line can be changed only when the
clock signal on the SCL line is “Low”.)
During the SCL line is “High”, the state of data on the SDA line is changed only when a start condition or a stop condition
is generated.

SCL

SDA

DATA LINE
STABLE :
DATA VALID

CHANGE
OF DATA
ALLOWED

Figure 7.1.  Data Change

7.1.2.   Start/Stop Condition
If the SDA line is driven to “Low” from “High” when the SCL line is “High”, a start condition is generated. Every
instruction starts with a start condition.
If the SDA line is driven to “High” from “Low” when the SCL line is “High”, a stop condition is generated. Every
instruction stops with a stop condition.

SCL

SDA

START CONDITION

STOP CONDITION

Figure 7.2.  Start and Stop Condition

MS1526-E-01

                                            - 20 -

          2014/7

                                                                                                                                      [AK09911]

7.1.3.   Acknowledge
The IC that is transmitting data releases the SDA line (in the “High” state) after sending 1-byte data.
The IC that receives the data drives the SDA line to “Low” on the next clock pulse. This operation is referred as
acknowledge. With this operation, whether data has been transferred successfully can be checked.
AK09911 generates an acknowledge after reception of a start condition and slave address.
When a WRITE instruction is executed, AK09911 generates an acknowledge after every byte is received.
When a READ instruction is executed, AK09911 generates an acknowledge then transfers the data stored at the specified
address. Next, AK09911 releases the SDA line then monitors the SDA line. If a master IC generates an acknowledge
instead of a stop condition, AK09911 transmits the 8bit data stored at the next address. If no acknowledge is generated,
AK09911 stops data transmission.

SCL FROM
MASTER

DATA
OUTPUT BY
TRANSMITTER

DATA
OUTPUT BY
RECEIVER

START
CONDITION

Clock pulse
for acknowledge

1

8

9

not acknowledge

acknowledge

Figure 7.3.  Generation of Acknowledge

7.1.4.   Slave Address
The slave address of AK09911 can be selected from the following list by setting CAD pin. When CAD pin is fixed to VSS,
the corresponding slave address bit is “0“. When CAD pin is fixed to VDD, the corresponding slave address bit is “1“.

Table 7.1.  Slave Address and CAD pin

CAD
0
1

Slave Address
0CH
0DH

                                        MSB                                                                                                      LSB

0

0

0

1

1

0

CAD

R/W

Figure 7.4.  Slave Address

The first byte including a slave address is transmitted after a start condition, and an IC to be accessed is selected from the
ICs on the bus according to the slave address.
When a slave address is transferred, the IC whose device address matches the transferred slave address generates an
acknowledge then executes an instruction. The 8th bit (least significant bit) of the first byte is a R/W bit.
When the R/W bit is set to  “1“, READ instruction is executed. When the R/W bit is set to  “0“, WRITE instruction is
executed.

MS1526-E-01

                                            - 21 -

          2014/7

                                                                                                                                      [AK09911]

7.2.   WRITE Instruction
When the R/W bit is set to “0”, AK09911 performs write operation.
In write operation, AK09911 generates an acknowledge after receiving a start condition and the first byte (slave address)
then receives the second byte. The second byte is used to specify the address of an internal control register and is based on
the MSB-first configuration.

                                        MSB                                                                                                      LSB

A7

A6

A5

A4

A3

A2

A1

A0

Figure 7.5.  Register Address

After receiving the second byte (register address), AK09911 generates an acknowledge then receives the third byte.
The third and the following bytes represent control data. Control data consists of 8 bits and is based on the MSB-first
configuration. AK09911 generates an acknowledge after every byte is received. Data transfer always stops with a stop
condition generated by the master.

                                        MSB                                                                                                      LSB

D7

D6

D5

D4

D3

D2

D1

D0

Figure 7.6.  Control Data

AK09911 can write multiple bytes of data at a time.
After reception of the third byte (control data), AK09911 generates an acknowledge then receives the next data. If
additional data is received instead of a stop condition after receiving one byte of data, the address counter inside the LSI
chip is automatically incremented and the data is written at the next address.
The address is incremented from 00H to 18H, from 30H to 32H, or from 60H to 62H. When the address is 00H to 18H, the
address is incremented 00H  01H  02H  03H  10H  11H ...  18H,and the address goes back to 00H after
18H. When the address is 30H to 32H, the address goes back to 30H after 32H. When the address is 60H to 62H, the
address goes back to 60H after 62H.
Actual data is written only to Read/Write registers (refer to Table 8.2. ).

S
T
A
R
T

R/W="0"

SDA

S  Slave

Address

Register
Address(n)

Data(n)

Data(n+1)

Data(n+x)

A
C
K

A
C
K

A
C
K

A
C
K

A
C
K

Figure 7.7.  WRITE Instruction

S
T
O
P

P

A
C
K

MS1526-E-01

                                            - 22 -

          2014/7

                                                                                                                                      [AK09911]

7.3.   READ Instruction
When the R/W bit is set to “1”, AK09911 performs read operation.
If a master IC generates an acknowledge instead of a stop condition after AK09911 transfers the data at a specified address,
the data at the next address can be read.
Address can be 00H to 18H, 30H to 32H, and 60H to 62H. When the address is 00H to 18H, the address is incremented 00H
 01H  02H  03H  10H  11H ...  18H,and the address goes back to 00H after 18H. When the address is 30H
to 32H, the address goes back to 30H after 32H. When the address is 60H to 62H, the address goes back to 60H after 62H.
AK09911 supports one byte read and multiple byte read.

7.3.1.   One Byte READ
AK09911 has an address counter inside the LSI chip. In current address read operation, the data at an address specified by
this counter is read.
The internal address counter holds the next address of the most recently accessed address.
For example, if the address most recently accessed (for READ instruction) is address “n”, and a current address read
operation is attempted, the data at address “n+1” is read.
In one byte read operation, AK09911 generates an acknowledge after receiving a slave address for the READ instruction
(R/W bit=“1”). Next, AK09911 transfers the data specified by the internal address counter starting with the next clock pulse,
then increments the internal counter by one. If the master IC generates a stop condition instead of an acknowledge after
AK09911 transmits one byte of data, the read operation stops.

S
T
A
R
T

R/W="1"

SDA

S  Slave

Address

Data(n)

Data(n+1)

Data(n+2)

Data(n+x)

A
C
K

A
C
K

A
C
K

A
C
K

A
C
K

Figure 7.8.  One Byte READ

S
T
O
P

P

7.3.2.   Multiple Byte READ
By multiple byte read operation, data at an arbitrary address can be read.
The multiple byte read operation requires to execute WRITE instruction as dummy before a slave address for the READ
instruction (R/W bit=“1”) is transmitted. In random read operation, a start condition is first generated then a slave address
for the WRITE instruction (R/W bit=“0”) and a read address are transmitted sequentially.
After AK09911 generates an acknowledge in response to this address transmission, a start condition and a slave address for
the READ instruction (R/W bit=“1”) are generated again. AK09911 generates an acknowledge in response to this slave
address transmission. Next, AK09911 transfers the data at the specified address then increments the internal address counter
by one. If the master IC generates a stop condition instead of an acknowledge after data is transferred, the read operation
stops.

S
T
A
R
T

R/W="0"

S
T
A
R
T

R/W="1"

SDA

S  Slave

Address

Register
Address(n)

S  Slave

Address

Data(n)

Data(n+1)

Data(n+x)

A
C
K

A
C
K

A
C
K

A
C
K

A
C
K

A
C
K

Figure 7.9.  Multiple Byte READ

S
T
O
P

P

MS1526-E-01

                                            - 23 -

          2014/7

                                                                                                                                      [AK09911]

7.4. High-speed Mode (Hs-mode)
AK09911 supports the Hs-mode.
Hs-mode can only commence after the following conditions (all of which are in Fast/Standard-mode):

  START condition (S)
  8-bit master code (00001XXX)
  not-acknowledge bit (Ā)

The diagram below shows data flow of the Hs-mode.
After start condition, feed master code 00001XXX for transfer to the Hs-mode. And then AK09911 feeds back
not-acknowledge bit and swich over to circuit for the Hs-mode between times t1 and tH. AK09911 can communicate at the
Hs-mode from next START condition. At time tFS, AK09911 switchs its internal circuit from the Hs-mode to the First
mode with the STOP condition (P). This transfer completes in the bus free time (tBUF).

Figure 7.10.  Data transfer format in Hs-mode

Figure 7.11.  Hs-mode transfer

MS1526-E-01

                                            - 24 -

          2014/7

                                                                                                                                      [AK09911]

8. Registers

8.1.   Description of Registers
AK09911 has registers of 20 addresses as indicated in Every address consists of 8 bits data. Data is transferred to or
received from the external CPU via the serial interface described previously.

Name  Address

WIA1

WIA2

INFO1

INFO2

ST1

HXL

HXH

HYL

HYH

HZL

HZH

TMPS

ST2

CNTL1

00H

01H

02H

03H

10H

11H

12H

13H

14H

15H

16H

17H

18H

30H

CNTL2

31H

CNTL3

32H

TS1

33H

ASAX

ASAY

ASAZ

60H

61H

62H

READ/
WRITE
READ

READ

READ

READ

READ

READ

READ

READ

READ

READ

READ

READ

READ

READ/
WRITE
READ/
WRITE
READ/
WRITE
READ/
WRITE
READ

READ

READ

Table 8.1.  Register Table

Description

Campany ID

Device ID

Information 1

Information 2

Status 1

Measurement Magnetic Data

Dummy Register

Status 2

Dummy Register

Control 2

Control 3

Test

X-axis sensitivity adjustment value

Y-axis sensitivity adjustment value

Z-axis sensitivity adjustment value

Bit
width
8

Remarks

8

8

8

8

8

8

8

8

8

8

8

8

8

8

8

8

8

8

8

Data status
X-axis data

Y-axis data

Z-axis data

Dummy
Data status
Dummy

Control settings

Control settings

DO NOT ACCESS

Fuse ROM

Fuse ROM

Fuse ROM

Addresses 00H to 18H, 30H to 32H and 60H to 62H are compliant with automatic increment function of serial interface
respectively. Values of addresses 60H to 62H can be read only in Fuse ROM access mode. In other modes, read data is not
correct. When the address is in 00H to 18H, the address is incremented 00H  01H  02H  03H  10H  11H ... 
18H, and the address goes back to 00H after 18H. When the address is in 30H to 32H, the address goes back to 30H after
32H. When the address is in 60H to 62H, the address goes back to 60H after 62H.

MS1526-E-01

                                            - 25 -

          2014/7

                                                                                                                                      [AK09911]

8.2.   Register Map

Table 8.2.  Register Map

Addr.

Register
name

D7

D6

D5

D4

D3

D2

D1

D0

Read-only register

00H
01H
02H
03H
10H
11H
12H
13H
14H
15H
16H
17H
18H

30H

31H

32H

33H

60H
61H
62H

WIA1
WIA2
INFO1
INFO2
ST1
HXL
HXH
HYL
HYH
HZL
HZH
TMPS
ST2

CNTL1

CNTL2

CNTL3

TS1

0
0
INFO17
INFO27
HSM
HX7
HX15
HY7
HY15
HZ7
HZ15
0
0

1
0
INFO16
INFO26
0
HX6
HX14
HY6
HY14
HZ6
HZ14
0
0

0
0
INFO15
INFO25
0
HX5
HX13
HY5
HY13
HZ5
HZ13
0
0

0
0
INFO14
INFO24
0
HX4
HX12
HY4
HY12
HZ4
HZ12
0
0

1
0
INFO13
INFO23
0
HX3
HX11
HY3
HY11
HZ3
HZ11
0
HOFL

0
1
INFO12
INFO22
0
HX2
HX10
HY2
HY10
HZ2
HZ10
0
0

0
0
INFO11
INFO21
DOR
HX1
HX9
HY1
HY9
HZ1
HZ9
0
0

0
1
INFO10
INFO20
DRDY
HX0
HX8
HY0
HY8
HZ0
HZ8
0
0

Read/Wright register

0

0

0

-

0

0

0

-

0

0

0

-

0

0

0

0

0

MODE4  MODE3  MODE2  MODE1  MODE0

0

-

0

-

0

-

0

-

SRST

-

Read-only register

ASAX  COEFX7  COEFX6  COEFX5  COEFX4  COEFX3  COEFX2  COEFX1  COEFX0
ASAY  COEFY7  COEFY6  COEFY5  COEFY4  COEFY3  COEFY2  COEFY1  COEFY0
ASAZ  COEFZ7  COEFZ6  COEFZ5  COEFZ4  COEFZ3  COEFZ2  COEFZ1  COEFZ0

When VDD is turned ON, POR function works and all registers of AK09911 are initialized regardless of VID status. To
write data to or to read data from register, VID must be ON.
TS1 is test registers for shipment test. Do not use these registers.

MS1526-E-01

                                            - 26 -

          2014/7

                                                                                                                                      [AK09911]

8.3.   Detailed of Description of Register

8.3.1.   WIA: Who I Am

Addr

Register
name

D7

D6

D5

D4

D3

D2

D1

D0

Read-only register

00H
01H

WIA1
WIA2

0
0

1
0

0
0

0
0

1
0

0
1

0
0

0
1

WIA1[7:0]: Company ID of AKM. It is described in one byte and fixed value.
          48H: fixed
WIA2[7:0]: Device ID of AK09911. It is described in one byte and fixed value.
          05H: fixed

8.3.2.   INFO: Information

Addr

Register
name

02H
03H

INFO1
INFO2

D7

D6

D5

D4

D3

D2

D1

D0

Read-only register
INFO17  INFO16  INFO15  INFO14
INFO27  INFO26  INFO25  INFO24

INFO13  INFO12  INFO11
INFO23  INFO22  INFO21

INFO10
INFO20

INFO1[7:0]/INFO2[7:0]: Device information of AKM.

8.3.3.   ST1: Status 1

Addr

Register
name

D7

D6

D5

D4

D3

D2

D1

D0

Read-only register

10H

ST1

Reset

HSM
0

0
0

0
0

0
0

0
0

0
0

DOR  DRDY

0

0

DRDY: Data Ready
“0“: Normal
“1“: Data is ready

DRDY bit turns to “1” when data is ready in Single measurement mode, Continuous measurement mode 1, 2, 3, 4 or
Self-test mode. It returns to “0” when any one of ST2 register or measurement data register (HXL to TMPS) is read.

DOR: Data Overrun
“0”: Normal
“1”: Data overrun

DOR bit turns to “1” when data has been skipped in Continuous measurement mode 1, 2, 3, 4. It returns to “0” when any
one of ST2 register or measurement data register (HXL to TMPS) is read.

HSM: I2C Hs-mode

“0”: Standard/Fast mode
“1”: Hs-mode

HSM bit turns to “1” when I2C bus interface is changed from Standard or Fast mode to High-speed mode (Hs-mode).

MS1526-E-01

                                            - 27 -

          2014/7

                                                                                                                                      [AK09911]

8.3.4.   HXL to HZH: Measurement data

Addr

Register
name

11H
12H
13H
14H
15H
16H

HXL
HXH
HYL
HYH
HZL
HZH

Reset

D7

D6

D5

D4

D3

D2

D1

D0

Read-only register
HX4
HX12
HY4
HY12
HZ4
HZ12
0

HX5
HX13
HY5
HY13
HZ5
HZ13
0

HX6
HX14
HY6
HY14
HZ6
HZ14
0

HX7
HX15
HY7
HY15
HZ7
HZ15
0

HX3
HX11
HY3
HY11
HZ3
HZ11
0

HX2
HX10
HY2
HY10
HZ2
HZ10
0

HX1
HX9
HY1
HY9
HZ1
HZ9
0

HX0
HX8
HY0
HY8
HZ0
HZ8
0

Measurement data of magnetic sensor X-axis/Y-axis/Z-axis

HXL[7:0]: X-axis measurement data lower 8-bit
HXH[15:8]: X-axis measurement data higher 8-bit
HYL[7:0]: Y-axis measurement data lower 8-bit
HYH[15:8]: Y-axis measurement data higher 8-bit
HZL[7:0]: Z-axis measurement data lower 8-bit
HZH[15:8]: Z-axis measurement data higher 8-bit

Measurement data is stored in two’s complement and Little Endian format. Measurement range of each axis is -8190 to
8190.

Table 8.3.  Measurement magnetic data format

Measurement data (each axis) [15:0]

Two’s complement
0001 1111 1111 1110
|
0000 0000 0000 0001
0000 0000 0000 0000
1111 1111 1111 1111
|
1110 0000 0000 0010

Hex
1FFE
|
0001
0000
FFFF
|
E002

Decimal
8190
|
1
0
-1
|
-8190

Magnetic flux
density [µＴ]
4912(max.)
|
0.6
0
-0.6
|
-4912(min.)

8.3.5.   TMPS: Dummy Register

Addr

Register
name

D7

D6

D5

D4

D3

D2

D1

D0

Read-only register

17H

TMPS

Reset

0
0

0
0

0
0

0
0

0
0

0
0

0
0

0
0

Dummy register.

MS1526-E-01

                                            - 28 -

          2014/7

                                                                                                                                      [AK09911]

8.3.6.   ST2: Status 2

Addr

Register
name

D7

D6

D5

D4

D3

D2

D1

D0

18H

ST2

Reset

0
0

0
0

0
0

0
0

HOFL
0

0
0

0
0

0
0

Read-only register

HOFL: Magnetic sensor overflow

“0”: Normal
“1”: Magnetic sensor overflow occurred

In Single measurement mode, Continuous measurement mode 1, 2, 3, 4, and Self-test mode, magnetic sensor may overflow
even though measurement data regiseter is not saturated. In this case, measurement data is not correct and HOFL bit turns to
“1”. When next measurement stars, it returns to “0”. Refer to 6.4.3.6 for detailed information.

ST2 register has a role as data reading end register, also. When any of measurement data register (HXL to TMPS) is read in
Continuous measurement mode 1, 2, 3, 4, it means data reading start and taken as data reading until ST2 register is read.
Therefore, when any of measurement data is read, be sure to read ST2 register at the end.

8.3.7.   CNTL1: Dummy Register

Addr

Register
name

D7

D6

D5

D4

D3

D2

D1

D0

Read/Write register

30H  CNTL1
Reset

0
0

0
0

0
0

0
0

0
0

0
0

0
0

0
0

Dummy register.

8.3.8.   CNTL2: Control 2

Addr

Register
name

D7

D6

D5

D4

D3

D2

D1

D0

Read/Write register

MODE4  MODE3  MODE2  MODE1  MODE0
0

0

0

0

0

31H  CNTL2
Reset

0
0

0
0

0
0

MODE[4:0]: Operation mode setting
“00000”: Power-down mode
“00001”: Single measurement mode
“00010”: Continuous measurement mode 1
“00100”: Continuous measurement mode 2
“00110”: Continuous measurement mode 3
“01000”: Continuous measurement mode 4
“10000”: Self-test mode
“11111”: Fuse ROM access mode
Other code settings are prohibited
.

When each mode is set, AK09911 transits to the set mode. Refer to 6.3 for detailed information.

MS1526-E-01

                                            - 29 -

          2014/7

                                                                                                                                      [AK09911]

8.3.9.   CNTL3: Control 3

Addr

Register
name

D7

D6

D5

D4

D3

D2

D1

D0

Read/Write register

32H  CNTL3
Reset

0
0

0
0

0
0

0
0

0
0

0
0

0
0

SRST
0

SRST: Soft reset

“0”: Normal
“1”: Reset

When “1” is set, all registers are initialized. After reset, SRST bit turns to “0” automatically.

8.3.10.   TS1: Test

Addr

Register
name

D7

D6

D5

D4

D3

D2

D1

D0

33H

TS1

Reset

-
0

-
0

-
0

-
0

-
0

-
0

-
0

-
0

Read/Write register

TS1 register is AKM internal test register. Do not use this register.

8.3.11.   ASAX, ASAY, ASAZ: Sensitivity Adjustment Values

Addr

Register
name

D7

D6

D5

D4

D3

D2

D1

D0

Read-only    register

60H
61H
62H

ASAX  COEFX7  COEFX6  COEFX5  COEFX4  COEFX3  COEFX2  COEFX1  COEFX0
ASAY  COEFY7  COEFY6  COEFY5  COEFY4  COEFY3  COEFY2  COEFY1  COEFY0
ASAZ  COEFZ7  COEFZ6  COEFZ5  COEFZ4  COEFZ3  COEFZ2  COEFZ1  COEFZ0
-

-

-

-

-

-

-

-

Reset

Sensitivity adjustment data for each axis is stored to fuse ROM on shipment.

ASAX[7:0]: Magnetic sensor X-axis sensitivity adjustment value
ASAY[7:0]: Magnetic sensor Y-axis sensitivity adjustment value
ASAZ[7:0]: Magnetic sensor Z-axis sensitivity adjustment value

  How to adjust sensitivity

The sensitivity adjustment is done by the equation below,

Hadj

=

H

×





ASA
128

+



1


where H is the measurement data read out from the measurement data register, ASA is the sensitivity adjustment
value, and Hadj is the adjusted measurement data.

MS1526-E-01

                                            - 30 -

          2014/7

                                                                                                                                      [AK09911]

9. Example of Recommended External Connection

Host CPU

Power for I/F

GPIO

I2C I/F

VID
POWER 1.65V to Vdd

VDD
POWER 2.4V to 3.6V

0.1µF

0.1µF

SDA

RSTN

VID

AK09911C

SCL

VSS

(Top view)

TST

CAD

VDD

3                      2              1

C

B

A

Slave address select

CAD          address
VSS          0 0 0 1 1 0 0 R/W
VDD        0 0 0 1 1 0 1 R/W

Pins of dot circle should be kept non-connected.

MS1526-E-01

                                            - 31 -

          2014/7

                                                                                                                                      [AK09911]

10.   Package

10.1.   Marking
  Date code:  X1X2X3X4X5
  X1  = ID
  X2  = Year code
  X3X4 = Week code
  X5  = Lot
  Product name: 9911

X1X2X3X4X5

9911

<Top view>

10.2.   Pin Assignment

C
B
A

3
SDA
SCL
TST

2
RSTN

CAD

1
VID
VSS
VDD

                                                                                      <Top view>

MS1526-E-01

                                            - 32 -

          2014/7

                                                                                                                                      [AK09911]

10.3.   Outline Dimensions

[mm]

1.19±0.03

0.8

3            2              1

1              2            3

8
.
0

0.4

0.4

0.24±0.03

0.57 max.

0.05    C

C

3
0
.
0
±
9
1
.
1

C

B

A

0.40

0.13

10.4.   Recommended Foot Print Pattern

[mm]

3            2              1

0.4

C

B

A

0.4

0.23

MS1526-E-01

                                            - 33 -

          2014/7

                                                                                                                                      [AK09911]

11.   Relationsip between the Magnetic Field and Output Code

The measurement data increases as the magnetic flux density increases in the arrow directions.

MS1526-E-01

                                            - 34 -

          2014/7

                                                                                                                                      [AK09911]

Important Notice

0.  Asahi Kasei Microdevices Corporation (“AKM”) reserves the right to make changes to the information contained in this
document  without  notice.  When  you  consider  any  use  or  application  of  AKM  product  stipulated  in  this  document
(“Product”), please make inquiries the sales office of AKM or authorized distributors as to current status of the Products.
1.  All information included in this document are provided only to illustrate the operation and application examples of AKM
Products.  AKM  neither  makes  warranties  or  representations  with  respect  to  the  accuracy  or  completeness  of  the
information contained in this  document  nor grants any license  to any intellectual property rights or any other rights of
AKM  or  any  third  party  with  respect  to  the  information  in  this  document.  You  are  fully  responsible  for  use  of  such
information contained in this document in your product design or applications. AKM ASSUMES NO LIABILITY FOR
ANY LOSSES INCURRED BY YOU OR THIRD PARTIES ARISING FROM THE USE OF SUCH INFORMATION
IN YOUR PRODUCT DESIGN OR APPLICATIONS.

2.  The Product is neither intended nor warranted for use in equipment or systems that require extraordinarily high levels of
quality  and/or  reliability  and/or  a  malfunction  or  failure  of  which  may  cause  loss  of  human  life,  bodily  injury,  serious
property damage or serious public impact, including but not limited to, equipment used in nuclear facilities, equipment
used in the aerospace industry, medical equipment, equipment used for automobiles, trains, ships and other transportation,
traffic  signaling  equipment,  equipment  used  to  control  combustions  or  explosions,  safety  devices,  elevators  and
escalators,  devices  related  to  electric  power,  and  equipment  used  in  finance-related  fields.  Do  not  use  Product  for  the
above use unless specifically agreed by AKM in writing.

3.  Though AKM works continually to improve the Product’s quality and reliability, you are responsible for complying with
safety  standards  and  for  providing  adequate  designs  and  safeguards  for  your  hardware,  software  and  systems  which
minimize risk and avoid situations in which a malfunction or failure of the Product could cause loss of human life, bodily
injury or damage to property, including data loss or corruption.

4.  Do not use or otherwise make available the Product or related technology or any information contained in this document
for any military purposes, including without limitation, for the design, development, use, stockpiling or manufacturing of
nuclear, chemical, or biological weapons or missile technology products (mass destruction weapons). When exporting the
Products  or  related  technology  or  any  information  contained  in  this  document,  you  should  comply  with  the  applicable
export control laws and regulations and follow the procedures required by such laws and regulations. The Products and
related technology may not be used for or incorporated into any products or systems whose manufacture, use, or sale is
prohibited under any applicable domestic or foreign laws or regulations.

5.  Please contact AKM sales representative for details as to environmental matters such as the RoHS compatibility of the
Product. Please use the Product in compliance with all applicable laws and regulations that regulate the inclusion or use
of controlled substances, including without limitation, the EU RoHS Directive. AKM assumes no liability for damages or
losses occurring as a result of noncompliance with applicable laws and regulations.

6.  Resale  of  the  Product  with  provisions  different  from  the  statement  and/or  technical  features  set  forth  in  this  document
shall  immediately  void  any  warranty  granted  by  AKM  for  the  Product  and  shall  not  create  or  extend  in  any  manner
whatsoever, any liability of AKM.
AKM.

7.  This document may not be reproduced or duplicated, in any form, in whole or in part, without prior written consent of

MS1526-E-01

                                            - 35 -

          2014/7

