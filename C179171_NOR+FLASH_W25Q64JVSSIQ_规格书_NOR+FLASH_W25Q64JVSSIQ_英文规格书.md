W25Q64JV

3V 64M-BIT
SERIAL FLASH MEMORY WITH
DUAL, QUAD SPI

Publication Release Date: March 27, 2018
                                                                                        Revision J

W25Q64JV

1.
2.
3.

4.

5.
6.

7.

Table of Contents

GENERAL DESCRIPTIONS .................................................................................................................. 4
FEATURES ............................................................................................................................................ 4
PACKAGE TYPES AND PIN CONFIGURATIONS ............................................................................... 5
Pin Configuration SOIC 208-mil ................................................................................................ 5
3.1
Pad Configuration WSON 6x5-mm/ 8x6-mm, XSON 4x4-mm .................................................. 5
3.2
Pin Description SOIC 208-mil, WSON 6x5-mm/ 8x6-mm, XSON 4x4-mm ............................... 5
3.3
Pin Configuration SOIC 300-mil ................................................................................................ 6
3.4
Pin Description SOIC 300-mil .................................................................................................... 6
3.5
Ball Configuration TFBGA 8x6-mm (5x5 or 6x4 Ball Array) ...................................................... 7
3.6
Ball Description TFBGA 5x5 or 8x6-mm ................................................................................... 7
3.7
Ball Configuration WLCSP ........................................................................................................ 8
3.8
Ball Description WLCSP12 ........................................................................................................ 8
3.9
PIN DESCRIPTIONS ............................................................................................................................. 9
Chip Select (/CS) ....................................................................................................................... 9
4.1
Serial Data Input, Output and IOs (DI, DO and IO0, IO1, IO2, IO3) ......................................... 9
4.2
4.3  Write Protect (/WP) .................................................................................................................... 9
HOLD (/HOLD) .......................................................................................................................... 9
4.4
Serial Clock (CLK) ..................................................................................................................... 9
4.5
Reset (/RESET)(1) ...................................................................................................................... 9
4.6
BLOCK DIAGRAM ............................................................................................................................... 10
FUNCTIONAL DESCRIPTIONS .......................................................................................................... 11
Standard SPI Instructions ........................................................................................................ 11
6.1
Dual SPI Instructions ............................................................................................................... 11
6.2
Quad SPI Instructions.............................................................................................................. 11
6.3
6.4
Software Reset & Hardware /RESET pin ................................................................................ 11
6.5  Write Protection ....................................................................................................................... 12
  Write Protect Features ............................................................................................................... 12
STATUS AND CONFIGURATION REGISTERS ................................................................................. 13
Status Registers ...................................................................................................................... 13
7.1
  Erase/Write In Progress (BUSY)  –  Status Only ...................................................................... 13
  Write Enable Latch (WEL)  –  Status Only ................................................................................ 13
  Block Protect Bits (BP2, BP1, BP0)  –  Volatile/Non-Volatile Writable ...................................... 13
  Top/Bottom Block Protect (TB)  –  Volatile/Non-Volatile Writable ............................................. 14
  Sector/Block Protect Bit (SEC)  –  Volatile/Non-Volatile Writable ............................................. 14
  Complement Protect (CMP)  –  Volatile/Non-Volatile Writable ................................................. 14
  Status Register Protect (SRP, SRL)  –  Volatile/Non-Volatile Writable ..................................... 15
  Erase/Program Suspend Status (SUS)  –  Status Only ............................................................ 16
  Security Register Lock Bits (LB3, LB2, LB1)  –  Volatile/Non-Volatile OTP Writable ................ 16
  Quad Enable (QE)  –  Volatile/Non-Volatile Writable ................................................................ 16
  Write Protect Selection (WPS)  –  Volatile/Non-Volatile Writable ............................................. 17
  Output Driver Strength (DRV1, DRV0)  –  Volatile/Non-Volatile Writable ................................. 17

Publication Release Date: March 27, 2018
- 1 -                                                                        Revision J

W25Q64JV

8.

8.2

  Reserved Bits  –  Non Functional ............................................................................................. 17
  Status Register Memory Protection (WPS = 0, CMP = 0) .......................................................... 18
  Status Register Memory Protection (WPS = 0, CMP = 1) .......................................................... 19
Individual Block Memory Protection (WPS=1) ......................................................................... 20
INSTRUCTIONS .................................................................................................................................. 21
Device ID and Instruction Set Tables ...................................................................................... 21
8.1
  Manufacturer and Device Identification ...................................................................................... 21
Instruction Set Table 1 (Standard SPI Instructions)(1) ................................................................ 22
Instruction Set Table 2 (Dual/Quad SPI Instructions)(1) .............................................................. 23
Instruction Descriptions ........................................................................................................... 24
  Write Enable (06h) ..................................................................................................................... 24
  Write Enable for Volatile Status Register (50h) .......................................................................... 24
  Write Disable (04h) .................................................................................................................... 25
  Read Status Register-1 (05h), Status Register-2 (35h) & Status Register-3 (15h) .................... 25
  Write Status Register-1 (01h), Status Register-2 (31h) & Status Register-3 (11h) .................... 26
  Read Data (03h) ........................................................................................................................ 28
  Fast Read (0Bh) ........................................................................................................................ 29
  Fast Read Dual Output (3Bh) .................................................................................................... 30
  Fast Read Quad Output (6Bh) ................................................................................................... 31
  Fast Read Dual I/O (BBh) ........................................................................................................ 32
  Fast Read Quad I/O (EBh) ....................................................................................................... 33
  Set Burst with Wrap (77h) ........................................................................................................ 34
  Page Program (02h) ................................................................................................................ 35
  Quad Input Page Program (32h) .............................................................................................. 36
Sector Erase (20h) .................................................................................................................. 37
  32KB Block Erase (52h) ............................................................................................................. 38
  64KB Block Erase (D8h) ............................................................................................................ 39
  Chip Erase (C7h / 60h) .............................................................................................................. 40
  Erase / Program Suspend (75h) ................................................................................................ 41
  Erase / Program Resume (7Ah) ................................................................................................. 42
  Power-down (B9h) ..................................................................................................................... 43
  Release Power-down / Device ID (ABh) .................................................................................... 44
  Read Manufacturer / Device ID (90h) ........................................................................................ 45
  Read Manufacturer / Device ID Dual I/O (92h) .......................................................................... 46
  Read Manufacturer / Device ID Quad I/O (94h) ....................................................................... 47
  Read Unique ID Number (4Bh) ................................................................................................ 48
  Read JEDEC ID (9Fh) ............................................................................................................. 49
  Read SFDP Register (5Ah) ...................................................................................................... 50
  Erase Security Registers (44h) ................................................................................................ 51
  Program Security Registers (42h) ............................................................................................ 52
  Read Security Registers (48h) ................................................................................................. 53
Individual Block/Sector Lock (36h) ........................................................................................... 54
Individual Block/Sector Unlock (39h) ....................................................................................... 55
  Read Block/Sector Lock (3Dh) ................................................................................................. 56
  Global Block/Sector Lock (7Eh) ............................................................................................... 57

8.3

Publication Release Date: March 27, 2018
- 2 -                                                                        Revision J

W25Q64JV

  Global Block/Sector Unlock (98h) ............................................................................................ 57
  Enable Reset (66h) and Reset Device (99h) ........................................................................... 58
ELECTRICAL CHARACTERISTICS.................................................................................................... 59
Absolute Maximum Ratings (1) ............................................................................................... 59
9.1
Operating Ranges ................................................................................................................... 59
9.2
Power-Up Power-Down Timing and Requirements ................................................................ 60
9.3
DC Electrical Characteristics- .................................................................................................. 61
9.4
AC Measurement Conditions .................................................................................................. 62
9.5
AC Electrical Characteristics(6) ................................................................................................ 63
9.6
Serial Output Timing ................................................................................................................ 65
9.7
Serial Input Timing ................................................................................................................... 65
9.8
9.9
/WP Timing .............................................................................................................................. 65
PACKAGE SPECIFICATIONS ............................................................................................................ 66
8-Pin SOIC 208-mil (Package Code SS) ................................................................................. 66
10.1
8-Pad WSON 6x5-mm (Package Code ZP) ............................................................................ 67
10.2
8-Pad WSON 8x6mm (Package Code ZE) ............................................................................. 68
10.3
8-Pad XSON 4x4x0.45-mm (Package Code XG) ................................................................... 69
10.4
16-Pin SOIC 300-mil (Package Code SF) ............................................................................... 70
10.5
24-Ball TFBGA 8x6-mm (Package Code TB, 5x5 Ball Array) ................................................. 71
10.7
10.8
24-Ball TFBGA 8x6-mm (Package Code TC, 6x4 ball array).................................................. 72
12-Ball WLCSP (Package Code BY) ....................................................................................... 73
10.9
ORDERING INFORMATION ............................................................................................................... 74
11.1  Valid Part Numbers and Top Side Marking ............................................................................. 75
REVISION JISTORY............................................................................................................................ 77

9.

10.

11.

12.

Publication Release Date: March 27, 2018
- 3 -                                                                        Revision J

W25Q64JV

1.  GENERAL DESCRIPTIONS
The W25Q64JV (64M-bit) Serial Flash memory provides a storage solution for systems with limited space,
pins and power. The 25Q series offers flexibility and performance well beyond ordinary Serial Flash devices.
They are  ideal for code shadowing to RAM, executing code directly from Dual/Quad SPI (XIP) and storing
voice, text and data. The device operates on 2.7V to 3.6V power supply with current consumption as low as
1µA for power-down. All devices are offered in space-saving packages.

The W25Q64JV array is organized into 32,768 programmable pages of 256-bytes each. Up to 256 bytes can
be programmed at a time. Pages can be erased in groups of 16 (4KB sector erase), groups of 128 (32KB
block erase), groups of 256 (64KB block erase) or the entire chip (chip erase).  The  W25Q64JV has 2,048
erasable sectors and 128 erasable blocks respectively. The small 4KB sectors allow for greater flexibility in
applications that require data and parameter storage. (See Figure 2.)

The W25Q64JV supports the standard Serial Peripheral Interface (SPI), Dual/Quad I/O SPI: Serial Clock, Chip
Select, Serial Data I/O0 (DI), I/O1 (DO), I/O2 and I/O3. SPI clock frequencies of W25Q64JV of up to 133MHz
are supported allowing equivalent clock rates of 266MHz (133MHz x 2) for Dual I/O and 532MHz (133MHz x
4)  for  Quad  I/O  when  using  the  Fast  Read  Dual/Quad  I/O.  These  transfer  rates  can  outperform  standard
Asynchronous 8 and 16-bit Parallel Flash memories.

Additionally,  the  device supports JEDEC standard manufacturer  and device ID, and  a 64-bit  Unique Serial
Number and three 256-bytes Security Registers.

2.  FEATURES

  New Family of SpiFlash Memories

  Advanced Security Features

– W25Q64JV: 64M-bit / 8M-byte
– Standard SPI: CLK, /CS, DI, DO
– Dual SPI: CLK, /CS, IO0, IO1
– Quad SPI: CLK, /CS, IO0, IO1, IO2, IO3
– Software & Hardware Reset(1)

  Highest Performance Serial Flash
– 133MHz Single, Dual/Quad SPI clocks
– 266/532MHz equivalent Dual/Quad SPI
– Min. 100K Program-Erase cycles per sector
– More than 20-year data retention

  Low Power, Wide Temperature Range

– Single 2.7 to 3.6V supply
– <1µA Power-down (typ.)
– -40°C to +85°C operating range
– -40°C to +105°C operating  range

  Flexible Architecture with 4KB sectors

– Uniform Sector/Block Erase (4K/32K/64K-Byte)
– Program 1 to 256 byte per programmable page
– Erase/Program Suspend & Resume

– Software and Hardware Write-Protect
– Special OTP protection
– Top/Bottom, Complement array protection
– Individual Block/Sector array protection
– 64-Bit Unique ID for each device
– Discoverable Parameters (SFDP) Register
– 3X256-Bytes Security Registers
– Volatile & Non-volatile Status Register Bits

  Space Efficient Packaging

– 8-pin SOIC 208-mil
– 8-pad WSON 6x5-mm/8x6-mm
– 16-pin SOIC 300-mil
– 8-pad XSON 4x4-mm
– 24-ball TFBGA 8x6-mm (6x4 ball array)
– 24-ball TFBGA 8x6-mm (6x4/5x5 ball array)
– 12-ball WLCSP
– Contact Winbond for KGD and other options

  Note: 1. Hardware /RESET pin is only available on TFBGA or SOIC16 packages

Publication Release Date: March 27, 2018
- 4 -                                                                        Revision J

3.  PACKAGE TYPES AND PIN CONFIGURATIONS

3.1  Pin Configuration SOIC 208-mil

W25Q64JV

Figure 1a. W25Q64JV Pin Assignments, 8-pin SOIC 208-mil (Package Code SS)

3.2  Pad Configuration WSON 6x5-mm/ 8x6-mm, XSON 4x4-mm

Figure 1b. W25Q64JV Pad Assignments, 8-pad WSON 6x5-mm/8x6 (Package Code ZP, ZE)

3.3  Pin Description SOIC 208-mil, WSON 6x5-mm/ 8x6-mm, XSON 4x4-mm

PAD NO.

PAD NAME

/CS

DO (IO1)

/WP (IO2)

GND

DI (IO0)

CLK

/HOLD or /RESET
(IO3)

I/O

I

I/O

I/O

I/O

I

I/O

FUNCTION

Chip Select Input

Data Output (Data Input Output 1)(1)

Write Protect Input ( Data Input Output 2)(2)

Ground

Data Input (Data Input Output 0)(1)

Serial Clock Input

Hold or Reset Input (Data Input Output 3)(2)

VCC

Power Supply

1

2

3

4

5

6

7

8

Notes:
1.
2.

IO0 and IO1 are used for Standard and Dual SPI instructions
IO0 – IO3 are used for Quad SPI instructions, /HOLD (or /RESET) function is only available for Standard/Dual SPI.

Publication Release Date: March 27, 2018
- 5 -                                                                        Revision J

12348765/CSDO (IO1)/WP (IO2)GNDVCC/HOLD or /RESET(IO3)DI (IO0)CLKTop View  1234/CSDO (IO1)/WP (IO2)GNDVCC/HOLD or /RESET(IO3)DI (IO0)CLKTop View  8765

3.4  Pin Configuration SOIC 300-mil

W25Q64JV

Figure 1c. W25Q64JV Pin Assignments, 16-pin SOIC 300-mil (Package Code SF)

3.5  Pin Description SOIC 300-mil

PIN NO.

PIN NAME

/HOLD or
/RESET (IO3)

VCC

/RESET

N/C

N/C

N/C

/CS

DO (IO1)

/WP (IO2)

GND

N/C

N/C

N/C

N/C

I/O

I/O

FUNCTION

Hold or Reset Input (Data Input Output 3)(2)

Power Supply

I

Reset Input(3)

No Connect

No Connect

No Connect

Chip Select Input

Data Output (Data Input Output 1)(1)

Write Protect Input (Data Input Output 2)(2)

I

I/O

I/O

Ground

No Connect

No Connect

No Connect

No Connect

DI (IO0)

CLK

I/O

I

Data Input (Data Input Output 0)(1)

Serial Clock Input

1

2

3

4

5

6

7

8

9

10

11

12

13

14

15

16

Notes:

1. IO0 and IO1 are used for Standard and Dual SPI instructions.
2. IO0 – IO3 are used for Quad SPI instructions, /HOLD (or /RESET) function is only available for Standard/Dual SPI.
3. The /RESET pin is a dedicated hardware reset pin regardless of device settings or operation states. If the hardware reset function

is not used, this pin can be left floating or connected to VCC in the system.

Publication Release Date: March 27, 2018
- 6 -                                                                        Revision J

1234/CSDO (IO1)IO2GNDVCCIO3DI (IO0)CLKTop View  NC/RESETNCNCNCNCNCNC5678109111213141516

3.6  Ball Configuration TFBGA 8x6-mm (5x5 or 6x4 Ball Array)

W25Q64JV

Figure 1d. W25Q64JV Ball Assignments, 24-ball TFBGA 8x6-mm (Package Code TB/TC)

3.7  Ball Description TFBGA 5x5 or 8x6-mm

BALL NO.

PIN NAME

I/O

FUNCTION

A4

B2

B3

B4

C2

C4

D2

D3

D4

/RESET

CLK

GND

VCC

/CS

/WP (IO2)

DO (IO1)

DI (IO0)

/HOLD (IO3)

I

I

Reset Input(3)

Serial Clock Input

Ground

Power Supply

Chip Select Input

Write Protect Input (Data Input Output 2)(2)

Data Output (Data Input Output 1)(1)

Data Input (Data Input Output 0)(1)

Hold or Reset Input (Data Input Output 3)(2)

I

I/O

I/O

I/O

I/O

Multiple

NC

No Connect

IO0 and IO1 are used for Standard and Dual SPI instructions
IO0 – IO3 are used for Quad SPI instructions, /HOLD (or /RESET) function is only available for Standard/Dual SPI.

Notes:
1.
2.
  3.    The /RESET pin is a dedicated hardware reset pin regardless of device settings or operation states.
          If the hardware reset function is not used, this pin can be left floating or connected to VCC in the system

Publication Release Date: March 27, 2018
- 7 -                                                                        Revision J

D1/HOLD(IO3)DI(IO0)DO(IO1)/WP (IO2)D2D3D4NCE1NCNCNCE2E3E4NCF1NCNCNCF2F3F4NCA1/RESETNCNCA2A3A4NCB1VCCGNDCLKB2B3B4NCC1NC/CSC2C3C4NCTop ViewPackage Code CTop ViewD1/HOLD(IO3)DI(IO0)DO(IO1)/WP (IO2)D2D3D4NCE1NCNCNCE2E3E4NC/RESETNCNCA2A3A4B1VCCGNDCLKB2B3B4NCC1NC/CSC2C3C4NCD5E5A5B5C5NCNCNCNCNCPackage Code B

3.8  Ball Configuration WLCSP

W25Q64JV

Figure 1e. W25Q64JV Ball Assignments, 12-ball WLCSP (Package Code BY)

3.9  Ball Description WLCSP12

BALL NO.

PIN NAME

I/O

FUNCTION

B2

B3

C2

C3

D2

D3

E2

E3

VCC

/CS

Power Supply

I

Chip Select Input

/HOLD or /RESET
(IO3)

DO (IO1)

CLK

/WP (IO2)

DI (IO0)

GND

I/O

I/O

I

I/O

I/O

Hold Input or /RESET (Data Input Output 3)(2)

Data Output (Data Input Output 1)(1)

Serial Clock Input

Write Protect Input (Data Input Output 2)(2)

Data Input (Data Input Output 0)(1)

Ground

Notes:
1.
2.

IO0 and IO1 are used for Standard and Dual SPI instructions
IO0 – IO3 are used for Quad SPI instructions, /HOLD (or /RESET) function is only available for Standard/Dual SPI.

Publication Release Date: March 27, 2018
- 8 -                                                                        Revision J

W25Q64JV

4.  PIN DESCRIPTIONS

4.1  Chip Select (/CS)

The  SPI  Chip  Select  (/CS)  pin  enables  and  disables  device  operation.  When  /CS  is  high  the  device  is
deselected  and  the  Serial  Data  Output  (DO,  or  IO0,  IO1,  IO2,  IO3)  pins  are  at  high  impedance.  When
deselected, the devices power consumption will be at standby levels unless an internal erase, program or
write  status  register  cycle  is  in  progress.  When  /CS  is  brought  low  the  device  will  be  selected,  power
consumption will increase to active levels and instructions can be written to and data read from the device.
After power-up, /CS must transition from high to low before a new instruction will be accepted. The /CS input
must track the VCC supply level at  power-up and  power-down (see “Write Protection” and Figure 58). If
needed a pull-up resister on the /CS pin can be used to accomplish this.

4.2  Serial Data Input, Output and IOs (DI, DO and IO0, IO1, IO2, IO3)

The W25Q64JV supports standard SPI, Dual SPI and Quad SPI operation. Standard SPI instructions use
the unidirectional DI (input) pin to serially write instructions, addresses or data to the device on the rising
edge of the Serial Clock (CLK) input pin. Standard SPI also uses the unidirectional DO (output) to read data
or status from the device on the falling edge of CLK.

Dual and Quad SPI instructions use the bidirectional IO pins to serially write instructions, addresses or data
to the device on the rising edge of CLK and read data or status from the device on the falling edge of CLK.
Quad SPI instructions require the non-volatile Quad Enable bit (QE) in Status Register-2 to be set. When
QE=1, the /WP pin becomes IO2 and the /HOLD pin becomes IO3.

4.3  Write Protect (/WP)

The  Write  Protect  (/WP)  pin  can  be  used  to  prevent  the  Status  Register  from  being  written.  Used  in
conjunction with the Status Register’s Block Protect (CMP, SEC, TB, BP2, BP1 and BP0) bits and Status
Register Protect (SRP) bits, a portion as small as a 4KB sector or the entire memory array can be hardware
protected. The /WP pin is active low.

4.4  HOLD (/HOLD)

The /HOLD pin allows the device to be paused while it is actively selected. When /HOLD is brought low,
while /CS is low, the DO pin will be at high impedance and signals on the DI and CLK pins will be ignored
(don’t care). When /HOLD is brought high, device operation can resume. The /HOLD function can be useful
when multiple devices are sharing the same SPI signals. The /HOLD pin is active low. When the QE bit of
Status Register-2 is set for Quad I/O, the /HOLD pin function is not available since this pin is used for IO3.
See Figure 1a-c for the pin configuration of Quad I/O operation.

4.5  Serial Clock (CLK)

The SPI Serial Clock Input (CLK) pin provides the timing for serial input and output operations. ("See SPI
Operations")

4.6  Reset (/RESET)(1)

A dedicated hardware /RESET pin is available on SOIC-16 and TFBGA packages. When it’s driven low for
a minimum period of ~1µS, this device will terminate any external or internal operations and return to its
power-on state.

Note:

1. Hardware /RESET pin is available on SOIC-16 or TFBGA; please contact Winbond for this package.

Publication Release Date: March 27, 2018
- 9 -                                                                        Revision J

5.  BLOCK DIAGRAM

W25Q64JV

    Figure 2. W25Q64JV Serial Flash Memory Block Diagram

Publication Release Date: March 27, 2018
- 10 -                                                                        Revision J

 003000h                                                0030FFh002000h                                                0020FFh001000h                                                0010FFhColumn DecodeAnd 256-Byte Page BufferBeginningPage AddressEndingPage AddressW25Q64FVSPICommand &Control LogicByte AddressLatch / CounterStatusRegisterWrite ControlLogicPage AddressLatch / CounterDO (IO1)DI (IO0)/CSCLK/HOLD (IO3)/WP (IO2)High VoltageGeneratorsxx0F00h                                    xx0FFFh•           Sector 0 (4KB)            •xx0000h                                    xx00FFhxx1F00h                                    xx1FFFh•           Sector 1 (4KB)            •xx1000h                                    xx10FFhxx2F00h                                    xx2FFFh•           Sector 2 (4KB)            •xx2000h                                    xx20FFh•••xxDF00h                                    xxDFFFh•           Sector 13 (4KB)            •xxD000h                                    xxD0FFhxxEF00h                                    xxEFFFh•           Sector 14 (4KB)            •xxE000h                                    xxE0FFhxxFF00h                                    xxFFFFh•           Sector 15 (4KB)            •xxF000h                                    xxF0FFhBlock SegmentationDataSecurity Register 1 -3Write Protect Logic and Row Decode000000h                                   0000FFhSFDP Register00FF00h                                  00FFFFh•            Block 0 (64KB)             •000000h                                   0000FFh•••1FFF00h                                  1FFFFFh•           Block 31 (64KB)            •1F0000h                                   1F00FFh20FF00h                                  20FFFFh•           Block 32 (64KB)            •200000h                                   2000FFh•••3FFF00h                                  3FFFFFh•           Block 63 (64KB)            •3F0000h                                   3F00FFh40FF00h                                  40FFFFh•           Block 64 (64KB)            •400000h                                   4000FFh•••7FFF00h                                  7FFFFFh•           Block 127 (64KB)            •7F0000h                                   7F00FFhW25Q64JV

W25Q64JV

6.  FUNCTIONAL DESCRIPTIONS

6.1  Standard SPI Instructions

The W25Q64JV is accessed through an SPI compatible bus consisting of four signals: Serial Clock (CLK),
Chip Select (/CS), Serial Data Input (DI) and Serial Data Output (DO). Standard SPI instructions use the DI
input pin to serially write instructions, addresses or data to the device on the rising edge of CLK. The DO
output pin is used to read data or status from the device on the falling edge of CLK.

SPI bus operation Mode 0 (0,0) and 3 (1,1) are supported. The primary difference between Mode 0 and
Mode 3 concerns the normal state of the CLK signal when the SPI bus master is in standby and data is not
being transferred to the Serial Flash. For Mode 0, the CLK signal is normally low on the falling and rising
edges of /CS. For Mode 3, the CLK signal is normally high on the falling and rising edges of /CS.

6.2  Dual SPI Instructions

The  W25Q64JV  supports  Dual  SPI  operation  when  using  instructions  such  as  “Fast  Read  Dual  Output
(3Bh)” and “Fast Read Dual I/O (BBh)”. These instructions allow data to be transferred to or from the device
at two to three times the rate of ordinary Serial Flash devices. The Dual SPI Read instructions are ideal for
quickly  downloading  code  to  RAM  upon  power-up  (code-shadowing)  or  for  executing  non-speed-critical
code  directly  from  the  SPI  bus  (XIP).  When  using  Dual  SPI  instructions,  the  DI  and  DO  pins  become
bidirectional I/O pins: IO0 and IO1.

6.3  Quad SPI Instructions

The W25Q64JV supports Quad SPI operation when using instructions such as “Fast Read  Quad Output
(6Bh)”, and “Fast Read Quad I/O (EBh). These instructions allow data to be transferred to or from the device
four to six times the rate of ordinary Serial Flash. The Quad Read instructions offer a significant improvement
in continuous and random access transfer rates allowing fast code-shadowing to RAM or execution directly
from the SPI bus (XIP). When using Quad SPI instructions the DI and DO pins become bidirectional IO0
and IO1, with the additional I/O pins: IO2, IO3. Quad SPI instructions require the non-volatile Quad Enable
bit (QE) in Status Register-2 to be set.

6.4  Software Reset & Hardware /RESET pin

The W25Q64JV can be reset to the initial power-on state by a software Reset sequence. This sequence
must include two consecutive instructions: Enable Reset (66h) & Reset (99h). If the instruction sequence is
successfully  accepted,  the  device  will  take  approximately  30µS  (tRST)  to  reset.  No  instruction  will  be
accepted during the reset period. For the SOIC-16 and TFBGA packages, W25Q64JV provides a dedicated
hardware /RESET pin. Drive the /RESET pin low for a minimum period of ~1µS (tRESET*) will interrupt any
on-going external/internal operations and reset the device to its initial power-on state. Hardware /RESET
pin has higher priority than other SPI input signals (/CS, CLK, IOs).

Note:
1.  Hardware /RESET pin is available on SOIC-16 or TFBGA; please contact Winbond for his package.
2.  While a faster /RESET pulse (as short as a few hundred nanoseconds) will often reset the device, a 1us minimum is recommended

to ensure reliable operation.

3.  There is an internal pull-up resistor for the dedicated /RESET pin on the SOIC-16 and TFBGA-24 package. If the reset function is

not needed, this pin can be left floating in the system.

Publication Release Date: March 27, 2018
- 11 -                                                                        Revision J

W25Q64JV

6.5 Write Protection

Applications that use non-volatile memory must take into consideration the possibility of noise and other
adverse system conditions that may compromise data integrity. To address this concern, the W 25Q64JV
provides several means to protect the data from inadvertent writes.

 Write Protect Features

  Device resets when VCC is below threshold
  Time delay write disable after Power-up
  Write enable/disable instructions and automatic write disable after erase or program
  Software and Hardware (/WP pin) write protection using Status Registers
  Additional Individual Block/Sector Locks for array protection
  Write Protection using Power-down instruction
  Lock Down write protection for Status Register until the next power-up
  One Time Program (OTP) write protection for array and Security Registers using Status Register*

* Note: This feature is available upon special flow. Please contact Winbond for details.

Upon power-up or at power-down, the W25Q64JV will maintain a reset condition while VCC is below the
threshold value of VWI, (See Power-up Timing and Voltage Levels and Figure 43). While reset, all operations
are disabled and no instructions are recognized. During power-up and after the VCC voltage exceeds VWI,
all program and erase related instructions are further disabled for a time delay of tPUW. This includes the
Write  Enable,  Page  Program,  Sector  Erase,  Block  Erase,  Chip  Erase  and  the  Write  Status  Register
instructions. Note that the chip select pin (/CS) must track the VCC supply level at power-up until the VCC-
min level and  tVSL time delay  is reached, and  it must also track the  VCC supply  level at  power-down  to
prevent adverse command sequence. If needed a pull-up resister on /CS can be used to accomplish this.
After power-up the device is automatically placed in a write-disabled state with the Status Register Write
Enable Latch (WEL) set to a 0. A Write Enable instruction must be issued before a Page Program, Sector
Erase, Block Erase, Chip Erase or Write Status Register instruction will be accepted. After completing a
program, erase or write instruction the Write Enable Latch (WEL) is automatically cleared to a write-disabled
state of 0.
Software controlled write protection is facilitated using the Write Status Register instruction and setting the
Status  Register  Protect  (SRP,  SRL)  and  Block  Protect  (CMP,  TB,  BP[3:0])  bits.  These  settings  allow  a
portion or the entire memory array to be configured as read only. Used in conjunction with the Write Protect
(/WP) pin, changes to the Status Register can be enabled or disabled under hardware control. See Status
Register section for further information. Additionally, the Power-down instruction offers an extra level of write
protection as all instructions are ignored except for the Release Power-down instruction.

The W25Q64JV also provides another Write Protect method using the Individual Block Locks. Each 64KB
block (except the top and bottom blocks, total of  126 blocks) and each 4KB sector within the top/bottom
blocks  (total  of  32  sectors)  are  equipped  with  an  Individual  Block  Lock  bit.  When  the  lock  bit  is  0,  the
corresponding sector or block can be erased or programmed; when the lock bit is set to 1, Erase or Program
commands issued to the corresponding sector or block will be ignored. When the device is powered on, all
Individual  Block  Lock  bits  will  be  1,  so  the  entire  memory  array  is  protected  from  Erase/Program.  An
“Individual Block Unlock (39h)” instruction must be issued to unlock any specific sector or block.
The WPS bit in Status Register-3  is used to decide  which Write Protect scheme should be used. When
WPS=0 (factory default), the device will only utilize CMP, SEC, TB, BP[2:0] bits to protect specific areas of
the array; when WPS=1, the device will utilize the Individual Block Locks for write protection.

Publication Release Date: March 27, 2018
- 12 -                                                                        Revision J

W25Q64JV

7.  STATUS AND CONFIGURATION REGISTERS
Three Status and Configuration Registers are provided for W25Q64JV. The Read Status Register-1/2/3
instructions can be used to provide status on the availability of the flash memory array, whether the device
is write enabled or disabled, the state of write protection, Quad SPI setting, Security Register lock status,
Erase/Program Suspend status, output driver strength, power-up. The Write Status Register instruction
can be used to configure the device write protection features, Quad SPI setting, Security Register OTP
locks, and output driver strength. Write access to the Status Register is controlled by the state of the non-
volatile Status Register Protect bits (SRL), the Write Enable instruction, and during Standard/Dual SPI
operations

7.1  Status Registers

S 7

S 6

S 5

S 4

S 3

S 2

S 1

S 0

SRP  SEC  TB

BP 2  BP 1  BP 0  WEL  BUSY

STATUS REGISTER PROTECT
(volatile/non-volatile)
SECTOR PROTECT
(volatile/non-volatile)
TOP/BOTTOM PROTECT
(volatile/non-volatile)
BLOCK PROTECT BITS
(volatile/non-volatile)
WRITE ENABLE LATCH

WRITE IN PROGRESS

 Erase/Write In Progress (BUSY) – Status Only

Figure 4a. Status Register-1

BUSY is a read only bit in the status register (S0) that is set to a 1 state when the device is executing a
Page  Program,  Quad  Page  Program,  Sector  Erase,  Block  Erase,  Chip  Erase,  Write  Status  Register  or
Erase/Program  Security  Register  instruction.  During  this  time  the  device  will  ignore  further  instructions
except for the Read Status Register and Erase/Program Suspend instruction (see tW, tPP, tSE, tBE, and tCE
in AC Characteristics). When the program, erase or write status/security register instruction has completed,
the BUSY bit will be cleared to a 0 state indicating the device is ready for further instructions.

 Write Enable Latch (WEL) – Status Only

Write Enable Latch (WEL) is a read only bit in the status register (S1) that is set to 1 after executing a Write
Enable Instruction. The WEL status bit is cleared to 0 when the device is write disabled. A write disable
state occurs upon power-up or after any of the following instructions: Write Disable, Page Program, Quad
Page Program, Sector Erase, Block Erase, Chip Erase, Write Status Register, Erase Security Register and
Program Security Register.

 Block Protect Bits (BP2, BP1, BP0) – Volatile/Non-Volatile Writable

The Block Protect Bits (BP2, BP1, BP0) are non-volatile read/write bits in the status register (S4, S3, and
S2) that provide Write Protection control and status. Block Protect bits can be set using the Write Status
Register  Instruction  (see  tW  in  AC  characteristics).  All,  none  or  a  portion  of  the  memory  array  can  be
protected from Program and Erase instructions (see Status Register Memory Protection table). The factory
default setting for the Block Protection Bits is 0, none of the array protected.

Publication Release Date: March 27, 2018
- 13 -                                                                        Revision J

W25Q64JV

 Top/Bottom Block Protect (TB) – Volatile/Non-Volatile Writable

The non-volatile Top/Bottom bit (TB) controls if the Block Protect Bits (BP2, BP1, BP0) protect from the Top
(TB=0) or the Bottom (TB=1) of the array as shown in the Status Register Memory Protection table. The
factory default setting is TB=0. The TB bit can be set with the Write Status Register Instruction depending
on the state of the SRP/SRL and WEL bits.

 Sector/Block Protect Bit (SEC) – Volatile/Non-Volatile Writable

The non-volatile Sector/Block Protect bit (SEC) controls if the Block Protect Bits (BP2, BP1, BP0) protect
either 4KB Sectors (SEC=1) or 64KB Blocks (SEC=0) in the Top (TB=0) or the Bottom (TB=1) of the array
as shown in the Status Register Memory Protection table. The default setting is SEC=0.

 Complement Protect (CMP) – Volatile/Non-Volatile Writable

The Complement Protect bit (CMP) is a non-volatile read/write bit in the status register (S14). It is used in
conjunction with SEC, TB, BP2, BP1 and BP0 bits to provide more flexibility for the array protection. Once
CMP is set to 1, previous array protection set by SEC, TB, BP2, BP1 and BP0 will be reversed. For instance,
when CMP=0, a top 64KB block can be protected while the rest of the array is not; when CMP=1, the top
64KB block will become unprotected while the rest of the array become read-only. Please refer to the Status
Register Memory Protection table for details. The default setting is CMP=0.

Publication Release Date: March 27, 2018
- 14 -                                                                        Revision J

W25Q64JV

 Status Register Protect (SRP, SRL) – Volatile/Non-Volatile Writable

Three  Status  and  Configuration  Registers  are  provided  for W25Q64JV.  The  Read  Status  Register-1/2/3
instructions can be used to provide status on the availability of the flash memory array, whether the device
is write enabled or disabled, the state of write protection, Quad SPI setting, Security Register lock status,
Erase/Program  Suspend  status,  and  output  driver  strength,  The Write  Status Register  instruction can  be
used  to  configure  the  device  write  protection  features,  Quad  SPI  setting,    Security  Register  OTP  locks,
output driver. Write access to the Status Register is controlled by the state of the non-volatile Status Register
Protect bits (SRP, SRL), the Write Enable instruction, and during Standard/Dual SPI operations, the /WP
pin.

SRL

SRP

/WP

Status
Register

Description

0

0

0

1

1

0

1

1

X

X

X

0

1

X

X

Software
Protection

/WP  pin  has  no  control.  The  Status  register  can  be  written  to
after a Write Enable instruction, WEL=1. [Factory Default]

Hardware
Protected

When /WP pin is low the Status Register locked and cannot be
written to.

Hardware
Unprotected

When /WP pin is high the Status register is unlocked and can be
written to after a Write Enable instruction, WEL=1.

Power Supply
Lock-Down

Status Register is protected and cannot be written to again until
the next power-down, power-up cycle.(1)

One Time
Program(2)

Status Register is permanently protected and cannot be written
to. (enabled by adding prefix command AAh, 55h)

1.    When SRL =1, a power-down, power-up cycle will change SRL =0 state.
2.    Please contact Winbond for details regarding the special instruction sequence.

Publication Release Date: March 27, 2018
- 15 -                                                                        Revision J

W25Q64JV

Figure 4b. Status Register-2

 Erase/Program Suspend Status (SUS) – Status Only

The  Suspend  Status  bit  is  a  read  only  bit  in  the  status  register  (S15)  that  is  set  to  1  after  executing  a
Erase/Program Suspend (75h) instruction. The SUS status bit is cleared to 0 by Erase/Program Resume
(7Ah) instruction as well as a power-down, power-up cycle.

 Security Register Lock Bits (LB3, LB2, LB1) – Volatile/Non-Volatile OTP Writable

The Security Register Lock Bits (LB3, LB2, LB1) are non-volatile One Time Program (OTP) bits in Status
Register (S13, S12, S11) that provide the write protect control and status to the Security Registers. The
default state of LB3-1 is 0, Security Registers are unlocked.  LB3-1 can be set to 1 individually using the
Write  Status  Register  instruction.  LB3-1  are  One  Time  Programmable  (OTP),  once  it’s  set  to  1,  the
corresponding 256-Byte Security Register will become read-only permanently.

 Quad Enable (QE) – Volatile/Non-Volatile Writable

The Quad Enable (QE) bit is a non-volatile read/write bit in the status register (S9) that enables Quad SPI
operation. When the QE bit is set to a 0 state (factory default for part numbers with ordering options “IM”
&“JM”), the /HOLD are enabled, the device operates in Standard/Dual SPI modes. When the QE bit is set
to a 1 (factory fixed default for part numbers with ordering options “IQ” & “JQ”), the Quad IO2 and IO3 pins
are enabled, and /HOLD function is disabled, the device operates in Standard/Dual/Quad SPI modes.

Publication Release Date: March 27, 2018
- 16 -                                                                        Revision J

 S15S14S13S12S11S10S9S8SUSCMPLB3LB2LB1(R)QESUSPEND STATUS(Status-Only)COMPLEMENT PROTECT(Volatile/Non-Volatile Writable)SECURITY REGISTER LOCK BITS(Volatile/Non-Volatile Writable)S15S14S13S12S11S10S9SUSCMPLB3LB2LB1SRL QUAD ENABLE(Volatile/Non-Volatile Writable)STATUS REGISTER Lock(Volatile/Non-Volatile Writable)Reserved

W25Q64JV

Figure 4c. Status Register-3

 Write Protect Selection (WPS) – Volatile/Non-Volatile Writable

The WPS bit is used to select which Write Protect scheme should be used. When WPS=0, the device will
use the combination of CMP, SEC, TB, BP[2:0] bits to protect a specific area of the memory array. When
WPS=1,  the  device  will  utilize  the  Individual  Block  Locks  to  protect  any  individual  sector  or  blocks.  The
default value for all Individual Block Lock bits is 1 upon device power on or after reset.

 Output Driver Strength (DRV1, DRV0) – Volatile/Non-Volatile Writable

The DRV1 & DRV0 bits are used to determine the output driver strength for the Read operations.

DRV1, DRV0

Driver Strength

0, 0

0, 1

1, 0

1, 1

100%

75%

50%

25% (default)

 Reserved Bits – Non Functional

There are a few reserved Status Register bits that may be read out as a “0” or “1”. It is  recommended to
ignore the values of those bits. During a “Write Status Register” instruction, the Reserved Bits can be written
as “0”, but there will not be any effects.

Publication Release Date: March 27, 2018
- 17 -                                                                        Revision J

W25Q64JV

  Status Register Memory Protection (WPS = 0, CMP = 0)

STATUS REGISTER(1)

W25Q64JV (64M-BIT) MEMORY PROTECTION(3)

SEC  TB  BP2  BP1  BP0

PROTECTED
BLOCK(S)

PROTECTED
ADDRESSES

PROTECTED
DENSITY

PROTECTED
PORTION(2)

0

0

0

0

1

1

1

0

0

0

1

1

1

1

0

0

0

1

0

0

0

1

X

X

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

0

0

1

1

1

1

1

1

X

X

1

1

1

1

1

1

1

1

0

0

0

0

1

1

1

1

Notes:

0

0

1

1

0

0

1

0

1

1

0

0

1

1

0

1

1

0

0

1

1

0

0

1

0

1

0

1

0

1

0

1

0

1

0

1

1

0

1

X

1

0

1

X

NONE

NONE

NONE

NONE

126 and 127  7E0000h – 7FFFFFh

128KB

Upper 1/64

124 thru 127  7C0000h – 7FFFFFh

256KB

Upper 1/32

120 thru 127  780000h – 7FFFFFh

512KB

Upper 1/16

112 thru 127  700000h – 7FFFFFh

96 thru 127

600000h – 7FFFFFh

64 thru 127

400000h – 7FFFFFh

1MB

2MB

4MB

Upper 1/8

Upper 1/4

Upper 1/2

0 and 1

000000h – 01FFFFh

128KB

Lower 1/64

0 thru 3

000000h – 03FFFFh

256KB

Lower 1/32

0 thru 7

000000h – 07FFFFh

512KB

Lower 1/16

0 thru 15

000000h – 0FFFFFh

0 thru 31

000000h – 1FFFFFh

0 thru 63

000000h – 3FFFFFh

0 thru 127

000000h – 7FFFFFh

7FF000h – 7FFFFFh

7FE000h – 7FFFFFh

1MB

2MB

4MB

8MB

4KB

8KB

Lower 1/8

Lower 1/4

Lower 1/2

ALL

U – 1/2048

U – 1/1024

7FC000h – 7FFFFFh

16KB

U – 1/512

7F8000h – 7FFFFFh

32KB

U – 1/256

000000h – 000FFFh

000000h – 001FFFh

000000h – 003FFFh

000000h – 007FFFh

4KB

8KB

16KB

32KB

L – 1/2048

L – 1/1024

L – 1/512

L – 1/256

127

127

127

127

0

0

0

0

1.  X = don’t care
2.  L = Lower; U = Upper
3.  If any Erase or Program command specifies a memory region that contains protected data portion, this command

will be ignored.

Publication Release Date: March 27, 2018
- 18 -                                                                        Revision J

W25Q64JV

 Status Register Memory Protection (WPS = 0, CMP = 1)

STATUS REGISTER(1)

W25Q64JV (64M-BIT) MEMORY PROTECTION(3)

SEC

TB  BP2  BP1  BP0

PROTECTED
BLOCK(S)

PROTECTED
ADDRESSES

PROTECTED
DENSITY

PROTECTED
PORTION(2)

0

0

0

0

1

1

1

0

0

0

1

1

1

1

0

0

0

1

0

0

0

1

X

X

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

0

0

1

1

1

1

1

1

X

X

1

1

1

1

1

1

1

1

0

0

0

0

1

1

1

1

Notes:

0

0

1

1

0

0

1

0

1

1

0

0

1

1

0

1

1

0

0

1

1

0

0

1

0

1

0

1

0

1

0

1

0

1

0

1

1

0

1

X

1

0

1

X

0 thru 127

000000h – 7FFFFFh

8MB

ALL

0 thru 125

000000h – 7DFFFFh

8,064KB

Lower 63/64

0 thru 123

000000h – 7BFFFFh

7,936KB

Lower 31/32

0 thru 119

000000h – 77FFFFh

7,680KB

Lower 15/16

0 thru 111

000000h – 6FFFFFh

0 thru 95

000000h – 5FFFFFh

0 thru 63

000000h – 3FFFFFh

7MB

5MB

4MB

Lower 7/8

Lower 3/4

Lower 1/2

2 thru 127

020000h – 7FFFFFh

8,064KB

Upper 63/64

4 thru 127

040000h – 7FFFFFh

7,936KB

Upper 31/32

8 thru 127

080000h – 7FFFFFh

7,680KB

Upper 15/16

16 thru 127

100000h – 7FFFFFh

32 thru 127

200000h – 7FFFFFh

64 thru 127

400000h – 7FFFFFh

7MB

5MB

4MB

Upper 7/8

Upper 3/4

Upper 1/2

NONE

NONE

NONE

NONE

0 thru 127

000000h – 7FEFFFh

8,188KB

L – 2047/2048

0 thru 127

000000h – 7FDFFFh

8,184KB

L – 1023/1024

0 thru 127

000000h – 7FBFFFh

8,176KB

L – 511/512

0 thru 127

000000h – 7F7FFFh

8,160KB

L – 255/256

0 thru 127

001000h – 7FFFFFh

8,188KB

L – 2047/2048

0 thru 127

002000h – 7FFFFFh

8,184KB

L – 1023/1024

0 thru 127

004000h – 7FFFFFh

8,176KB

L – 511/512

0 thru 127

008000h – 7FFFFFh

8,160KB

L – 255/256

1.  X = don’t care
2.  L = Lower; U = Upper
3.

If any Erase or Program command specifies a memory region that contains protected data portion, this command
will be ignored.

Publication Release Date: March 27, 2018
- 19 -                                                                        Revision J

Individual Block Memory Protection (WPS=1)

W25Q64JV

Figure 4d. Individual Block/Sector Locks

Notes:
1.
2.  All individual block/sector lock bits are set to 1 by default after power up, all memory array is protected.

Individual Block/Sector protection is only valid when WPS=1.

Publication Release Date: March 27, 2018
- 20 -                                                                        Revision J

Sector 0 (4KB)Sector 1 (4KB)Sector 14 (4KB)Sector 15 (4KB)Block 1 (64KB)Block 126 (64KB)Sector 0 (4KB)Sector 1 (4KB)Sector 14 (4KB)Sector 15 (4KB)Block 0 (64KB)Block 127 (64KB)Individual Block Locks:32 Sectors (Top/Bottom)126 BlocksIndividual Block Lock: 36h + AddressIndividual Block Unlock: 39h + AddressRead Block Lock:3Dh + AddressGlobal Block Lock:7EhGlobal Block Unlock:98h

W25Q64JV

8.

INSTRUCTIONS

The Standard/Dual/Quad SPI instruction set of the W25Q64JV consists of 48 basic instructions that are fully
controlled through the SPI bus (see Instruction Set Table1-2). Instructions are initiated with the falling edge
of Chip Select (/CS). The first byte of data clocked into the DI input provides the instruction code. Data on
the DI input is sampled on the rising edge of clock with most significant bit (MSB) first.

Instructions vary in length from a single byte to several bytes and may be followed by address bytes, data
bytes, dummy bytes (don’t care), and in some cases, a combination. Instructions are completed with the
rising edge of edge /CS. Clock relative timing diagrams for each instruction are included in Figures 5 through
57. All read instructions can be completed after any clocked bit. However, all instructions that Write, Program
or Erase must complete on a byte boundary (/CS driven high after a full 8-bits have been clocked) otherwise
the instruction will be ignored. This feature further protects the device from inadvertent writes. Additionally,
while  the  memory  is  being  programmed  or  erased,  or  when  the  Status  Register  is  being  written,  all
instructions except for Read Status Register will be ignored until the program or erase cycle has completed.

8.1  Device ID and Instruction Set Tables

 Manufacturer and Device Identification

MANUFACTURER ID

(MF7 - MF0)

Winbond Serial Flash

EFh

Device ID

Instruction

(ID7 - ID0)

(ID15 - ID0)

ABh, 90h, 92h, 94h

W25Q64JV-IQ/JQ

W25Q64JV-IM/JM*

16h

16h

9Fh

4017h

7017h

Note: For DTR, QPI supporting, please refer to W25Q64JV DTR datasheet.

Publication Release Date: March 27, 2018
- 21 -                                                                        Revision J

 Instruction Set Table 1 (Standard SPI Instructions)(1)

Data Input Output

Byte 1

Byte 2

Byte 3

Byte 4

Byte 5

Byte 6

Byte 7

W25Q64JV

8

8

8

8

8

8

Dummy

Dummy

Dummy

Dummy

Dummy

(ID7-ID0)(2)

00h

(MF7-MF0)

(ID7-ID0)

(MF7-MF0)

(ID15-ID8)

(ID7-ID0)

Dummy

Dummy

Dummy

A23-A16

A23-A16

A23-A16

A23-A16

A23-A16

A23-A16

(S7-S0)(2)

(S7-S0)(4)

(S15-S8)(2)

(S15-S8)

(S23-S16)(2)

(S23-S16)

00h

A23-A16

A23-A16

A23-A16

A23-A16

A23-A16

A23-A16

A15-A8

A15-A8

A15-A8

A15-A8

A15-A8

A15-A8

00h

A15-A8

A15-A8

A15-A8

A15-A8

A15-A8

A15-A8

A7-A0

A7-A0

A7-A0

A7-A0

A7-A0

A7-A0

A7–A0

A7-A0

A7-A0

A7-A0

A7-A0

A7-A0

A7-A0

Dummy

(D7-D0)

Dummy

D7-D0

(UID63-0)

(D7-D0)

D7-D0(3)

dummy

  (D7-0)

D7-D0

Dummy

D7-D0(3)

(D7-D0)

(L7-L0)

Number of Clock(1-1-1)

Write Enable

Volatile SR Write Enable

Write Disable

Release Power-down / ID

Manufacturer/Device ID

JEDEC ID

Read Unique ID

Read Data

Fast Read

Page Program

Sector Erase (4KB)

Block Erase (32KB)

Block Erase (64KB)

8

06h

50h

04h

ABh

90h

9Fh

4Bh

03h

0Bh

02h

20h

52h

D8h

Chip Erase

C7h/60h

Read Status Register-1

Write Status Register-1(4)

Read Status Register-2

Write Status Register-2

Read Status Register-3

Write Status Register-3

Read SFDP Register

Erase Security Register(5)

Program Security Register(5)

Read Security Register(5)

Global Block Lock

Global Block Unlock

Read Block Lock

Individual Block Lock

Individual Block Unlock

Erase / Program Suspend

Erase / Program Resume

Power-down

Enable Reset

Reset Device

05h

01h

35h

31h

15h

11h

5Ah

44h

42h

48h

7Eh

98h

3Dh

36h

39h

75h

7Ah

B9h

66h

99h

Publication Release Date: March 27, 2018
- 22 -                                                                        Revision J

W25Q64JV

 Instruction Set Table 2 (Dual/Quad SPI Instructions)(1)

Data Input Output
Number of Clock(1-1-2)

Byte 1
8

Byte 2
8

Byte 3
8

Fast Read Dual Output

3Bh

A23-A16

A15-A8

Byte 4
8

A7-A0

Byte 5
4

Byte 6
4

Byte 7
4

Byte 8
4

Byte 9
4

Dummy

Dummy

(D7-D0)(7)

Number of Clock(1-2-2)

8

4

4

4

4

4

4

Fast Read Dual I/O

BBh

A23-A16(6)

A15-A8(6)

A7-A0(6)

Dummy(11)

(D7-D0)(7)

Mftr./Device ID Dual I/O

92h

A23-A16(6)

A15-A8(6)

00(6)

Dummy(11)

(MF7-MF0)

(ID7-ID0)(7)

Number of Clock(1-1-4)

8

8

8

8

2

2

2

Quad Input Page Program

32h

A23-A16

A15-A8

A7-A0

(D7-D0)(9)

(D7-D0)(3)  …

4

2

4

2

Fast Read Quad Output

6Bh

A23-A16

A15-A8

A7-A0

Dummy

Dummy

Dummy

Dummy

(D7-D0)(10)

Number of Clock(1-4-4)

8

2(8)

2(8)

Mftr./Device ID Quad I/O

94h

A23-A16

A15-A8

2(8)

00

2

2

2

2

2

Dummy(11)

Dummy

Dummy

(MF7-MF0)

(ID7-ID0)

Fast Read Quad I/O

EBh

A23-A16

A15-A8

A7-A0

Dummy(11)

Dummy

Dummy

(D7-D0)

Set Burst with Wrap

77h

Dummy

Dummy

Dummy

W8-W0

1.  Data bytes are shifted with Most Significant Bit first. Byte fields with data in parenthesis “( )” indicate data

output from the device on either 1, 2 or 4 IO pins.

2.  The Status Register contents and Device ID will repeat continuously until /CS terminates the instruction.
3.  At least one byte of data input is required for Page Program, Quad Page Program and Program Security

Registers, up to 256 bytes of data input. If more than 256 bytes of data are sent to the device, the addressing
will wrap to the beginning of the page and overwrite previously sent data.

4.  Write Status Register-1 (01h) can also be used to program Status Register-1&2, see section 8.2.5.
5.  Security Register Address:

                    Security Register 1:    A23-16 = 00h;    A15-8 = 10h;    A7-0 = byte address
                    Security Register 2:    A23-16 = 00h;    A15-8 = 20h;    A7-0 = byte address
                    Security Register 3:    A23-16 = 00h;    A15-8 = 30h;    A7-0 = byte address

6.  Dual SPI address input format:

                    IO0 = A22, A20, A18, A16, A14, A12, A10, A8    A6, A4, A2, A0, M6, M4, M2, M0
                    IO1 = A23, A21, A19, A17, A15, A13, A11, A9    A7, A5, A3, A1, M7, M5, M3, M1

7.  Dual SPI data output format:

                    IO0 = (D6, D4, D2, D0)
                    IO1 = (D7, D5, D3, D1)

8.  Quad SPI address input format:                                                  Set Burst with Wrap input format:

                    IO0 = A20, A16, A12, A8,    A4, A0, M4, M0                    IO0 = x, x, x, x, x, x, W4, x
                    IO1 = A21, A17, A13, A9,    A5, A1, M5, M1                    IO1 = x, x, x, x, x, x, W5, x
                    IO2 = A22, A18, A14, A10, A6, A2, M6, M2                    IO2 = x, x, x, x, x, x, W6, x
                    IO3 = A23, A19, A15, A11, A7, A3, M7, M3                    IO3 = x, x, x, x, x, x, x,    x

9.  Quad SPI data input/output format:
                    IO0 = (D4, D0, …..)
                    IO1 = (D5, D1, …..)
                    IO2 = (D6, D2, …..)
                    IO3 = (D7, D3, …..)

10.  Fast Read Quad I/O data output format:

                    IO0 = (x, x, x, x, D4, D0, D4, D0)
                    IO1 = (x, x, x, x, D5, D1, D5, D1)
                    IO2 = (x, x, x, x, D6, D2, D6, D2)
                    IO3 = (x, x, x, x, D7, D3, D7, D3)
11.  The first dummy is M7-M0 should be set to Fxh

Publication Release Date: March 27, 2018
- 23 -                                                                        Revision J

W25Q64JV

8.2

Instruction Descriptions

 Write Enable (06h)

The Write Enable instruction (Figure 5) sets the Write Enable Latch (WEL) bit in the Status Register to a 1.
The WEL bit must be set prior to every Page Program, Quad Page Program, Sector Erase, Block Erase,
Chip  Erase,  Write  Status  Register  and  Erase/Program  Security  Registers  instruction.  The  Write  Enable
instruction is entered by driving /CS low, shifting the instruction code “06h” into the Data Input (DI) pin on
the rising edge of CLK, and then driving /CS high.

 Write Enable for Volatile Status Register (50h)

Figure 5. Write Enable Instruction for SPI Mode

The non-volatile Status Register bits described in section 7.1 can also be written to as volatile bits. This
gives more flexibility to change the system configuration and memory protection schemes quickly without
waiting for the typical non-volatile  bit  write cycles or  affecting the endurance of the Status Register non-
volatile bits. To write the volatile values into the Status Register bits, the Write Enable for Volatile Status
Register (50h) instruction must be issued prior to a Write Status Register (01h) instruction. Write Enable for
Volatile Status Register instruction (Figure 6) will not set the Write Enable Latch (WEL) bit, it is only valid
for the Write Status Register instruction to change the volatile Status Register bit values.

Figure 6. Write Enable for Volatile Status Register Instruction for SPI Mode)

Publication Release Date: March 27, 2018
- 24 -                                                                        Revision J

/CSCLKDI(IO0)DO(IO1)Mode 0Mode 301234567Mode 0Mode 3Instruction (06h)High Impedance/CSCLKDI(IO0)DO(IO1)Mode 0Mode 301234567Mode 0Mode 3Instruction (50h)High Impedance

W25Q64JV

 Write Disable (04h)

The Write Disable instruction (Figure 7) resets the Write Enable Latch (WEL) bit in the Status Register to a
0. The Write Disable instruction is entered by driving /CS low, shifting the instruction code “04h” into the DI
pin  and  then  driving  /CS  high.  Note  that  the  WEL  bit  is  automatically  reset  after  Power-up  and  upon
completion  of  the  Write  Status  Register,  Erase/Program  Security  Registers,  Page  Program,  Quad  Page
Program, Sector Erase, Block Erase, Chip Erase and Reset instructions.

Figure 7. Write Disable Instruction for SPI Mode

 Read Status Register-1 (05h), Status Register-2 (35h) & Status Register-3 (15h)

The Read Status Register instructions allow the 8-bit Status Registers to be read. The instruction is entered
by driving /CS low and shifting the instruction code “05h” for Status Register-1, “35h” for Status Register-2
or “15h” for Status Register-3 into the DI pin on the rising edge of CLK. The status register bits are then
shifted out on the DO pin at the falling edge of CLK with most significant bit (MSB) first as shown in Figure
8. Refer to section 7.1 for Status Register descriptions.

The Read Status Register instruction may be used at any time, even while a Program, Erase or Write Status
Register cycle is in progress. This allows the BUSY status bit to be checked to determine when the cycle is
complete and if the device can accept another instruction. The Status Register can be read continuously,
as shown in Figure 8. The instruction is completed by driving /CS high.

Figure 8. Read Status Register Instruction

Publication Release Date: March 27, 2018
- 25 -                                                                        Revision J

/CSCLKDI(IO0)DO(IO1)Mode 0Mode 301234567Mode 0Mode 3Instruction (04h)High Impedance/CSCLKDI(IO0)DO(IO1)Mode 0Mode 301234567Instruction (05h/35h/15h)High Impedance89101112131415161718192021222376543210765432107Status Register-1/2/3 outStatus Register-1/2/3 out**= MSB*

W25Q64JV

 Write Status Register-1 (01h), Status Register-2 (31h) & Status Register-3 (11h)

The Write Status Register instruction allows the Status Registers to be written. The writable Status Register
bits  include:SEC, TB, BP[2:0]  in Status  Register-1;  CMP, LB[3:1], QE, SRL  in Status Register-2; DRV1,
DRV0, WPS in Status Register-3. All other Status Register bit locations are read-only and will not be affected
by the Write Status Register instruction. LB[3:1] are non-volatile OTP bits, once it is set to 1, it cannot be
cleared to 0.

To write non-volatile Status Register bits, a standard Write Enable (06h) instruction must previously have
been executed for the device to accept the Write Status Register instruction (Status Register bit WEL must
equal 1). Once  write  enabled, the instruction is entered by driving /CS  low, sending the  instruction code
“01h/31h/11h”, and then writing the status register data byte as illustrated in Figure 9a.

To write volatile Status Register bits, a Write Enable for Volatile Status Register (50h) instruction must have
been executed prior to the Write Status Register instruction (Status Register bit WEL remains 0). However,
SRL  and LB[3:1] cannot  be changed from “1” to “0” because  of the OTP protection for these bits. Upon
power off or the execution of a Software/Hardware Reset, the volatile Status Register bit values will be lost,
and the non-volatile Status Register bit values will be restored.

During non-volatile Status Register write operation (06h combined with 01h/31h/11h), after /CS is driven
high,  the  self-timed  Write  Status  Register  cycle  will  commence  for  a  time  duration  of  tW  (See  AC
Characteristics). While the Write Status Register cycle is in progress, the Read Status Register instruction
may still be accessed to check the status  of the  BUSY bit. The  BUSY  bit is  a  1 during the Write Status
Register cycle and a 0 when the cycle is finished and ready to accept other instructions again. After the
Write Status Register cycle has finished, the Write Enable Latch (WEL) bit in the Status Register will be
cleared to 0.

During volatile Status Register write operation (50h combined with 01h/31h/11h), after /CS is driven high,
the  Status  Register  bits  will  be  refreshed  to  the  new  values  within  the  time  period  of  tSHSL2  (See  AC
Characteristics). BUSY bit will remain 0 during the Status Register bit refresh period.

Refer to section 7.1 for Status Register descriptions.

Figure 9a. Write Status Register-1/2/3 Instruction

Publication Release Date: March 27, 2018
- 26 -                                                                        Revision J

/CSCLKDI(IO0)DO(IO1)Mode 0Mode 301234567Instruction(01h/31h/11h)High Impedance8910111213141576543210Register-1/2/3 inMode 0Mode 3*= MSB*

W25Q64JV

The W25Q64JV is also backward compatible to Winbond’s previous generations of serial flash memories,
in which the Status Register-1&2 can be written using a single “Write Status Register-1 (01h)” command.
To complete the Write Status Register-1&2 instruction, the /CS pin must be driven high after the sixteenth
bit of data that is clocked in as shown in Figure 9c. If /CS is driven high after the eighth clock, the Write
Status Register-1 (01h) instruction will only program the Status Register-1, the Status Register-2 will not be
affected (Previous generations will clear CMP and QE bits).

Figure 9c. Write Status Register-1/2 Instruction

Publication Release Date: March 27, 2018
- 27 -                                                                        Revision J

/CSCLKDI(IO0)DO(IO1)Mode 0Mode 301234567Instruction (01h)High Impedance8910111213141516171819202122237654321015141312111098Status Register 1 inStatus Register 2 inMode 0Mode 3**= MSB*

W25Q64JV

 Read Data (03h)

The Read Data instruction allows one or more data bytes to be sequentially read from the memory. The
instruction is initiated by driving the /CS pin low and then shifting the instruction code “03h” followed by a
24-bit address (A23-A0) into the DI pin. The code and address bits are latched on the rising edge of the
CLK pin. After the address is received, the data byte of the addressed memory location will be shifted out
on the DO pin at the falling edge of CLK with most significant bit (MSB) first. The address is automatically
incremented to the next higher address after each byte of data is shifted out allowing for a continuous stream
of data. This means that the entire memory can be accessed with a single instruction as long as the clock
continues. The instruction is completed by driving /CS high.

The Read Data instruction sequence is shown in Figure 14. If a Read Data instruction is issued while an
Erase, Program or Write cycle is in process (BUSY=1) the instruction is ignored and will not have any effects
on the current cycle. The Read Data instruction allows clock rates from D.C. to a maximum of fR (see AC
Electrical Characteristics).

The Read Data (03h) instruction is only supported in Standard SPI mode.

Figure 14. Read Data Instruction

Publication Release Date: March 27, 2018
- 28 -                                                                        Revision J

/CSCLKDI(IO0)DO(IO1)Mode 0Mode 301234567Instruction (03h)High Impedance891028293031323334353637383976543210724-Bit Address2322213210Data Out 1**= MSB*

W25Q64JV

 Fast Read (0Bh)

The Fast Read instruction is similar to the Read Data instruction except that it can operate at the highest
possible frequency of FR (see AC Electrical Characteristics). This is accomplished by adding eight “dummy”
clocks after the 24-bit address as shown in Figure 16. The dummy clocks allow the devices internal circuits
additional time for setting up the initial address. During the dummy clocks the data value on the DO pin is a
“don’t care”.

Figure 16. Fast Read Instruction

Publication Release Date: March 27, 2018
- 29 -                                                                        Revision J

/CSCLKDI(IO0)DO(IO1)Mode 0Mode 301234567Instruction (0Bh)High Impedance89102829303124-Bit Address2322213210Data Out 1*/CSCLKDI(IO0)DO(IO1)3233343536373839Dummy ClocksHigh Impedance404142444546474849505152535455765432107Data Out 2*76543210*43310= MSB*

W25Q64JV

 Fast Read Dual Output (3Bh)

The Fast Read Dual Output (3Bh) instruction is similar to the standard Fast Read (0Bh) instruction except
that data is output on two pins; IO0 and IO1. This allows data to be transferred at twice the rate of standard
SPI  devices. The Fast Read Dual Output  instruction is ideal for quickly downloading code from Flash to
RAM upon power-up or for applications that cache code-segments to RAM for execution.

Similar  to  the  Fast  Read  instruction,  the  Fast  Read  Dual  Output  instruction  can  operate  at  the  highest
possible frequency of FR (see AC Electrical Characteristics). This is accomplished by adding eight “dummy”
clocks after the 24-bit address as shown in Figure 18. The dummy clocks allow the device's internal circuits
additional time for setting up the  initial address. The input data during the dummy clocks is “don’t care”.
However, the IO0 pin should be high-impedance prior to the falling edge of the first data out clock.

Figure 18. Fast Read Dual Output Instruction

Publication Release Date: March 27, 2018
- 30 -                                                                        Revision J

/CSCLKDI(IO0)DO(IO1)Mode 0Mode 301234567Instruction (3Bh)High Impedance89102829303233343536373839642024-Bit Address2322213210**3131/CSCLKDI(IO0)DO(IO1)Dummy Clocks0404142434445464748495051525354557531High Impedance642075316420753164207531IO0 switches fromInput to Output67Data Out 1*Data Out 2*Data Out 3*Data Out 4= MSB*

W25Q64JV

 Fast Read Quad Output (6Bh)

The  Fast  Read  Quad  Output  (6Bh)  instruction  is  similar  to  the  Fast  Read  Dual  Output  (3Bh)  instruction
except that data is output on four pins, IO0, IO1, IO2, and IO3. The Quad Enable (QE) bit in Status Register-
2 must be set to 1 before the device will accept the  Fast Read Quad Output Instruction. The Fast Read
Quad Output Instruction allows data to be transferred at four times the rate of standard SPI devices.

The  Fast  Read  Quad  Output  instruction  can  operate  at  the  highest  possible  frequency  of  FR  (see  AC
Electrical Characteristics). This is accomplished by adding eight “dummy” clocks after the 24-bit address as
shown in Figure 20. The dummy clocks allow the device's internal circuits additional time for setting up the
initial address. The input data during the dummy clocks is “don’t care”. However, the IO pins should be high-
impedance prior to the falling edge of the first data out clock.

Figure 20. Fast Read Quad Output Instruction

Publication Release Date: March 27, 2018
- 31 -                                                                        Revision J

/CSCLKMode 0Mode 301234567Instruction (6Bh)High Impedance891028293032333435363738394024-Bit Address2322213210*3131/CSCLKDummy Clocks0404142434445464751High Impedance45Byte 1High ImpedanceHigh Impedance6273High Impedance67High Impedance405162734051627340516273Byte 2Byte 3Byte 4IO0 switches fromInput to OutputIO0IO1IO2IO3IO0IO1IO2IO3= MSB*

W25Q64JV

Fast Read Dual I/O (BBh)

The Fast Read Dual I/O (BBh) instruction allows for improved random access while maintaining two IO pins,
IO0 and IO1. It is similar to the Fast Read Dual Output (3Bh) instruction but with the capability to input the
Address bits (A23-0) two bits per clock. This reduced instruction overhead may allow for code execution
(XIP) directly from the Dual SPI in some applications.

Figure 22. Fast Read Dual I/O Instruction (M5-4=Fxh)

Publication Release Date: March 27, 2018
- 32 -                                                                        Revision J

/CSCLKDI(IO0)DO(IO1)Mode 0Mode 301234567Instruction (BBh)891012131424252627282930316420**23/CSCLKDI(IO0)DO(IO1)032333435363738397531*642075316420753164207531**IOs switch fromInput to Output672220181623211917141210815131196420753164207531111516171820212219231A23-16A15-8A7-0M7-0Byte 1Byte 2Byte 3Byte 4= MSB**

W25Q64JV

Fast Read Quad I/O (EBh)

The Fast Read Quad I/O (EBh) instruction is similar to the Fast Read Dual I/O (BBh) instruction except that
address and data bits are input and output through four pins IO0, IO1, IO2 and IO3 and four Dummy clocks
are required in SPI mode prior to the data output. The Quad I/O dramatically reduces instruction overhead
allowing faster random access for code execution (XIP) directly from the Quad SPI. The Quad Enable bit
(QE) of Status Register-2 must be set to enable the Fast Read Quad I/O Instruction.

Figure 24a. Fast Read Quad I/O Instruction (M7-M0 should be set to Fxh)

Fast Read Quad I/O with “8/16/32/64-Byte Wrap Around” in Standard SPI mode

The Fast Read Quad I/O instruction can also be used to access a specific portion within a page by issuing
a “Set Burst with Wrap” (77h) command prior to EBh. The “Set Burst with Wrap” (77h) command can either
enable  or  disable  the  “Wrap  Around”  feature  for  the  following  EBh  commands.  When  “Wrap  Around”  is
enabled, the data being accessed can be limited to either an 8, 16, 32 or 64-byte section of a 256-byte page.
The output data starts at the initial address specified in the instruction, once it reaches the ending boundary
of the 8/16/32/64-byte section, the output will wrap around to the beginning boundary automatically until /CS
is pulled high to terminate the command.

The Burst with Wrap feature allows applications that use cache to quickly fetch a critical address and then
fill  the  cache  afterwards  within  a  fixed  length  (8/16/32/64-byte)  of  data  without  issuing  multiple  read
commands.

The “Set Burst with Wrap” instruction allows three “Wrap Bits”, W6-4 to be set. The W4 bit is used to enable
or disable the “Wrap Around” operation while W6-5 are used to specify the length of the wrap around section
within a page. Refer to section 8.2.37 for detail descriptions.

Publication Release Date: March 27, 2018
- 33 -                                                                        Revision J

M7-0/CSCLKMode 0Mode 301IO0IO1IO2IO32345201612821172218231913914101511A23-16678940516273A15-8A7-0Byte 1Byte 240516273405162734051627310111213144567IOs switch fromInput to OutputByte 3151617181920212223DummyDummyInstruction (EBh)

W25Q64JV

  Set Burst with Wrap (77h)

In Standard SPI mode, the Set Burst with Wrap (77h) instruction is used  in conjunction  with “Fast Read
Quad I/O” instructions to access a fixed length of 8/16/32/64-byte section within a 256-byte page. Certain
applications can benefit from this feature and improve the overall system code execution performance.

Similar to a Quad I/O instruction, the Set Burst with Wrap instruction is initiated by driving the /CS pin low
and  then  shifting  the  instruction  code  “77h”  followed  by  24  dummy  bits  and  8  “Wrap  Bits”,  W7-0.  The
instruction sequence is shown in Figure 28. Wrap bit W7 and the lower nibble W3-0 are not used.

W6, W5

0    0
0    1
1    0
1    1

W4 = 0

W4 =1 (DEFAULT)

Wrap Around

Wrap Length

Wrap Around  Wrap Length

Yes
Yes
Yes
Yes

8-byte
16-byte
32-byte
64-byte

No
No
No
No

N/A
N/A
N/A
N/A

Once W6-4 is set by a Set Burst with Wrap instruction, the following “Fast Read Quad I/O” instructions will
use  the W6-4  setting  to  access  the  8/16/32/64-byte  section  within  any  page.  To  exit  the  “Wrap  Around”
function and return to normal read operation, another Set Burst with Wrap instruction should be issued to
set W4 = 1. The default value of W4 upon power on or after a software/hardware reset is 1.

Figure 28. Set Burst with Wrap Instruction

Publication Release Date: March 27, 2018
- 34 -                                                                        Revision J

Wrap Bit/CSCLKMode 0Mode 301IO0IO1IO2IO32345XXXXXXXXdon'tcare6789don'tcaredon'tcare101112131415Instruction (77h)Mode 0Mode 3XXXXXXXXXXXXXXXXw4Xw5Xw6XXX

W25Q64JV

Page Program (02h)

The Page  Program instruction allows from one byte to 256  bytes (a page) of  data to be programmed at
previously erased (FFh) memory locations. A Write Enable instruction must be executed before the device
will accept the Page Program Instruction (Status Register bit WEL= 1). The instruction is initiated by driving
the /CS pin low then shifting the instruction code “02h” followed by a 24-bit address (A23-A0) and at least
one data byte, into the DI pin. The /CS pin must be held low for the entire length of the instruction while data
is being sent to the device. The Page Program instruction sequence is shown in Figure 29.

If an entire 256 byte page is to be programmed, the last address byte (the 8 least significant address bits)
should be set to 0. If the last address byte is not zero, and the number of clocks exceeds the remaining
page length, the addressing will wrap to the beginning of the page. In some cases, less than 256 bytes (a
partial  page)  can  be  programmed  without  having  any  effect  on  other  bytes  within  the  same  page.  One
condition to perform a partial page program is that the number of clocks cannot exceed the remaining page
length. If more than 256 bytes are sent to the device the addressing will wrap to the beginning of the page
and overwrite previously sent data.

As with the write and erase instructions, the /CS pin must be driven high after the eighth bit of the last byte
has been latched. If this is not done the Page Program instruction will not be executed. After /CS is driven
high,  the  self-timed  Page  Program  instruction  will  commence  for  a  time  duration  of  tpp  (See  AC
Characteristics). While the Page Program cycle is in progress, the Read Status Register instruction may still
be accessed for checking the status of the BUSY bit. The BUSY bit is a 1 during the Page Program cycle
and becomes a 0 when the cycle is finished and the device is ready to accept other instructions again. After
the Page Program cycle has finished the Write Enable Latch (WEL) bit in the Status Register is cleared to
0. The Page Program instruction will not be executed if the addressed page is protected by the Block Protect
(CMP, SEC, TB, BP2, BP1, and BP0) bits or the Individual Block/Sector Locks.

Figure 29. Page Program Instruction

Publication Release Date: March 27, 2018
- 35 -                                                                        Revision J

/CSCLKDI(IO0)Mode 0Mode 301234567Instruction (02h)89102829303924-Bit Address232221321*/CSCLKDI(IO0)4041424344454647Data Byte 2484950525354552072765432105139031032333435363738Data Byte 17654321*Mode 0Mode 3Data Byte 320732074207520762077207820790Data Byte 256*76543210*76543210*= MSB*

W25Q64JV

Quad Input Page Program (32h)

The Quad Page Program instruction allows up to 256 bytes of data to be programmed at previously erased
(FFh)  memory  locations  using  four  pins:  IO0,  IO1,  IO2,  and  IO3.    The  Quad  Page  Program  can  improve
performance for PROM Programmer and applications that have slow clock speeds <5MHz. Systems with
faster clock speed will not realize much benefit for the Quad Page Program instruction since the inherent
page program time is much greater than the time it take to clock-in the data.

To use Quad Page Program the Quad Enable (QE) bit in Status Register-2 must be set to 1. A Write Enable
instruction  must  be  executed  before  the  device  will  accept  the  Quad  Page  Program  instruction  (Status
Register-1, WEL=1). The instruction is initiated by driving the /CS pin low then shifting the instruction code
“32h” followed by a 24-bit address (A23-A0) and at least one data byte, into the IO pins. The /CS pin must
be held low for the entire length of the instruction while data is being sent to the device. All other functions
of  Quad  Page  Program  are  identical  to  standard  Page  Program.  The  Quad  Page  Program  instruction
sequence is shown in Figure 30.

Figure 30. Quad Input Page Program Instruction

Publication Release Date: March 27, 2018
- 36 -                                                                        Revision J

/CSCLKMode 0Mode 301234567Instruction (32h)89102829303233343536374024-Bit Address2322213210*3131/CSCLK51Byte 16273405162734051627340516273Byte 2Byte 3Byte2560405162734051627340516273536537538539540541542543Mode 0Mode 3Byte253Byte254Byte255IO0IO1IO2IO3IO0IO1IO2IO3*******= MSB*

W25Q64JV

8.3  Sector Erase (20h)
The Sector Erase instruction sets all memory within a specified sector (4K-bytes) to the erased state of all
1s  (FFh).  A  Write  Enable  instruction  must  be  executed  before  the  device  will  accept  the  Sector  Erase
Instruction (Status Register bit WEL must equal 1). The instruction is initiated by driving the /CS pin low and
shifting the instruction code “20h” followed a 24-bit sector address (A23-A0). The Sector Erase instruction
sequence is shown in Figure 31.

The /CS pin must be driven high after the eighth bit of the last byte has been latched. If this is not done the
Sector Erase instruction will not be executed. After /CS is driven high, the self-timed Sector Erase instruction
will  commence  for  a  time  duration  of  tSE  (See  AC  Characteristics).  While  the  Sector  Erase  cycle  is  in
progress, the Read Status Register instruction may still be accessed for checking the status of the BUSY
bit. The BUSY bit is a 1 during the Sector Erase cycle and becomes a 0 when the cycle is finished and the
device is ready to accept other instructions again. After the Sector Erase cycle has finished the Write Enable
Latch (WEL) bit in the Status Register is cleared to 0. The Sector Erase instruction will not be executed if
the  addressed  page  is  protected  by  the  Block  Protect  (CMP,  SEC,  TB,  BP2,  BP1,  and  BP0)  bits  or  the
Individual Block/Sector Locks.

Figure 31. Sector Erase Instruction

Publication Release Date: March 27, 2018
- 37 -                                                                        Revision J

/CSCLKDI(IO0)DO(IO1)Mode 0Mode 301234567Instruction (20h)High Impedance8929303124-Bit Address2322210*Mode 0Mode 3= MSB*

W25Q64JV

 32KB Block Erase (52h)

The Block Erase instruction sets all memory within a specified block (32K-bytes) to the erased state of all
1s  (FFh).  A  Write  Enable  instruction  must  be  executed  before  the  device  will  accept  the  Block  Erase
Instruction (Status Register bit WEL must equal 1). The instruction is initiated by driving the /CS pin low and
shifting the instruction code “52h” followed  a 24-bit block address (A23-A0). The Block Erase instruction
sequence is shown in Figure 32.

The /CS pin must be driven high after the eighth bit of the last byte has been latched. If this is not done the
Block Erase instruction will not be executed. After /CS is driven high, the self-timed Block Erase instruction
will  commence  for  a  time  duration  of  tBE1  (See  AC  Characteristics).  While  the  Block  Erase  cycle  is  in
progress, the Read Status Register instruction may still be accessed for checking the status of the BUSY
bit. The BUSY bit is a 1 during the Block Erase cycle and becomes a 0 when the cycle is finished and the
device is ready to accept other instructions again. After the Block Erase cycle has finished the Write Enable
Latch (WEL) bit in the Status Register is cleared to 0. The Block Erase instruction will not be executed if the
addressed page is protected by the Block Protect (CMP, SEC, TB, BP2, BP1, and BP0) bits or the Individual
Block/Sector Locks.

Figure 32. 32KB Block Erase Instruction

Publication Release Date: March 27, 2018
- 38 -                                                                        Revision J

/CSCLKDI(IO0)DO(IO1)Mode 0Mode 301234567Instruction (52h)High Impedance8929303124-Bit Address2322210*Mode 0Mode 3= MSB*

W25Q64JV

 64KB Block Erase (D8h)

The Block Erase instruction sets all memory within a specified block (64K-bytes) to the erased state of all
1s  (FFh).  A  Write  Enable  instruction  must  be  executed  before  the  device  will  accept  the  Block  Erase
Instruction (Status Register bit WEL must equal 1). The instruction is initiated by driving the /CS pin low and
shifting the instruction code “D8h” followed a 24-bit block address (A23-A0). The Block Erase instruction
sequence is shown in Figure 33.

The /CS pin must be driven high after the eighth bit of the last byte has been latched. If this is not done the
Block Erase instruction will not be executed. After /CS is driven high, the self-timed Block Erase instruction
will  commence  for  a  time  duration  of  tBE  (See  AC  Characteristics).  While  the  Block  Erase  cycle  is  in
progress, the Read Status Register instruction may still be accessed for checking the status of the BUSY
bit. The BUSY bit is a 1 during the Block Erase cycle and becomes a 0 when the cycle is finished and the
device is ready to accept other instructions again. After the Block Erase cycle has finished the Write Enable
Latch (WEL) bit in the Status Register is cleared to 0. The Block Erase instruction will not be executed if the
addressed page is protected by the Block Protect (CMP, SEC, TB, BP2, BP1, and BP0) bits or the Individual
Block/Sector Locks.

Figure 33. 64KB Block Erase Instruction

Publication Release Date: March 27, 2018
- 39 -                                                                        Revision J

/CSCLKDI(IO0)DO(IO1)Mode 0Mode 301234567Instruction (D8h)High Impedance8929303124-Bit Address2322210*Mode 0Mode 3= MSB*

W25Q64JV

  Chip Erase (C7h / 60h)

The Chip Erase instruction sets all memory within the device to the  erased state of all 1s (FFh). A Write
Enable  instruction  must  be  executed  before  the  device  will  accept  the  Chip  Erase  Instruction  (Status
Register  bit  WEL  must  equal  1).  The  instruction  is  initiated  by  driving  the  /CS  pin  low  and  shifting  the
instruction code “C7h” or “60h”. The Chip Erase instruction sequence is shown in Figure 34.

The /CS pin must be driven high after the eighth bit has been latched. If this is not done the Chip Erase
instruction will not be executed. After /CS is driven high, the self-timed Chip Erase instruction will commence
for a time duration of tCE (See AC Characteristics). While the Chip Erase cycle is in progress, the Read
Status Register instruction may still be accessed to check the status of the BUSY bit. The BUSY bit is a 1
during  the  Chip  Erase  cycle  and  becomes  a  0  when  finished  and  the  device  is  ready  to  accept  other
instructions again. After the Chip Erase cycle has finished the Write Enable Latch (WEL) bit in the Status
Register is cleared to 0. The Chip Erase instruction will not be executed if any memory region is protected
by the Block Protect (CMP, SEC, TB, BP2, BP1, and BP0) bits or the Individual Block/Sector Locks.

Figure 34. Chip Erase Instruction

Publication Release Date: March 27, 2018
- 40 -                                                                        Revision J

/CSCLKDI(IO0)DO(IO1)Mode 0Mode 301234567Instruction (C7h/60h)High ImpedanceMode 0Mode 3

W25Q64JV

 Erase / Program Suspend (75h)

The  Erase/Program  Suspend  instruction  “75h”,  allows  the  system  to  interrupt  a  Sector  or  Block  Erase
operation or a Page Program operation and then read from or program/erase data to, any other sectors or
blocks. The Erase/Program Suspend instruction sequence is shown in Figure 35.

The Write Status Register instruction (01h) and Erase instructions (20h, 52h, D8h, C7h, 60h, 44h) are not
allowed during Erase Suspend. Erase Suspend is valid only during the Sector or Block erase operation. If
written during the Chip Erase operation, the Erase Suspend instruction is ignored. The Write Status Register
instruction  (01h)  and  Program  instructions  (02h,  32h,  42h)  are  not  allowed  during  Program  Suspend.
Program Suspend is valid only during the Page Program or Quad Page Program operation.

The Erase/Program Suspend instruction “75h” will be accepted by the device only if the SUS bit in the Status
Register  equals  to  0  and  the  BUSY  bit  equals  to  1  while  a  Sector  or  Block  Erase  or  a  Page  Program
operation is on-going. If the SUS bit equals to 1 or the BUSY bit equals to 0, the Suspend instruction will be
ignored  by  the device. A  maximum of time of “tSUS” (See  AC Characteristics) is required to suspend the
erase or program operation. The BUSY bit in the Status Register will be cleared from 1 to 0 within “tSUS” and
the SUS bit in the Status Register will be set from 0 to 1 immediately after Erase/Program Suspend. For a
previously resumed Erase/Program operation, it is also required that the Suspend instruction “75h” is not
issued earlier than a minimum of time of “tSUS” following the preceding Resume instruction “7Ah”.

Unexpected  power  off  during  the  Erase/Program  suspend  state  will  reset  the  device  and  release  the
suspend state. SUS bit in the Status Register will also reset to 0. The data within the page, sector or block
that was being suspended may become corrupted. It is recommended for the user to implement system
design  techniques  against  the  accidental  power  interruption  and  preserve  data  integrity  during
erase/program suspend state.

Figure 35. Erase/Program Suspend Instruction

Publication Release Date: March 27, 2018
- 41 -                                                                        Revision J

/CSCLKDI(IO0)DO(IO1)Mode 0Mode 301234567Instruction (75h)High ImpedanceMode 0Mode 3tSUSAccept instructions

W25Q64JV

 Erase / Program Resume (7Ah)

The  Erase/Program  Resume  instruction  “7Ah”  must  be  written  to  resume  the  Sector  or  Block  Erase
operation or the Page Program operation after an Erase/Program Suspend. The Resume instruction “7Ah”
will be accepted by the device only if the SUS bit in the Status Register equals to 1 and the BUSY bit equals
to 0. After issued the SUS bit will be cleared from 1 to 0 immediately, the BUSY bit will be set from 0 to 1
within 200ns and the Sector or Block will complete the erase operation or the page will complete the program
operation.  If  the  SUS  bit  equals  to  0  or  the  BUSY  bit  equals  to  1,  the  Resume  instruction  “7Ah”  will  be
ignored by the device. The Erase/Program Resume instruction sequence is shown in Figure 36.

Resume  instruction  is  ignored  if  the  previous  Erase/Program  Suspend  operation  was  interrupted  by
unexpected power off. It is also required that a subsequent Erase/Program Suspend instruction not to be
issued within a minimum of time of “tSUS” following a previous Resume instruction.

Figure 36. Erase/Program Resume Instruction

Publication Release Date: March 27, 2018
- 42 -                                                                        Revision J

/CSCLKDI(IO0)Mode 0Mode 301234567Instruction (7Ah)Mode 0Mode 3Resume previouslysuspended Program orErase

W25Q64JV

 Power-down (B9h)

Although  the  standby  current  during  normal  operation  is  relatively  low,  standby  current  can  be  further
reduced with the Power-down instruction. The lower power consumption makes the Power-down instruction
especially  useful  for  battery  powered  applications  (See  ICC1  and  ICC2  in  AC  Characteristics).  The
instruction is initiated by driving the /CS pin low and shifting the instruction code “B9h” as shown in Figure
37.
The /CS pin must be driven high after the eighth bit has been latched. If this is not done the Power-down
instruction will not be executed. After /CS is driven high, the power-down state will entered within the time
duration of tDP (See AC Characteristics). While in the power-down state only the Release Power-down /
Device  ID  (ABh)  instruction,  which  restores  the  device  to  normal  operation,  will  be  recognized.  All  other
instructions are ignored. This includes the Read Status Register instruction, which is always available during
normal  operation.  Ignoring  all  but  one  instruction  makes  the  Power  Down  state  a  useful  condition  for
securing maximum write protection. The device always powers-up in the normal operation with the standby
current of ICC1.

Figure 37. Deep Power-down Instruction

Publication Release Date: March 27, 2018
- 43 -                                                                        Revision J

/CSCLKDI(IO0)Mode 0Mode 301234567Instruction (B9h)Mode 0Mode 3tDPPower-down currentStand-by current

W25Q64JV

 Release Power-down / Device ID (ABh)

The  Release  from  Power-down  /  Device  ID  instruction  is  a  multi-purpose  instruction.  It  can  be  used  to
release the device from the power-down state, or obtain the devices electronic identification (ID) number.
To release the device from the power-down state, the instruction is issued by driving the /CS pin low, shifting
the instruction code “ABh” and driving /CS high as shown in Figure 38a. Release from power-down will take
the  time  duration  of  tRES1 (See  AC  Characteristics)  before  the  device  will  resume  normal  operation  and
other instructions are accepted. The /CS pin must remain high during the tRES1 time duration.
When used only to obtain the Device ID while not in the power-down state, the instruction is initiated by
driving the /CS pin low and shifting the instruction code “ABh” followed by 3-dummy bytes. The Device ID
bits are then shifted out on the falling edge of CLK with most significant bit (MSB) first. The Device ID value
for  the W25Q64JV  is  listed  in  Manufacturer  and  Device  Identification  table.  The  Device  ID  can  be  read
continuously. The instruction is completed by driving /CS high.
When used to release the device from the power-down state and obtain the Device ID, the instruction is the
same as previously described, and shown in Figure 38b, except that after /CS is driven high it must remain
high for a time duration of tRES2 (See AC Characteristics). After this time duration the device will resume
normal  operation  and  other  instructions  will  be  accepted.  If  the  Release  from  Power-down  /  Device  ID
instruction  is  issued  while  an  Erase,  Program  or  Write  cycle  is  in  process  (when  BUSY  equals  1)  the
instruction is ignored and will not have any effects on the current cycle.

Figure 38a. Release Power-down Instruction

Figure 38c. Release Power-down / Device ID Instruction

Publication Release Date: March 27, 2018
- 44 -                                                                        Revision J

/CSCLKDI(IO0)Mode 0Mode 301234567Instruction (ABh)Mode 0Mode 3tRES1Power-down currentStand-by currenttRES2/CSCLKDI(IO0)DO(IO1)Mode 0Mode 301234567Instruction (ABh)High Impedance892930313 Dummy Bytes2322210*Mode 0Mode 376543210*32333435363738Device IDPower-down currentStand-by current= MSB*

W25Q64JV

 Read Manufacturer / Device ID (90h)

The Read Manufacturer/Device ID instruction is an alternative to the Release from Power-down / Device ID
instruction that provides both the JEDEC assigned manufacturer ID and the specific device ID.

The Read Manufacturer/Device ID instruction is very similar to the Release from Power-down / Device ID
instruction.  The  instruction  is  initiated  by  driving  the  /CS  pin  low  and  shifting  the  instruction  code  “90h”
followed by a 24-bit address (A23-A0) of 000000h. After which, the Manufacturer ID for Winbond (EFh) and
the Device ID are shifted out on the falling edge of CLK with most significant bit (MSB) first as shown in
Figure 39. The  Device ID  values for the W25Q64JV are  listed  in Manufacturer  and Device Identification
table. The instruction is completed by driving /CS high.

Figure 39. Read Manufacturer / Device ID Instruction

Publication Release Date: March 27, 2018
- 45 -                                                                        Revision J

/CSCLKDI(IO0)DO(IO1)Mode 0Mode 301234567Instruction (90h)High Impedance891028293031Address (000000h)2322213210Device ID*/CSCLKDI(IO0)DO(IO1)3233343536373839Manufacturer ID (EFh)40414244454676543210*43310Mode 0Mode 3= MSB*

W25Q64JV

 Read Manufacturer / Device ID Dual I/O (92h)

The Read Manufacturer / Device ID Dual I/O instruction is an alternative to the Read Manufacturer / Device
ID  instruction  that  provides  both  the  JEDEC  assigned  manufacturer  ID  and  the  specific  device  ID  at  2x
speed.

The Read Manufacturer / Device ID Dual I/O instruction is similar to the Fast Read Dual I/O instruction. The
instruction is initiated by driving the /CS pin low and shifting the instruction code “92h” followed by a 24-bit
address (A23-A0) of 000000h, but with the capability to input the Address bits two bits per clock. After which,
the Manufacturer ID for Winbond (EFh) and the Device ID are shifted out 2 bits per clock on the falling edge
of CLK with most significant bits (MSB) first as shown in Figure 40. The Device ID values for the W25Q64JV
are listed in Manufacturer and Device Identification table. The Manufacturer and Device IDs can be read
continuously, alternating from one to the other. The instruction is completed by driving /CS high.

Figure 40. Read Manufacturer / Device ID Dual I/O Instruction

Note:
The “Continuous Read Mode” bits M(7-0) must be set to Fxh to be compatible with Fast Read Dual I/O instruction.

Publication Release Date: March 27, 2018
- 46 -                                                                        Revision J

/CSCLKDI(IO0)DO(IO1)Mode 0Mode 301234567Instruction (92h)High Impedance89101112131415161718192021227531**642075316420753164207531642023**A23-16A15-8A7-0 (00h)M7-0/CSCLKDI(IO0)DO(IO1)242526272829303132333436373835230Mode 0Mode 3753164207531642075316420753642101MFR IDDevice IDMFR ID(repeat)Device ID(repeat)IOs switch fromInput to Output****= MSB*

W25Q64JV

Read Manufacturer / Device ID Quad I/O (94h)

The Read Manufacturer / Device ID Quad I/O instruction is an alternative to the Read Manufacturer / Device
ID  instruction  that  provides  both  the  JEDEC  assigned  manufacturer  ID  and  the  specific  device  ID  at  4x
speed.

The Read Manufacturer / Device ID Quad I/O instruction is similar to the Fast Read Quad I/O instruction.
The instruction is initiated by driving the /CS pin low and shifting the instruction code “94h” followed by a
four clock dummy cycles and then a 24-bit address (A23-A0) of 000000h, but with the capability to input the
Address bits four bits per clock. After which, the Manufacturer ID for Winbond (EFh) and the Device ID are
shifted out four bits per clock on the falling edge of CLK with most significant bit (MSB) first as shown in
Figure 41. The Manufacturer and Device IDs can be read continuously, alternating from one to the other.
The instruction is completed by driving /CS high.

Figure 41. Read Manufacturer / Device ID Quad I/O Instruction

Note:
The “Continuous Read Mode” bits M(7-0) must be set to Fxh to be compatible with Fast Read Quad I/O instruction.

Publication Release Date: March 27, 2018
- 47 -                                                                        Revision J

Mode 0Mode 301234567Instruction (94h)High Impedance8910111213141516171819202122514023Mode 0Mode 3IOs switch fromInput to OutputHigh Impedance7362/CSCLKIO0IO1IO2IO3High ImpedanceA23-16A15-8A7-0(00h)M7-0MFR IDDevice IDDummyDummy/CSCLKIO0IO1IO2IO323012351407362514073625140736251407362514073625140736251407362514073625140736224252627282930MFR ID(repeat)Device ID(repeat)MFR ID(repeat)Device ID(repeat)

W25Q64JV

Read Unique ID Number (4Bh)

The Read Unique ID Number instruction accesses a factory-set read-only 64-bit number that is unique to
each W25Q64JV device. The ID number can be used in conjunction with user software methods to help
prevent copying or cloning of a system. The Read Unique ID instruction is initiated by driving the /CS pin
low and shifting the instruction code “4Bh” followed by a four bytes of dummy clocks. After which, the 64-bit
ID is shifted out on the falling edge of CLK as shown in Figure 42.

Figure 42. Read Unique ID Number Instruction

Publication Release Date: March 27, 2018
- 48 -                                                                        Revision J

/CSCLKDI(IO0)DO(IO1)Mode 0Mode 301234567Instruction (4Bh)High Impedance891011121314151617181920212223/CSCLKDI(IO0)DO(IO1)24252627282930313233343637383523Mode 0Mode 3*Dummy Byte 1Dummy Byte 239404142Dummy Byte 3Dummy Byte 463626121064-bit Unique Serial Number100101102High Impedance= MSB*

W25Q64JV

Read JEDEC ID (9Fh)

For  compatibility  reasons,  the  W25Q64JV  provides  several  instructions  to  electronically  determine  the
identity  of  the  device.  The  Read  JEDEC  ID  instruction  is  compatible  with  the  JEDEC  standard  for  SPI
compatible serial memories that was adopted in 2003. The instruction is initiated by driving the /CS pin low
and shifting the instruction code “9Fh”. The JEDEC assigned Manufacturer ID byte for Winbond (EFh) and
two Device ID bytes, Memory Type (ID15-ID8) and Capacity (ID7-ID0) are then shifted out on the falling
edge  of  CLK  with  most  significant  bit  (MSB)  first  as  shown  in  Figure  43.  For  memory  type  and  capacity
values refer to Manufacturer and Device Identification table.

Figure 43. Read JEDEC ID Instruction

Publication Release Date: March 27, 2018
- 49 -                                                                        Revision J

/CSCLKDI(IO0)DO(IO1)Mode 0Mode 301234567Instruction (9Fh)High Impedance891012131415Capacity ID7-0/CSCLKDI(IO0)DO(IO1)1617181920212223Manufacturer ID (EFh)24252628293076543210*2715Mode 0Mode 31176543210*Memory Type ID15-8= MSB*

W25Q64JV

Read SFDP Register (5Ah)

The W25Q64JV features a 256-Byte  Serial Flash Discoverable Parameter (SFDP) register that contains
information about device configurations, available instructions and other features. The SFDP parameters
are stored in one or more Parameter Identification (PID) tables. Currently only one PID table is specified,
but more may be added in the future. The Read SFDP Register instruction is compatible with the SFDP
standard  initially  established  in  2010  for  PC  and  other  applications,  as  well  as  the  JEDEC  standard
JESD216-serials that is published in 2011. Most Winbond SpiFlash Memories shipped after June 2011 (date
code 1124 and beyond) support the SFDP feature as specified in the applicable datasheet.

The Read SFDP instruction is initiated by driving the /CS pin low and shifting the instruction code “5Ah”
followed by a 24-bit address (A23-A0)(1) into the DI pin. Eight “dummy” clocks are also required before the
SFDP register contents are shifted out on the falling edge of the 40th CLK with most significant bit (MSB)
first  as  shown  in  Figure  44.  For  SFDP  register  values  and  descriptions,  please  refer  to  the  Winbond
Application Note for SFDP Definition Table.

Note 1: A23-A8 = 0; A7-A0 are used to define the starting byte address for the 256-Byte SFDP Register.

Figure 44. Read SFDP Register Instruction Sequence Diagram

Publication Release Date: March 27, 2018
- 50 -                                                                        Revision J

/CSCLKDI(IO0)DO(IO1)Mode 0Mode 301234567Instruction (5Ah)High Impedance89102829303124-Bit Address2322213210Data Out 1*/CSCLKDI(IO0)DO(IO1)3233343536373839Dummy ByteHigh Impedance404142444546474849505152535455765432107Data Out 2*76543210*7654321043310= MSB*

W25Q64JV

Erase Security Registers (44h)

The W25Q64JV offers three 256-byte Security Registers which can be erased and programmed individually.
These registers may be used by the system manufacturers to store security and other important information
separately from the main memory array.

The Erase Security Register instruction is similar to the Sector Erase instruction. A Write Enable instruction
must be executed before the device will accept the Erase Security Register Instruction (Status Register bit
WEL must equal 1). The instruction is initiated by driving the /CS pin low and shifting the instruction code
“44h” followed by a 24-bit address (A23-A0) to erase one of the three security registers.

ADDRESS

A23-16

Security Register #1

Security Register #2

Security Register #3

00h

00h

00h

A15-12

0 0 0 1

0 0 1 0

0 0 1 1

A11-8

0 0 0 0

0 0 0 0

0 0 0 0

A7-0

Don’t Care

Don’t Care

Don’t Care

The Erase Security Register instruction sequence is shown in Figure 45. The /CS pin must be driven high
after the eighth bit of the last byte has been latched. If this is not done the instruction will not be executed.
After /CS is driven high, the self-timed Erase Security Register operation will commence for a time duration
of tSE (See AC Characteristics). While the Erase Security Register cycle is in progress, the Read Status
Register instruction may still be accessed for checking the status of the BUSY bit. The BUSY bit is a 1 during
the  erase  cycle  and  becomes  a  0  when  the  cycle  is  finished  and  the  device  is  ready  to  accept  other
instructions again. After the Erase Security Register cycle has finished the Write Enable Latch (WEL) bit in
the Status Register is cleared to 0. The Security Register Lock Bits (LB3-1) in the Status Register-2 can be
used to OTP protect the security registers. Once a lock bit is set to 1, the corresponding security register
will  be  permanently  locked,  Erase  Security  Register  instruction  to  that  register  will  be  ignored  (Refer  to
section 7.1.8 for detail descriptions).

Figure 45. Erase Security Registers Instruction

Publication Release Date: March 27, 2018
- 51 -                                                                        Revision J

/CSCLKDI(IO0)DO(IO1)Mode 0Mode 301234567Instruction (44h)High Impedance8929303124-Bit Address2322210*Mode 0Mode 3= MSB*

W25Q64JV

Program Security Registers (42h)

The Program Security Register instruction is similar to the Page Program instruction. It allows from one byte
to 256 bytes of security register data to be programmed at previously erased (FFh) memory locations. A
Write  Enable  instruction  must  be  executed  before  the  device  will  accept  the  Program  Security  Register
Instruction (Status Register bit WEL= 1). The instruction is initiated by driving the /CS pin low then shifting
the instruction code “42h” followed by a 24-bit address (A23-A0) and at least one data byte, into the DI pin.
The /CS pin must be held low for the entire length of the instruction while data is being sent to the device.

ADDRESS

A23-16

Security Register #1

Security Register #2

Security Register #3

00h

00h

00h

A15-12

0 0 0 1

0 0 1 0

0 0 1 1

A11-8

0 0 0 0

0 0 0 0

0 0 0 0

A7-0

Byte Address

Byte Address

Byte Address

The Program Security Register instruction sequence is shown in Figure 46. The Security Register Lock Bits
(LB3-1) in the Status Register-2 can be used to OTP protect the security registers. Once a lock bit is set to
1, the corresponding security register will be permanently locked, Program Security Register instruction to
that register will be ignored (See 7.1.8, 8.2.25 for detail descriptions).

Figure 46. Program Security Registers Instruction

Publication Release Date: March 27, 2018
- 52 -                                                                        Revision J

/CSCLKDI(IO0)Mode 0Mode 301234567Instruction (42h)89102829303924-Bit Address232221321*/CSCLKDI(IO0)4041424344454647Data Byte 2484950525354552072765432105139031032333435363738Data Byte 17654321*Mode 0Mode 3Data Byte 320732074207520762077207820790Data Byte 256*76543210*76543210*= MSB*

W25Q64JV

Read Security Registers (48h)

The Read Security Register instruction is similar to the Fast Read instruction and allows one or more data
bytes to be sequentially read from one of the four security registers. The instruction is initiated by driving
the /CS pin low and then shifting the instruction code “48h” followed by a 24-bit address (A23-A0) and eight
“dummy” clocks into the DI pin. The code and address bits are latched on the rising edge of the CLK pin.
After the address is received, the data byte of the addressed memory location will be shifted out on the DO
pin  at  the  falling  edge  of  CLK  with  most  significant  bit  (MSB)  first.  The  byte  address  is  automatically
incremented to the next byte address after each byte of data is shifted out. Once the byte address reaches
the last byte of the register (byte address FFh), it will reset to address 00h, the first byte of the register, and
continue  to  increment.  The  instruction  is  completed  by  driving  /CS  high.  The  Read  Security  Register
instruction sequence is shown in Figure 47. If a Read Security Register instruction is issued while an Erase,
Program or Write cycle is in process (BUSY=1) the instruction is ignored and will not have any effects on
the current cycle. The Read Security Register instruction allows clock rates from D.C. to a maximum of FR
(see AC Electrical Characteristics).

ADDRESS

A23-16

Security Register #1

Security Register #2

Security Register #3

00h

00h

00h

A15-12

0 0 0 1

0 0 1 0

0 0 1 1

A11-8

0 0 0 0

0 0 0 0

0 0 0 0

A7-0

Byte Address

Byte Address

Byte Address

Figure 47. Read Security Registers Instruction

Publication Release Date: March 27, 2018
- 53 -                                                                        Revision J

/CSCLKDI(IO0)DO(IO1)Mode 0Mode 301234567Instruction (48h)High Impedance89102829303124-Bit Address2322213210Data Out 1*/CSCLKDI(IO0)DO(IO1)3233343536373839Dummy ByteHigh Impedance404142444546474849505152535455765432107Data Out 2*76543210*7654321043310= MSB*

W25Q64JV

Individual Block/Sector Lock (36h)

The  Individual  Block/Sector  Lock  provides  an  alternative  way  to  protect  the  memory  array  from  adverse
Erase/Program. In order to use the Individual Block/Sector Locks, the WPS bit in Status Register-3 must be
set to 1. If WPS=0, the write protection will be determined by the combination of CMP, SEC, TB, BP[2:0]
bits in the Status Registers. The Individual Block/Sector Lock bits are volatile bits. The default values after
device power up or after a Reset are 1, so the entire memory array is being protected.

To lock a specific block or sector as illustrated in Figure 4d, an Individual Block/Sector Lock command must
be issued by driving /CS low, shifting the instruction code “36h” into the Data Input (DI) pin on the rising
edge of CLK, followed by a 24-bit address and then driving /CS high. A Write Enable instruction must be
executed  before  the  device  will  accept  the  Individual  Block/Sector  Lock  Instruction  (Status  Register  bit
WEL= 1).

Figure 53. Individual Block/Sector Lock Instruction

Publication Release Date: March 27, 2018
- 54 -                                                                        Revision J

/CSCLKDI(IO0)DO(IO1)Mode 0Mode 301234567Instruction (36h)High Impedance8929303124-Bit Address2322210*Mode 0Mode 3= MSB*

W25Q64JV

Individual Block/Sector Unlock (39h)

The  Individual  Block/Sector  Lock  provides  an  alternative  way  to  protect  the  memory  array  from  adverse
Erase/Program. In order to use the Individual Block/Sector Locks, the WPS bit in Status Register-3 must be
set to 1. If WPS=0, the write protection will be determined by the combination of CMP, SEC, TB, BP[2:0]
bits in the Status Registers. The Individual Block/Sector Lock bits are volatile bits. The default values after
device power up or after a Reset are 1, so the entire memory array is being protected.

To unlock a specific block or sector as illustrated in Figure 4d, an Individual Block/Sector Unlock command
must be issued by driving /CS low, shifting the instruction code “39h” into the Data Input (DI) pin on the
rising edge of CLK, followed by a 24-bit address and then driving /CS high. A Write Enable instruction must
be executed before the device will accept the Individual Block/Sector Unlock Instruction (Status Register bit
WEL= 1).

Figure 54. Individual Block Unlock Instruction

Publication Release Date: March 27, 2018
- 55 -                                                                        Revision J

/CSCLKDI(IO0)DO(IO1)Mode 0Mode 301234567Instruction (39h)High Impedance8929303124-Bit Address2322210*Mode 0Mode 3= MSB*

W25Q64JV

Read Block/Sector Lock (3Dh)

The  Individual  Block/Sector  Lock  provides  an  alternative  way  to  protect  the  memory  array  from  adverse
Erase/Program. In order to use the Individual Block/Sector Locks, the WPS bit in Status Register-3 must be
set to 1. If WPS=0, the write protection will be determined by the combination of CMP, SEC, TB, BP[2:0]
bits in the Status Registers. The Individual Block/Sector Lock bits are volatile bits. The default values after
device power up or after a Reset are 1, so the entire memory array is being protected.

To read out the lock bit value of a specific block or sector as illustrated in Figure 4d, a Read Block/Sector
Lock command must be issued by driving /CS low, shifting the instruction code “3Dh” into the Data  Input
(DI) pin on the rising edge of CLK, followed by a 24-bit address. The Block/Sector Lock bit value will be
shifted out on the DO pin at the falling edge of CLK with most significant bit (MSB) first as shown in Figure
55.  If  the  least  significant  bit  (LSB)  is  1,  the  corresponding  block/sector  is  locked;  if  LSB=0,  the
corresponding block/sector is unlocked, Erase/Program operation can be performed.

Figure 55. Read Block Lock Instruction

Publication Release Date: March 27, 2018
- 56 -                                                                        Revision J

/CSCLKDI(IO0)DO(IO1)Mode 0Mode 301234567Instruction (3Dh)High Impedance8910282930313233343536373839XXXXXXX024-Bit Address2322213210Lock Value Out**= MSB*Mode 0Mode 3

W25Q64JV

Global Block/Sector Lock (7Eh)

All Block/Sector Lock bits can be set to 1 by the Global Block/Sector Lock instruction. The command must
be issued by driving /CS low, shifting the instruction code “7Eh” into the Data Input (DI) pin on the rising
edge of CLK, and then driving /CS high. A Write Enable instruction must be executed before the device will
accept the Global Block/Sector Lock Instruction (Status Register bit WEL= 1).

Figure 56. Global Block Lock Instruction for SPI Mode

Global Block/Sector Unlock (98h)

All Block/Sector Lock bits can be set to 0 by the Global Block/Sector Unlock instruction. The command must
be issued by driving /CS low, shifting the instruction code “98h” into the Data Input (DI) pin on the rising
edge of CLK, and then driving /CS high. A Write Enable instruction must be executed before the device will
accept the Global Block/Sector Unlock Instruction (Status Register bit WEL= 1).

Figure 57. Global Block Unlock Instruction for SPI Mode

Publication Release Date: March 27, 2018
- 57 -                                                                        Revision J

/CSCLKDI(IO0)DO(IO1)Mode 0Mode 301234567Mode 0Mode 3Instruction (7Eh)High Impedance/CSCLKDI(IO0)DO(IO1)Mode 0Mode 301234567Mode 0Mode 3Instruction (98h)High Impedance

W25Q64JV

Enable Reset (66h) and Reset Device (99h)

Because of the small package and the limitation on the number of pins, the W25Q64JV provide a software
Reset instruction instead of a dedicated RESET pin. Once the Reset instruction is accepted, any on-going
internal operations will be terminated and the device will return to its default power-on state and lose all the
current  volatile  settings,  such  as  Volatile  Status  Register  bits,  Write  Enable  Latch  (WEL)  status,
Program/Erase Suspend status, Read parameter setting (P7-P0), and Wrap Bit setting (W6-W4).

“Enable Reset (66h)” and “Reset (99h)” instructions can be issued in SPI mode. To avoid accidental reset,
both  instructions  must  be  issued  in  sequence.  Any  other  commands  other  than  “Reset  (99h)”  after  the
“Enable Reset (66h)” command will disable the “Reset Enable” state. A new sequence of “Enable Reset
(66h)” and “Reset (99h)” is needed to reset the device. Once the Reset command is accepted by the device,
the device will take approximately tRST=30us to reset. During this period, no command will be accepted.

Data corruption may happen if there is an on-going or suspended internal Erase or Program operation when
Reset command sequence is accepted by the device. It is recommended to check the BUSY bit and the
SUS bit in Status Register before issuing the Reset command sequence.

Figure 58. Enable Reset and Reset Instruction Sequence

Publication Release Date: March 27, 2018
- 58 -                                                                        Revision J

Mode 0Mode 301234567Instruction (99h)Mode 0Mode 3/CSCLKDI(IO0)DO(IO1)Mode 0Mode 301234567Instruction (66h)High Impedance

W25Q64JV

9.  ELECTRICAL CHARACTERISTICS

9.1 Absolute Maximum Ratings (1)

SYMBOL  CONDITIONS

RANGE

UNIT

PARAMETERS

Supply Voltage

Voltage Applied to Any Pin

VCC

VIO

Transient Voltage on any Pin

VIOT

Storage Temperature

Lead Temperature

TSTG

TLEAD

–0.6 to 4.6

Relative to Ground

–0.6 to VCC+0.4

<20nS Transient
Relative to Ground

–2.0V to VCC+2.0V

–65 to +150

See Note (2)

V

V

V

°C

°C

V

Electrostatic Discharge Voltage

VESD

Human Body Model(3)  –2000 to +2000

Notes:
1. This device has been designed and tested for the specified operation ranges. Proper operation outside
of these  levels is  not guaranteed.  Exposure to absolute maximum ratings may  affect device reliability.
Exposure beyond absolute maximum ratings may cause permanent damage.

2. Compliant with JEDEC Standard J-STD-20C for small body Sn-Pb or Pb-free (Green) assembly and the

European directive on restrictions on hazardous substances (RoHS) 2002/95/EU.

3. JEDEC Std JESD22-A114A (C1=100pF, R1=1500 ohms, R2=500 ohms).

9.2 Operating Ranges

PARAMETER

SYMBOL  CONDITIONS

Supply Voltage(1)

VCC

FR = 133MHz,      fR = 50MHz

FR = 104MHz,      fR = 50MHz

Ambient Temperature,
Operating

TA

Industrial

Industrial Plus

SPEC

MIN

MAX

  UNIT

3.0

2.7

–40

–40

3.6

3.0

+85

+105

V

V

°C

°C

Note:
1.  VCC voltage during Read can operate across the min and max range but should not exceed ±10% of

the programming (erase/write) voltage.

Publication Release Date: March 27, 2018
- 59 -                                                                        Revision J

W25Q64JV

9.3 Power-Up Power-Down Timing and Requirements

PARAMETER

VCC (min) to /CS Low

SYMBOL

tVSL(1)

Time Delay Before Write Instruction

tPUW(1)

Write Inhibit Threshold Voltage

VWI(1)

Note:
1. These parameters are characterized only.

SPEC

MIN

20

5

1.0

MAX

2.0

UNIT

µs

ms

V

Figure 58a. Power-up Timing and Voltage Levels

Figure 58b. Power-up, Power-Down Requirement

Publication Release Date: March 27, 2018
- 60 -                                                                        Revision J

VCCtVSLRead InstructionsAllowedDevice is fullyAccessibletPUW/CS must track VCCProgram, Erase and Write Instructions are ignoredResetStateVCC (max)VCC (min)VWITimeVCCTime/CS must track VCCduring VCC Ramp Up/Down/CS

9.4 DC Electrical Characteristics-

PARAMETER

SYMBOL  CONDITIONS

Input Capacitance

CIN(1)

VIN = 0V(1)

Output Capacitance

Cout(1)

VOUT = 0V(1)

Input Leakage

I/O Leakage

ILI

ILO

Standby Current

ICC1

Power-down Current

ICC2

Current Read Data /
Dual /Quad 50MHz(2)

Current Read Data /
Dual /Quad 80MHz(2)

Current Read Data /
Dual Output Read/Quad
Output Read 104MHz(2)

Current Write Status
Register

ICC3

ICC3

ICC3

/CS = VCC,
VIN = GND or VCC

/CS = VCC,
VIN = GND or VCC

C = 0.1 VCC / 0.9 VCC
DO = Open

C = 0.1 VCC / 0.9 VCC
DO = Open

C = 0.1 VCC / 0.9 VCC
DO = Open

ICC4

/CS = VCC

Current Page Program

ICC5

/CS = VCC

Current Sector/Block
Erase

Current Chip Erase

Input Low Voltage

Input High Voltage

Output Low Voltage

Output High Voltage

ICC6

ICC7

VIL

VIH

VOL

VOH

/CS = VCC

/CS = VCC

IOL = 100 µA

IOH = –100 µA

W25Q64JV

MIN

SPEC

TYP

MAX

UNIT

6

8

±2

±2

50

15

15

18

20

25

25

25

25

10

1

8

10

12

20

20

20

20

–0.5

VCC x 0.7

VCC – 0.2

VCC x 0.3

VCC + 0.4

0.2

pF

pF

µA

µA

µA

µA

mA

mA

mA

mA

mA

mA

mA

V

V

V

V

Notes:
1. Tested on sample basis and specified through design and characterization data. TA = 25° C, VCC = 3.0V.
2. Checker Board Pattern.

Publication Release Date: March 27, 2018
- 61 -                                                                        Revision J

W25Q64JV

9.5 AC Measurement Conditions

PARAMETER

SYMBOL

Load Capacitance

Input Rise and Fall Times

Input Pulse Voltages

Input Timing Reference Voltages

Output Timing Reference Voltages

CL

TR, TF

VIN

IN

OUT

Note:
1. Output Hi-Z is defined as the point where data out is no longer driven.

SPEC

MIN

MAX

30

5

0.1 VCC to 0.9 VCC

0.3 VCC to 0.7 VCC

0.5 VCC to 0.5 VCC

UNIT

pF

ns

V

V

V

Figure 59. AC Measurement I/O Waveform

Publication Release Date: March 27, 2018
- 62 -                                                                        Revision J

Input and OutputTiming Reference LevelsInput Levels0.9 VCC0.1 VCC0.5 VCC

W25Q64JV

SYMBOL

ALT

UNIT

MIN

TYP

MAX

SPEC

fC1

D.C.

133

MHz

fC2

D.C.

104

MHz

50

MHz

9.6 AC Electrical Characteristics(6)

DESCRIPTION

Clock frequency except for Read Data (03h)
instructions (3.0V-3.6V)

Clock frequency except for Read Data (03h)
instructions( 2.7V-3.0V)

Clock frequency for Read Data instruction (03h)

Clock High, Low Time
for all instructions except for Read Data (03h)

Clock High, Low Time
for Read Data (03h) instruction

Clock Rise Time peak to peak

Clock Fall Time peak to peak

FR

FR

fR
tCLH,
tCLL(1)

tCRLH,
tCRLL(1)

tCLCH(2)

tCHCL(2)

/CS Active Setup Time relative to CLK

tSLCH

tCSS

/CS Not Active Hold Time relative to CLK

tCHSL

Data In Setup Time

Data In Hold Time

/CS Active Hold Time relative to CLK

/CS Not Active Setup Time relative to CLK

tDVCH

tDSU

tCHDX

tDH

tCHSH

tSHCH

/CS Deselect Time (for Read)

tSHSL1

tCSH

/CS Deselect Time (for Erase or Program or Write)

tSHSL2

tCSH

D.C.

45%
PC

45%
PC

0.1

0.1

3

3

1

2

3

3

10

50

Output Disable Time

Clock Low to Output Valid
2.7V-3.6V

Output Hold Time

tSHQZ(2)

tDIS

tCLQV

tV

tCLQX

tHO

1.5

7

6

ns

ns

V/ns

V/ns

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

Continued – next page AC Electrical Characteristics (cont’d)

Publication Release Date: March 27, 2018
- 63 -                                                                        Revision J

W25Q64JV

DESCRIPTION

SYMBOL

ALT

UNIT

SPEC

Write Protect Setup Time Before /CS Low

Write Protect Hold Time After /CS High

/CS High to Power-down Mode

/CS High to Standby Mode without ID Read

/CS High to Standby Mode with ID Read

/CS High to next Instruction after Suspend

/CS High to next Instruction after Reset

tWHSL(3)

tSHWL(3)

tDP(2)

tRES1(2)

tRES2(2)

tSUS(2)

tRST(2)

/RESET pin Low period to reset the device

tRESET(2)

1(5)

Write Status Register Time

Page Program Time

Sector Erase Time (4KB)

Block Erase Time (32KB)

Block Erase Time (64KB)

Chip Erase Time

tW

tPP

tSE

tBE1

tBE2

tCE

MIN

TYP

MAX

20

100

3

3

1.8

20

30

15

3

400

1,600

2,000

100

10

0.4

45

120

150

20

ns

ns

µs

µs

µs

µs

µs

µs

ms

ms

ms

ms

ms

s

Notes:
1. Clock high or Clock low must be more than or equal to 45%Pc. Pc=1/fC(MAX)
2. Value guaranteed by design and/or characterization, not 100% tested in production.
3. Only applicable as a constraint for a Write Status Register instruction when SRP=1.
4. It’s possible to reset the device with shorter tRESET (as short as a few hundred ns), a 1us minimum is recommended to ensure reliable

operation.

5. Tested on sample basis and specified through design and characterization data. TA = 25° C, VCC = 3.0V, 25% driver strength.
6. 4-bytes address alignment for Quad Read, start address from [A1,A0]=(0,0).

Publication Release Date: March 27, 2018
- 64 -                                                                        Revision J

W25Q64JV

9.7  Serial Output Timing

9.8  Serial Input Timing

9.9

/WP Timing

Publication Release Date: March 27, 2018
- 65 -                                                                        Revision J

/CSCLKIOoutputtCLQXtCLQVtCLQXtCLQVtSHQZtCLLLSB OUTtCLHMSB OUT/CSCLKIOinputtCHSLMSB INtSLCHtDVCHtCHDXtSHCHtCHSHtCLCHtCHCLLSB INtSHSL/CSCLK/WPtWHSLtSHWLIOinputWrite Status Register is allowedWrite Status Register is not allowed

W25Q64JV

10. PACKAGE SPECIFICATIONS

10.1 8-Pin SOIC 208-mil (Package Code SS)

Symbol

A
A1
A2
b
C
D
D1
E
E1
e
H
L
y

θ

Millimeters

Nom
1.95
0.15
1.80
0.42
0.20
5.28
5.23
5.28
5.23
1.27 BSC
7.90
0.65

---

---

Max
2.16
0.25
1.91
0.48
0.25
5.38
5.33
5.38
5.33

8.10
0.80

0.10

8°

Min
1.75
0.05
1.70
0.35
0.19
5.18
5.13
5.18
5.13

7.70
0.50

---

0°

Inches

Nom
0.077
0.006
0.071
0.017
0.008
0.208
0.206
0.208
0.206
0.050 BSC
0.311
0.026

---

---

Max
0.085
0.010
0.075
0.019
0.010
0.212
0.210
0.212
0.210

0.319
0.031

0.004

8°

Min
0.069
0.002
0.067
0.014
0.007
0.204
0.202
0.204
0.202

0.303
0.020

---

0°

Publication Release Date: March 27, 2018
- 66 -                                                                        Revision J

θ

10.2 8-Pad WSON 6x5-mm (Package Code ZP)

W25Q64JV

Symbol

Millimeters

Min

Nom

Max

A

A1

b
C

D

D2
E

E2
e

L

y

0.70

0.00

0.35

---

5.90

3.35

4.90

4.25

0.55

0.00

0.75

0.02

0.40

0.20 REF

6.00

3.40

5.00

4.30

0.80

0.05

0.48

---

6.10

3.45

5.10

4.35

1.27 BSC

0.60

---

0.65

0.075

Inches

Nom

0.030

0.001

0.016

Max

0.031

0.002

0.019

Min

0.028

0.000

0.014

---

0.008 REF

---

0.232

0.132

0.193

0.167

0.022

0.000

0.236

0.134

0.197

0.169

0.050 BSC

0.024

---

0.240

0.136

0.201

0.171

0.026

0.003

Note:
The metal pad area on the bottom center of the package is not connected to any internal electrical signals. It can be left
floating or connected to the device ground (GND pin). Avoid placement of exposed PCB vias under the pad.

Publication Release Date: March 27, 2018
- 67 -                                                                        Revision J

10.3  8-Pad WSON 8x6mm (Package Code ZE)

W25Q64JV

SYMBOL

A

A1

b

C

D

D2

E

E2

e

L

y

MILLIMETERS
Nom
0.75

Max
0.80

0.02

0.40

0.20 Ref.

8.00

3.40

6.00

4.30

1.27 BSC

0.50

---

0.05

0.48

---

8.10

3.45

6.10

4.35

0.55

0.05

Min
0.70

0.00

0.35

---

7.90

3.35

5.90

4.25

0.45

0.00

INCHES
Nom
0.030

0.001

0.016

Max
0.031

0.002

0.019

Min
0.028

0.000

0.014

---

0.008 Ref.

---

0.311

0.132

0.232

0.167

0.018

0.000

0.315

0.134

0.236

0.169

0.050 BSC

0.020

---

0.319

0.136

0.240

0.171

0.022

0.002

Note:
The metal pad area on the bottom center of the package is not connected to any internal electrical signals. It can be left
floating or connected to the device ground (GND pin). Avoid placement of exposed PCB vias under the pad.

Publication Release Date: March 27, 2018
- 68 -                                                                        Revision J

10.4  8-Pad XSON 4x4x0.45-mm (Package Code XG)

W25Q64JV

Publication Release Date: March 27, 2018
- 69 -                                                                        Revision J

10.5  16-Pin SOIC 300-mil (Package Code SF)

W25Q64JV

Symbol

A

A1

A2

b

C

D

E

E1

e

L

y

θ

Millimeters
Nom
2.49

---

2.31

0.41

0.23

10.31

10.31

7.49

1.27 BSC

0.81

---

---

Max
2.64

0.30

---

0.51

0.28

10.49

10.64

7.59

1.27

0.076

8°

Min
2.36

0.10

---

0.33

0.18

10.08

10.01

7.39

0.38

---

0°

Min
0.093

0.004

---

0.013

0.007

0.397

0.394

0.291

Inches
Nom
0.098

---

0.091

0.016

0.009

0.406

0.406

0.295

0.050 BSC

0.015

0.032

---

0°

---

---

Max
0.104

0.012

---

0.020

0.011

0.413

0.419

0.299

0.050

0.003

8°

Publication Release Date: March 27, 2018
- 70 -                                                                        Revision J

10.7  24-Ball TFBGA 8x6-mm (Package Code TB, 5x5 Ball Array)

W25Q64JV

Note:
Ball land: 0.45mm.      Ball Opening: 0.35mm
PCB ball land suggested <= 0.35mm

Symbol

A

A1

A2

b

D

D1

E

E1

SE

SD

e

Min
---

0.26

---

0.35

7.90

Millimeters
Nom
---

0.31

0.85

0.40

8.00

4.00 BSC

Max
1.20

0.36

---

0.45

8.10

Min
---

0.010

---

0.014

0.311

Inches
Nom
---

0.012

0.033

0.016

0.315

0.157 BSC

Max
0.047

0.014

---

0.018

0.319

5.90

6.00

6.10

0.232

0.236

0.240

4.00 BSC

1.00 TYP

1.00 TYP

1.00 BSC

0.157 BSC

0.039 TYP

0.039 TYP

0.039 BSC

ccc

---

---

0.10

---

---

0.0039

Publication Release Date: March 27, 2018
- 71 -                                                                        Revision J

10.8 24-Ball TFBGA 8x6-mm (Package Code TC, 6x4 ball array)

W25Q64JV

Note:
Ball land: 0.45mm.      Ball Opening: 0.35mm
PCB ball land suggested <= 0.35mm

Symbol

A

A1

b

D

D1

E

E1

e

ccc

Min
---

0.25

0.35

7.95

Millimeters
Nom
---

0.30

0.40

8.00

5.00 BSC

Max
1.20

0.35

0.45

8.05

Min
---

0.010

0.014

0.313

Inches
Nom
---

0.012

0.016

0.315

0.197 BSC

Max
0.047

0.014

0.018

0.317

5.95

6.00

6.05

0.234

0.236

0.238

3.00 BSC

1.00 BSC

0.118 BSC

0.039 BSC

---

---

0.10

---

---

0.039

Publication Release Date: March 27, 2018
- 72 -                                                                        Revision J

10.9  12-Ball WLCSP (Package Code BY)

W25Q64JV

Max
0.512
0.182
0.330
0.360

Min
0.017
0.006
0.012
0.009

Symbol

A
A1
c
b
D1
D2
D3
E1
E2
E3
eD
eE
aaa
bbb
ccc
ddd

Min
0.432
0.152
0.280
0.240

Millimeters
Nom
0.472
0.167
0.305
0.300
1.200
0.301
0.301
2.200
0.396
0.396
0.5 BSC
0.5 BSC
0.10
0.10
0.03
0.15

Max
0.020
0.007
0.014
0.014

Inches
Nom
0.019
0.007
0.013
0.012
0.0472
0.0119
0.0119
0.0866
0.0156
0.0156
0.020 BSC
0.020 BSC
0.004
0.004
0.001
0.006

Notes:

1.  Dimension b is measured at the maximum solder bump diameter, parallel to primary datum C.
2.  Dimension D and E; please contact Winbond for details.

Publication Release Date: March 27, 2018
- 73 -                                                                        Revision J

11.  ORDERING INFORMATION

W25Q64JV

  W(1) 25Q    64J    V xx(2)    I

W    =    Winbond

25Q    =    SpiFlash Serial Flash Memory with 4KB sectors, Dual/Quad I/O

64J    =    64M-bit

V    =    2.7V to 3.6V

SS = 8-pin SOIC 208-mil
ZP = WSON8 6x5-mm
XG = XSON 4x4x0.45-mm
TC = TFBGA 8x6-mm (6x4 ball array)

SF = 16-pin SOIC 300-mil
ZE = WSON8 8x6-mm
TB = TFBGA 8x6-mm (5x5 ball array)
BY = 12-ball WLCSP

  I    =    Industrial (-40°C to +85°C)

                  J    =    Industrial Plus (-40°C to +105°C)

                                  (3,4)

Q(5)    =    Green Package (Lead-free, RoHS Compliant, Halogen-free (TBBA), Antimony-Oxide-free Sb2O3)

with QE = 1 (fixed) in Status register-2. Backward compatible to FV family.

M(6)    =    Green Package (Lead-free, RoHS Compliant, Halogen-free (TBBA), Antimony-Oxide-free Sb2O3)

with QE = 0 (programmable) in Status register-2. New device ID is used to identify JV family

Notes:

1.  The “W” prefix is not included on the part marking.
2.  Only the 2nd letter is used for the part marking; WSON package type ZP is not used for the part marking.
3.  Standard bulk shipments are in Tube (shape E). Please specify alternate packing method, such as Tape and

Reel (shape T) or Tray (shape S), when placing orders.

4.  For shipments with OTP feature enabled, please specify when placing orders.
5.
6.  For DTR, QPI supporting, please refer to W25Q64JV DTR datasheet.

/HOLD function is disabled to support Standard, Dual and Quad I/O without user setting.

Publication Release Date: March 27, 2018
- 74 -                                                                        Revision J

W25Q64JV

11.1  Valid Part Numbers and Top Side Marking

The following table provides the valid part numbers for the W25Q64JV SpiFlash Memory. Please contact
Winbond for specific availability by density and package type. Winbond SpiFlash memories use a 12-digit
Product Number for ordering. However, due to limited space, the Top Side Marking on all packages uses
an abbreviated 10-digit number.
  W25Q64JV-IQ/JQ valid part numbers:

PACKAGE TYPE

DENSITY

PRODUCT NUMBER

TOP SIDE MARKING

SOIC-8 208-mil

SOIC-16 300-mil

64M-bit

64M-bit

SF

SS

W25Q64JVSSIQ
W25Q64JVSSJQ
W25Q64JVSFIQ
W25Q64JVSFJQ
W25Q64JVZPIQ
ZP(1)
W25Q64JVZPJQ
W25Q64JVZEIQ
ZE(1)
W25Q64JVZEJQ
W25Q64JVXGIQ
W25Q64JVXGJQ

XG
64M-bit

64M-bit

64M-bit

XSON-8 4x4-mm

WSON-8 6x5-mm

WSON-8 8x6-mm

TB(2)

TFBGA-24 8x6-mm
(5x5 Ball Array)

64M-bit

TC(2)

TFBGA-24 8x6-mm
(6x4 Ball Array)

64M-bit

BY(2)

64M-bit

12-ball WLCSP

W25Q64JVTBIQ
W25Q64JVTBJQ

W25Q64JVTCIQ
W25Q64JVTCJQ

W25Q64JVBYIQ

25Q64JVSIQ
25Q64JVSJQ
25Q64JVFIQ
25Q64JVFJQ
25Q64JVIQ
25Q64JVJQ
25Q64JVIQ
25Q64JVJQ
Q64JVXGIQ
Q64JVXGJQ

25Q64JVBIQ
25Q64JVBJQ

25Q64JVCIQ
25Q64JVCJQ

6CJI
•Qyw(4)

Publication Release Date: March 27, 2018
- 75 -                                                                        Revision J

W25Q64JV

  W25Q64JV-IM/JM(3) valid part numbers:

PACKAGE TYPE

DENSITY

PRODUCT NUMBER

TOP SIDE MARKING

SS

64M-bit

SOIC-8 208-mil

SF

64M-bit

SOIC-16 300-mil

WSON-8 6x5-mm

WSON-8 8x6-mm

64M-bit

64M-bit

XG
64M-bit

XSON-8 4x4-mm

TB(2)

W25Q64JVSSIM
W25Q64JVSSJM
W25Q64JVSFIM
W25Q64JVSFJM
W25Q64JVZPIM
W25Q64JVZPJM
W25Q64JVZEIM
W25Q64JVZEJM
W25Q64JVXGIM
W25Q64JVXGJM

ZP

ZE(1)

25Q64JVSIM
25Q64JVSJM
25Q64JVFIM
25Q64JVFJM
25Q64JVIM
25Q64JVJM
25Q64JVIM
25Q64JVJM
Q64JVXGIM
Q64JVXGJM

TFBGA-24 8x6-mm
(5x5 Ball Array)

64M-bit

W25Q64JVTBIM

25Q64JVBIM

Note:
1.     For WSON packages, the package type ZP and ZE is not used in the top side marking.
2.  These package types are special order, please contact Winbond for more information
3.  For DTR, QPI supporting, please refer to W25Q64JV DTR datasheet.
4.  yw: year/ week.

Publication Release Date: March 27, 2018
- 76 -                                                                        Revision J

W25Q64JV

12.  REVISION JISTORY

VERSION

DATE

PAGE

DESCRIPTION

A

B

C

D

E

F

G

H

I

J

2015/01/07

2016/04/12

2016/06/03

2016/08/30

2016/10/24

2016/11/15

16
73-76

4
8

New Create Datasheet

Removed “Preliminary”

Updated QE description
Added TFBGA information

Added data retention
Added TFBGA 5X5

Removed PDIP 8 information

12

Updated Status Register-1 table

2016/12/14

4-5, 67, 71-72

Updated XSON package information

2017/05/11

20, 73-74

Updated /WP information
Updated W25Q64JV-IM order information

2017/11/22

8, 72-75

Added WLCSP package type

2018/03/27

12, 13
61,63,64
4, 74-76

Updated OTP Notice and SR1 Figure
Updated ICC3, tPP, tSLCH, tCHSL, tDVCH, tCHDX
Added industrial plus information

Trademarks

Winbond and SpiFlash are trademarks of Winbond Electronics Corporation.
All other marks are the property of their respective owner.

Important Notice

Winbond products are not designed, intended, authorized or warranted for use as components in systems or
equipment  intended  for  surgical  implantation,  atomic  energy  control  instruments,  airplane  or  spaceship
instruments, transportation instruments, traffic signal instruments, combustion control instruments, or for other
applications  intended  to  support  or  sustain  life.  Furthermore,  Winbond  products  are  not  intended  for
applications wherein failure of Winbond products could result or lead to a situation wherein personal injury,
death or severe property or environmental damage could occur.  Winbond customers using or selling these
products for use in such  applications do so  at their  own risk and agree to fully  indemnify Winbond for any
damages resulting from such improper use or sales.

Information  in  this  document  is  provided  solely  in  connection  with  Winbond  products.  Winbond
reserves the right to make changes, corrections, modifications or improvements to this document and
the products and services described herein at any time, without notice.

Publication Release Date: March 27, 2018
- 77 -                                                                        Revision J

