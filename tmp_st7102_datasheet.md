ST7102

Datasheet

Sitronix reserves the right to change the contents in this document
without  prior  notice,  please  contact  Sitronix  to  obtain  the  latest
version of datasheet before placing your order. No responsibility is
assumed by Sitronix for any infringement of patent or other rights
of third parties which may result from its use.
© 2023 Sitronix Technology Corporation. All rights reserved.

V0.22

2024/10

深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006                                                                               ST7102

LIST OF CONTENT

1

2

3

GENERAL DESCRIPTION ....................................................................................................................... 7

FEATURES ............................................................................................................................................... 8

BLOCK DIAGRAM ................................................................................................................................. 10

3.1

FUNCTION BLOCK ............................................................................................................................. 10

4

PIN INFOMATION .................................................................................................................................... 11

4.1

4.2

PAD ARRANGEMENT ........................................................................................................................... 11

PIN DEFINITION................................................................................................................................. 12

4.2.1  Voltage Pins ............................................................................................................................... 12

4.2.2  Control Pins................................................................................................................................ 13

4.2.3  MIPI Interface Pins ..................................................................................................................... 15

4.2.4  RGB Interface Pins .................................................................................................................... 15

4.2.5  Driver Panel Related Pins .......................................................................................................... 16

4.2.6  Touch Related Pins .................................................................................................................... 16

4.2.7  External power Supply Pins ....................................................................................................... 17

4.2.8  Other .......................................................................................................................................... 17

4.3

POWER SUPPLY CONFIGURATION ...................................................................................................... 18

5

FUNCTION DESCRIPTION .................................................................................................................... 19

5.1

5.2

FRAME TEARING EFFECT INTERFACE ................................................................................................. 19

CONTENT ADAPTIVE BACKLIGHT CONTROL (CABC2.0) ...................................................................... 21

5.2.1  Definition of CABC ..................................................................................................................... 21

5.2.2  Minimum Brightness Setting of CABC Function ........................................................................ 25

5.2.3  Display Dimming ........................................................................................................................ 27

5.2.3.1  General Description ............................................................................................................... 27

5.2.3.2

Dimming Requirement ........................................................................................................... 27

5.2.4  Definition of Brightness Transition Time .................................................................................... 28
COLOR ENHANCEMENT (CE2.0) ........................................................................................................ 30

5.3

5.4

MIPI-DSI INTERFACE ........................................................................................................................ 31

5.4.1  Display Module Pin Configuration for DSI ................................................................................. 32

5.4.2  Display Serial Interface (DSI) ..................................................................................................... 33

5.4.2.1  General description ............................................................................................................... 33

5.4.2.2

Interface level communication ............................................................................................... 33

5.4.2.2.1

General .............................................................................................................................. 33

5.4.2.2.2

DSI-CLOCK Lanes ............................................................................................................ 36

5.4.2.2.2.1  Low-Power Mode (LPM) ................................................................................................... 37

V0.22                                                                          Page  2  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006                                                                               ST7102
5.4.2.2.2.2  Ultra-Low Power Mode (ULPM) ........................................................................................ 39

5.4.2.2.2.3  High-Speed Clock Mode (HSCM) ..................................................................................... 40

5.4.2.2.3

DSI-DATA Lanes ............................................................................................................... 42

5.4.2.2.3.1  General .............................................................................................................................. 42

5.4.2.2.3.2  ESCAPE MODE ................................................................................................................ 42

5.4.2.2.3.3  High-Speed Data Transmission (HSDT) ........................................................................... 49

5.4.2.2.3.4  Bus Turnaround (BTA) ....................................................................................................... 52

5.4.2.3

Packer Level Communication ................................................................................................ 53

5.4.2.3.1

Short Packet (SPa) and Long Packet (LPa) Structure ...................................................... 53

5.4.2.3.1.1  Bit Order of the Byte on Packets ....................................................................................... 55

5.4.2.3.1.2  Byte Order of the Multiple Byte Information on Packets ................................................... 55

5.4.2.3.1.3  Packet Header (PH) .......................................................................................................... 56

5.4.2.3.1.4  Packet Date (PD) on the Long Packet (LPa) .................................................................... 71

5.4.2.3.1.5  Packet Footer (PF) on the Long Packet (LPa) .................................................................. 71

5.4.2.3.2

Packet Transmissions ....................................................................................................... 73

5.4.2.3.2.1  Packet from the MCU to the Display Module .................................................................... 73

5.4.2.3.2.2  Packet from the Display Module to the MCU .................................................................... 74

5.5

SERIAL INTERFACE_ CONTROL BUS ( RGB_VIDEO / MIPI VIDEO) ........................................................ 79

5.5.1  SPI Write mode .......................................................................................................................... 79

5.5.1.1

SPI8 & SPI9 ........................................................................................................................... 79

5.5.1.2

SPI16 ..................................................................................................................................... 81

5.5.2  SPI Read mode .......................................................................................................................... 84

5.5.2.1

SPI8 & SPI9 ........................................................................................................................... 84

5.5.2.1

SPI16 ..................................................................................................................................... 87

5.6

RGB INTERFACE .............................................................................................................................. 89

5.6.1  RGB Color Format ..................................................................................................................... 90

5.6.2  RGB Interface Definition ............................................................................................................ 92

5.6.1  RGB Interface Mode Selection .................................................................................................. 93

5.7
5.8

DIGITAL GAMMA ................................................................................................................................ 95
GAMMA CORRECTION FUNCTION ....................................................................................................... 96

5.8.1  Gamma Correction Registers .................................................................................................... 96

5.8.2  Gamma function architecture ..................................................................................................... 98

5.8.3  Grayscale voltage formula ......................................................................................................... 99

5.9

RESET FUNCTION ............................................................................................................................ 103

5.9.1  Reset Timing Diagram ............................................................................................................. 103

5.9.1.1

Power On Reset & HWRST Reset ...................................................................................... 103

5.10

ABNORMAL POWER OFF FUNCTION .................................................................................................. 104

V0.22                                                                          Page  3  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006                                                                               ST7102
Abnormal Power Off ............................................................................................................ 104

5.10.1

5.11

5.12

BASIC OPERATION MODE ................................................................................................................ 105

POWER ON/OFF SEQUENCE............................................................................................................ 106

5.12.1

5.12.2

Power On/Off Timing ........................................................................................................... 107

Power Ramp Up/Down Specifications ................................................................................. 108

5.13

INSTRUCTION SETTING SEQUENCE .................................................................................................. 109

5.13.1

5.13.2

Sleep Enter/Exit Sequences ................................................................................................ 109

Deep Standby Mode Enter/Exit Sequences ......................................................................... 110

5.14

TOUCH INTERFACE PROTOCOL .......................................................................................................... 111

5.14.1

SPI interface ......................................................................................................................... 111

5.14.1.1

5.14.1.2

5.14.2

5.14.3

Command Protocol ........................................................................................................... 111

Data Protocol .................................................................................................................... 112

I2C ........................................................................................................................................ 114

IRQ ....................................................................................................................................... 115

6

COMMAND DESCRIPTION .................................................................................................................. 116

6.1

6.2

USER COMMAND SET (UCS) LIST .................................................................................................... 116

USER COMMAND SET (UCS) DESCRIPTION .................................................................................. 118

6.2.1  NOP (00H) : No Operation ........................................................................................................ 118

6.2.2  SWRESET (01H): Software Reset ........................................................................................... 119

6.2.3  RDDID (04H) Read Display Identification Information ............................................................. 120

6.2.4  RDNUMED (05H) Read Number of Errors on DSI .................................................................. 121

6.2.5  RDDST (09H) Read Display Status ......................................................................................... 122

6.2.6  RDDPM (0AH): Read Display Power Mode ............................................................................. 124

6.2.7  RDDMADCTR (0BH): Read Display MADCTR ....................................................................... 125

6.2.8  RDDCOLM (0CH): Read Display Color Mode ......................................................................... 126

6.2.9  RDDIM (0DH): Read Display Image Mode .............................................................................. 127

6.2.10

6.2.11
6.2.12

6.2.13

6.2.14

6.2.15

6.2.16

6.2.17

6.2.18

6.2.19

6.2.20

RDDSM (0EH): Read Display Signal Mode ........................................................................ 128

RDDSDR(0FH): Read Display Self-Diagnostic Result ........................................................ 129
SLPIN (10H): Sleep In ......................................................................................................... 130

SLPOUT (11H): Sleep Out .................................................................................................. 131

NORON (13H): Normal display mode On ........................................................................... 132

INVOFF (20H) : Display Inversion Off ................................................................................. 133

INVON (21H) : Display Inversion On ................................................................................... 134

ALLPOFF (22H): All pixel off ............................................................................................... 135

ALLPON (23H): All pixel on ................................................................................................. 136

GAMSEL (26H): Gamma Curve Select ............................................................................... 137

DISPOFF (28H): Display Off ............................................................................................... 138

V0.22                                                                          Page  4  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006                                                                               ST7102
DISPON (29H): Display On ................................................................................................. 138

TEOFF (34H): Tearing Effect Line OFF ............................................................................... 140

TEON (35H): Tearing Effect Line ON .................................................................................. 141

MADCTR (36H): Memory Data Access Control .................................................................. 142

IDMOFF (38H): Idle Mode Off ............................................................................................. 144

IDMON (39H): Idle Mode On ............................................................................................... 145

TESLWR (44H): Write TE Scan Line ................................................................................... 146

RDSCNL (45H): Read Scan Line ........................................................................................ 147

DSTB (4FH): Deep Standby Mode ON................................................................................ 148

6.2.21

6.2.22

6.2.23

6.2.24

6.2.25

6.2.26

6.2.27

6.2.28

6.2.29

6.2.30  WRDISBV (51H) Write Display Brightness ......................................................................... 149

6.2.31

RDDISBV (52H) Read Display Brightness Value ................................................................ 150

6.2.32  WRCTRLD (53H) Write CTRL Display ................................................................................ 151

6.2.33

RDCTRLD (54H) Read CTRL Display ................................................................................. 152

6.2.34  WRCABC (55H) Write Content Adaptive Brightness Control .............................................. 153

6.2.35

RDCABC (56H) Read Content Adaptive Brightness Control .............................................. 155

6.2.36  WRCABCMB (5EH) Write CABC Minimum Brightness....................................................... 156

6.2.37

6.2.38

6.2.39

6.2.40

6.2.41

6.2.42

6.2.43

6.2.44

6.2.45

6.2.46

RDCABCMB (5FH) Read CABC Minimum Brightness ....................................................... 157

RDDID1 (DAH) Read Display Identification Information ..................................................... 158

RDDID2 (DBH) Read Display Identification Information ..................................................... 159

RDDID3 (DCH) Read Display Identification Information ..................................................... 160

RDDDBS (A1H) : Read DDB Start ...................................................................................... 161

RDDDBC (A8H) : Read DDB Continue ............................................................................... 162

RDFCS (AAH) : Read First Checksum ................................................................................ 163

RDCCS (AFH) : Read Continue Checksum ........................................................................ 164

RDICID (F4H) : Read Sitronix IC ID .................................................................................... 165

MIPIEXTFMAT (F9H) : MIPI Extension Format ................................................................... 166

7

ELECTRICAL CHARACTERISTICS .................................................................................................... 167

7.1
7.2

ABSOLUTE MAXIMUM RATINGS ........................................................................................................ 167
DC CHARACTERISTICS .................................................................................................................... 168

7.2.1  Basic Characteristics ............................................................................................................... 168

7.2.2  Current Consumption ............................................................................................................... 169

7.2.3  MIPI DC Characteristic ............................................................................................................ 170

7.3

AC CHARACTERISTICS .................................................................................................................... 171

7.3.1  MIPI Timing .............................................................................................................................. 171

7.3.2  MIPI Interface Timing ............................................................................................................... 175

7.3.3  RGB timing ............................................................................................................................... 176

7.3.4  SPI9 & SPI16 Timing ............................................................................................................... 177

V0.22                                                                          Page  5  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006                                                                               ST7102
7.3.5  SPI8 (4 line) Timing ................................................................................................................. 178

7.3.6  Touch SPI Timing ..................................................................................................................... 179

7.3.7  Touch I2C timing ...................................................................................................................... 180

7.3.8  Reset Timing ............................................................................................................................ 181

7.3.9  Abnormal Timing: ..................................................................................................................... 182

8

REVISION HISTORY ............................................................................................................................ 183

V0.22                                                                          Page  6  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006                                                                               ST7102

1  GENERAL DESCRIPTION

The ST7102 is a System-on-Chip (SoC) driver LSI designed for TFT LCD controller with a build-in touch panel controller

and  suitable  for  small  to  medium  size  portable  devices  such  as  mobile  phone  or  tablet.  ST7102  can  support  up  to

480RGBx1280  dots  panel  and  support  16,777,216-color.  There  is  no  internal  RAM  in  ST7102.  The  1440-channel

source driver has true 8-bit resolution, which generates 256 Gamma-corrected values by an internal D/A converter.

The  ST7102  incorporates  with  several  charge  pumps  to  generate  various  voltage  levels  that  form  an on-chip power

management  system  for  gate  driver  and  source  driver.  The  built-in  timing  controller  in  ST7102  can  support  MIPI

interface (included 4-lane/1-port) and RGB interface display serial interface with low EMI noise and touch protocol via

standard integrated circuit bus(I2C) or serial peripheral interface (SPI). The ST7102 also supports a standby mode for

power  control  consideration.  For  further  power  control,  the  dynamic  backlight  control  function  basing  on  displaying

image content is also supported.

V0.22                                                                          Page  7  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006

                                                                               ST7102

2  FEATURES

–Single-chip FWVGA Amorphous TFT Controller/Driver.

–Display Resolution

◼

◼

◼

480RGB(H) x 864(V) (WVGA)

480RGB(H) x 960(V) (FWVGA)

480RGB(H) x 1280(V)

–Display Modes (Color Mode)

◼

◼

Full Color: 16M, RGB=(888) max. Idle Mode Off

Color Reduce: 8-color, RGB=(111), Idle Mode On

–System Interfaces

◼

◼

MIPI DSI: MIPI DSI (DSI v1.01.00, D-PHY v1.00.00 and DCS v1.01)

16/18/24 RGB interface (with 8bits / 9bits command interface)

–Display Features

◼

◼

◼

◼

◼

◼

Outputs 256γ-corrected values using an internal true 8-bit resolution D/A converter to achieve 16,777,216

colors

Supports CGOUTR[1:16]/CGOUTL[1:16] GIP control signal

Individual gamma correction setting for RGB dots (1 analog/ 3 digital gamma)

Supports column/1-dot/2-dot/4-dot inversion

Power saving mode (standby mode)

Supports low frame rate mode

–Built-in Color Image Processing Functions

◼

◼

◼

Support WB function

Color enhance (CE 2.0)

Content adaptive brightness control (CABC 2.0)

–On Chip Function

◼

◼

◼

◼

◼

◼

Support VCOM ground level driving scheme

Internal oscillator for display clock generation

Timing controller

Built–in NVM to store VCOM/GVDD calibration and ID1~ID3

Built–in NVM to store analog gamma, digital gamma and color enhancement.

Built–in NVM to store panel timing, analog power setting, and etc.

–Supply Power Range

◼

◼

◼

Logic power supply voltage (IOVCC): 1.65 ~ 3.3V

Positive analog power supply voltage (AVDD): 4.5 ~ 6.3V

Negative analog power supply voltage (AVEE): -4.5 ~ -6.3V

–Touch Feature

V0.22                                                                          Page  8  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006                                                                               ST7102

16-bits MCU optimized for capacitive sensing and other human interactions

64 analog front ends (AFEs) support up to 512 receiver pads

Supports I2C and SPI protocol for communication with the host

64 analog front ends (AFEs) support up to 512 receiver pads

10 fingers support

Support Long-V sensing mode

Support noise detection and automatic frequency hopping

Support passive stylus

Support proximity

Hopping frequency range from 50kHz to 120kHz to minimize noise interference

High signal- to noise ratio (>50dB SNR) touch AFE enables

Wake-up gesture

Touch FW host download

◼

◼

◼

◼

◼

◼

◼

◼

◼

◼

◼

◼

◼

–Optimized Layout for COG Assembly

– Operating Temperature Range: −30ºC to +75 ºC

V0.22                                                                          Page  9  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006                                                                               ST7102

3  BLOCK DIAGRAM

3.1

Function Block

V0.22                                                                          Page  10  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

DataRAMProgramRAMEmbedded FlashSITRONIX TDDI16-bit 80251 MCUDynamicBacklightControllerRx480Dot InversionSource DriversDigital Image Processing Block· Content Adaptive Backlight Control (CABC)· Color Enhancement Control Circuit (CE)· Paper mode· Scaling down functionNVMDSIInterfaceDisplayCommandControllerPartialSRAMVPPMIPI_CLKP/CLKNMIPI_D0P/D0NMIPI_D1P/D1NMIPI_D2P/D2NMIPI_D3P/D3NIOVCCMIPI VDDVoltageLogicVDDVoltageDPHYVCC(Internal VPP)Touch Timing ControlOSCTimingGeneratorDC/DC ConvertorLevel Shift(GIP)DisplayTiming ControlVGH/VGLVoltage321440AVDDAVEEVCOMCGOUTL[1:16]CGOUTR[1:16]S[1:1440]SDUM[0:3]FRMLevel ShiftReferenceVoltageVREFVGHVGLTouchI2CTouchSPITouch AnalogTouchInterfaceInternalRegisterTP_I2C_SDATP_I2C_SCLTP_SPI_MOSITP_SPI_CSTP_SPI_SCLKTP_SPI_MISOTP_INTTE512RX[1:512]LEDPWMPNSWDSW[0:1]Hardware AccelerationFlash SPI MasterFLASH_SPI_MISOFLASH_SPI_CSNFLASH_SPI_SCKFLASH_SPI_MOSIDigital IOGPIO[2:0]SPI3/4/16Data0~23(RGB_interface)RGB control pins深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006
                                                                               ST7102

4  PIN INFOMATION

4.1

Pad Arrangement

⚫

Chip Information

Chip size

22500um x 918um

Chip height & width tolerance

Chip thickness

Pad Location

Coordinate Origin

Note:

Chip size do not include scribe line

20um

250um

Pad center

Chip center

V0.22                                                                          Page  11  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006                                                                               ST7102

4.2

Pin Definition

4.2.1

Voltage Pins

Name

I/O

PAD Type

(Voltage Level)

Description

IOVCC

AVDD

AVEE

AGND

DGND

VSS

DPHYGND

VDD

VDDM

DPHYVCC

TVH

TVL

VAG

VGH

VGHO1

VGL

VGLI

GVDDP

GVDDN

I

I

I

-

-

--

-

O

O

I

O

O

O

O

O

O

O

O

O

Power Supply

External power supply for internal logic circuit

Power Supply

External positive power supply for analog circuit

Power Supply

External negative power supply for analog circuit

GND

GND

GND

GND

Analog

Analog ground, need to connect to GND from FPC

Digital ground, need to connect to GND from FPC

TP ground, need to connect to GND from FPC

Ground for MIPI DPHY circuit, need to connect to GND from FPC

Regulator  output  for  logic,  all  pins  need  to  connect  together  from

FPC

Analog

Regulator output for MIPI DSI,please keep it open

Analog

Analog

Analog

External power supply for MIPI PHY circuit

This pin need connect to IOVCC from FPC

Touch  output high  voltage  level,  all  pins  need to  connect  together

from FPC.

Touch  output  low  voltage  level,  all  pins  need  to  connect  together

from FPC.

Analog

Active guard signal, all pins need to connect together from FPC.

Analog

Analog

Analog

Analog

Analog

For IGZO panel, VGH/VGHO1/VGL need have capacitor to VAG

Step-up output voltage for panel, all pins need to connect together

from FPC.

Step-up output voltage, all pins need to connect together from FPC.

Step-up output voltage for panel, all pins need to connect together

from FPC.

For Test used, please keep it open

Positive LDO output for gamma circuit.

If not used, please keep it open.

Analog

Negative LDO output for gamma circuit.

If not used, please keep it open.

VPP

Analog

Programming OTP Power.

I/O

Internal power : keep it open.

External power :supply voltage (8.5V); the current of Ivpp must

be more than 10mA.

V0.22                                                                          Page  12  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006                                                                               ST7102

4.2.2

Control Pins

Name

I/O

PAD Type

(Voltage Level)

Description

RESX

I

Digital(IOVCC)  Global reset signal. Low active.

IM[2:0]

I

Digital(IOVCC)

LANSEL[1:0]

I

Digital(IOVCC)

Interface mode select pins

IM2

IM1

IM0

Interface Selection

0

0

0

0

1

1

1

1

0

0

1

1

0

0

1

1

0

1

0

1

0

1

0

1

MIPI

Reserved

Reserved

RGB(video)+SPI9 control

RGB(video)+SPI8 control

RGB(video)+SPI16 (rise) control

RGB(video) +SPI16 (fall) control

Reserved

Input pin to select 1 data lane to 4 data lanes in MIPI interface.

Lanesel 1

Lanesel 0

Interface Selection

0

0

1

1

0

1

0

1

1 Lane

2 Lanes

3 Lanes

4 Lanes

TE

O

Digital(IOVCC)

S/W command. When this pin is not activated (TE function OFF), this pin is

Tearing effect output pin to synchronies MCU to frame writing, activated by

GND level.

LEDPWM

O

Digital(IOVCC)

LCD backlight control PWM output pin

FRM

PSWAP

I

I

Digital(IOVCC)

Test pin, please connect to ground or floating.

Digital(IOVCC)  MIPI Lane polarity swap pin.

V0.22                                                                          Page  13  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006

                                                                               ST7102

DSWAP[1:0]

I

Digital(IOVCC)

MIPI data lane swap and polarity swap table.

PNSWAP

DSWAP[1:0]

D2P

D2N

D1P

D1N

CLKP

CLKN

D0P

D0N

D3P

D3N

0

1

00

01

10

11

00

01

10

11

D3N

D3P

D2N

D2P

CLKN

CLKP

D1N

D1P

D0N

D0P

D3N

D3P

D0N

D0P

CLKN

CLKP

D1N

D1P

D2N

D2P

D0N

D0P

D1N

D1P

CLKN

CLKP

D2N

D2P

D3N

D3P

D2N

D2P

D1N

D1P

CLKN

CLKP

D0N

D0P

D3N

D3P

D3P

D3N

D2P

D2N

CLKP

CLKN

D1P

D1N

D0P

D0N

D3P

D3N

D0P

D0N

CLKP

CLKN

D1P

D1N

D2P

D2N

D0P

D0N

D1P

D1N

CLKP

CLKN

D2P

D2N

D3P

D3N

D2P

D2N

D1P

D1N

CLKP

CLKN

D0P

D0N

D3P

D3N

SDO

O

IOVCC

-If not used, please floating

RGB/MIPI SPI9, RGB/MIPI SPI8, RGB/MIPI SP16 output data Pin.

SDA

DCX

CSX

SCL

I

I

I

I

RGB/MIPI SPI9, RGB/MIPI SPI8, RGB/MIPI SP16 input data Pin.

IOVCC

-If not used, please fix this pin at GND or IOVCC.

- The 8 bit SPI interface (DCX): The signal for command or

IOVCC

parameter select.    Low: Command    High: Parameter

-If not used, please fix this pin at IOVCC or GND.

RGB/MIPI SPI9, RGB/MIPI SPI8, RGB/MIPI SP16 Chip select pin.

IOVCC

CSX=’0’ : Low enable.

CSX=’1’ : High disable

-If not used, please fix this pin at IOVCC.

RGB/MIPI SPI9, RGB/MIPI SPI8, RGB/MIPI SP16 CLK Pin.

IOVCC

-If not used, please fix this pin at IOVCC.

V0.22                                                                          Page  14  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006
                                                                               ST7102

4.2.3

MIPI Interface Pins

Name

I/O

PAD Type

(Voltage Level)

Description

CLKP

CLKN

I

I

MIPI

MIPI

MIPI-DSI clock lane positive-end input pin

MIPI-DSI clock lane negative-end input pin

DATA0P

I/O

MIPI

DATA0N

I/O

MIPI

MIPI-DSI data lane 0 positive-end input/output pin

* Please connected to GND if not used

MIPI-DSI data lane 0 negative-end input/output pin

DATA1P

DATA1N

DATA 2P

DATA 2N

DATA 3P

DATA 3N

I

I

I

I

I

I

MIPI

MIPI

MIPI

MIPI

MIPI

MIPI

4.2.4

RGB Interface Pins

Name

I/O

PAD Type

(Voltage Level)

* Please connected to GND if not used

MIPI-DSI data lane 1 positive-end input pin

* Please connected to GND if not used

MIPI-DSI data lane 1 negative-end input pin

* Please connected to GND if not used.

MIPI-DSI data lane 2 positive-end input pin

* Please connected to GND if not used

MIPI-DSI data lane 2 negative-end input pin

* Please connected to GND if not used

MIPI-DSI data lane 3 positive-end input pin

* Please connected to GND if not used

MIPI-DSI data lane 3 negative-end input pin

* Please connected to GND if not used

Description

D0~D23

ENABLE

PCLK

HSYNC

VSYNC

I

I

I

I

I

IOVCC

RGB interface data bus. -If not used, please fix this pin at GND or IOVCC.

Data enable signal in RGB interface. -If not used, please fix this pin at

IOVCC

IOVCC

IOVCC

IOVCC

IOVCC or Gnd.

Dot clock signal in RGB interface. (DOTCLK)

-If not used, please fix this pin at IOVCC or Gnd

-Horizontal (Line) synchronizing input signal in RGB interface.

-If not used, please fix to the IOVCC or Gnd.

Vertical (Frame) synchronizing input signal in RGB interface.

-If not used, please fix to the IOVCC. or Gnd

V0.22                                                                          Page  15  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006

                                                                               ST7102

4.2.5

Driver Panel Related Pins

Name

I/O

PAD Type

(Voltage Level)

Description

CGOUTL[16:1]

O

Analog

Panel control signal output pads for left side GIP.

CGOUTR[16:1]  O

Analog

Panel control signal output pads for right side GIP.

S1:S1440

VCOM

COGTEST12

COGTEST34

O

O

O

Output  source  driver  signals.  The  D/A  converted  256-gray-scale  analog

Analog

voltage is output.

Analog

VCOM voltage output, for monitor

No level

For ITO resistance trace, if not use please floating

VCOM_PASS_R

VCOM_PASS_L

VCOM_OPT_R

VCOM_OPT_L

-

No level

Pass line for VCOM_OPT

If use, connect to VCOM_OPT

O

Analog

VCOM optional buffer output. (Connect to Panel Vcom)

4.2.6

Touch Related Pins

PAD Type

Name

I/O

(Voltage

Description

Level)

TP_RESX

I

Digital(IOVCC)  External reset for TP

TP_UART_TX

I/O  Digital(IOVCC)

TP_INT

O

Digital(IOVCC)

TP_I2C_SDA

I/O  Digital(IOVCC)

-UART TX pad.

-If not used, please let this pin open.

-Touch screen interrupt.

-If not used, please let this pin open.

-I2C interface data pin.

-If not used, please let this pin open.

Digital(IOVCC)

-I2C interface clock pin.

TP_I2C_SCL

TP_SPI_CS

I

I

Digital(IOVCC)

-If not used, please let this pin open.

Slave chip select pin in SPI interface

-If not used, please fix this pin at IOVCC

Slave input data pin in SPI interface

-If not used, please let this pin open.

TP_SPI_MISO

O

Digital(IOVCC)

TP_SPI_MOSI

TP_SPI_SCL

I

I

Digital(IOVCC)

Slave output data pin in SPI interface

-If not used, please let this pin open.

Digital(IOVCC)

Slave clock pin in SPI interface

-If not used, please let this pin open.

V0.22                                                                          Page  16  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006
                                                                               ST7102

FLASH_HOLD

O

Digital(IOVCC)

FLASH_WP

O

Digital(IOVCC)

FLASH_CS

O

Digital(IOVCC)

Hold signal to flash.

-If not used, please let this pin open.

Write protect signal to flash.

-If not used, please let this pin open.

Master chip select in SPI interface

-If not used, please let this pin open.

FLASH_MISO

I

Digital(IOVCC)  Master input data in SPI interface, If not used, please let this pin open

FLASH_MOSI

O

Digital(IOVCC)  Master output data in SPI interface, If not used, please let this pin open

FLASH_SCL

RX[512:1]

I

O

Digital(IOVCC)  Master clock signal in SPI interface, If not used, please let this pin open

Analog

Output RX signals.

-Boot From Flash Pin ,

TP_OPT_1

I

Digital(IOVCC)

TP_OPT_1P =’0’ : Host download.

TP_OPT_1P =’1’ : Flash Boot.

IM_SPI

I

Digital(IOVCC)

IM_SPI: “0”    SPI mode 0/3    (normal using , SCK rising edge trigger )

IM_SPI: “1”    SPI mode 1/2    (SCK falling edge trigger)

4.2.7

External power Supply Pins

PAD Type

Nami

I/O

(Voltage

Description

Level)

VCI

PWR_MODE

I

I

Power Suppl)  Connect to external power IC level (VCSW1 & VCSW2 output level)

-Power Mode Selectn

VCI

Normal:”1” IOVCC

External :”0” GND ( when used VCSW1& VCSW2)

VCSW1/VCSW2

O

VCI

VCSW1/VCSW2 CLK Out Pin, connect to external power IC

4.2.8

Other

PAD Type

Name

I/O

(Voltage

Description

Level)

RST_OR_ENB

SWIRE

I

O

IOVCC

For text, connect to IOVCC or Floating.

IOVCC

Let it open.

V0.22                                                                          Page  17  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006

                                                                               ST7102

4.3

Power Supply Configuration

V0.22                                                                          Page  18  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

AVDDIOVCCDPHYVCCAGNDDGNDVGL (-7 ~ -18 V, 100 mV/step)AVEEGVDDP (20 mV/step)GVDDN (20 mV/step)VGH (7 ~ 18 V, 100 mV/step)1.65 ~ 3.3 V4.5V~6.3V-4.5V~-6.3V0 V深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006

                                                                               ST7102

5  FUNCTION DESCRIPTION

5.1

Frame Tearing Effect Interface

The Tearing Effect output can be show the synchronize status with MPU (Host). This signal can be enabled or disabled by

the TE off & on commands. The mode of the Tearing Effect signal is defined by the parameter of the Tearing Effect Line

On command.

  Tearing Effect Line Modes

Mode 1, the Tearing Effect Output signal consists of V-Blanking Information only (Figure 1):

tVDH= The LCD display is not updated video signal.

tVDL= The LCD display is updated video signal

Figure 1 Mode1

Mode 2, the Tearing Effect Output signal consists of V-Blanking and H-Blanking Information, there is one V-sync and 800

H-sync pulses per field. (Figure 2)

tHDH= The LCD display is not updated video signal.

tHDL= The LCD display is updated f video signal.

Figure 2 Mode2

Mode3, in this mode, the tearing effect output when the display reaches line N. The output signal length of the high level is

one line period. In below figure, it shows the TE pulse that can be select from 1st line to 800th line by CMD44h.P1 and

CMD44.P2 (Figure 3)

Figure 3 Mode3

CMD 35h

CMD 44h

TEM

TESN

TE output

0

1

0

1

0

0

≠0

≠0

TE high in V-porch region (mode1)

TE high in all V-porch and H-porch region (mode2)

TE high at N-th line (mode3)

Same as mode2

V0.22                                                                          Page  19  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

Vertical Time ScaletVDLtVDHVertical Time Scale1st LineV-porch2nd Line3rd Line799th Line800th LineV-porchtHDHtHDLVertical Time ScaleFor reference, not belong to mode31st LineN=12nd LineN=2799th LineN=799800th LineN=800For reference, not belong to mode3V-porchV-porch深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006

                                                                               ST7102

Where mode1, mode2 and mode3 timing chart is shown in below (Figure 4):

Note: during sleep in mode, the Tearing output pin is active low.l

Figure 4 TE Mode

V0.22                                                                          Page  20  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

Bottom Line -1Bottom Line1st Line2nd LineTE Mode1TE Mode2TE Mode3 (N=800)Porch深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006

                                                                               ST7102

5.2

Content Adaptive Backlight Control (CABC2.0)

5.2.1

Definition of CABC

A  Content  Adaptive  Brightness  Control  function  can  be  used  to  reduce  the  power  consumption  of  the  luminance

source.  Content  adaptation  means  that  content  gray  level  scale  can  be  increased  while  simultaneously  lowering

brightness  of  the  backlight  to  achieve  same  perceived  brightness.  The  adjusted  gray  level  scale  and  thus  the  power

consumption reduction

Definition of Modes and target power reduction ratio:

Off mode: Content Adaptive Brightness Control functionality is totally off.

UI [User Interface] image mode: Optimized for UI image. It is kept image quality as much as possible. Target power

consumption reduction ratio: 10% or less.

Still picture mode: Optimized for still picture. Some image quality degradation would be acceptable. Target power

consumption reduction ratio: more than 30%.

Moving image mode: Optimized for moving image. It is focused on the biggest power reduction with image quality

degradation. Target power consumption reduction ratio: more than 30%.

Off ode

UI image mode

Still picture mode

Moving image mode

Power Reduction

0%

10% or Less

More than 30%

More than 30%

Image Quality

Best

Approaching Best

Some image

Some image

degradation

degradation

Note 1: Updating partial area of the image data should be supported by CABC functionality.
Note 2: Processing power consumption of CABC should be minimized.
Note 3: Customer need program NVM GAMMA when using CABC.

V0.22                                                                          Page  21  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006

                                                                               ST7102

The transition time for dimming function is illustrated below.

⚫

Content Adaptive Brightness Control

Display brightness is changed, according to the image contents. The following graph mentions the case of

displaying three different images.

⚫

⚫

⚫

Image A: -20% brightness reduction

Image B: -30% brightness reduction

Image C: -10% brightness reduction

Transition time from the previous image to the current displayed image is “transition time A”.

Figure 5 Transition time A

V0.22                                                                          Page  22  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

Image A Brightness reduction ratio: -20%Image B Brightness reduction ratio: -30%Image C Brightness reduction ratio: -10%50%100%0%80%Display brightnessTransition time ATimeContent Adaptive Brightness ControlTransition time ATransition time A70%90%深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006

                                                                               ST7102

⚫  Manual brightness setting and Dimming function

Figure 6    Manual brightness setting and Dimming function

V0.22                                                                          Page  23  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

50%100%0%85%Display brightnessTimeTransition time BTransition time B60%85%Manual Brightness Setting深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006

                                                                               ST7102

⚫  Combine Display brightness

Green line in the following graph is for the output brightness of display. It is combined with both display brightness,

which are defined in the above graphs.

Maximum transition time is transition time A+B.

Figure 7    Maximum transition time is transition time A+B

Brightness level calculates with the following formula.

Display Output brightness = Manual Brightness setting * CABC brightness ratio

Manual Brightness setting

Brightness ratio [CABC]

Display Output brightness

Case 1

Case 2

Case 3

85%

60%

85%

80%

70%

90%

68%

42%

76.5%

Transition time from the current brightness to target brightness is A+B in the worst case.

V0.22                                                                          Page  24  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

50%100%0%68%Display brightnessTransition time ATimeDisplay Brightness“Manual brightness setting”Transition time ATransition time A+B70%90%Case 1Transition time B42%Case 2Case 376.5%Display Brightness“Content Adaptive Brightness Control”Display Output Brightness深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006

                                                                               ST7102

5.2.2

Minimum Brightness Setting of CABC Function

CABC function is automatically reduced backlight brightness based on image contents. In the case of the combination

with the LABC or manual brightness setting, display brightness is too dark. It must affect to image quality degradation.

CABC minimum brightness setting is to avoid too much brightness reduction. When CABC is active, CABC cannot reduce

the display brightness to less than CABC minimum brightness setting. If CABC algorithm works without any abnormal

visual effect, image processing function can operate even when the brightness cannot be changed.

This function does not affect to the other function, manual brightness setting. Manual brightness can be set the display

brightness to less than CABC minimum brightness. Smooth transition and dimming function can be worked as normal.

When display brightness is turned off (BCTRL=0 of “Write CTRL Display (53h)”), CABC minimum brightness setting is

ignored. “Read CABC minimum brightness (5Fh)” always read the setting value of “Write CABC minimum brightness

(5Eh)”.

Sleep-in

CABC off

CABC on

WRCABC (55h)

Function

RDCABCMB (5Fh)

Image

NA

WRCABCMB (5Eh)

00b

01b/10b/11b

Disable

Enable

WRCABCMB (5Eh)

Original

WRCABCMB (5Eh)

CABC modified

Brightness level calculates with the following formula.

Display Output Brightness = Manual brightness setting * CABC brightness ratio

V0.22                                                                          Page  25  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006

                                                                               ST7102

Below drawing is for the explanation of the CABC minimum brightness setting.

CABC minimum brightness value = 51 (33h: 20% display brightness)

Figure 8 CABC minimum brightness setting

Display Brightness

Brightness ratio

Calculation result of the

Display

Image

[manual setting]

[CABC]

display brightness

Output

Case 1

Case 2

Case 3

50%

20%

50%

70%

70%

70%

formula

Brightness

35%

14%

35%

35%

20%

35%

CABC modified

CABC modified

CABC modified

At the case 2, the calculation result of the display brightness is 14%. CABC minimum brightness value is set to 20%

brightness. Actual display brightness is 20% as the CABC minimum brightness setting.

V0.22                                                                          Page  26  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

50%100%0%Display brightnessTransition time ATimeDisplay Brightness“Manual brightness setting”Transition time BTransition time B70%35%Case 1Transition time BCase 2Case 320%Display Brightness“Content Adaptive Brightness Control”Display Output Brightness50%35%50%深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006

                                                                               ST7102

5.2.3

Display Dimming

5.2.3.1

General Description

A dimming function (how fast to change the brightness from old to new level and what are brightness levels during the

change)  is  used  when  changing  from  one  brightness  level  to  another.  This  dimming  function  curve  is  the  same  in

increment and decrement. The basic idea is described below.

Dimming function can be enable and disable. See “Write CTRL Display (53h)” (bit DD) for more information.

5.2.3.2

Dimming Requirement

Dimming function in the display module should be implemented so that 400-600ms is used for the transition between

the original brightness value and the target brightness value. The transferring time steps between these two brightness

values are equal making the transition linear.

The dimming function is working similarly in both upward and downward directions.

An upward example is illustrating below

Figure 9 Dimming Requirement

V0.22                                                                          Page  27  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

Hysteresis Step UpLuminanceTimeWithout DimmingDimmingLuminanceTimeWith Dimming tOriginal Luminance Value (v)t+1t+2t+3t+4t+5TimeTarget Luminance Value (v+4)v+1v+2v+3400~600ms深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006

                                                                               ST7102

5.2.4

Definition of Brightness Transition Time

⚫

Shorter transition time than 500ms.

There is some stable time between transitions. Below drawing is for transition time: 400ms.

Figure 10 Shorter Transition time than 500ms

V0.22                                                                          Page  28  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

50%60%85%50%100%60%85%Target brightnessDisplay brightnessTransition time: 400msTransition time: 400msTransition time: 400msTime深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006

                                                                               ST7102

⚫

Longer transition time than 500ms

There is no any stable time between transitions. Below drawing is for transition time: 600ms.

Figure 11 Longer Transition time than 500ms

V0.22                                                                          Page  29  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

50%60%85%50%100%60%85%Target brightnessDisplay brightnessTransition time: 600msTransition time: 600msTransition time: 600msTimeIt starts 2nd transition when the 2nd brightness target, 60%, is fixed during first transition time. Change point for the new target is at 41.6% brightness.It starts 3rd transition at 57% brightness深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006

                                                                               ST7102

5.3

Color Enhancement (CE2.0)

Color enhancement function enhances the color saturation by gamut expansion.

Figure 12    Color Gamut Expansion

An example of the color enhancement function is illustrated below:

Color Enhancement Off

Color Enhancement On

V0.22                                                                          Page  30  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006
                                                                               ST7102

5.4  MIPI-DSI Interface

The Display Serial Interface standard defines protocols between a host processor and peripheral devices that adhere to

MIPI Alliance standards for mobile device interfaces. The DSI standard builds on existing standards by adopting pixel

formats and command set defined in MIPI Alliance standards.

DSI-compliant peripherals support either of two basic modes of operation: Command Mode and Video Mode.

Which mode is used depends on the architecture and capabilities of the peripheral. The mode definitions reflect the

primary intended use of DSI for display interconnect, but are not intended to restrict DSI from operating in other

applications.

Typically, a peripheral is capable of Command Mode operation or Video Mode operation. Some Video Mode display

modules also include a simplified form of Command Mode operation in which the display module may refresh its screen

from a reduced-size, or partial, frame buffer, and the interface (DSI) to the host processor may be shut down to reduce

power consumption.

V0.22                                                                          Page  31  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006

                                                                               ST7102

Command Mode refers to operation in which transactions primarily take the form of sending commands to a peripheral,

such as a display module, that incorporates a display controller. The display controller may include local registers and a

frame buffer. Systems using Command Mode write to, and read from, the registers. The host processor indirectly controls

activity at the peripheral by sending commands, parameters to the display controller. The host processor can also read

display module status information. Command Mode operation requires a bidirectional interface.

Video Mode refers to operation in which transfers from the host processor to the peripheral take the form of a real-time

pixel stream. In normal operation, the display module relies on the host processor to provide image data at sufficient

bandwidth to avoid flicker or other visible artifacts in the displayed image. Video information should only be transmitted

using High-Speed Mode. Some Video Mode architectures may include a simple timing controller and partial frame buffer,

used to maintain a partial-screen or lower-resolution image in standby or Low-Power Mode. This permits the interface to

be shut down to reduce power consumption. To reduce complexity and cost, systems that only operate in Video Mode

may use a unidirectional data path.

5.4.1

Display Module Pin Configuration for DSI

V0.22                                                                          Page  32  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

+-HS-RXHS/LPCapacitanceInductiance:TBDnH typResistance:TBDohm typCapacitance:TBDpF typModule ConnectorInductiance:TBDnH typResistance:TBDohm typCapacitance:TBDpF typModule ConnectorLP-RXLP-TXLP-CD+-HS-RXHS/LPCapacitanceInductiance:TBDnH typResistance:TBDohm typCapacitance:TBDpF typModule ConnectorInductiance:TBDnH typResistance:TBDohm typCapacitance:TBDpF typModule ConnectorLP-RXDSI-CLKDSI-D0ResistanceResistanceResistanceResistance深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006

                                                                               ST7102

5.4.2

Display Serial Interface (DSI)

5.4.2.1

General description

The communication can be separated 2 different levels between the MCU and the display module:

- Interface level : Low level communication

- Packet level : High level communication

5.4.2.2

Interface level communication

5.4.2.2.1

General

The display module uses data and clock lane differential pairs for DSI. Both clock lane and data lane0 can be driven

Low-Power (LP) or High-Speed (HS) mode. Data lane1, data lane2 and data lane3 can be driven High-Speed mode only.

Lane support mode

Unidirectional lane

High-Speed Clock only

Clock Lane

Simplified Escape Mode (ULPS

Only)

V0.22                                                                          Page  33  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

+-HS-RXHS/LPCapacitanceInductiance:TBDnH typResistance:TBDohm typCapacitance:TBDpF typModule ConnectorInductiance:TBDnH typResistance:TBDohm typCapacitance:TBDpF typModule ConnectorLP-RXDSI-D1ResistanceResistance+-HS-RXHS/LPCapacitanceInductiance:TBDnH typResistance:TBDohm typCapacitance:TBDpF typModule ConnectorInductiance:TBDnH typResistance:TBDohm typCapacitance:TBDpF typModule ConnectorLP-RXDSI-D2ResistanceResistance+-HS-RXHS/LPCapacitanceInductiance:TBDnH typResistance:TBDohm typCapacitance:TBDpF typModule ConnectorInductiance:TBDnH typResistance:TBDohm typCapacitance:TBDpF typModule ConnectorLP-RXDSI-D3ResistanceResistanceD-PHYLane ModuleD-PHYLane ModulePPIPPI深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006

                                                                               ST7102

Bi-directional lane

Data

Forward high-speed only

Lane0

Bi-directional Escape Mode

Bi-direction LPDT

Unidirectional lane

Data

Forward high-speed only

Lane1/2/3

Simplified Escape Mode(ULPS

Only)

Figure 13 The Interface Color Lane Types and Support Mode

Low-Power mode means that each line of the differential pair is used in single end mode and a differential receiver is

disable (A termination resistor of the receiver is disable) and it can be driven into a Low-Power mode.

High-Speed mode means that differential pairs (The termination resistor of the receiver is enable) are not used in the

single end mode.

There are used different modes and protocols in each mode when there are wanted to transfer information from the MCU

to the display module and vice versa.

The State Codes of the High-Speed (HS) and Low-Power (LP) lane pair are defined below.

V0.22                                                                          Page  34  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

D-PHYLane ModuleD-PHYLane ModulePPIPPID-PHYLane ModuleD-PHYLane ModulePPIPPI深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006

                                                                               ST7102

Lane Pair

Line Voltage Levels

High-Speed(HS)

Low-Power(LP)

State Code

Dn+ Line

Dn- Line

Burst Mode

Control Mode

Escape Mode

HS-0

HS-1

LP-00

LP-01

LP-10

LP-11

HS Low

HS High

Differential-0

N/A, Note 1

N/A, Note 1

HS High

HS Low

Differential-1

N/A, Note 1

N/A, Note 1

LP Low

LP Low

LP Low

LP High

LP High

LP Low

LP High

LP High

N/A

N/A

N/A

N/A

Bridge

HS-Request

LP-Request

Space

Mark-0

Mark-1

Stop

N/A, Note 2

High-Speed and Low-Power Lane Pair State Descriptions

Notes:

1. During High-Speed transmission the Low-Power observe LP-00 on the Lines.

2. If LP-11 occurs during Escape mode the Lane returns to Stop state.

V0.22                                                                          Page  35  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006

                                                                               ST7102

5.4.2.2.2

DSI-CLOCK Lanes

DSI-CLK+/- lanes can be driven into three different power modes: Low-Power Mode (LPM LP-11), Ultra-Low Power Mode

(ULPM) or High-Speed Clock Mode (HSCM).

Clock lanes are in a single end mode (LP = Low-Power) when there is entering or leaving Low-Power Mode (LPM) or

Ultra-Low Power Mode (ULPM).

Clock lanes are in the single end mode (LP = Low-Power) when there is entering in or leaving out High-Speed Clock Mode

(HSCM).

These entering and leaving protocols are using clock lanes in the single end mode to generate an entering or leaving

sequences.

The principal flow chart of the different clock lanes power modes is illustrated below.

Figure 14 Clock Lanes Power Modes

V0.22                                                                          Page  36  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

SW ResetHW ResetPower On SequenceLPMLP-11LP-10LP-01LP-10LP-00LP-00ULPMLP-00HS-0HS-0HS-0HS-1HSCM (HS Clocking)深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006

                                                                               ST7102

5.4.2.2.2.1

Low-Power Mode (LPM)

DSI-CLK+/- lanes can be driven to the Low-Power Mode (LPM),when DSI-CLK lanes are entering LP-11 State Code , in

three different ways:

After SW Reset, HW Reset or Power On Sequence=>LP-11

After DSI-CLK+/- lanes are leaving Ultra Low Power Mode (ULPM, LP-00 State Code) =>LP10=>LP-11(LPM).

This sequence is illustrated below.

Figure 15 From ULPM to LPM

After DSI-CLK+/- lanes are leaving High-Speed Clock Mode (HSCM, HS-0 or HS-1 State Code) =>HS-0 =>LP-11 (LPM).

This sequence is illustrated below.

Figure 16 From HSCM to LPM

V0.22                                                                          Page  37  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

DSI-CLK+DSI-CLK-ULPMLPMLP-00LP-10LP-11TimeDSI-CLK+DSI-CLK-DSI-CLK+DSI-CLK-DSI-CLK+DSI-CLK-Termination Resistor is disableHS-0/1HS-0HSCMLPMLP-11Time深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006

                                                                               ST7102

All three mode changes are illustrated a flow chart below.

Figure 17 All three mode changes to LPM

V0.22                                                                          Page  38  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

SW ResetHW ResetPower On SequenceLPMLP-11LP-10LP-01LP-10LP-00LP-00ULPMLP-00HS-0HS-0HS-0HS-1HSCM (HS Clocking)Mode Change深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006

                                                                               ST7102

5.4.2.2.2.2

Ultra-Low Power Mode (ULPM)

DSI-CLK+/- lanes can be driven to the Ultra-Low Power Mode (ULPM), when DSI-CLK lanes are entering LP-00 State

Code. The only entering possibility if from the Low-Power Mode (LPM, LP-11 State Code) => LP-10 => LP-00 (ULPM).

This sequence is illustrated below:

The mode change is also illustrated below:

Figure 18 From LPM to UPLM

Figure 19 The mode change from LPM to UPLM

V0.22                                                                          Page  39  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

DSI-CLK+DSI-CLK-LPMULPMLP-11LP-10LP-00TimeDSI-CLK+DSI-CLK-SW ResetHW ResetPower On SequenceLPMLP-11LP-10LP-01LP-10LP-00LP-00ULPMLP-00HS-0HS-0HS-0HS-1HSCM (HS Clocking)Mode Change深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006

                                                                               ST7102

5.4.2.2.2.3

High-Speed Clock Mode (HSCM)

DSI-CLK+/- lanes can be driven to the High-Speed Clock Mode (HSCM), when DSI-CLK lanes are starting to work

between HS-0 and HS-1 State Codes. The only entering possibility is from the Low-Power Mode (LPM, LP-11 State Code)

=>LP-01 =>LP-00 =>HS-0 =>HS-0/1 (HSCM). This sequence is illustrated below.

The mode change is also illustrated below:

Figure 20 From LPM to HSCM

Figure 21 Mode Change from LPM to HSCM on the Flow Chart

V0.22                                                                          Page  40  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

DSI-CLK+DSI-CLK-LPMTermination Resistor is enableLP-11LP-01LP-00TimeDSI-CLK+DSI-CLK-HS-0HS-0/1HSCMSW ResetHW ResetPower On SequenceLPMLP-11LP-10LP-01LP-10LP-00LP-00ULPMLP-00HS-0HS-0HS-0HS-1HSCM (HS Clocking)Mode Change深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006

                                                                               ST7102

The High-Speed clock (DSI-CLK+/-) is started before High-Speed data is sent via DSI-Dn+/- lanes. The High-Speed clock

continues clocking after the High-Speed data sending has been stopped.

Figure 22 High-Speed Clock Burst

Note:

If the last load bits is HS-0, the transmitter changes form HS-0 to HS-1.

If the last load bits is HS-1, the transmitter changes form HS-1 to HS-0.

V0.22                                                                          Page  41  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

DSI-CLK+DSI-CLK-LPMTermination Resistor is enableLP-11LP-01LP-00TimeHS-0HS-0/1HSCMDSI-CLK+DSI-CLK-Termination Resistor is disableHS-0LPMLP-11DSI-Dn+DSI-Dn-LPMLP-11TimeData Lanes in High-Speed ModeDSI-Dn+DSI-Dn-LPMLP-11DSI-CLK+DSI-CLK-DSI-CLK+DSI-CLK-LPMLP-11LP-01LP-00HS-011101000Tx SynchronizedRx SynchronizedDSI-Dn+DSI-Dn-Low-Power Mode, Disable Rx Line TerminationHigh-Speed Mode, Enable Rx Line TerminationDSI-Dn+DSI-Dn-DSI-CLK+DSI-CLK-DSI-CLK+DSI-CLK-DSI-Dn+DSI-Dn-DSI-Dn+DSI-Dn-LPMLP-11Low Power Mode, Disable Rx Line TerminationHS-0 or HS-1The last load bitHigh Speed Mode, Enable Rx Line TerminationHSDTHS-0/1HS-0/1HSDT深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006
                                                                               ST7102

5.4.2.2.3

DSI-DATA Lanes

5.4.2.2.3.1

General

DSI-D0+/- data lanes can be driven in different modes which are:

• Escape Mode (Only DSI-D0+/- data lane is used)

• High-Speed Data Transmission (DSI-D0+/-, DSI-D1+/-, DSI-D2+/- and DSI-D3+/- data lanes are used)

• Bus Turnaround Request (Only DSI-D0+/- data lane is used)

These modes and their entering codes are defined on the following table.

Mode

Entering Mode Sequence

Leaving Mode Sequence

Escape Mode

LP-11=>LP-10=>LP-00=>LP-01=>LP-00

LP-00=>LP-10=>LP11(Mark1)

High-Speed Data Transmission

LP-11=>LP-01=>LP-00=>HS-0

(HS-0 or HS-1) =>LP-11

Bus Turnaround Request

LP-11=>LP-10=>LP-00=>LP-10=>LP-00

High-Z

Entering and leaving sequence

5.4.2.2.3.2

ESCAPE MODE

Data lane0 (DSI-D0+/-) can be used in different Escape Modes when data lanes are in Low-Power (LP) mode.

These Escape Modes are used to:

• Send “Low-Power Data Transmission” (LPDT) e.g. from the MCU to the display module

• Drive data lanes to “Ultra-Low Power State” (ULPS)

• Indicate “Remote Application Reset” (RAR), which is reset the display module

• Indicate “Tearing Effect” (TEE), which is used for a TE trigger event from the display module to the MCU

• Indicate “Acknowledge” (ACK), which is used for a non-error event from the display module to the MCU

The basic sequence of the Escape Mode is as follow

• Start: LP-11

• Escape Mode Entry (EME): LP-11 =>LP-10 =>LP-00 =>LP-01 =>LP-00

• Escape Command (EC), which is coded, when one of the data lanes is changing from low-to-high-to-low then this

changed data lane is presenting a value of the current data bit (DSI-D0+ = 1, DSI-D0- = 0) e.g. when DSI-D0- is changing

from low-to-high-to-low, the receiver is latching a data bit, which value is logical 0. The receiver is using this

low-to-high-to-low transition for its internal clock.

• A load if it is needed

• Exit Escape (Mark-1) LP-00 =>LP-10 =>LP-11

• End: LP-11

This basic construction is illustrated below:

V0.22                                                                          Page  42  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006

                                                                               ST7102

Figure 23 General Escape Mode Sequence

The number of the different Escape Commands (EC) is eight. These eight different escape commands (EC) can be

divided 2 different groups: Mode or Trigger. The MCU is informing to the display module that it is controlling data lanes

(DSI-D0+/-) with the mode e.g. The MCU can inform to the display module that it can put data lanes in the Low-Power

mode. The MCU is waiting from the display module event information, which has been set by the MCU, with the trigger e.g.

when the display module reaches a new V-synch, the display module sent to the MCU a TE trigger (TEE), if the MCU has

been requested it.

Escape commands are defined on the next table.

This basic construction is illustrated below:

Escape Command

Command Type

Entry Command Pattern

Mode/Trigger

(First Bit➔Last Bit Transmitted)

Dn

D0

Low-Power Data Transmission

Ultra-Low Power Mode

Underfined-1, Note 1

Underfined-2, Note 1

Remote Application Reset

Tearing Effect

Acknowledge

Unknow-5, Note 1

Mode

Mode

Mode

Mode

Trigger

Trigger

Trigger

Trigger

1110 0001 bin

0001 1110 bin

1001 1111 bin

1101 1110 bin

0110 0010 bin

0101 1101 bin

0010 0001 bin

1010 0000 bin

-

○

-

-

-

-

-

-

○

○

-

-

○

-

○

-

Notes:

1. This Escape command support has not been implemented on the display module.

2. n=1.

3. “○”=Supported

4. “-“=Not Supported

5. Tearing Effect Trigger cannot be used in MIPI Video mode.

V0.22                                                                          Page  43  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

LP-00LP-10LP-11LP-11LP-10LP-00TimeLP-01LP-00Escape Mode Entry (EME)Escape CommandLoadIf neededMark-1DSI-D0+DSI-D0-DSI-D0+DSI-D0-深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006

                                                                               ST7102

Low-Power Data Transmission (LPDT)

The MCU can send data to the display module in Low-Power Data Transmission (LPDT) mode when data lanes are

entering in Escape Mode and Low-Power Data Transmission (LPDT) command has been sent to the display module. The

display module is also using the same sequence when it is sending data to the MCU.

The Low-Power Data Transmission (LPDT) is using a following sequence:

• Start: LP-11

• Escape Mode Entry (EME): LP-11 =>LP-10 =>LP-00 =>LP-01 =>LP-00

• Low-Power Data Transmission (LPDT) command in Escape Mode: 1110 0001 (First to Last bit)

• Load (Data):

One or more bytes (8 bits)

Data lanes are in pause mode when data lanes are stopped (Both lanes are low ) between bytes

• Mark-1: LP-00 =>LP-10 =>LP-11

• End: LP-11

This sequence is illustrated for reference purposes below:

Figure 24 Low-Power Data Transmission (LPDT)

Note:

Load(Data) is presenting that the first bit is logical ‘1’ in this example

V0.22                                                                          Page  44  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

LP-11TimeLow-Power Data Transmission (LPDT)Load (Data)Mark-1LP-11111000011LP-11LP-10LP-00LP-01LP-00Escape Mode Entry (EME)LP-00LP-10LP-11Mark-1Escape ModeEntry (EME)DSI-D0+DSI-D0-DSI-D0+DSI-D0-深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006

                                                                               ST7102

Figure 25 Pause (Example)

Ultra-Low Power State (ULPS)

The MCU can force data lanes in Ultra-Low Power State (ULPS) mode when data lanes are entering in Escape Mode.

The Ultra-Low Power State (ULPS) is using a following sequence:

• Start: LP-11

• Escape Mode Entry (EME): LP-11 =>LP-10 =>LP-00 =>LP-01 =>LP-00

• Ultra-Low Power State (ULPS) command in Escape Mode: 0001 1110 (First to Last bit)

• Ultra-Low Power State (ULPS) when the MCU is keeping data lanes low

• Mark-1: LP-00 =>LP-10 =>LP-11

• End: LP-11

This sequence is illustrated for reference purposes below:

Figure 26 Ultra-Low Power State (ULPS)

V0.22                                                                          Page  45  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

Time1111PauseLoad Byte n+1Load Byte nLoad (Data)DSI-D0+DSI-D0-DSI-D0+DSI-D0-LP-11TimeUltra-Low Power State (ULPS)Ultra-Low Power State Mark-1LP-11000111101LP-11LP-10LP-00LP-01LP-00Escape Mode Entry (EME)LP-00LP-10LP-11Mark-1Escape ModeEntry (EME)DSI-D0+DSI-D0-DSI-D0+DSI-D0-深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006

                                                                               ST7102

Remote Application Reset (RAR)

The MCU can inform to the display module that it should be reset in Remote Application Reset (RAR) trigger when data

lanes are entering in Escape Mode.

The Remote Application Reset (RAR) is using a following sequence:

• Start: LP-11

• Escape Mode Entry (EME): LP-11 =>LP-10 =>LP-00 =>LP-01 =>LP-00

• Remote Application Reset (RAR) command in Escape Mode: 0110 0010 (First to Last bit)

• Mark-1: LP-00 =>LP-10 =>LP-11

• End: LP-11

This sequence is illustrated for reference purposes below:

Figure 27 Remote Application Reset (RAR)

V0.22                                                                          Page  46  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

LP-11TimeRemote Application Reset (RAR)Mark-1LP-1101100010LP-11LP-10LP-00LP-01LP-00Escape Mode Entry (EME)LP-00LP-10LP-11Mark-1Escape ModeEntry (EME)DSI-D0+DSI-D0-DSI-D0+DSI-D0-深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006

                                                                               ST7102

Tearing Effect (TEE)

The display module can inform to the MCU when a tearing effect event (New V-synch) has been happen on the display

module by Tearing Effect (TEE).

The Tearing Effect (TEE) is using a following sequence:

• Start: LP-11

• Escape Mode Entry (EME): LP-11 =>LP-10 =>LP-00 =>LP-01 =>LP-00

• Tearing Effect (TEE) trigger in Escape Mode: 0101 1101 (First to Last bit)

• Mark-1: LP-00 =>LP-10 =>LP-11

• End: LP-11

This sequence is illustrated for reference purposes below:

Note: Tearing Effect (TEE) can not be used in MIPI Video Mode

Figure 28 Tearing Effect (TEE)

V0.22                                                                          Page  47  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

LP-11TimeTearing Effect Trigger (TEE)Mark-1LP-1101011101LP-11LP-10LP-00LP-01LP-00Escape Mode Entry (EME)LP-00LP-10LP-11Mark-1Escape ModeEntry (EME)DSI-D0+DSI-D0-DSI-D0+DSI-D0-深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006

                                                                               ST7102

Acknowledge (ACK)

The display module can inform to the MCU when an error has not recognized on it by Acknowledge (ACK).

The Acknowledge (ACK) is using a following sequence:

• Start: LP-11

• Escape Mode Entry (EME): LP-11 =>LP-10 =>LP-00 =>LP-01 =>LP-00

• Acknowledge (ACK) command in Escape Mode: 0010 0001 (First to Last bit)

• Mark-1: LP-00 =>LP-10 =>LP-11

• End: LP-11

This sequence is illustrated for reference purposes below:

Figure 29 Acknowledge (ACK)

V0.22                                                                          Page  48  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

LP-11TimeAcknowledge (ACK)Mark-1LP-1100100001LP-11LP-10LP-00LP-01LP-00Escape Mode Entry (EME)LP-00LP-10LP-11Mark-1Escape ModeEntry (EME)DSI-D0+DSI-D0-DSI-D0+DSI-D0-深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006

                                                                               ST7102

5.4.2.2.3.3

High-Speed Data Transmission (HSDT)

Entering High-Speed Data Transmission (TSOT of HSDT)

The display module is entering High-Speed Data Transmission (HSDT) when Clock lanes DSI-CLK+/- have already been

entered in the High-Speed Clock Mode (HSCM) by the MCU. See more information on chapter “High-Speed Clock Mode

(HSCM)”.

Data lanes of the display module are entering (TSOT) in the High-Speed Data Transmission (HSDT) as follows

• Start: LP-11

• HS-Request: LP-01

• HS-Settle: LP-00 => HS-0 (Rx: Lane Termination Enable)

• Rx Synchronization: 011101 (Tx (= MCU) Synchronization: 0001 1101)

• End: High-Speed Data Transmission (HSDT) – Ready to receive High-Speed Data Load

This same entering High-Speed Data Transmission (TSOT of HSDT) sequence is illustrated below

Figure 30 Entering High-Speed Data transmission (TSOT of HSDT)

V0.22                                                                          Page  49  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

DSI-CLK+DSI-CLK-DSI-CLK+DSI-CLK-HS-011101000Tx SynchronizedRx SynchronizedDSI-Dn+DSI-Dn-Low Power Mode, Disable Rx Line TerminationHigh Speed Mode, Enable Rx Line TerminationDSI-Dn+DSI-Dn-Preparation from Low Power Mode to High Speed Mode (TSOT = Start of the Transmission)HSDTLPMLP-11LP-01LP-00HS-0/1深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006

                                                                               ST7102

Leaving High-Speed Data Transmission (TEOT of HSDT)

The display module is leaving the High-Speed Data Transmission (TEOT of HSDT) when Clock lanes DSI-CLK+/- are in the

High-Speed Clock Mode (HSCM) by the MCU and this HSCM is kept until data lanes are in LP-11 mode. See more

information on chapter “High-Speed Clock Mode (HSCM)”.

Data lanes of the display module are leaving from the High-Speed Data Transmission (TEOT of HSDT) as follows

• Start: High-Speed Data Transmission (HSDT)

• Stops High-Speed Data Transmission

MCU changes to HS-1, if the last load bit is HS-0

MCU changes to HS-0, if the last load bit is HS-1

• End: LP-11 (Rx: Lane Termination Disable)

This same leaving High-Speed Data Transmission (TEOT of HSDT) sequence is illustrated below

Figure 31 Leaving High-Speed data Transmission (TEOT of HSDT)

V0.22                                                                          Page  50  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

DSI-CLK+DSI-CLK-DSI-CLK+DSI-CLK-DSI-Dn+DSI-Dn-DSI-Dn+DSI-Dn-LP-11Low Power Mode, Disable Rx Line TerminationHS-0 or HS-1The last load bitHigh Speed Mode, Enable Rx Line TerminationHS-0/1TEOTHSDTLPM深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006

                                                                               ST7102

Burst of the High-Speed Data Transmission (HSDT)

The burst of the High-Speed Data Transmission (HSDT) can consist of one data packet or several data packets. These

data packets can be Long (LPa) or Short (SPa) packets.

These different burst of the High-Speed Data Transmission (HSDT) cases are illustrated for reference purposes below.

Figure 32 Single Packet in High-Speed Data Transmission with EoT packet disabled

Figure 33 Multiple Packets in High-Speed Data Transmission with EoT packet disabled

Figure 34 Single Packet in High-Speed Data Transmission with EoT packet enable

Figure 35 Multiple Packets in High-Speed Data Transmission with EoT packet enable

V0.22                                                                          Page  51  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

DSI-Dn+/-SOTLPaEOTLP-11LP-11DSI-Dn+/-SOTEOTLP-11LP-11SPaDSI-Dn+/-SOTLPaEOTLP-11LP-11DSI-Dn+/-SOTEOTLP-11LP-11SPaSPaSPaSPaSPaDSI-Dn+/-SOTLPaEOTLP-11LP-11DSI-Dn+/-SOTEOTLP-11LP-11SPaSPaEoT PacketSPaEoT PacketDSI-Dn+/-SOTLPaEOTLP-11LP-11DSI-Dn+/-SOTEOTLP-11LP-11SPaSPaSPaSPaSPaSPaEoT PacketSPaEoT Packet深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006

                                                                               ST7102

Abbreviation

Explanation

EoT

LPa

End of the Transmission

Long Packet

LP-11

Low-Power Mode, Data lanes are’1’s (Stop Mode)

SPa

SoT

Short Packet

Start of the Transmission

5.4.2.2.3.4

Bus Turnaround (BTA)

The MCU or display module, which is controlling DSI-D0+/- Data Lanes, can start a bus turnaround procedure when it

wants information from a receiver, which can be the MCU or display module.

The MCU or display module is using the same sequence when this bus turnaround procedure is used.

This sequence is described for reference purposes, when the MCU wants to do the bus turnaround procedure to the

display module, as follow.

• Start (MCU): LP-11

• Turnaround Request (MCU): LP-11 => LP-10 => LP-00 => LP-10 => LP-00

• The MCU wait until the display module is starting to control DSI-D0+/- data lanes and the MCU stop to control DSI-D0+/-

data lanes (=High-Z)

• The display module changes to the stop mode: LP-00 => LP-10 => LP-11

The same bus turnaround .procedure (From the MCU to the display module) is illustrated below.

Figure 36 Bus Turnaround Procedure

MCU and the display module terms are switched on above figure, if the Bus Turnaround (BTA) is from the display module

to the MCU..

V0.22                                                                          Page  52  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

LP-00LP-10LP-11LP-11LP-10LP-00TimeLP-10LP-00Turnaround Request (TAR)The MCU waits until the display module starts to control data lanes (its output drivers) when the MCU can put output drivers in the High-Z mode.LP-RequestDSI-D0+DSI-D0-DSI-D0+DSI-D0-LP-00MCU Controls Data LanesDisplay Module ControlsData Lanes深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006

                                                                               ST7102
Packer Level Communication

5.4.2.3

5.4.2.3.1

Short Packet (SPa) and Long Packet (LPa) Structure

Short Packet (SPa) and Long Packet (LPa) are always used when data transmission is done in Low-Power Data

Transmission (LPDT) or High-Speed Data Transmission (HSDT) modes.

The lengths of the packets are

• Short Packet (SPa): 4 bytes

• Long Packet (LPa): From 6 to 65,541 bytes

The type (SPa or LPa) of the packet can be recognized from their package headers (PH).

Figure 37 Short Packet (SPa) Structure

Figure 38 Long Packet (LPa) Structure

Note:

Short Packet (SPa) Structure and Long Packet (LPa) Structure are presenting a single packet sending (= Includes LP-11,

SoT and EoT for each packet sending).

The other possibility is that there is not needed SoT, EoT and LP-11 between packets if packets have sent in multiple

V0.22                                                                          Page  53  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

LP-11SoTDIData0Data1ECCEoTLP-11Packet DataPacket Header (PH)TimeLP-11:Low Power-Stop StateSoT:Start of transmissionDI:Data Identification (8 bit)Data 0 and Data 1:Packet Data (8+8 bit)ECC:Error Correction Code (8 bit)EoT:End of TransmissionLP-11SoTDIWord Count (WC)ECCEoTLP-11Packet Header (PH)LP-11:Low Power-Stop StateSoT:Start of transmissionDI:Data Identification (8 bit)Data 0 and Data 1:Packet Data (8+8 bit)ECC:Error Correction Code (8 bit)EoT:End of TransmissionData0Data1Data WC-2Data WC-1Checksum(CS)深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006

                                                                               ST7102

packet format e.g.

* LP-11 =>SoT =>SPa =>LPa =>SPa =>SPa =>EoT =>LP-11

* LP-11 =>SoT =>SPa =>SPa =>SPa =>EoT =>LP-11

* LP-11 =>SoT =>LPa =>LPa =>LPa =>EoT =>LP-11

V0.22                                                                          Page  54  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006                                                                               ST7102

5.4.2.3.1.1

Bit Order of the Byte on Packets

The bit order of the byte, what is used on packets, is that the Least Significant Bit (LSB) of the byte is sent in the first and

the Most Significant Bit (MSB) of the byte is sent in the last.

This same order is illustrated for reference purposes below.

Figure 39 Bit Order of Byte on Packets

5.4.2.3.1.2

Byte Order of the Multiple Byte Information on Packets

Byte order of the multiple bytes information, what is used on packets, is that the Least Significant (LS) Byte of the

information is sent in the first and the Most Significant (MS) Byte of the information is sent in the last e.g. Word Count (WC)

consists of 2 bytes (16 bits) when the LS byte is sent in the first and the MS byte is sent in the last.

This same order is illustrated for reference purposes below.

Figure 40 Byte Order of the Multiple Byte on Packets

V0.22                                                                          Page  55  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

DI29 hex10010100B0B1B2B3B4B5B6B7LSBMSBWC (LS Byte)01 hex10000000B0B1B2B3B4B5B6B7LSBMSBWC (MS Byte)00 hex00000000B0B1B2B3B4B5B6B7LSBMSBECC06 hex01100000B0B1B2B3B4B5B6B7LSBMSBTimeWC (LS Byte)01 hex10000000B0B1B2B3B4B5B6B7LSBMSBWC (MS Byte)00 hex00000000B0B1B2B3B4B5B6B7LSBMSBTime深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006

                                                                               ST7102

5.4.2.3.1.3

Packet Header (PH)

The packet header is always consisting of 4 bytes. The content of these 4 bytes are different if it is used to Short

Packet (SPa) or Long Packet (LPa).

Short Packet (SPa):

• 1st byte: Data Identification (DI) => Identification that this is Short Packet (SPa)

• 2nd and 3rd bytes: Packet Data (PD), Data 0 and Data 1

• 4th byte: Error Correction Code (ECC)

Figure 41 Packet Header (PH) on the Short Packet(SPa)

Long Packet (LPa):

• 1st byte: Data Identification (DI) => Identification that this is Long Packet (LPa)

• 2nd and 3rd bytes: Word Count (WC)

• 4th byte: Error Correction Code (ECC)

Figure 42 Packet Header (PH) on the Long Packet (LPa)

V0.22                                                                          Page  56  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

DI15 hex10101000B0B1B2B3B4B5B6B7LSBMSBData 03A hex01011100B0B1B2B3B4B5B6B7LSBMSBData 107 hex11100000B0B1B2B3B4B5B6B7LSBMSBECC18 hex00011000B0B1B2B3B4B5B6B7LSBMSBTimePacket Header (PH)DI29 hex10010100B0B1B2B3B4B5B6B7LSBMSBWC (LS Byte)01 hex10000000B0B1B2B3B4B5B6B7LSBMSBWC (MS Byte)00 hex00000000B0B1B2B3B4B5B6B7LSBMSBECC06 hex00011000B0B1B2B3B4B5B6B7LSBMSBTimePacket Header (PH)深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006

                                                                               ST7102

⚫

Data Identification (DI)

Data Identification (DI) is a part of Packet Header (PH) and it consists of 2 parts:

• Virtual Channel (VC), 2 bits, DI[7...6]

• Data Type (DT), 6 bits, DI[5…0]

The Data Identification (DI) structure is illustrated on a table below.

Figure 43 Data Identification (DI) Structure

Figure 44 Data Identification (DI) on the Packet Header(PH)

V0.22                                                                          Page  57  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

Bit 7Bit 6Bit 5Bit 4Bit 3Bit 2Bit 1Bit 0Virtual Channel (VC)Data Type (DT)Data Identification (DI)DI29 hex10010100B0B1B2B3B4B5B6B7LSBMSBWC (LS Byte)01 hex10000000B0B1B2B3B4B5B6B7LSBMSBWC (MS Byte)00 hex00000000B0B1B2B3B4B5B6B7LSBMSBECC06 hex01100000B0B1B2B3B4B5B6B7LSBMSBTime深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006

                                                                               ST7102

⚫

Virtual Channel (VC)

Virtual Channel (VC) is a part of Data Identification (DI[7…6]) structure and it is used to address where a packet is wanted

to send from the MCU.

Bits of the Virtual Channel (VC) are illustrated for reference purposes below.

Figure 45 Virtual Channel (VC) on the Packet Header (PH)

Virtual Channel (VC) can address 4 different channels for e.g. 4 different display modules. Devices are using the same

virtual channel what the MCU is using to send packets to them e.g.

• The MCU is using the virtual channel 0 when it sends packets to this display module

• This display module is also using the virtual channel 0 when it sends packets to the MCU

This functionality is illustrated below.

Figure 46 Virtual Channel (VC) Configuration

V0.22                                                                          Page  58  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

DI29 hex10010100B0B1B2B3B4B5B6B7LSBMSBWC (LS Byte)01 hex10000000B0B1B2B3B4B5B6B7LSBMSBWC (MS Byte)00 hex00000000B0B1B2B3B4B5B6B7LSBMSBECC06 hex01100000B0B1B2B3B4B5B6B7LSBMSBTimePacket Header (PH)VirtualChannelSelectorMCUDI[7:6]=VC[1…0]=00b (This display Module)ReservedReservedReservedThis Display ModuleLong and ShortPackets深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006

                                                                               ST7102

⚫

Data Type (DT)

Data Type (DT) is a part of Data Identification (DI[5…0]) structure and it is used to define a type of the used data on a

packet.

Bits of the Data Type (DT) are illustrated for reference purposes below.

Figure 47 Data Type (DT) on the Packet Header (PH)

V0.22                                                                          Page  59  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

DI29 hex10010100B0B1B2B3B4B5B6B7LSBMSBWC (LS Byte)01 hex10000000B0B1B2B3B4B5B6B7LSBMSBWC (MS Byte)00 hex00000000B0B1B2B3B4B5B6B7LSBMSBECC06 hex01100000B0B1B2B3B4B5B6B7LSBMSBTimePacket Header (PH)深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006
                                                                               ST7102

This Data Type (DT) also defines what the used packet is: Short Packet (SPa) or Long Packet (LPa). Data Types (DT) are

different from the MCU to the display module (or other devices) and vice versa.

These Data Type (DT) are defined on tables below.

Data Type

Data Type

Packet

Description

Hex

01h

21h

08h

02h

12h

Binary

00 0001

Sync Event, V Sync Start.

10 0001

Sync Event, H Sync Start.

00 1000

End of Transmission (EoT) packet.

00 0010

Color Mode (CM) Off Command.

01 0010

Color Mode (CM) On Command.

hh22h

10 0010

Shut Down Peripheral Command.

32h

13h

23h

14h

24h

05h

15h

06h

37h

09h

19h

29h

39h

11 0010

Turn On Peripheral Command.

01 0011

Generic Short WRITE, 1 parameter.

10 0011

Generic Short WRITE, 2 parameters.

01 0100

Generic READ, 1 parameter.

10 0100

Generic READ, 2 parameters.

00 0101

DCS WRITE, no parameter.

01 0101

DCS WRITE, 1 parameter.

00 0110

DCS READ, no parameter.

11 0111

Set Maximum Return Packet Size.

00 1001

Null Packet, no data.

01 1001

Blanking Packet, no data.

10 1001

Generic Long Write.

11 1001

DCS Long Write/write_LUT Command Packet.

3Eh

11 1110

Packed Pixel Stream,24-bit RGB,8-8-8 Format.

Data Type (DT) from MCU to the Display Module (or Other Devices)

Size

Short

Short

Short

Short

Short

Short

Short

Short

Short

Short

Short

Short

Short

Short

Short

Long

Long

Long

Long

Long

V0.22                                                                          Page  60  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006                                                                               ST7102

From the Display Module (or Other Devices) to the MCU

Data Type

B

B

B

B

B

B

Hex

5

4

3

2

1

0

Description

Packet

Abbreviation

02h

11h

12h

1Ah

1Ch

21h

22h

0

0

0

0

0

1

1

0

1

1

1

1

0

0

0

0

0

1

1

0

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

1

1

0

0

1

0  Acknowledge and Error Report

Short

AwER

1  Generic Short READ Response,1 byte returned

Short

GENRR1-S

0  Generic Short READ Response,2 bytes returned

Short

GENRR2-S

0  Generic Long READ Response

Short

GENRR-L

0  DCS Long READ Response

Short

DCSRR_L

1  DCS Short READ Response, 1 byte returned

Short

DCSRR1_S

0  DCS Short READ Response, 2 bytes returned

Short

DCSRR2_S

Data Type (DT) from the Display Module (or Other Devices) to the MCU

The receiver will ignore other Data Type (DT) if they are not defined on tables: “Data Type (DT) from the MCU to the

Display Module (or Other Devices)” or “ Data Type (DT) from the Display Module (or Other Devices) to the MCU”.

V0.22                                                                          Page  61  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006

                                                                               ST7102

⚫

Packet Data (PD) on the Short Packet (SPa)

Packet Data (PD) of the Short Packet (SPa) is defined after Data Type (DT) of the Data Identification (DI) has indicated

that Short Packet (SPa) is wanted to send.

The Word Count (WC) indicates the number of Bytes of Packet of Packet Data (PD) send after the Packet Header.

Packet Data (PD) of the Short Packet (SPa) consists of 2 data bytes: Data 0 and Data 1.

Packet Data (PD) sending order is that Data 0 is sent in the first and the Data 1 is sent in the last.

Bits of Data 1 are set to ‘0’ if the information length is 1 byte.

Packet Data (PD) of the Short Packet (SPa), when the length of the information is 1 or 2 bytes are illustrated for reference

purposes below, when Virtual Channel (VC) is 0.

Packet Data (PD) information:

• Data 0: 35hex (Display Command Set (DCS) with 1 Parameter => DI(Data Type (DT)) = 15hex)

• Data 1: 01hex (DCS’s parameter)

Figure 48 Packet Data (PD) for Short Packet (SPa), 2 Bytes Information

Packet Data (PD) information:

• Data 0: 10hex (DCS without parameter => DI(Data Type (DT)) = 05hex)

• Data 1: 00hex (Null)

V0.22                                                                          Page  62  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

DI15 hex10101000B0B1B2B3B4B5B6B7LSBMSBData 035 hex10101100B0B1B2B3B4B5B6B7LSBMSBData 101 hex00001000B0B1B2B3B4B5B6B7LSBMSBECC1E hex01111000B0B1B2B3B4B5B6B7LSBMSBTimePacket Header (PH)DI05 hex10100000B0B1B2B3B4B5B6B7LSBMSBData 010 hex00001000B0B1B2B3B4B5B6B7LSBMSBData 100 hex00000000B0B1B2B3B4B5B6B7LSBMSBECC2C hex00110100B0B1B2B3B4B5B6B7LSBMSBTimePacket Header (PH)深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006

                                                                               ST7102

Figure 49 Packet Data (PD) for Short Packet (SPa), 1 Bytes Information

V0.22                                                                          Page  63  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006                                                                               ST7102

⚫  Word Count (WC) on the Long Packet (LPa)

Word Count (WC) of the Long Packet (LPa) is defined after Data Type (DT) of the Data Identification (DI) has

indicated that Long Packet (LPa) is wanted to send.

Word Count (WC) indicates a number of the data bytes of the Packet Data (PD) what is wanted to send after

Packet Header (PH) versus Packet Data (PD) of the Short Packet (SPa) is placed in the Packet Header (PH).

Word Count (WC) of the Long Packet (LPa) consists of 2 bytes.

These 2 bytes of the Word Count (WC) sending order is that the Least Significant (LS) Byte is sent in the first and the Most

Significant (MS) Byte is sent in the last.

Word Count (WC) of the Long Packet (LPa) is illustrated for reference purposes below.

Figure 50 Word Count (WC) on the Long Packet (LPa)

Figure 51 Packet Data (PD) on the Short Packet (SPa)

V0.22                                                                          Page  64  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

DI29 hex10010100B0B1B2B3B4B5B6B7LSBMSBWC (LS Byte)01 hex10000000B0B1B2B3B4B5B6B7LSBMSBWC (MS Byte)00 hex00000000B0B1B2B3B4B5B6B7LSBMSBECC06 hex01100000B0B1B2B3B4B5B6B7LSBMSBTimePacket Header (PH)LP-11SOTDIData0Data1ECCEoTLP-11Packet DataPacket Header (PH)TimeLP-11:Low Power-Stop StateSoT:Start of transmissionDI:Data Identification (8 bit)Data 0 and Data 1:Packet Data (8+8 bit)ECC:Error Correction Code (8 bit)EoT:End of Transmission深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006

                                                                               ST7102

Figure 52 Packet Data (PD) on the Long Packet (LPa)

V0.22                                                                          Page  65  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

LP-11SOTDIWord Count (WC)ECCEoTLP-11Packet Header (PH)LP-11:Low Power-Stop StateSoT:Start of transmissionDI:Data Identification (8 bit)Data 0 and Data 1:Packet Data (8+8 bit)ECC:Error Correction Code (8 bit)EoT:End of TransmissionData0Data1Data WC-2Data WC-1Checksum(CS)深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006
                                                                               ST7102

⚫

Error Correction Code (ECC)

Error Correction Code (ECC) is a part of Packet Header (PH) and its purpose is to identify an error or errors on the Packet

Header (PH):

The ECC protects the following field”

• Short Packet (SPa): Data Identification (DI) byte (8 bits, D[0...7]), Packet Data (PD) bytes (16 bits, D[8...23]) and ECC(8

bits: P[0…7])

• Long Packet (LPa): Data Identification (DI) byte (8 bits, D[0…7]), Word Count (WC) bytes (16 bits: D[8…23]) and ECC (8

bits, P[0…7])

D[23…0] and P[7…0] are illustrated for reference purposes below.

Figure 53 D[23..0] and P[7…0] on the Short Packet (SPa)

Figure 54 D[23…0] and P[7…0] on the Long Packet (LPa)

Error Correction Code (ECC) can recognize one error or several errors and makes correction in one bit error case

Bits (P[7…0]) of the Error Correction Code (ECC) are defined, where the symbol ‘^’ is presenting XOR function

V0.22                                                                          Page  66  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

TimePacket Header (PH)DI05 hex10100000B0B1B2B3B4B5B6B7LSBMSBData 010 hex00001000B0B1B2B3B4B5B6B7LSBMSBData 100 hex00000000B0B1B2B3B4B5B6B7LSBMSBECC2C hex00110100B0B1B2B3B4B5B6B7LSBMSBD0D1D2D3D4D5D6D7D8D9D10D11D12D13D14D15D16D17D18D19D20D21D22D23P0P1P2P3P4P5P6P7TimePacket Header (PH)DI29 hex10010100B0B1B2B3B4B5B6B7LSBMSBWC (LS Byte)01 hex10000000B0B1B2B3B4B5B6B7LSBMSBWC (MS Byte)00 hex00000000B0B1B2B3B4B5B6B7LSBMSBECC06 hex01100000B0B1B2B3B4B5B6B7LSBMSBD0D1D2D3D4D5D6D7D8D9D10D11D12D13D14D15D16D17D18D19D20D21D22D23P0P1P2P3P4P5P6P7深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006

                                                                               ST7102

(Pn is ‘1’ if there is odd number of ‘1’s and Pn is ‘0’ if there is even number of ‘1’s), as follows.

• P7 = 0

• P6 = 0

• P5 = D10^D11^D12^D13^D14^D15^D16^D17^D18^D19^D21^D22^D23

• P4 = D4^D5^D6^D7^D8^D9^D16^D17^D18^D19^D20^D22^D23

• P3 = D1^D2^D3^D7^D8^D9^D13^D14^D15^D19^D20^D21^D23

• P2 = D0^D2^D3^D5^D6^D9^D11^D12^D15^D18^D20^D21^D22

• P1 = D0^D1^D3^D4^D6^D8^D10^D12^D14^D17^D20^D21^D22^D23

• P0 = D0^D1^D2^D4^D5^D7^D10^D11^D13^D16^D20^D21^D22^D23

P7 and P6 are set to ‘0’ because Error Correction Code (ECC) is based on 64 bit value ([D63…0]), but this

implementation is based on 24 bit value (D[23…0]). Therefore, there is only needed 6 bits (P[5…0]) for Error

Correction Code (ECC).

Figure 55 XOR Functionality on the Short Packet (SPa)

V0.22                                                                          Page  67  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

TimePacket Header (PH)DI05 hex10100000B0B1B2B3B4B5B6B7LSBMSBData 010 hex00001000B0B1B2B3B4B5B6B7LSBMSBData 100 hex00000000B0B1B2B3B4B5B6B7LSBMSBECC2C hex00110100B0B1B2B3B4B5B6B7LSBMSBD0D1D2D4D5D7D10D11D13D16D20D21D22D23P0D0D1D3D4D6D8D10D12D14D17D20D21D22D23P1D0D2D3D5D6D9D11D12D15D18D20D21D22P2D1D2D3D7D8D9D13D14D15D19D20D21D23P3D4D5D6D7D8D9D16D17D18D19D20D22D23P4D10D11D12D13D14D15D16D17D18D19D21D22D23P5深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006

                                                                               ST7102

Figure 56 XOR Functionality on the Long Packet (LPa)

V0.22                                                                          Page  68  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

TimePacket Header (PH)DI29 hex10010100B0B1B2B3B4B5B6B7LSBMSBWC (LS Byte)01 hex10001000B0B1B2B3B4B5B6B7LSBMSBWC (MS Byte)00 hex00000000B0B1B2B3B4B5B6B7LSBMSBECC06 hex01100000B0B1B2B3B4B5B6B7LSBMSBD0D1D2D4D5D7D10D11D13D16D20D21D22D23P0D0D1D3D4D6D8D10D12D14D17D20D21D22D23P1D0D2D3D5D6D9D11D12D15D18D20D21D22P2D1D2D3D7D8D9D13D14D15D19D20D21D23P3D4D5D6D7D8D9D16D17D18D19D20D22D23P4D10D11D12D13D14D15D16D17D18D19D21D22D23P5深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006
                                                                               ST7102

The transmitter (The MCU or the Display Module) is sending data bits D[23…0] and Error Correction Code (ECC) P[7…0].

The receiver (The Display module or the MCU) is calculate an Internal Error Correction Code (IECC) and compares the

received Error Correction Code (ECC) and the Internal Error Correction Code (IECC). This comparison is done when

each power bit of ECC and IECC have been done XOR function. The result of this function is PO[7…0].

This functionality, where the transmitter is the MCU and the receiver is the display module, is illustrated for reference

purposes below.

Figure 57 Internal Error Correction Code (IECC) on the Display Module (The Receiver)

The sent data bits (D[23…0]) and ECC (P[7…0]) are received correctly, if a value of the PO[7…0]) is 0 0h. The

sent data bits (D[23…0]) and ECC (P[7…0]) are not received correctly, if a value of the PO[7…0]) is not 00h.

ECC P[7…0]

IECC PI[7…0]

XOR(ECC,IECC)

=>PO[7…0]

ECC P[7…0]

IECC PI[7…0]

XOR(ECC,IECC)

=>PO[7…0]

1

1

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

0

L

S

B

03h

03h

=00h=>No Error

0

0

0

M

S

B

Internal XOR Calculation between ECC and IECC Values-No Error

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

0

0

0

0

0

0

0

1

1

0

L

S

B

03h

0Fh

=0Ch=> Error

0

0

0

M

S

B

Internal XOR Calculation between ECC and IECC Values- Error

V0.22                                                                          Page  69  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

MCUThis Display ModuleInternal ECC (IECC)Generator PI[0…7]XORXORPO[7]PO[0]DSIData : D[0…23]ECC : P[0…7]深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006

                                                                               ST7102

The received Error Correction Code (ECC) can be 00h when the Error Correction Code (ECC) functionality is not used for

data values D[23…0] on the transmitter side.

The number of the errors (one or more) can be defined when the value of the PO[7…0] is compared to values on the

following table.

Data Bit

PO7

PO6

PO5

PO4

PO3

PO2

PO1

PO0

D[0]

D[1]

D[2]

D[3]

D[4]

D[5]

D[6]

D[7]

D[8]

D[9]

D[10]

D[11]

D[12]

D[13]

D[14]

D[15]

D[16]

D[17]

D[18]

D[19]

D[20]

D[21]

D[22]

D[23]

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

1

1

1

1

0

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

1

1

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

0

1

1

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

0

1

1

0

1

1

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

0

0

1

0

1

1

1

0

1

1

0

1

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

0

1

0

0

1

1

1

1

1

1

1

0

1

1

0

1

0

0

1

1

0

1

0

0

1

0

0

0

1

1

1

1

Hex

07h

0Bh

0Dh

0Eh

13h

15h

16h

19h

1Ah

1Ch

23h

25h

26h

29h

2Ah

2Ch

31h

32h

34h

38h

1Fh

2Fh

37h

3Bh

One error is detected if the value of the PO[7…0] is on : One Bit Error Value of the Error Correction Code (ECC) and the

receiver can correct this one bit error because this found value also defines what is a location of the corrupt bit e.g.

• PO[7…0] = 0Eh

• The bit of the data (D[23…0]), what is not correct, is D[3]

More than one error is detected if the value of the PO[7…0] is not on: One Bit Error Value of the Error Correction Code

(ECC) e.g. PO[7…0] = 0Ch.

V0.22                                                                          Page  70  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006
                                                                               ST7102

5.4.2.3.1.4

Packet Date (PD) on the Long Packet (LPa)

Packet Data (PD) of the Long Packet (LPa) is defined after Packet Header (PH) of the Long Packet (LPa). The number of

the data bytes is defined on chapter “Word Count (WC) on the Long Packet (LPa)”.

5.4.2.3.1.5

Packet Footer (PF) on the Long Packet (LPa)

Packet Footer (PF) of the Long Packet (LPa) is defined after the Packet Data (PD) of the Long Packet (LPa). The Packet

Footer (PF) is a checksum value what is calculated from the Packet Data of the Long Packet (LPa).

The checksum is using a 16-bit Cyclic Redundancy Check (CRC) value which is generated with a polynomial

X16+X12+X5+X0 as it is illustrated below.

Figure 58 16-bit Cyclic Redundancy Check (CRC) Calculation

The 16-bit Cyclic Redundancy Check (CRC) generator is initialized to FFFFh before calculations. The Least

Significant Bit (LSB) of the data byte of the Packet Data (PD) is the first bit what is inputted into the 16-bit Cyclic

Redundancy Check (CRC).

An example of the 16-bit Cyclic Redundancy Check (CRC), where the Packet Data (PD) of the Long Packet (LPa) is 01h,

is illustrated (step-by-step) below.

Step

In

In^C0  C15  C14  C13  C12  C11  In^C0^C11 C10  C9  C8  C7  C6  C5  C4  In^C0^C4  C3  C2  C1  C0

0

1

2

3

4

5

6

1(LSB)

0

0

0

0

0

0

7  0(MSB)

0

1

1

1

1

0

0

0

8

X

X

CRC Result:

1

0

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

0

1

1

1

1

0

0

0

1

1

1

0

1

1

1

1

0

0

MSB

1

1

1

1

0

1

1

1

1

1

1

1

1

1

1

0

1

1

1

1

1

0

0

0

0

0

1

1

X

1

1

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

1

0

0

0

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

1

0

0

0

0

1

1

1

X

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

1

1

1

0

0

0

0

0

  LSB

Figure 59 CRC Calculation – Packet Data (PD) is 01h

V0.22                                                                          Page  71  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

C15C14C13C12C11InC10C9C8C7C6C5C4In^C0In^C0^C11C3C2C1C0In^C0^C4深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006

                                                                               ST7102

A value of the Packet Footer (PF) is 1E0Eh in this example. This example (Command 01h has been sent) is Illustrated

below.

Figure 60 Packet Footer (PF) Example

The receiver is calculated own checksum value from received Packet Data (PD). The receiver compares own checksum

and the Packet Footer (PF) what the transmitter has sent. The received Packet Data (PD) and Packet Footer (PF) are

correct if the own checksum of the receiver and Packet Footer (PF) are equal and vice versa the received Packet Data

(PD) and Packet Footer (PF) are not correct if the own checksum of the receiver and Packet Footer (PF) are not equal.

V0.22                                                                          Page  72  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

DI39 hex10011100B0B1B2B3B4B5B6B7LSBMSBWC (LS Byte)01 hex10000000B0B1B2B3B4B5B6B7LSBMSBWC (MS Byte)00 hex00000000B0B1B2B3B4B5B6B7LSBMSBECC15 hex10101000B0B1B2B3B4B5B6B7LSBMSBPacket Header (PH)Data 001 hex10000000B0B1B2B3B4B5B6B7LSBMSBCRC(LS Byte)0E hex01110000B0B1B2B3B4B5B6B7LSBMSBCRC(MS Byte)1E hex01111000B0B1B2B3B4B5B6B7LSBMSBPacket Data (PD)Packet Footer (PF)Time深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006
                                                                               ST7102

5.4.2.3.2

Packet Transmissions

5.4.2.3.2.1

Packet from the MCU to the Display Module

⚫

Display Command Set (DCS)

Display Command Set (DCS), which is defined on chapter “Command Description”, is used from the MCU to the display

module. This Display Command Set (DCS) is always defined on the Data 0 of the Packet Data (PD),which is included in

Short Packet (SPa) and Long Packet (LPa) as these are illustrated below.

Figure 61 Display Command Set (DCS) on Short Packet (SPa) and Long Packet (LPa)

V0.22                                                                          Page  73  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

LP-11SOTDIData0Data1ECCEoTLP-11Packet DataPacket Header (PH)TimeLP-11:Low Power-Stop StateSoT:Start of transmissionDI:Data Identification (8 bit)Data 0 and Data 1:Packet Data (8+8 bit)ECC:Error Correction Code (8 bit)EoT:End of TransmissionLP-11SOTDIWord Count (WC)ECCEoTLP-11Packet Header (PH)LP-11:Low Power-Stop StateSoT:Start of transmissionDI:Data Identification (8 bit)Data 0 and Data 1:Packet Data (8+8 bit)ECC:Error Correction Code (8 bit)EoT:End of TransmissionData0Data1Data WC-2Data WC-1Checksum(CS)Packet DataDisplay Command Set (DCS)深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006
                                                                               ST7102

5.4.2.3.2.2

Packet from the Display Module to the MCU

⚫

Used Packet Types

The display module is always using Short Packet (SPa) or Long Packet (LPa), when it is returning information to the MCU

after the MCU has requested information from the Display Module. This information can be a response of the Display

Command Set (DCS) or an Acknowledge with Error Report.

The used packet type is defined on Data Type (DT).

A number of the return bytes are more than the maximum size of the Packet Data (PD) on Long Packet (LPa) or Short

Packet (SPa) when the display module is sending return bytes in several packets until all return bytes have been sent

from the display module to the MCU.

It is not possible that the display module is sending return bytes in several packets even if the maximum size of the Packet

Data (PD) could be sent on a packet.

Figure 62 Return Bytes on Signal Packet

V0.22                                                                          Page  74  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

LPaLP-11LP-11LP-11LP-11SPaReturn BytesReturn Bytes深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006

                                                                               ST7102

From the Display Module (or Other Devices) to the MCU

Data Type

B

B

B

B

B

B

Hex

5

4

3

2

1

0

Description

Packet

Abbreviation

02h

11h

12h

1Ah

1Ch

21h

22h

0

0

0

0

0

1

1

0

1

1

1

1

0

0

0

0

0

1

1

0

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

1

1

0

0

1

0  Acknowledge and Error Report

Short

AwER

1  Generic Short READ Response,1 byte returned

Short

GENRR1-S

0  Generic Short READ Response,2 bytes returned

Short

GENRR2-S

0  Generic Long READ Response

Short

GENRR-L

0  DCS Long READ Response

Short

DCSRR_L

1  DCS Short READ Response, 1 byte returned

Short

DCSRR1_S

0  DCS Short READ Response, 2 bytes returned

Short

DCSRR2_S

Data Type for Display Module-sourced Packets

The display module is return 2 packets (1st packet: Data, 2nd packet: Acknowledge with Error Report ) to the MCU when

the display module has received a read command. These return packets are illustrated for reference purpose below.

Figure 63 Exception When Return Bytes on Several Packet

Note:

1. AwER=Acknowledge with Error Report

V0.22                                                                          Page  75  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

LP-11LP-111st SPa,DataReturn Bytes2nd SPa,AwER深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006

                                                                               ST7102

⚫

Acknowledge with Error Report (AwER), Data Type = 00 0010(02h)

“Acknowledge with Error Report” (AwER) is always using a Short Packet (SPa), what is defined on Data Type (DT,00

0010b), from the display module to the MCU.

The Packet Data (PD) can include bits, which are defining the current error, when a corresponding bit is set to ‘1’, as they

are defined on the following table.

Bit

Description

Sitronix LCD Driver Implementation

0

1

2

3

4

5

6

7

8

9

SoT Error

SoT Sync Error

EoT Sync Error

Escape Mode Entry Command Error

Low-Power Transmit Sync Error

Any Protocol Timer Time-Out

False Control Error

Contention is Detected on the Display Module

ECC Error, single-bit (detected and corrected)

ECC Error, multi-bit (detected, not corrected)

10

Set to “0” internally (Only for Long Packet (LP))

11

DSI Data Type (DT) Not Recognized

12

DSI Virtual Channel (VC) ID Invalid

13

Invalid Transmission Length

14

Reserved, Set to ‘0’ internally

15

DSI Protocol Violation

NO

NO

NO

YES

YES

NO

YES

NO

YES

YES

YES

YES

YES

NO

NO

NO

Acknowledge with Error Report (AwER) for Short Packet (SPa) Response

Note

AwER will return1-bit zero if the item is no implementation.

V0.22                                                                          Page  76  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006
                                                                               ST7102

These errors are only included on the last packet, which has been received from the MCU to the display module before

Bus Turnaround (BTA).

The display module ignores the received packet which includes error or errors

Acknowledge with Error Report (AwER) of the Short Packet (SPa) is defined e.g.

• Data Identification (DI)

Virtual Channel (VC, DI[7…6]): 00b

Data Type (DT, DI[5…0]): 00 0010b

• Packet Data (PD):

Bit 8: ECC Error, single-bit (detected and corrected)

AwER: 0100h

• Error Correction Code (ECC)

This is defined on the Short Packet (SPa) as follows.

Figure 64 Acknowledge with Error Report (AwER)-Example

It is possible that the display module receivers several packets, which include error, from the MPU before the MPU

performs the Bus Turnaround (BTA).Some examples are illustrated below for reference purpose.

V0.22                                                                          Page  77  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

DI02 hex01000000B0B1B2B3B4B5B6B7LSBMSBAwER(LS Byte)00 hex00000000B0B1B2B3B4B5B6B7LSBMSBAwER(MS Byte)01 hex10000000B0B1B2B3B4B5B6B7LSBMSBECC3A hex01011100B0B1B2B3B4B5B6B7LSBMSBPacket Header (PH)TimePacket Data(PD)LPaLP-11LP-11LP-11LP-11SPaPackets from the MCUSPaIncludes an errorSPaSPaPackets from the MCUIncludes an errorSOTEOTSOTEOTDSI-Dn+/-DSI-Dn+/-BTABTA深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006

                                                                               ST7102

Figure 65 Error Packet

Therefore, there is needed a method to check if there has been errors on the previous packets. These errors of the

previous packets can check “Read Number of the Errors on DSI (05h)" command.

The number of the packets, which are including an ECC or CRC error, are calculated on the RDNUMED register, which

can read “Read Number of the Errors on DSI (05h)” command. This command also sets the RDNUMED register to 00h

after the MCU has read the RDNUMED register from the display module.

V0.22                                                                          Page  78  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006

                                                                               ST7102

5.5

Serial Interface_ Control Bus ( RGB_video / MIPI video)

The  serial  interface  is  either  SPI9,  SPI16  or  SPI4/8-bits  interface  for  communication  between  the  micro

controller and  the  LCD  driver.  The  SPI9,SPI16  serial  interface  use:  CSX  (chip  enable), SCL  (serial  clock)  ,  SDA

(serial data input) and SDO(serial data output), and the SPI8 serial interface use: CSX (chip enable), D/CX (data/

command flag), SCL (serial clock) , SDA (serial data input) and SDO(serial data output). Serial clock (SCL) is used

for interface with MCU only, so it can be stopped when no communication is necessary.

  Pin description

SPI9_3-line serial interface (9 bits) & SPI16_3-line serial interface (16 bits)

Pin Name

CSX

SCL

SDA

SDO

SPI8_4-line serial interface (8 bits)

Pin Name

CSX

DCX

SCL

SDA

SDO

5.5.1

SPI Write mode

5.5.1.1

SPI8 & SPI9

Description

Chip selection signal

Serial input CLK

Serial input data

Serial output data

Description

Chip selection signal

Data is regarded as a command when SCL is low

Data is regarded as a parameter or data when SCL is high

Clock signal

Serial input data

Serial output data

The  write  mode  of  the  interface  means  the  micro  controller  writes  commands  and  data  to  the  LCD  driver.

3-lines serial data packet contains a control bit D/CX and a transmission byte. In 4-lines serial interface, data packet

contains  just  transmission  byte  and  control  bit  D/CX  is  transferred  by  the  D/CX  pin.  If  D/CX  is  “low”,  the

transmission byte is interpreted as a command byte. If D/CX is “high”, the transmission byte is command register as

parameter.

Any  instruction  can  be  sent  in  any  order  to  the  driver.  The  MSB  is  transmitted  first.  The  serial  interface  is

initialized  when  CSX  is  high.  In  this  state,  SCL  clock  pulse  or  SDA  data  have  no  effect.  A  falling  edge  on  CSX

enables the serial interface and indicates the start of data transmission.

V0.22                                                                          Page  79  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006

                                                                               ST7102

Serial interface data stream format

When CSX is “high”, SCL clock is ignored. During the high period of CSX the serial interface is initialized. At the

falling edge of CSX, SCL can be high or low. SDA is sampled at the rising edge of SCL. D/CX indicates whether the

byte  is  command  (D/CX=’0’)  or  parameter  data  (D/CX=’1’).  D/CX  is  sampled  when  first  rising  edge  of  SCL

(SPI9:3-line serial interface) or 8th rising edge of SCL (SPI8:4-line serial interface). If CSX stays low after the last bit

of  command/data  byte,  the  serial  interface  expects  the  D/CX  bit  (SPI9:3-line  serial  interface)  or  D7  (SPI8:4-line

serial interface) of the next byte at the next rising edge of SCL..

SPI9:3-line serial interface write protocol (write to register with control bit in transmission)

V0.22                                                                          Page  80  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

D/CXD7D6D5D4D3D2D1D0D/CXD7D6D5D4D3D2D1D0D/CXD7D6D5D4D3D2D1D0D7D6D5D4D3D2D1D0D7D6D5D4D3D2D1D0D7D6D5D4D3D2D1D0Transmission byte (TB) may be command or data SPI9:3-line serial data stream format:  TBTB   TBTBTransmission byte (TB) may be command or dataSPI8:4-line serial data stream format:D7D6D5D4D3D2D1D0D/CD7D6D5D4D3D2D1D0D/CSPTBTBCSXSDASCL Host(MCU to driver)CommandCommand/ParameterCSX can be “H”between parameter/command and parameter/command SCL, and SDA during CSX=”H” is ignored.深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006

                                                                               ST7102

SPI8:4-line serial interface write protocol (write to register with control bit in transmission)

5.5.1.2

SPI16

The write mode of the interface means the micro controller writes commands and data to the ST7102. The serial

interface is initialized when CSX is high. In this state, SCL clock pulse or SDI data have no effect. A falling edge on

CSX enables the serial interface and indicates the start of data transmission.

When CSX is high, SCL clock is ignored. During the high time of CSX the serial interface is initialized. At the

falling CSX edge, SCL can be high or low. SDI/SDO are sampled at the rising edge of SCL. R/W indicates, whether

the byte is read command (R/W = '1') or write command (R/W = '0'). It is sampled when first rising SCL edge. If CSX

stays low after the last bit of command/data byte, the serial interface expects the R/W bit of the next byte at the next

rising edge of SCL

V0.22                                                                          Page  81  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

D7D6D5D4D3D2D1D0D7D6D5D4D3D2D1D00SPTBTBCSXSDASCL Host(MCU to driver)CommandCommand/ParameterCSX can be “H”between parameter/command and parameter/command SCL, and SDA during CSX=”H” is ignored.TBTB0D/CX深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006

                                                                               ST7102

V0.22                                                                          Page  82  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

STBTBCSXSCL(rising)SDOPSR/W00000SDIADD[15]ADD[14]ADD[13]ADD[12]ADD[11]ADD[10]ADD[09]ADD[08]D/CXH/LHi-ZHi-ZHi-ZHi-ZR/W0D/CXH/LSCL(falling)FirstTransmitSTBTBCSXSCL(rising)SDOPSR/W00000SDID[7]D[6]D[5]D[4]D[3]D[2]D[1]D[0]D/CXH/LHi-ZHi-ZHi-ZHi-ZR/W0D/CXH/LSCL(falling)ThirdTransmitSTBTBCSXSCL(rising)SDOPSR/W00000SDIADD[07]ADD[06]ADD[05]ADD[04]ADD[03]ADD[02]ADD[01]ADD[00]D/CXH/LHi-ZHi-ZHi-ZHi-ZR/W0D/CXH/LSCL(falling)SecondTransmitR/W=0  for write command/addressD/CX=0 for command /address transmissionH/L=1 for command/address High Byte transmissionR/W=0  for write command/addressD/CX=0 for command /address transmissionH/L=0 for command/address Low Byte transmissionR/W=0  for write parameter /dataD/CX=1 for parameter /data transmissionH/L=0 for parameter /data Low Byte transmission深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006
                                                                               ST7102

serial 16 bit interface write mode

V0.22                                                                          Page  83  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006                                                                               ST7102

5.5.2

SPI Read mode

5.5.2.1

SPI8 & SPI9

The read mode of the interface means that the micro controller reads register value from the driver. To achieve

read  function,  the  micro  controller  first  has  to  send  a  command  (read  ID  or  register  command)  and  then  the

following  byte  is  transmitted  in  the  opposite  direction.  After  that  CSX  is  required  to  go  to  high  before  a  new

command is send (see the below figure). The driver samples the SDA (input data) at rising edge of SCL, but shifts

SDO (output data) at the falling edge of SCL. Thus the micro controller is supported to read at the rising edge of

SCL.

After the read status command has been sent, the SDA line must be set to tri-state no later than at the falling

edge of SCL of the last bit.

SPI9:3-line serial interface protocol

SPI9:3-line serial protocol (for RDID1/RDID2/RDID3/0Ah/0Bh/0Ch/0Dh/0Eh/0Fh command: 8-bit read):

SPI9:3-line serial protocol (for RDDID command: 24-bit read)

V0.22                                                                          Page  84  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

D23D22D21D20D3D2D1D0STBTBCSXSCLSDAD7D6D5D4D3D2D1D0D/CD/CXPSSDOHi-ZHi-ZDummy clock cycle 深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006

                                                                               ST7102

SPI9:3-line Serial Protocol (for RDDST command: 32-bit read)

SPI9:3-line serial interface read protocol

V0.22                                                                          Page  85  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

D31D30D29D28D3D2D1D0STBTBCSXSCLSDAD7D6D5D4D3D2D1D0D/CD/CXPSSDOHi-ZHi-ZDummy clock cycle D7D6D5D4D3D2D1D0STBTBCSXSCLSDAD7D6D5D4D3D2D1D0D/CD/CXPSSDOHi-ZHi-Z 深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006

                                                                               ST7102

SPI8:4-line serial protocol

SPI8:4-line serial protocol (for RDID1/RDID2/RDID3/0Ah/0Bh/0Ch/0Dh/0Eh/0Fh command: 8-bit read):

SPI8:4-line serial protocol (for RDDID command: 24-bit read)

V0.22                                                                          Page  86  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

D7D6D5D4D3D2D1D0STBTBCSXSCLSDOD7D6D5D4D3D2D1D00D7PSSDAHi-ZHi-ZD/CX D23D22D21D20D3D2D1D0STBTBCSXSCLD7D6D5D4D3D2D1D0D7PSHi-ZHi-ZDummy clock cycle0SDOSDAD/CX 深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006

                                                                               ST7102

SPI8:4-line Serial Protocol (for RDDST command: 32-bit read)

5.5.2.1

SPI16

SPI8:4-line serial interface read protocol

The read mode of the interface means that the micro controller reads register value from the IC. To do so the micro

controller first has to send a command and then the following byte is transmitted in the opposite

direction. After that CSX is required to go high before a new command is send. The IC samples the SDI (input data)

at the rising edges, but shifts SDO (output data) at the falling SCL edges. Thus the micro controller is supported to

read data at the rising SCL edges. After the read status command has been sent, the SDI line must be set to

tri-state no later than at the falling SCL edge of the last bit. It doesn't need any dummy clock when execute the

command data read.

V0.22                                                                          Page  87  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

D31D30D29D28D3D2D1D0STBTBCSXSCLD7D6D5D4D3D2D1D0D7PSHi-ZHi-ZDummy clock cycle0SDOSDAD/CX 深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006

                                                                               ST7102

V0.22                                                                          Page  88  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

STBTBCSXSCL(rising)SDOPSR/W00000SDIADD[15]ADD[14]ADD[13]ADD[12]ADD[11]ADD[10]ADD[09]ADD[08]D/CXH/LHi-ZHi-ZHi-ZHi-ZR/W0D/CXH/LSCL(falling)FirstTransmitSTBTBCSXSCL(rising)SDOPSR/W00000SDIADD[07]ADD[06]ADD[05]ADD[04]ADD[03]ADD[02]ADD[01]ADD[00]D/CXH/LHi-ZHi-ZHi-ZHi-ZR/W0D/CXH/LSCL(falling)SecondTransmitSTBTBCSXSCL(rising)SDOPSR/W00000SDID[7]D[6]D[5]D[4]D[3]D[2]D[1]D[0]D/CXH/LHi-ZHi-ZHi-ZHi-ZR/W0D/CXH/LSCL(falling)ThirdTransmitR/W=0  for write command/addressD/CX=0 for command /address transmissionH/L=1 for command/address High Byte transmissionR/W=0  for write command/addressD/CX=0 for command /address transmissionH/L=0 for command/address High Byte transmissionR/W=1  for read command/addressD/CX=0 for command /address transmissionH/L=0 for command/address High Byte transmission深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006
                                                                               ST7102

5.6

RGB Interface

The ST7102 support RGB interface Mode 1 and Mode 2. The interface signals as shown in following table.

The Mode 1 and Mode 2 function is select by setting in the Command 2, please reference application note.

In RGB Mode 1, writing data to line buffer is done by PCLK and Video Data Bus (D[23:0]), when Enable is high

state. The external clocks (PCLK, VS and HS) are used for internal displaying clock. So, controller must always

transfer PCLK, VS and HS signal to ST7102.

In RGB Mode 2, back porch of Vsync is defined by VBP_HVRGB [7:0] of RGBCTR command. And back porch of

Hsync is defined by HBP_HVRGB [7:0] of RGBCTR command. Front porch of Vsync are not setting by this mode

RGB I/F Mode

PCLK

DE

VS

HS

DB[23:0]

Register for Blanking Porch setting

RGB Mode 1

Used

Used

Used  Used

Used

RGB Mode 2

Used

Not Used  Used  Used

Used

Not Used

Used

Symbol

PCLK

HS

VS

DE

Name

Description

Pixel clock

Pixel clock for capturing pixels at display interface

Horizontal sync

Horizontal synchronization timing signal

Vertical sync

Vertical synchronization timing signal

Data enable

Data enable signal (assertion indicates valid pixels)

DB[23:0]

Pixel data

Pixel data in 16-bit,18-bit and 24-bit format

The interface signals of RGB interface

V0.22                                                                          Page  89  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006

                                                                               ST7102

5.6.1

RGB Color Format

ST7102 supports two kinds of RGB interface, DE mode (mode 1) and HV mode (mode 2), and 16bit/18bit and 24 bit

data format. When DE mode is selected and the VSYNC, HSYNC, DOTCLK, ENABLE, D[23:0] pins can be used;

when HV mode is selected and the VSYNC, HSYNC, PCLK, D[23:0] pins can be used. When using RGB interface,

only serial interface can be selected.

Pad name

24 bits configuration

VIPF[3:0]=0111

18 bits configuration

VIPF[3:0]=0110

16 bits configuration

VIPF[3:0]=0101

DB[23]

DB[22]

DB[21]

DB[20]

DB[19]

DB[18]

DB[17]

DB[16]

DB[15]

DB[14]

DB[13]

DB[12]

DB[11]

DB[10]

DB[09]

DB[08]

DB[07]

DB[06]

DB[05]

DB[04]

DB[03]

DB[02]

DB[01]

DB[00]

R7

R6

R5

R4

R3

R2

R1

R0

G7

G6

G5

G4

G3

G2

G1

G0

B7

B6

B5

B4

B3

B2

B1

B0

MDT=0

Not used

Not used

R5

R4

R3

R2

R1

R0

Not used

Not used

G5

G4

G3

G2

G1

G0

Not used

Not used

B5

B4

B3

B2

B1

B0

MDT=1

Not used

Not used

Not used

Not used

Not used

Not used

R5

R4

R3

R2

R1

R0

G5

G4

G3

G2

G1

G0

B5

B4

B3

B2

B1

B0

The interface color mapping of RGB interface

Not used

Not used

Not used

R4

R3

R2

R1

R0

Not used

Not used

G5

G4

G3

G2

G1

G0

Not used

Not used

Not used

B4

B3

B2

B1

B0

V0.22                                                                          Page  90  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006

                                                                               ST7102

Other Application TABLE

Pad name

24 bits configuration

VIPF[3:0]=0111

For 24 Bit

For 18 Bit

For 16 Bit

DB[23]

DB[22]

DB[21]

DB[20]

DB[19]

DB[18]

DB[17]

DB[16]

DB[15]

DB[14]

DB[13]

DB[12]

DB[11]

DB[10]

DB[09]

DB[08]

DB[07]

DB[06]

DB[05]

DB[04]

DB[03]

DB[02]

DB[01]

DB[00]

R7

R6

R5

R4

R3

R2

R1

R0

G7

G6

G5

G4

G3

G2

G1

G0

B7

B6

B5

B4

B3

B2

B1

B0

R5

R4

R3

R2

R1

R0

R5

R4

G5

G4

G3

G2

G1

G0

G5

G4

B5

B4

B3

B2

B1

B0

B5

B4

R4

R3

R2

R1

R0

R4

R3

R2

G5

G4

G3

G2

G1

G0

G5

G4

B4

B3

B2

B1

B0

B4

B3

B2

V0.22                                                                          Page  91  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006                                                                               ST7102

5.6.2

RGB Interface Definition

The display operation via the RGB interface is synchronized with the VSYNC, HSYNC, and PCLK signals. The

data can be written only within the specified area with low power consumption by using window address function.

The back porch and front porch are used to set the RGB interface timing.

Please refer to the following table for the setting limitation of RGB interface signals.

Access Area by RGB Interface

Parameter

Symbol

Min.

Typ.

Horizontal Sync. Width

Horizontal Sync. Back Porch

Horizontal Sync. Front Porch

Vertical Sync. Width

Vertical Sync. Back Porch

Vertical Sync. Front Porch

hpw

hbp

hfp

vs

vbp

vfp

2

2

2

2

2

2

-

--

--

--

--

--

Max.

TBD

TBD

-

TBD

TBD

--

Unit

Clock

Clock

Clock

Line

Line

Line

Note:

1、Typical value are related to the setting frame rate is 60Hz..

2、VS+VBP<= TBD, HPW+HBP<= TBD

V0.22                                                                          Page  92  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

Visible image= whick can be seen on the display= active areaDE="1" (high)Invisible image= Timing information which cannot be seen on the display= blank timeDE="0" (low)vsvbpvdispvfpVPhpwhbphdisphfpHPVertical Sync.Horizontal Sync.深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006

                                                                               ST7102

5.6.1

RGB Interface Mode Selection

ST7102 supports two kinds of RGB interface, DE mode and HV mode. The table shown below uses command

C3h to select RGB interface mode.

DE/HV(Sync)

RGB Mode

0

1

DE mode

HV mode

RGB Interface Timing

The timing chart of RGB interface DE mode is shown as follows.

Note: The setting of front porch and back porch in host must match that in IC as this mode.

Timing Chart of Signals in RGB Interface DE Mode

V0.22                                                                          Page  93  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

VSHSDEHSDOTCLKDEData busLatch dataV back porch (Tvs+Tvbp)1 frame (TVP)V front porch (Tvfp)1 line (THP)H back porch (Thpw+Thbp)Valid data (Thdisp)H front porch (Thfp)InvalidInvalidInvalidDnDnD1D1D2D2D3D3深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006

                                                                               ST7102

The timing chart of RGB interface HV mode is shown as follows.

Timing chart of RGB interface HV mode

V0.22                                                                          Page  94  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

VSHSDEHSPCLKDEData busLatch dataV back porch (Tvs+Tvbp)1 frame (TVP)V front porch (Tvfp)1 line (THP)H back porch (Thpw+Thbp)Valid data (Thdisp)H front porch (Thfp)InvalidInvalidInvalidDnDnD1D1D2D2D3D3“1"“1"深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006
                                                                               ST7102

5.7

Digital Gamma

Digital gamma correct makes three sub-pixel (Red, Green, Blue) gamma curve by using one analog gamma curve.

By setting the digital gamma correction registers we can make three equivalent gamma curve Vgamma_total_r,

Vgamma_total_g, Vgamma_total_b as desired.

V0.22                                                                          Page  95  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

Vgamma_anasub_pixel_rsub_pixel_gsub_pixel_bdgc_rdgc_bdgc_gVgamma_total_rdigital gammaanalog gammaVgamma_total_gVgamma_total_b深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006

                                                                               ST7102

5.8

Gamma Correction Function

ST7102 incorporates the gamma correction function to display 16,777,216 colors for the LCD panel. There is a

analog gamma correction is performed with 2 registers to set both positive and negative polarity voltage for gamma

curve.

5.8.1

Gamma Correction Registers

The all of grayscale reference levels are shown blow.

Register Groups

Gamma Polarity

Description

Positive

Negative

Grayscale

VGMP0[9:0]

VGMN0[9:0]

1023-to-1 selector (voltage level of R grayscale 0)

adjustment

VGMP1[9:0]

VGMN1[9:0]

1023-to-1 selector (voltage level of R grayscale 1)

VGMP2[9:0]

VGMN2[9:0]

1023-to-1 selector (voltage level of R grayscale 2)

VGMP3[9:0]

VGMN3[9:0]

1023-to-1 selector (voltage level of R grayscale 4)

VGMP4[9:0]

VGMN4[9:0]

1023-to-1 selector (voltage level of R grayscale 8)

VGMP5[5:0]

VGMN5[5:0]

1023-to-1 selector (voltage level of R grayscale 12)

VGMP6[9:0]

VGMN6[9:0]

1023-to-1 selector (voltage level of R grayscale 16)

VGMP7[5:0]

VGMN7[5:0]

1023-to-1 selector (voltage level of R grayscale 24)

VGMP8[9:0]

VGMN8[9:0]

1023-to-1 selector (voltage level of R grayscale 32)

VGMP9[5:0]

VGMN9[5:0]

1023-to-1 selector (voltage level of R grayscale 48)

VGMP10[9:0]

VGMN10[9:0]

1023-to-1 selector (voltage level of R grayscale 64)

VGMP11[3:0]

VGMN11[3:0]

1023-to-1 selector (voltage level of R grayscale 80)

VGMP12[9:0]

VGMN12[9:0]

1023-to-1 selector (voltage level of R grayscale 96)

VGMP13[3:0]

VGMN13[3:0]

1023-to-1 selector (voltage level of R grayscale 112)

VGMP14[9:0]

VGMN14[9:0]

1023-to-1 selector (voltage level of R grayscale 128)

VGMP15[3:0]

VGMN15[3:0]

1023-to-1 selector (voltage level of R grayscale 144)

VGMP16[9:0]

VGMN16[9:0]

1023-to-1 selector (voltage level of R grayscale 160)

VGMP17[3:0]

VGMN17[3:0]

1023-to-1 selector (voltage level of R grayscale 176)

VGMP18[9:0]

VGMN18[9:0]

1023-to-1 selector (voltage level of R grayscale 192)

VGMP19[3:0]

VGMN19[3:0]

1023-to-1 selector (voltage level of R grayscale 208)

VGMP20[9:0]

VGMN20[9:0]

1023-to-1 selector (voltage level of R grayscale 224)

VGMP21[3:0]

VGMN21[3:0]

1023-to-1 selector (voltage level of R grayscale 232)

VGMP22[9:0]

VGMN22[9:0]

1023-to-1 selector (voltage level of R grayscale 240)

VGMP23[3:0]

VGMN23[3:0]

1023-to-1 selector (voltage level of R grayscale 244)

VGMP24[9:0]

VGMN24[9:0]

1023-to-1 selector (voltage level of R grayscale 248)

VGMP25[3:0]

VGMN25[3:0]

1023-to-1 selector (voltage level of R grayscale 250)

V0.22                                                                          Page  96  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006
                                                                               ST7102

VGMP26[9:0]

VGMN26[9:0]

1023-to-1 selector (voltage level of R grayscale 252)

VGMP27[9:0]

VGMN27[9:0]

1023-to-1 selector (voltage level of R grayscale 254)

VGMP28[9:0]

VGMN28[9:0]

1023-to-1 selector (voltage level of R grayscale 255)

V0.22                                                                          Page  97  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006

                                                                               ST7102

5.8.2

Gamma function architecture

Figure 66 positive and Negative grayscale voltage generation

V0.22                                                                          Page  98  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

VREGP/VREGNVFT1024-to-1 MUX1024-to-1 MUX1024-to-1 MUX1024-to-1 MUXVGP/N[0]VGP/N[1]VGP/N[2]VGP/N[4]VGP/N[8]VGP/N[12]VGP/N[16]VGP/N[24]VGP/N[32]VGP/N[48]VGP/N[64]VGP/N[80]VGP/N[96]VGP/N[112]VGP/N[128]VGP/N[144]VGP/N[160]VGP/N[176]VGP/N[192]VGP/N[208]VGP/N[224]VGP/N[232]VGP/N[240]VGP/N[244]VGP/N[248]VGP/N[250]VGP/N[252]VGP/N[254]VGP/N[255]1024-to-1 MUX1024-to-1 MUX1024-to-1 MUX1024-to-1 MUXR1R2R3R4R1020R1021R1022R1023VGMP/N0[9:0]VGMP/N1[9:0]VGMP/N27[9:0]VGMP/N28[9:0]10101010深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006
                                                                               ST7102

Note1. VFT: Feed through voltage

Note2.VREGP: VFT+VGPAMP=feed through voltage + positive gamma amplitude.

Note3.VREGN: VFT+VGNAMP=feed through voltage + negative gamma amplitude.

5.8.3

Grayscale voltage formula

Grayscale Voltage  Formula

Grayscale Voltage

Formula

V0

V1

V2

V3

V4

V5

V6

V7

V8

V9

V10

V11

V12

V13

V14

V15

V16

V17

V18

V19

V20

V21

V22

V23

V24

V25

V26

VGP/N0

VGP/N1

VGP/N2

1/2 * VGP/N2 + 1/2* VGP/N3

VGP/N3

3/4 * VGP/N3 + 1/4* VGP/N4

2/4 * VGP/N3 + 2/4* VGP/N4

1/4 * VGP/N3 + 3/4* VGP/N4

VGP/N4

3/4 * VGP/N4 + 1/4* VGP/N5

2/4 * VGP/N4 + 2/4* VGP/N5

1/4 * VGP/N4 + 3/4* VGP/N5

VGP/N5

3/4 * VGP/N5 + 1/4* VGP/N6

2/4 * VGP/N5 + 2/4* VGP/N6

1/4 * VGP/N5 + 3/4* VGP/N6

VGP/N6

7/8 * VGP/N6 + 1/8 * VGP/N7

6/8 * VGP/N6 + 2/8 * VGP/N7

5/8 * VGP/N6 + 3/8 * VGP/N7

4/8 * VGP/N6 + 4/8 * VGP/N7

3/8 * VGP/N6 + 5/8 * VGP/N7

2/8 * VGP/N6 + 6/8 * VGP/N7

1/8 * VGP/N6 + 7/8 * VGP/N7

VGP/N7

7/8 * VGP/N7 + 1/8 * VGP/N8

6/8 * VGP/N7 + 2/8 * VGP/N8

V64

V65

V66

V67

V68

V69

V70

V71

V72

V73

V74

V75

V76

V77

V78

V79

V80

V81

V82

V83

V84

V85

V86

V87

V88

V89

V90

VGP/N10

15/16 * VGP/N10 + 1/16 * VGP/N11

14/16 * VGP/N10 + 2/16 * VGP/N11

13/16 * VGP/N10 + 3/16 * VGP/N11

12/16 * VGP/N10 + 4/16 * VGP/N11

11/16 * VGP/N10 + 5/16 * VGP/N11

10/16 * VGP/N10 + 6/16 * VGP/N11

9/16 * VGP/N10 + 7/16 * VGP/N11

8/16 * VGP/N10 + 8/16 * VGP/N11

7/16 * VGP/N10 + 9/16 * VGP/N11

6/16 * VGP/N10 + 10/16 * VGP/N11

5/16 * VGP/N10 + 11/16 * VGP/N11

4/16 * VGP/N10 + 12/16 * VGP/N11

3/16 * VGP/N10 + 13/16 * VGP/N11

2/16 * VGP/N10 + 14/16 * VGP/N11

1/16 * VGP/N10 + 15/16 * VGP/N11

VGP/N11

15/16 * VGP/N11 + 1/16 * VGP/N12

14/16 * VGP/N11 + 2/16 * VGP/N12

13/16 * VGP/N11 + 3/16 * VGP/N12

12/16 * VGP/N11 + 4/16 * VGP/N12

11/16 * VGP/N11 + 5/16 * VGP/N12

10/16 * VGP/N11 + 6/16 * VGP/N12

9/16 * VGP/N11 + 7/16 * VGP/N12

8/16 * VGP/N11 + 8/16 * VGP/N12

7/16 * VGP/N11 + 9/16 * VGP/N12

6/16 * VGP/N11 + 10/16 * VGP/N12

V0.22                                                                          Page  99  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006

V27

V28

V29

V30

V31

V32

V33

V34

V35

V36

V37

V38

V39

V40

V41

V42

V43

V44

V45

V46

V47

V48

V49

V50

V51

V52

V53

V54

V55

V56

V57

V58

V59

V60

V61

V62

                                                                               ST7102

5/8 * VGP/N7 + 3/8 * VGP/N8

4/8 * VGP/N7 + 4/8 * VGP/N8

3/8 * VGP/N7 + 5/8 * VGP/N8

2/8 * VGP/N7 + 6/8 * VGP/N8

1/8 * VGP/N7 + 7/8 * VGP/N8

VGP/N8

V91

V92

V93

V94

V95

V96

5/16 * VGP/N11 + 11/16 * VGP/N12

4/16 * VGP/N11 + 12/16 * VGP/N12

3/16 * VGP/N11 + 13/16 * VGP/N12

2/16 * VGP/N11 + 14/16 * VGP/N12

1/16 * VGP/N11 + 15/16 * VGP/N12

VGP/N12

15/16 * VGP/N8 + 1/16 * VGP/N9  V97

15/16 * VGP/N12 + 1/16 * VGP/N13

14/16 * VGP/N8 + 2/16 * VGP/N9  V98

14/16 * VGP/N12 + 2/16 * VGP/N13

13/16 * VGP/N8 + 3/16 * VGP/N9  V99

13/16 * VGP/N12 + 3/16 * VGP/N13

12/16 * VGP/N8 + 4/16 * VGP/N9  V100

12/16 * VGP/N12 + 4/16 * VGP/N13

11/16 * VGP/N8 + 5/16 * VGP/N9  V101

11/16 * VGP/N12 + 5/16 * VGP/N13

10/16 * VGP/N8 + 6/16 * VGP/N9  V102

10/16 * VGP/N12 + 6/16 * VGP/N13

9/16 * VGP/N8 + 7/16 * VGP/N9  V103

9/16 * VGP/N12 + 7/16 * VGP/N13

8/16 * VGP/N8 + 8/16 * VGP/N9  V104

8/16 * VGP/N12 + 8/16 * VGP/N13

7/16 * VGP/N8 + 9/16 * VGP/N9  V105

7/16 * VGP/N12 + 9/16 * VGP/N13

6/16 * VGP/N8 + 10/16 * VGP/N9  V106

6/16 * VGP/N12 + 10/16 * VGP/N13

5/16 * VGP/N8 + 11/16 * VGP/N9  V107

5/16 * VGP/N12 + 11/16 * VGP/N13

4/16 * VGP/N8 + 12/16 * VGP/N9  V108

4/16 * VGP/N12 + 12/16 * VGP/N13

3/16 * VGP/N8 + 13/16 * VGP/N9  V109

3/16 * VGP/N12 + 13/16 * VGP/N13

2/16 * VGP/N8 + 14/16 * VGP/N9  V110

2/16 * VGP/N12 + 14/16 * VGP/N13

1/16 * VGP/N8 + 15/16 * VGP/N9  V111

1/16 * VGP/N12 + 15/16 * VGP/N13

VGP/N9

V112

VGP/N13

15/16 * VGP/N9 + 1/16 * VGP/N10  V113

15/16 * VGP/N13 + 1/16 * VGP/N14

14/16 * VGP/N9 + 2/16 * VGP/N10  V114

14/16 * VGP/N13 + 2/16 * VGP/N14

13/16 * VGP/N9 + 3/16 * VGP/N10  V115

13/16 * VGP/N13 + 3/16 * VGP/N14

12/16 * VGP/N9 + 4/16 * VGP/N10  V116

12/16 * VGP/N13 + 4/16 * VGP/N14

11/16 * VGP/N9 + 5/16 * VGP/N10  V117

11/16 * VGP/N13 + 5/16 * VGP/N14

10/16 * VGP/N9 + 6/16 * VGP/N10  V118

10/16 * VGP/N13 + 6/16 * VGP/N14

9/16 * VGP/N9 + 7/16 * VGP/N10  V119

9/16 * VGP/N13 + 7/16 * VGP/N14

8/16 * VGP/N9 + 8/16 * VGP/N10  V120

8/16 * VGP/N13 + 8/16 * VGP/N14

7/16 * VGP/N9 + 9/16 * VGP/N10  V121

7/16 * VGP/N13 + 9/16 * VGP/N14

6/16 * VGP/N9 + 10/16 * VGP/N10  V122

6/16 * VGP/N13 + 10/16 * VGP/N14

5/16 * VGP/N9 + 11/16 * VGP/N10  V123

5/16 * VGP/N13 + 11/16 * VGP/N14

4/16 * VGP/N9 + 12/16 * VGP/N10  V124

4/16 * VGP/N13 + 12/16 * VGP/N14

3/16 * VGP/N9 + 13/16 * VGP/N10  V125

3/16 * VGP/N13 + 13/16 * VGP/N14

2/16 * VGP/N9 + 14/16 * VGP/N10  V126

2/16 * VGP/N13 + 14/16 * VGP/N14

V0.22                                                                          Page  100  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006                                                                               ST7102

V63

1/16 * VGP/N9 + 15/16 * VGP/N10  V127

1/16 * VGP/N13 + 15/16 * VGP/N14

Grayscale Voltage  Formula

Grayscale Voltage

Formula

V128

V129

V130

V131

V132

V133

V134

V135

V136

V137

V138

V139

V140

V141

V142

V143

V144

V145

V146

V147

V148

V149

V150

V151

V152

V153

V154

V155

V156

V157

V158

V159

V160

VGP/N14

V192

VGP/N18

15/16 * VGP/N14 + 1/16 * VGP/N15 V193

15/16 * VGP/N18 + 1/16 * VGP/N19

14/16 * VGP/N14 + 2/16 * VGP/N15  V194

14/16 * VGP/N18 + 2/16 * VGP/N19

13/16 * VGP/N14 + 3/16 * VGP/N15  V195

13/16 * VGP/N18 + 3/16 * VGP/N19

12/16 * VGP/N14 + 4/16 * VGP/N15  V196

12/16 * VGP/N18 + 4/16 * VGP/N19

11/16 * VGP/N14 + 5/16 * VGP/N15  V197

11/16 * VGP/N18 + 5/16 * VGP/N19

10/16 * VGP/N14 + 6/16 * VGP/N15  V198

10/16 * VGP/N18 + 6/16 * VGP/N19

9/16 * VGP/N14 + 7/16 * VGP/N15  V199

9/16 * VGP/N18 + 7/16 * VGP/N19

8/16 * VGP/N14 + 8/16 * VGP/N15  V200

8/16 * VGP/N18 + 8/16 * VGP/N19

7/16 * VGP/N14 + 9/16 * VGP/N15  V201

7/16 * VGP/N18 + 9/16 * VGP/N19

6/16 * VGP/N14 + 10/16 * VGP/N15  V202

6/16 * VGP/N18 + 10/16 * VGP/N19

5/16 * VGP/N14 + 11/16 * VGP/N15  V203

5/16 * VGP/N18 + 11/16 * VGP/N19

4/16 * VGP/N14 + 12/16 * VGP/N15  V204

4/16 * VGP/N18 + 12/16 * VGP/N19

3/16 * VGP/N14 + 13/16 * VGP/N15  V205

3/16 * VGP/N18 + 13/16 * VGP/N19

2/16 * VGP/N14 + 14/16 * VGP/N15  V206

2/16 * VGP/N18 + 14/16 * VGP/N19

1/16 * VGP/N14 + 15/16 * VGP/N15  V207

1/16 * VGP/N18 + 15/16 * VGP/N19

VGP/N15

V208

VGP/N19

15/16 * VGP/N15 + 1/16 * VGP/N16 V209

15/16 * VGP/N19 + 1/16 * VGP/N20

14/16 * VGP/N15 + 2/16 * VGP/N16  V210

14/16 * VGP/N19 + 2/16 * VGP/N20

13/16 * VGP/N15 + 3/16 * VGP/N16  V211

13/16 * VGP/N19 + 3/16 * VGP/N20

12/16 * VGP/N15 + 4/16 * VGP/N16  V212

12/16 * VGP/N19 + 4/16 * VGP/N20

11/16 * VGP/N15 + 5/16 * VGP/N16  V213

11/16 * VGP/N19 + 5/16 * VGP/N20

10/16 * VGP/N15 + 6/16 * VGP/N16  V214

10/16 * VGP/N19 + 6/16 * VGP/N20

9/16 * VGP/N15 + 7/16 * VGP/N16  V215

9/16 * VGP/N19 + 7/16 * VGP/N20

8/16 * VGP/N15 + 8/16 * VGP/N16  V216

8/16 * VGP/N19 + 8/16 * VGP/N20

7/16 * VGP/N15 + 9/16 * VGP/N16  V217

7/16 * VGP/N19 + 9/16 * VGP/N20

6/16 * VGP/N15 + 10/16 * VGP/N16  V218

6/16 * VGP/N19 + 10/16 * VGP/N20

5/16 * VGP/N15 + 11/16 * VGP/N16  V219

5/16 * VGP/N19 + 11/16 * VGP/N20

4/16 * VGP/N15 + 12/16 * VGP/N16  V220

4/16 * VGP/N19 + 12/16 * VGP/N20

3/16 * VGP/N15 + 13/16 * VGP/N16  V221

3/16 * VGP/N19 + 13/16 * VGP/N20

2/16 * VGP/N15 + 14/16 * VGP/N16  V222

2/16 * VGP/N19 + 14/16 * VGP/N20

1/16 * VGP/N15 + 15/16 * VGP/N16  V223

1/16 * VGP/N19 + 15/16 * VGP/N20

VGP/N16

V224

VGP/N20

V0.22                                                                          Page  101  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006
V161

V162

V163

V164

V165

V166

V167

V168

V169

V170

V171

V172

V173

V174

V175

V176

V177

V178

V179

V180

V181

V182

V183

V184

V185

V186

V187

V188

V189

V190

V191

                                                                               ST7102
15/16 * VGP/N16 + 1/16 * VGP/N17 V225

7/8 * VGP/N20 + 1/8 * VGP/N21

14/16 * VGP/N16 + 2/16 * VGP/N17  V226

6/8 * VGP/N20 + 2/8 * VGP/N21

13/16 * VGP/N16 + 3/16 * VGP/N17  V227

5/8 * VGP/N20 + 3/8 * VGP/N21

12/16 * VGP/N16 + 4/16 * VGP/N17  V228

4/8 * VGP/N20 + 4/8 * VGP/N21

11/16 * VGP/N16 + 5/16 * VGP/N17  V229

3/8 * VGP/N20 + 5/8 * VGP/N21

10/16 * VGP/N16 + 6/16 * VGP/N17  V230

2/8 * VGP/N20 + 6/8 * VGP/N21

9/16 * VGP/N16 + 7/16 * VGP/N17  V231

1/8 * VGP/N20 + 7/8 * VGP/N21

8/16 * VGP/N16 + 8/16 * VGP/N17  V232

VGP/N21

7/16 * VGP/N16 + 9/16 * VGP/N17  V233

7/8 * VGP/N21 + 1/8 * VGP/N22

6/16 * VGP/N16 + 10/16 * VGP/N17  V234

6/8 * VGP/N21 + 2/8 * VGP/N22

5/16 * VGP/N16 + 11/16 * VGP/N17  V235

5/8 * VGP/N21 + 3/8 * VGP/N22

4/16 * VGP/N16 + 12/16 * VGP/N17  V236

4/8 * VGP/N21 + 4/8 * VGP/N22

3/16 * VGP/N16 + 13/16 * VGP/N17  V237

3/8 * VGP/N21 + 5/8 * VGP/N22

2/16 * VGP/N16 + 14/16 * VGP/N17  V238

2/8 * VGP/N21 + 6/8 * VGP/N22

1/16 * VGP/N16 + 15/16 * VGP/N17  V239

1/8 * VGP/N21 + 7/8 * VGP/N22

VGP/N17

V240

VGP/N22

15/16 * VGP/N17 + 1/16 * VGP/N18 V241

3/4 * VGP/N22 + 1/4* VGP/N23

14/16 * VGP/N17 + 2/16 * VGP/N18  V242

2/4 * VGP/N22 + 2/4* VGP/N23

13/16 * VGP/N17 + 3/16 * VGP/N18  V243

1/4 * VGP/N22 + 3/4* VGP/N23

12/16 * VGP/N17 + 4/16 * VGP/N18  V244

VGP/N23

11/16 * VGP/N17 + 5/16 * VGP/N18  V245

3/4 * VGP/N23 + 1/4* VGP/N24

10/16 * VGP/N17 + 6/16 * VGP/N18  V246

2/4 * VGP/N23 + 2/4* VGP/N24

9/16 * VGP/N17 + 7/16 * VGP/N18  V247

1/4 * VGP/N23 + 3/4* VGP/N24

8/16 * VGP/N17 + 8/16 * VGP/N18  V248

VGP/N24

7/16 * VGP/N17 + 9/16 * VGP/N18  V249

1/2 * VGP/N24 + 1/2* VGP/N25

6/16 * VGP/N17 + 10/16 * VGP/N18  V250

VGP/N25

5/16 * VGP/N17 + 11/16 * VGP/N18  V251

1/2 * VGP/N25 + 1/2* VGP/N26

4/16 * VGP/N17 + 12/16 * VGP/N18  V252

VGP/N26

3/16 * VGP/N17 + 13/16 * VGP/N18  V253

1/2 * VGP/N26 + 1/2* VGP/N27

2/16 * VGP/N17 + 14/16 * VGP/N18  V254

1/16 * VGP/N17 + 15/16 * VGP/N18  V255

VGP/N27

VGP/N28

V0.22                                                                          Page  102  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006
                                                                               ST7102

5.9

Reset function

The  Reset  function  of  ST7102  is triggered  by  RESX  input.  After  reset function  is  triggered,  the  ST7102 is into  a  reset

period,  and  the  duration  of  this  period  must  be  at  least  1ms.  During  this  period,  the  ST7102  and  its  power  circuit  is

initialized.

Initial State Of Output Pins

Output Pins Name

S[480:1] (source output)

VGH

VGL

VCOM

CGOUTL_R[16:1](GIP signal)

Initial State

GND

GND

GND

GND

GND

5.9.1

Reset Timing Diagram

  ST7102 provides Power On Reset and HWRST pin (RESX) for IC initialization and exit Deep Standby Mode (DSTB). For

exiting DSTB Mode, HWRST pin should be tied to high at least 10ms. The timing diagram is located at below.

5.9.1.1

Power On Reset & HWRST Reset

Note: RESX should be tied from low to high when exiting DSTB Mode.

Figure 67 Power on Reset and HWRST reset

V0.22                                                                          Page  103  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

RESXDisplaystatusIOVCCDPHYVCCInitial conditionMore than 120msCommand settingPower onHWRST ResetAVDDAVEE≥0ms≥0msMore than 10ms Wait for power stable深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006

                                                                               ST7102

5.10  Abnormal Power off Function

ST7102 provides Power Drop detection. If external power drops lower than circuit detection voltage, then the system will

enter into Sleep In Mode.

5.10.1

Abnormal Power Off

    Abnormal Power Off circuit can detect external voltage source IOVCC, AVDD, AVEE, if one of them is below detection

voltage, then the system will be into Sleep In Mode. The following schematic is to show how the detection circuit produces

abnormal signal.

Figure 68 Abnormal Off Function Block Diagram

Note1: Abnormal function is working only in Sleep Out Mode

Figure 69 Abnormal Off Sequence Diagram

V0.22                                                                          Page  104  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

AVDD DetectVTH, AVDDAVDDAVDDabnor5us Filter outVTH, AVEEAVEE5us Filter outAVEEabnorVTH, IOVCCIOVCCIOVCCabnorVTH, AVDDAVDDAVDDabnorAVEE DetectVTH, AVEEAVEEAVEEabnorIOVCC DetectVTH, IOVCCIOVCCIOVCCabnorDigitalAbnormal Off Sequence5us Filter outIOVCC/AVDD/AVEEDisplaystatusSleep Out ModeAbnormal signalSleep In ModeAbnormal Sequence ~10ms深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006

                                                                               ST7102

5.11  Basic Operation Mode

The basic operation mode of ST7102 is illustrated below. When changing from one mode to another, make sure to follow

the sequence indicated in below figure.

Figure 70 Basic Operation Mode

V0.22                                                                          Page  105  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

 Power OnHW Reset from low to highSleep in modeSleep Out CommandDisplaySleep In CommandSleep out SequenceSleep in SequenceDeep Standby CmdReset=LSleep Out ModeAbnormal Power off Sequence About 10msAbnormal Power Off Detection(IOVCC/AVDD/AVEE)YESNODeep Standby Mode深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006
                                                                               ST7102

5.12  Power On/Off Sequence

The power On/Off sequence is illustrated below.

Figure 71 The power On/Off sequence block diagram

V0.22                                                                          Page  106  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

Power supply onHardware RestSLPOUT CMD (11h)DISPON Command    (set DISPON (29h))Normal Display120msPower on sequencePower off sequence Normal Display onDISPOFF Command    (set DISPOFF (28h))SLPIN Command    (set SLPIN (10h))Power supply off120ms100ms or more深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006
                                                                               ST7102

5.12.1

Power On/Off Timing

The power On/Off timing diagram is illustrated below.

Figure 72 The power on sequence timing

Note 1: MIPI lanes must go to LP11 after Power IOVCC/DPHYVCC is ready

Note 2: After SLPOUT command, driver IC will start internal power on action. Any other settings should be set after SLPOUT command

with a minimum of 120mS.

Note 3: DISPOFF and SLPIN command should be set after SLPOUT command with a minimum delay time of 120mS.

Note 4: RESX tied low into deep standby mode with a minimum time of 10ms.

V0.22                                                                          Page  107  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

IOVCC/DPHYVCCAVDDAVEERESXMIPI Command Transfer>0msLP-11 State10%90%10%90%10%>0ms>120ms90%SLPOUT command>120ms>10msWait power stableDISPON commandIOVCC/DPHYVCCAVDDAVEERESX10%90%90%10%10%90%MIPI Command TransferDISPOFF + SLPIN command>0msVIHVIL>0msVIHVIL>10ms>33ms(Min. 2 frames)深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006

                                                                               ST7102

5.12.2

Power Ramp Up/Down Specifications

The power ramp up/down specifications are illustrate below

Figure 73 The power ramp up/down timing

Item

Symbol  Unit

Min.

Max.

System power (IOVCC) rise time (10% to 90%)

TRSP

ms

System power (IOVCC) on to AVDD on time

TDEPON  ms

AVDD-AVEE on delay time (10% to 10%)

AVDD-AVEE on delay time (90% to 90%)

AVEE-AVDD off delay time (10% to 10%)

AVEE-AVDD off delay time (90% to 90%)

TDON1

TDON2

TDOFF1

TDOFF2

ms

ms

ms

ms

AVDD off to system power off time

TDSPOFF  ms

-

0

0

0

0

0

0

Power down time

TPD

ms

10

2

-

-

-

-

-

-

V0.22                                                                          Page  108  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

IOVCC/DPHYVCCAVDDAVEETRSPTDEPON10%90%10%90%10%90%TDON1TDON2TDSPOFF10%90%90%10%10%90%TDOFF1TDOFF2PDT深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006

                                                                               ST7102

5.13

Instruction Setting Sequence

5.13.1

Sleep Enter/Exit Sequences

When setting instruction to the ST7102, the sequence shown in below figure must be followed to complete the instruction

setting.

Figure 74 Sleep Enter/Exit Setting Sequence

V0.22                                                                          Page  109  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

Display Off SequenceSleep In Command (10h)Enter Sleep In ModeSleep Out Command (11h)Exit Sleep In ModeWait >120msRelaod NVM and Power on SequenceDisplay On Sequence深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006

                                                                               ST7102

5.13.2

Deep Standby Mode Enter/Exit Sequences

        When setting instruction to the ST7102, the sequence shown in below figure must be followed to complete the

instruction setting.

Figure 75 Deep Standby Mode Enter/Exit Setting Sequence

V0.22                                                                          Page  110  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

Display Off Command (28h)Enter Display ModeSleep In Command (10h)Enter Sleep In ModeDeep Standby Command(Command=4Fh, Parameter=01h)Enter Deep Standby ModeNote: For MIPI IF, if deep standby mode used, pull HSSI_CLK & HSSI_D0~D3 PIN to VSS after executing deep standby commandExit Deep Standby Command(Set RESX Pin low pulse>1ms then rise to high)Note: After exiting deep standby mode, please enables HSSI_CLK &HSSI_D0~D3 Pin to 1.2VInitial Command SettingDisplay On SequenceWaiting time>100ms深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006
                                                                               ST7102

5.14  Touch interface protocol

ST7102 supports SPI and I2C interface, which allows full-duplex, synchronous, serial communications with host

controllers. ST7102 SPI equips four serial signals, SS, SCK, MISO and MOSI.

5.14.1

SPI interface

ST7102 operates as a SPI slave device and data length is 16-bit. TP_SPI_SS, TP_SPI_SCK and TP_SPI_MOSI are

Schmitt trigger inputs. The data on TP_SPI_MOSI is latched on rising edge of TP_SPI_SCK and the data on

TP_SPI_MISO output on falling edge of TP_SPI_SCK. The maximum SPI clock rate is 16 MHz.

The information transmitted by SPI can be divided into command packet and data packet. Command packet can

access register and data packet can access internal RAM. The most significant bit, called ID bit, after TP_SPI_SS falling

edge is used to identify command or data packet. And the following bit, called R/W bit, is used to identify write or read

operation.

5.14.1.1

Command Protocol

In command protocol, ID bit of 1st word after TP_SPI_SS falling edge is “0” to indicate register access. R/W bit is

“0” to indicate write operation and “1” to indicate read operation. Bit13 to bit0 of 1st word, called ADD bits, are address of

register to be accessed. If at write operation, the 2nd word on TP_SPI_MOSI will update the register which is addressed

V0.22                                                                          Page  111  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

HostTDDIIOVCCIOVCCIOVCCTP_SPI_SCKTP_SPI_MOSITP_INTSCKSDAMointorSDOTP_SPI_MISOCSTP_SPI_SS深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006

                                                                               ST7102

by ADD and afterward ADD will increase one automatically. If at read operation, the 2nd word on TP_SPI_MOSI is dummy

word (value doesn’t care). The 3rd word on TP_SPI_MISO will be outputted from register addressed by ADD and similarly

ADD is increment afterwards. Keep writing and reading words can access the consecutive registers during one SPI

transmission. If accessing non-consecutive register is happened, SS should return to “1” to end current transmission and

start another SPI command protocol to assign new register address.

Host write register command

Host read register command

5.14.1.2

Data Protocol

In data protocol, ID bit of 1st word after SS falling edge is “1” to indicate RAM access. R/W bit is “0” to indicate write

operation and “1” to indicate read operation. Bit13 to bit0 of 1st word, called ADD bits, are address of RAM to be accessed.

If at write operation, the 2nd word on MOSI will update RAM which is addressed by ADD and afterward ADD will increase

one automatically. If at read operation, the 2nd word on MISO is 16-bit header. The 3rd word on MISO will be outputted

from RAM addressed by ADD and similarly ADD is increment afterwards. Keep writing and reading words can access the

consecutive RAM addresses during one SPI transmission. If accessing non-consecutive RAM address is happened, SS

should return to “1” to end current transmission and start another SPI data protocol to assign new RAM address.

V0.22                                                                          Page  112  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

b15SSSCKMOSIMISO0b140MSBRegister AddressLSBb0b1. . .. . .Don`t Careb15b14Register Value (of Address+0)b0b1. . .. . .Don`t Careb15b14Register Value (of Address+1)b0b1. . .. . .Don`t Careb15b14Register Value (of Address+2)b0b1. . .. . .Don`t Careb15b14Register Value (of Address+N)b0b1. . .. . .Don`t Careb15SSSCKMOSIMISO0b141MSBRegister AddressLSBb0b1. . .. . .Don`t CareDon`t Careb15b14b0b1. . .. . .Register Value (of Address+0)Register Value (of Address+1)b15b14b0b1. . .. . .b15b14b0b1. . .. . .Register Value (of Address+N)Don`t CareDon`t CareDon`t CareDon`t Careb15SSSCKMOSIMISO1b140MSBRAM AddressLSBb0b1. . .. . .Don`t Careb15b14RAM Data (of Address+0)b0b1. . .. . .Don`t Careb15b14RAM Data (of Address+1)b0b1. . .. . .Don`t Careb15b14RAM Data (of Address+2)b0b1. . .. . .Don`t Careb15b14RAM Data (of Address+N)b0b1. . .. . .Don`t Care深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006

                                                                               ST7102

Host write RAM data

Host read RAM data

V0.22                                                                          Page  113  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

b15SSSCKMOSIMOSO1b141MSBRegister AddressLSBb0b1. . .. . .Don`t Careb15b14b0b1. . .. . .Data Header 1b15b14b0b1. . .. . .b15b14b0b1. . .. . .RAM Value (of Address+N)Don`t CareDon`t CareDon`t CareDon`t CareData Header 0b15b14b0b1. . .. . .RAM Value (of Address+0)Data Header 2b15b14b0b1. . .. . .Don`t Care深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006

                                                                               ST7102

5.14.2

I2C

ST7102 protocol supports operating speeds up to 400 kb/s with 7-bit addressing and 8-bit data bytes. The

TP_I2C_SCK, TP_I2C_SDA, and TP_TSIX pins are typically used in an I2C interface.

The values of the pull-up resistors should be chosen to ensure that the rise times of the TP_I2C_SDA and

TP_I2C_SCK signals are within the limits set by the I2C specification. These values depend on what other devices, if any,

are on the I2C bus. Typical values fall within the range of 2 kΩ to 10 kΩ.

⚫

I2C Pin Definition

Name

I/O

Description

IOVCC

SCK

SDA

IRQ

I

I

Power supply for I/O system

I2C clock pin

I/O

I2C data pin

O  Data ready interrupt pin

Connect Pin

IOVCC

TP_I2C_SCK

TP_I2C_SDA

TP_INT

⚫

I2C clock timing

When a transaction contains the slave address and R/W bit (start condition), the sensor can hold SCL low and

checks that the slave address matches. If the slave address fails to match, the sensor no longer clock stretches on

subsequent byte transmission until it detects the next start condition. When the slave address matches, the sensor

acknowledges and can continue to clock stretch at the end of subsequent bytes within the same transaction.

V0.22                                                                          Page  114  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

HostTDDIIOVCCIOVCCIOVCCTP_I2C_SCKTP_I2C_SDATP_INTSCKSDAMointor深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006

                                                                               ST7102

5.14.3

IRQ

ST7102 provides an interrupt pin(TP_TSIX) that is asserted to indicate that new data is available for reading by the

host. The TP_TSIX signal is intended to be used as interrupt source to a host. IRQ functionality is added by MCU firmware.

The IRQ to host connection as below.

The TP_INT pin behavior

V0.22                                                                          Page  115  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

A6A5A4A3A2A1A0R/WSTARTD6D5D4D3D2D1D0D7ACKACKTP_I2C_SDATP_I2C_SCKSTOPTouch PanelTouch ICHostSPI/ I2CTP_INT  Host send read cmdSlave output Coordinate dataTP_INTMOSI/MISONew data is readyDe-assert IRQ 深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006

                                                                               ST7102

6  COMMAND DESCRIPTION

6.1

User Command Set (UCS) List

Command

(Hex)

Write/Read

NVM

/Command

option

NOP

SWRESET

RDDID

RDNUMED

RDDST

RDDPM

RDDMADCTL

RDDIM

RDDSM

RDDSDR

SLPIN

SLPOUT

NORON

INVOFF

INVON

ALLPOFF

ALLPON

GAMSEL

DISPOFF

DISPON

TEOFF

TEEON

MADCTL

IDMOFF

IDMON

TESLWR

TESLRD

DSTB

WRDISBV

RDDISBV

WRCTRLD

00

01

04

05

09

0A

0B

0D

0E

0F

10

11

13

20

21

22

23

26

28

29

34

35

36

38

39

44

45

4F

51

52

53

C

C

R

R

R

R

R

R

R

R

C

C

C

C

C

C

C

C

C

C

C

W

W

W

W

W

R

W

W

R

W

Function

No Operation

Software Reset

Read Display Identification Information

Read Number of Errors on DSI

Read Display Status

Read Display Power Mode

Read Display MADCTL

Read Display Image Mode

Read Display Signal Mode

Read Display Self-Diagnostic Result

Enter Sleep In mode

Enter Sleep Out mode

Enter Normal Display mode

Display Inversion Off

Display Inversion On

All pixel off

All pixel on

Gamma curve select

Display off

Display on

Tearing Effect Line Off

Tearing Effect Line On

No

No

No

No

No

No

No

No

No

No

No

No

No

No

No

No

No

No

No

No

No

No

Yes

Memory Access Direction Control

No

No

No

No

No

No

No

No

Idle Mode OFF

Idle Mode ON

Write TE Scan Line

Read Scan Line

Deep standby mode on

Write Display Brightness

Read Display Brightness Value

Write CTRL Display

Parameter

Number

MIPI Transmission

0

0

3

1

4

1

1

1

1

1

0

0

0

0

0

0

0

1

0

0

0

1

1

0

0

2

2

1

1

1

1

LPDT/HSDT

LPDT/HSDT

LPDT/HSDT

LPDT/HSDT

LPDT/HSDT

LPDT/HSDT

LPDT/HSDT

LPDT/HSDT

LPDT/HSDT

LPDT/HSDT

LPDT/HSDT

LPDT/HSDT

LPDT/HSDT

LPDT/HSDT

LPDT/HSDT

LPDT/HSDT

LPDT/HSDT

LPDT/HSDT

LPDT/HSDT

LPDT/HSDT

LPDT/HSDT

LPDT/HSDT

LPDT/HSDT

LPDT/HSDT

LPDT/HSDT

LPDT/HSDT

LPDT/HSDT

LPDT/HSDT

LPDT/HSDT

LPDT/HSDT

LPDT/HSDT

V0.22                                                                          Page  116  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006                                                                               ST7102

RDCTRLD

WRCABC

RDCABC

WRCABCMB

RDCABCMB

RDID1

RDID2

RDID3

RDDDBS

RDDDBC

RDFCS

RDCCS

RDICCD

MIPIEXTFMAT

54

55

56

5E

5F

DA

DB

DC

A1

A8

AA

AF

F4

F9

R

W

R

W

R

R

R

R

R

R

R

R

R

No

No

No

No

No

Yes

Yes

Yes

No

Yes

No

No

No

Read CTRL Display

Write Content Adaptive Brightness Control

Read Content Adaptive Brightness Control

Write CABC Minimum Brightness

Read CABC Minimum Brightness

Read ID1

Read ID2

Read ID3

Read DDB Start

Read DDB Continue

Read First Checksum

Read Continue Checksum

Read Sitronix IC ID

W/R

YES

MIPI Extension Format

Note: LPDT (Low Power Mode), HSDT (High Speed Mode)

1

1

1

1

1

1

1

1

8

8

1

1

2

1

LPDT/HSDT

LPDT/HSDT

LPDT/HSDT

LPDT/HSDT

LPDT/HSDT

LPDT/HSDT

LPDT/HSDT

LPDT/HSDT

LPDT/HSDT

LPDT/HSDT

LPDT/HSDT

LPDT/HSDT

LPDT/HSDT

LPDT/HSDT

V0.22                                                                          Page  117  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006
                                                                               ST7102

6.2

User Command Set (UCS) DESCRIPTION

6.2.1

NOP (00H) : No Operation

00H

NOP (No Operation)

Inst / Para

Write/Read

NOP

Write

D7

0

D6

0

D5

0

D4

0

D3

0

D3

0

D1

0

D0

(Default)

0

(00H)

Parameter

No Parameter

-

Description

- This command is empty command. It does not have effect on the display module.

Restriction

-

Register

Availability

Status

Sleep Out, Display On

Sleep In

Availability

Yes

Yes

V0.22                                                                          Page  118  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006

                                                                               ST7102

6.2.2

SWRESET (01H): Software Reset

01H

SWRESET (Software Reset)

Inst / Para

Write/Read

SWRESET

Write

D7

0

D6

0

D5

0

D4

0

D3

0

D3

0

D1

0

D0

(Default)

1

(01H)

Parameter

Description

- When the Software Reset command is written, it causes a software reset. It resets the commands and parameters to their S/W

Reset default values and all source & gate outputs are set to VSS (display off).

No Parameter

-

Restriction

-

Register

Availability

Status

Sleep Out, Display On

Sleep In

Availability

Yes

Yes

V0.22                                                                          Page  119  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006

                                                                               ST7102

6.2.3

RDDID (04H) Read Display Identification Information

04H

RDDID (Read Display Identification Information)

Inst / Para

Write/Read

RDDID

1st Parameter

2nd Parameter

3rd Parameter

Write

Read

Read

Read

D7

0

D6

0

D5

0

D4

0

D3

0

D3

1

D1

0

D0

(Default)

0

(04H)

ID1[7:0]

ID2[7:0]

ID3[7:0]

FFh

FFh

FFh

- Read Display Identification Information

Description

-ID1 : LCD module's manufacturer ID (FFh: not programmed)

-ID2 : LCD module/driver version ID (FFh: not programmed)

-ID3 : LCD module/driver ID (FFh: not programmed)

Restriction

-

Register

Availability

Status

Sleep Out, Display On

Sleep In

Availability

Yes

Yes

V0.22                                                                          Page  120  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006
                                                                               ST7102

6.2.4

RDNUMED (05H) Read Number of Errors on DSI

05H

RDNUMED

Inst / Para

Write/Read

RDNUMED

1st Parameter

Write

Read

D7

0

D6

0

D5

0

D4

0

D3

0

D3

1

D1

0

D0

(Default)

1

(05H)

DSI_NUMBER[7:0]

00h

Description

- Read Number of Errors on DSI, DSI_NUMBER[7:0] is a number of the errors on DSI.

Restriction

-

Register

Availability

Status

Sleep Out, Display On

Sleep In

Availability

Yes

Yes

V0.22                                                                          Page  121  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006

                                                                               ST7102

6.2.5

RDDST (09H) Read Display Status

09H

RDNUMED(Read Display Status)

Inst / Para

Write/Read

RDDST

Write

D7

0

1st Parameter

Read

Booster_on

2nd Parameter

Read

3rd Parameter

Read

LA

0

0

0

0

0

D6

D5

D4

D3

0

0

0

0

0

0

D2

0

BGR

D1

D0

(Default)

0

0

1

CA

(09H)

00h

1

0

IDMON

0

SLPOUT  NORON

00h

INVON  ALLPON  ALLPOFF  DSPON

TEON

GAMM_SEL[2]

00h

00h

4th Parameter

Read

GAMMA_SEL[1]  GAMMA_SEL[0]  TEMOD

0

0

0

0

EOD

- This command indicates the current status of the display as described in the table below :

Bit

Description

Value

Booster_on

Booster status

‘1’ = Booster on.

‘0’ = Booster off

CA

LA

Column Address Order (CA)

‘1’ = Decrement, (Right to Left, when MADCTL (36h) CA=’1’)

‘0’ = Increment, (Left to Right, when MADCTL (36h) CA=’0’)

Row Address Order (LA)

‘1’ = Row/column exchange, (when MADCTL (36h) LA=’1’)

BGR

RGB/ BGR Order

IDMON

Idle Mode On/Off

SLPOUT

Sleep In/Out Mode

Description

NORON

Normal mode

DSPON

Display On/Off Mode

INVON

Inversion On/Off Mode

ALLPON

All pixel on

ALLPOFF

All pixel off

TEON

Tearing effect line on/off

‘0’ = Normal, (when MADCTL (36h) LA=’0’

‘1’ = BGR Order, (When MADCTL (36h) BGR=’1’)

‘0’ = RGB Order, (When MADCTL (36h) BGR=’0’)

‘1’ = Idle Mode On,

‘0’ = Idle Mode Off

‘1’ = Sleep Out Mode

‘0’ = Sleep In Mode

‘1’ = Normal Mode on

‘0’ = Normal Mode off

‘1’ = Display On Mode

‘0’ = Display Off Mode

‘1’ = Inversion On Mode

‘0’ = Inversion Off Mode

‘1’ = All pixel on

‘0’ = Normal mode

‘1’ = All pixel off

‘0’ = Normal mode

‘1’ = Inversion On Mode

‘0’ = Inversion Off Mode

GAMMA_SEL[2:0]  Gamma curve selection

‘000’ = Gamma curve 1(gamma 2.2)

‘others’ = reserved

V0.22                                                                          Page  122  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006                                                                               ST7102

TEMOD

Tearing effect line mode

‘1’ = V- blanking and H-blanking

EOD

Error on DSI

‘0’ = V-blanking only

‘1’ = Error

‘0’ = No Error

Restriction

-

Register

Availability

Status

Sleep Out, Display On

Sleep In

Availability

Yes

Yes

V0.22                                                                          Page  123  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006

                                                                               ST7102

6.2.6

RDDPM (0AH): Read Display Power Mode

0AH

RDDPM (Read Display Power Mode)

Inst / Para

Write/Read

RDDPM

Write

D7

0

D6

0

1st Parameter

Read

Booster_on

IDMON

D5

0

0

D4

0

D3

1

D3

0

SLPOUT  NORON

DSPON

D1

D0

(Default)

1

0

0

0

(0AH)

00h

- This command indicates the current status of the display as described in the table below:

Bit

Description

Value

Booster_on

Booster status

IDMON

Idle Mode On/Off

Description

SLPOUT

Sleep In/Out Mode

NORON

Normal mode

DSPON

Display On/Off Mode

‘1’ = Booster on.

‘0’ = Booster off

‘1’ = Idle Mode On,

‘0’ = Idle Mode Off

‘1’ = Sleep Out Mode

‘0’ = Sleep In Mode

‘1’ = Normal Mode on

‘0’ = Normal Mode off

‘1’ = Display On Mode

‘0’ = Display Off Mode

Restriction

-

Register

Availability

Status

Sleep Out, Display On

Sleep In

Availability

Yes

Yes

V0.22                                                                          Page  124  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006

                                                                               ST7102

6.2.7

RDDMADCTR (0BH): Read Display MADCTR

0BH

RDDMADCTR (Read Display MADCTR)

Inst / Para

Write/Read

D7

D6

D5

D4

RDDMADCTR

Write

1st Parameter

Read

0

0

0

0

0

0

0

0

D3

1

BGR

D3

0

0

D1

1

CA

D0

(Default)

1

LA

(0BH)

00h

- This command indicates the current status of the display as described in the table below:

Bit

Description

Value

CA

Column Address Order (CA)

‘1’ = Decrement, (Right to Left, when MADCTL (36h) CA=’1’)

‘0’ = Increment, (Left to Right, when MADCTL (36h) CA=’0’)

Description

‘1’ = Row/column exchange, (when MADCTL (36h) LA=’1’)

LA

Row Address Order (LA)

BGR

RGB/ BGR Order

‘0’ = Normal, (when MADCTL (36h) LA=’0’

‘1’ = BGR Order, (When MADCTL (36h) BGR=’1’)

‘0’ = RGB Order, (When MADCTL (36h) BGR=’0’)

Restriction

Register

Availability

Status

Sleep Out, Display On

Sleep In

-

Availability

Yes

Yes

V0.22                                                                          Page  125  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006

                                                                               ST7102

6.2.8

RDDCOLM (0CH): Read Display Color Mode

0DH

RDDIM (Read Display Image Mode)

Inst / Para

Write/Read

D7

D6

D5

D4

D3

RDDIM

1st Parameter

Write

Read

0

0

0

0

0

0

0

0

1

0

D3

1

D1

0

D0

(Default)

1

(0CH)

DBI[2:0]

00h

- This command indicates the current status of the display as described in the table.

DBI[2:0]

Color Format

Description

0h~4h

Reserved

5h

6h

7h

16-bit/pixel

18-bit/pixel

24-bit/pixel

  Note: For DBI[2:0] definition refer to interface Pixel Format (3Ah)

Restriction

-

Register

Availability

Status

Sleep Out, Display On

Sleep In

Availability

Yes

Yes

V0.22                                                                          Page  126  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006

                                                                               ST7102

6.2.9

RDDIM (0DH): Read Display Image Mode

0DH

RDDIM (Read Display Image Mode)

Inst / Para

Write/Read

D7

D6

RDDIM

1st Parameter

Write

Read

0

0

0

0

D5

0

D4

0

D3

1

D3

1

D1

0

D0

(Default)

1

(0DH)

INVON

ALLPON  ALLPOFF

GAMMA_SEL[2:0]

00h

- This command indicates the current status of the display as described in the table below:

Bit

Description

Value

INVON

Inversion On/Off Mode

Description

ALLPON

All pixel on

ALLPOFF

All pixel off

‘1’ = Inversion On Mode

‘0’ = Inversion Off Mode

‘1’ = All pixel on

‘0’ = Normal display

‘1’ = All pixel off

‘0’ = Normal display

GAMMA_SEL[2:0]

Gamma curve selection

‘000’ = Gamma curve 1(gamma 2.2)

‘others’ = Reserved

Restriction

-

Register

Availability

Status

Sleep Out, Display On

Sleep In

Availability

Yes

Yes

V0.22                                                                          Page  127  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006

                                                                               ST7102

6.2.10

RDDSM (0EH): Read Display Signal Mode

0EH

RDDSM (Read Display Signal Mode)

Inst / Para

Write/Read

RDDSM

Write

D7

0

D6

0

1st Parameter

Read

TEON

TEM

D5

D4

D3

D3

D1

D0

(Default)

0

0

0

0

1

0

1

0

0

0

1

(0EH)

EOD

00h

- This command indicates the current status of the display as described in the table below:

Bit

Description

Value

TEON

Tearing effect line on/off

‘1’ = Inversion On Mode

‘0’ = Inversion Off Mode

Description

‘1’ = V- blanking and H-blanking

TEM

Tearing effect line mode

EOD

Error on DSI

‘0’ = V-blanking only

‘1’ = Error

‘0’ = No Error

Restriction

-

Register

Availability

Status

Sleep Out, Display On

Sleep In

Availability

Yes

Yes

V0.22                                                                          Page  128  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006

                                                                               ST7102

6.2.11

RDDSDR(0FH): Read Display Self-Diagnostic Result

0FH

RDDSM (Read Display Signal Mode)

Inst / Para

Write/Read

RDDSDR

Write

D7

0

1st Parameter

Read

RLD

D6

0

FD

D5

D4

D3

D3

D1

D0

(Default)

0

0

0

0

1

0

1

0

1

0

1

(0FH)

CCR

00h

- This command indicates the current status of the display as described in the below:

-RLD: This bit is the Register Loading Detection

-FD: This bit is the Functionality Detection

Description

-CCR: The checksum compare result.

            0 = Checksum is the same.

            1 = Checksum is not the same

Restriction

-

Register

Availability

Status

Sleep Out, Display On

Sleep In

Availability

Yes

Yes

V0.22                                                                          Page  129  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006

                                                                               ST7102

6.2.12

SLPIN (10H): Sleep In

10H

SLPIN (Sleep In)

Inst / Para

Write/Read

SLPIN

Write

D7

0

D6

0

D5

0

D4

1

D3

0

D3

0

D1

0

D0

(Default)

0

(10H)

Parameter

No Parameter

-

- This command causes the LCD module to enter the minimum power consumption mode.

- In this mode the DC/DC converter is stopped, Internal display oscillator is stopped, and panel scanning is stopped.

Description

- This command has no effect when module is already in sleep in mode. Sleep In Mode can only be exit by the Sleep Out

Command (11H).

- It will be necessary to wait 5ms before sending next command; this is to allow time for the supply voltages and clock circuits to

Restriction

stabilize.

- It will be necessary to wait 120ms after sending Sleep Out command (when in Sleep In Mode) before Sleep In command can

be sent.

Register

Availability

Status

Sleep Out, Display On

Sleep In

Availability

Yes

No

V0.22                                                                          Page  130  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006

                                                                               ST7102

6.2.13

SLPOUT (11H): Sleep Out

11H

SLPOUT (Sleep Out)

Inst / Para

Write/Read

SLPOUT

Write

D7

0

D6

0

D5

0

D4

1

D3

0

D3

0

D1

0

D0

(Default)

1

(11H)

Parameter

No Parameter

-

- This command turns off sleep mode.

- In this mode the DC/DC converter is enabled, Internal display oscillator is started, and panel scanning is started.

Description

- This command has no effect when module is already in sleep out mode. Sleep Out Mode can only be exit by the Sleep In

Command (10H).

- It will be necessary to wait 5ms before sending next command; this is to allow time for the supply voltages and clock circuits to

stabilize.

Restriction

- DRIVER loads all default values of extended and test command to the registers during this 5msec and there cannot be any

abnormal visual effect on the display image if those default and register values are same when this load is done and when the

DRIVER is already Sleep Out mode.

- It will be necessary to wait 120ms after sending Sleep In command (when in Sleep Out mode) before Sleep Out command can

be sent

Register

Availability

Status

Sleep Out, Display On

Sleep In

Availability

No

Yes

V0.22                                                                          Page  131  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006

                                                                               ST7102

6.2.14

NORON (13H): Normal display mode On

13H

NORON (Normal Display Mode On )

Inst / Para

Write/Read

INVOFF

Write

D7

0

D6

0

D5

0

D4

1

D3

0

D3

0

D1

1

D0

(Default)

1

(13H)

Parameter

No Parameter

-

Description

Restriction

Register

Availability

- This command is used to return display to normal display mode.

-.This command can exit all pixel on/off mode to normal display mode.

Status

Sleep Out, Display On

Sleep In

Availability

Yes

Yes

V0.22                                                                          Page  132  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006

                                                                               ST7102

6.2.15

INVOFF (20H) : Display Inversion Off

20H

INVOFF (Display Inversion Off )

Inst / Para

Write/Read

INVOFF

Write

D7

0

D6

0

D5

1

D4

0

D3

0

D3

0

D1

0

D0

(Default)

0

(20H)

Parameter

No Parameter

-

- This command is used to recover from display inversion mode.

- This command makes no change of contents of frame memory.

- This command does not change any other status.

Description

Restriction

- This command has no effect when module is already in inversion off mode.

.

Register

Availability

Status

Sleep Out, Display On

Sleep In

Availability

Yes

Yes

V0.22                                                                          Page  133  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006

                                                                               ST7102

6.2.16

INVON (21H) : Display Inversion On

21H

INVON (Display Inversion On )

Inst / Para

Write/Read

INVON

Write

D7

0

D6

0

D5

1

D4

0

D3

0

D3

0

D1

0

D0

(Default)

1

(21H)

Parameter

No Parameter

-

- This command is used to enter into display inversion mode.

- This command makes no change of contents of frame memory. Every bit is inverted from the frame memory to the display.

- This command does not change any other status.

Description

Restriction

- This command has no effect when module is already in inversion off mode.

.

Register

Availability

Status

Sleep Out, Display On

Sleep In

Availability

Yes

Yes

V0.22                                                                          Page  134  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006

                                                                               ST7102

6.2.17

ALLPOFF (22H): All pixel off

22H

ALLPOFF (All pixel off)

Inst / Para

Write/Read

ALLPOFF

Write

D7

0

D6

0

D5

1

D4

0

D3

0

D3

0

D1

1

D0

(Default)

0

(22H)

Parameter

No Parameter

-

- This command is used to black display In sleep out mode.

Description

- Exit from this command by All pixel on(23H) or Normal display mode(13H)

Restriction

Register

Availability

Status

Sleep Out, Display On

Sleep In

Availability

Yes

No

V0.22                                                                          Page  135  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006

                                                                               ST7102

6.2.18

ALLPON (23H): All pixel on

23H

ALLPOFF (All pixel off)

Inst / Para

Write/Read

ALLPON

Write

D7

0

D6

0

D5

1

D4

0

D3

0

D3

0

D1

1

D0

(Default)

1

(23H)

Parameter

No Parameter

-

- This command is used to white display In sleep out mode.

Description

- Exit from this command by All pixel off(22H) or Normal display mode(13H)

Restriction

Register

Availability

Status

Sleep Out, Display On

Sleep In

Availability

Yes

No

V0.22                                                                          Page  136  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006

                                                                               ST7102

6.2.19  GAMSEL (26H): Gamma Curve Select

26H

  (Gamma Curve Select)

Inst / Para

Write/Read

D7

D6

D5

D4

GAMSEL

Parameter

Write

Write

0

0

0

0

1

0

0

0

D3

0

D3

1

D1

1

D0

(Default)

0

(26H)

GC[3:0]

- This command is used to select gamma curve, and only 1 curve (gamma 2.2) can be selected.

Description

Restriction

Register

Availability

GC[3:0]

1

others

Gamma Curve

Gamma 2.2

Reserved

Status

Sleep Out, Display On

Sleep In

Availability

Yes

No

V0.22                                                                          Page  137  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006

                                                                               ST7102

6.2.20

DISPOFF (28H): Display Off

28H

DISPOFF (Display Off)

Inst / Para

Write/Read

DISPOFF

Write

D7

0

D6

0

D5

1

D4

0

D3

1

D3

0

D1

0

D0

(Default)

0

(28H)

Parameter

No Parameter

-

- This command is used to enter into DISPLAY OFF mode. In this mode.

- This command makes no change of contents of frame memory.

- This command does not change any other status.

- There will be no abnormal visible effect on the display.

- Exit from this command by Display On (29H)

Description

Restriction

-This command has no effect when module is already in Display Off mode.

Register

Availability

Status

Sleep Out, Display On

Sleep In

Availability

Yes

No

6.2.21

DISPON (29H): Display On

29H

DISPON (Display On)

Inst / Para

Write/Read

DISPON

Write

D7

0

D6

0

D5

1

D4

0

D3

1

D3

0

D1

0

D0

(Default)

1

(29H)

Parameter

No Parameter

-

- This command is used to recover from DISPLAY OFF mode.

Description

- This command makes no change of contents of frame memory.

- This command does not change any other status.

V0.22                                                                          Page  138  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006

                                                                               ST7102

Restriction

-This command has no effect when module is already in Display Off mode.

Register

Availability

Status

Sleep Out, Display On

Sleep In

Availability

No

Yes

V0.22                                                                          Page  139  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006

                                                                               ST7102

6.2.22

TEOFF (34H): Tearing Effect Line OFF

34H

TEOFF (Tearing Effect Line OFF)

Inst / Para

Write/Read

TEOFF

Write

D7

0

D6

0

D5

1

D4

1

D3

0

D3

1

D1

0

D0

(Default)

0

(34H)

Parameter

No Parameter

-

Description

- This command is used to turn OFF (Active Low) the Tearing Effect output signal from the TE signal line.

Restriction

- This command has no effect when Tearing Effect output is already OFF.

Register

Availability

Status

Sleep Out, Display On

Sleep In

Availability

Yes

Yes

V0.22                                                                          Page  140  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006

                                                                               ST7102

6.2.23

TEON (35H): Tearing Effect Line ON

35H

TEON (Tearing Effect Line ON)

Inst / Para

Write/Read

D7

D6

D5

D4

D3

D3

D1

D0

(Default)

TEON

1st Parameter

Write

Write

0

0

0

0

1

0

1

0

0

0

1

0

0

0

1

(35H)

TEM

00h

-Tearing Effect Mode ON

- Mode 1: When TEM = ’0’, The tearing effect output line consists of V-blanking information only.

Description

- Mode 2: When TEM = ’1’: The tearing effect output line consists of both V-blanking and H-blanking information.

Note: During Sleep In Mode with Tearing Effect Line On, Tearing Effect Output pin will be active Low.

Restriction

- This command has no effect when Tearing Effect output is already ON.

Register

Availability

Status

Sleep Out, Display On

Sleep In

Availability

Yes

Yes

V0.22                                                                          Page  141  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006

                                                                               ST7102

6.2.24  MADCTR (36H): Memory Data Access Control

36H

MADCTR (Memory Data Access Control)

Inst / Para

Write/Read

D7

D6

D5

D4

MADCTR

1st Parameter

Write

Read

0

0

0

0

1

0

1

0

D3

0

BGR

D3

1

0

D1

1

CA

D0

(Default)

0

LA

(36H)

00h

- Bit Assignment

Bit

Description

Value

CA

Column Address Order (CA)

LA

Row Address Order (LA)

BGR

RGB/ BGR Order

‘1’ = Decrement, (Right to Left, when MADCTL (36h) CA=’1’)

‘0’ = Increment, (Left to Right, when MADCTL (36h) CA=’0’)

‘1’ = Row/column exchange, (when MADCTL (36h) LA=’1’)

‘0’ = Normal, (when MADCTL (36h) LA=’0’

‘1’ = BGR Order, (When MADCTL (36h) BGR=’1’)

‘0’ = RGB Order, (When MADCTL (36h) BGR=’0’)

-CA :

Description

V0.22                                                                          Page  142  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

    Send firstSend 2ndSend 3rdSend lastSend lastSend 3rdSend 2ndSend first......Top-Left (0,0)Top-Left (0,0)Top-Left (0,0)Top-Left (0,0)MIPI InputMIPI InputDisplayDisplayCA = ‘0’CA = ‘1’深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006

                                                                               ST7102

LA :

BGR :

Restriction

-

Register

Availability

Status

Sleep Out, Display On

Sleep In

Availability

Yes

Yes

V0.22                                                                          Page  143  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

    Send firstSend 2ndSend 3rdSend lastSend lastSend 3rdSend 2ndSend first......Top-Left (0,0)Top-Left (0,0)Top-Left (0,0)Top-Left (0,0)MIPI InputDisplayMIPI InputDisplayLA = ‘0’LA = ‘1’RGBRGBRGBSIG1SIG2SIG480Driver ICRGB=”0"RGBRGBRGBSIG1SIG2SIG480Driver ICRGB=”1"SIG1SIG2SIG480DisplayRGBRGBRGBRGBRGBRGBSIG1SIG2SIG480DisplayRGBRGBRGBRGBRGBRGB深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006

                                                                               ST7102

6.2.25

IDMOFF (38H): Idle Mode Off

38H

IDMOFF (Idle Mode Off)

Inst / Para

Write/Read

IDMOFF

Write

D7

0

D6

0

D5

1

Parameter

No Parameter

D4

1

D3

1

No Parameter

D3

0

D1

0

D0

(Default)

0

(38H)

-

Description

-This command is used to recover from Idle mode on.

Restriction

-This command has no effect when module is already in idle off mode

Register

Availability

Status

Sleep Out, Display On

Sleep In

Availability

Yes

Yes

V0.22                                                                          Page  144  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006

                                                                               ST7102

6.2.26

IDMON (39H): Idle Mode On

38H

IDMOFF (Idle Mode Off)

Inst / Para

Write/Read

IDMON

Write

D7

0

D6

0

D5

1

Parameter

No Parameter

D4

1

D3

1

No Parameter

D3

0

D1

0

D0

(Default)

1

(39H)

-

-This command is used to enter into Idle mode on.

-There will be no abnormal visible effect on the display mode change transition.

-In the idle on mode,

1. Color expression is reduced. The primary and the secondary colors using MSB of each R,G and B, 8 color depth data is

displayed.

2. 8-Color mode frame frequency is applied.

3. Exit from IDMON by Idle Mode Off (38h) command

Description

Color

Blank

Blue

Red

Magenta

Green

Cyan

Yellow

White

R[7:0]

0xxxxxxx

0xxxxxxx

1xxxxxxx

1xxxxxxx

0xxxxxxx

0xxxxxxx

1xxxxxxx

1xxxxxxx

G[7:0]

0xxxxxxx

0xxxxxxx

0xxxxxxx

0xxxxxxx

1xxxxxxx

1xxxxxxx

1xxxxxxx

1xxxxxxx

B[7:0]

0xxxxxxx

1xxxxxxx

0xxxxxxx

1xxxxxxx

0xxxxxxx

1xxxxxxx

0xxxxxxx

1xxxxxxx

Restriction

-This command has no effect when module is already in idle on mode

Register

Availability

Status

Sleep Out, Display On

Sleep In

Availability

Yes

Yes

V0.22                                                                          Page  145  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006

                                                                               ST7102

6.2.27

TESLWR (44H): Write TE Scan Line

44H

WRTESCN (Write TE Scan Line)

Inst / Para

Write/Read

D7

D6

D5

D4

0

-

1

-

0

-

0

-

D3

0

D3

1

D1

0

D0

(Default)

0

(44H)

TESN[7:0]

TESN[11:8]

00h

00h

- This command turns on the display module’s TE signal when the display module reaches line TESN.

Write

Write

Write

TESLWR

1st Parameter

2nd Parameter

Description

Restriction

- The command takes affect with the end of one frame.

Register

Availability

Status

Sleep Out, Display On

Sleep In

Availability

Yes

Yes

V0.22                                                                          Page  146  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006

                                                                               ST7102

6.2.28

RDSCNL (45H): Read Scan Line

45H

RDSCNL (Read Scan Line)

Inst / Para

Write/Read

D7

D6

D5

D4

RDSCNL

1st Parameter

2nd Parameter

Write

Read

Read

0

-

1

-

0

-

0

-

D3

0

D3

1

D1

0

D0

(Default)

1

(45H)

TESN[7:0]

TESN[11:8]

00h

00h

Description

- This read byte returns the current scan line.

Restriction

- The command takes affect with the end of one frame.

Register

Availability

Status

Sleep Out, Display On

Sleep In

Availability

Yes

Yes

V0.22                                                                          Page  147  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006

                                                                               ST7102

6.2.29

DSTB (4FH): Deep Standby Mode ON

4FH

DSTB (Deep Standby Mode ON)

Inst / Para

Write/Read

D7

D6

D5

D4

D3

D3

D1

D0

(Default)

DSTB

Parameter

Write

Write

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

1

0

1

0

1

(4FH)

DSTB

00H

- When DSTB = 1 :Deep Standby Mode ON

Description

Exit deep standby mode by pulling low "RESX" pin at least 1ms

Restriction

- Only effect at Sleep In mode.

Register

Availability

Status

Sleep Out, Display On

Sleep In

Availability

No

Yes

V0.22                                                                          Page  148  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006

                                                                               ST7102

6.2.30  WRDISBV (51H) Write Display Brightness

51H

WRDISBV (Write Display Brightness)

Inst / Para

Write/Read

RDSCNL

1st Parameter

Write

Write

D7

0

D6

1

D5

0

D4

1

D3

0

D3

0

D1

0

D0

(Default)

1

(51h)

DBV7

DBV6

DBV5

DBV4

DBV3

DBV2

DBV1

DBV0

00h

- This command is used to adjust the brightness value of the display.

- It should be checked what is the relationship between this written value and output brightness of the display. This relationship

Description

is defined on the display module specification.

- In principle relationship is that 00h value means the lowest brightness and FFh value means the highest brightness.

Restriction

-

Register

Availability

Status

Sleep Out, Display On

Sleep In

Availability

Yes

Yes

V0.22                                                                          Page  149  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006

                                                                               ST7102

6.2.31

RDDISBV (52H) Read Display Brightness Value

52H

RDDISBV (Read Display Brightness Value)

Inst / Para

Write/Read

RDDISBV

1st Parameter

Write

Read

D7

0

D6

1

D5

0

D4

1

D3

0

D3

0

D1

1

D0

(Default)

0

(52h)

DBV7

DBV6

DBV5

DBV4

DBV3

DBV2

DBV1

DBV0

00h

- This command returns the brightness value of the display.

- It should be checked what the relationship between this returned value and output brightness of the display. This relationship is

defined on the display module specification.

- In principle the relationship is that 00h value means the lowest brightness and FFh value means the highest brightness. - See

command “Write Display Brightness (51H)”.

Description

- This command can be used to read the brightness value of the display also when Display brightness control is in automatic

mode.

- DBV [7:0] is reset when display is in sleep-in mode.

- DBV [7:0] is ‘0’ when bit BCTRL of “Write CTRL Display (53H)” command is ‘0’.

- DBV[7:0] is manual set brightness specified with “Write CTRL Display (53H)” command when bit BCTRL is ‘1’

Restriction

-

Register

Availability

Status

Sleep Out, Display On

Sleep In

Availability

Yes

Yes

V0.22                                                                          Page  150  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006

                                                                               ST7102

6.2.32  WRCTRLD (53H) Write CTRL Display

53H

WRCTRLD (Write CTRL Display)

Inst / Para

Write/Read

D7

D6

WRCTRLD

1st Parameter

Write

Write

0

0

1

0

D5

0

BCTRL

D4

1

0

D3

0

DD

D3

0

BL

D1

D0

(Default)

1

0

1

0

(53h)

00h

- This command is used to control ambient light, brightness and gamma settings.

-BCTRL：Brightness Control Block On/Off. This bit is always used to switch brightness for display and keyboard.

‘0’ = Off (Brightness registers are 00h)

‘1’ = On (Brightness registers are active, according to the other parameters.)

-DD：Display Dimming

‘0’ = Display Dimming is off

Description

‘1’ = Display Dimming is on

-BL：Backlight On/Off

‘0’ = Off (Completely turn off backlight circuit. Control lines must be low. )

‘1’ = On

-Dimming function is adapted to the brightness registers for display and keyboard when bit BCTRL is changed at dimming-on

(DD=1).

-When BL bit changed from ‘on’ to ‘off’, backlight is turned off without gradual dimming, even if dimming-on (DD=1) are selected.

Restriction

-

Register

Availability

Status

Sleep Out, Display On

Sleep In

Availability

Yes

Yes

V0.22                                                                          Page  151  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006

                                                                               ST7102

6.2.33

RDCTRLD (54H) Read CTRL Display

54H

RDCTRLD (Read CTRL Display)

Inst / Para

Write/Read

D7

D6

RDCTRLD

1st Parameter

Write

Read

0

0

1

0

D5

0

BCTRL

D4

1

0

D3

0

DD

D3

1

BL

D1

D0

(Default)

0

0

0

0

(54h)

00h

-This command returns ambient light and brightness control values.

-BCTRL: Brightness Control Block On/Off, This bit is always used to switch brightness for display.

  ‘0’ = Off

  ‘1’ = On

-DD: Display Dimming On/Off (Only for manual brightness setting)

Description

‘0’ = Off

  ‘1’ = On

-BL: Backlight Control On/Off

‘0’ = Off

  ‘1’ = On

Restriction

-

Register

Availability

Status

Sleep Out, Display On

Sleep In

Availability

Yes

Yes

V0.22                                                                          Page  152  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006

                                                                               ST7102

6.2.34  WRCABC (55H) Write Content Adaptive Brightness Control

55H

WRCABC (Write Content Adaptive Brightness Control)

Inst / Para

Write/Read

WRCABD

1st Parameter

Write

Write

D7

0

D6

1

D5

0

D4

1

CEON

SREON

EN_LEVEL[1:0]

D3

D3

0

0

1

0

D1

0

D0

1

(Default)

(55h)

CABC_MODE[1:0]

00h

- CEON: Color and Skin Enhancement ON/OFF.

‘0’ = OFF

    ‘1’ = ON

- SREON: Sunlight Readability Enhancement ON/OFF

‘0’ = OFF

    ‘1’ = ON

-EN_LEVEL: Enhancement mode Selection

CEON

SREON

EN_LEVEL[1:0]

Function

0

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

1

0

0

0

1

0

0

1

1

0

0

1

0

0

1

1

0

1

0

1

0

1

X

Enhancement OFF

CE LOW

CE MIDDLE

CE HIGH

SRE LOW

SRE MIDDLE

SRE HIGH

SRE USER DEFINE

CE / SRE LOW

CE / SRE MIDDLE

CE / SRE HIGH

Description

Note: “X” is don’t care

-CABC_MODE:    CABC mode selection

CABC_MODE

0

1

2

3

Function

Off

User Interface Mode

Still Picture

Moving Image

Restriction

-

V0.22                                                                          Page  153  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006

                                                                               ST7102

Register

Availability

Status

Sleep Out, Display On

Sleep In

Availability

Yes

Yes

V0.22                                                                          Page  154  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006

                                                                               ST7102

6.2.35

RDCABC (56H) Read Content Adaptive Brightness Control

56H

WRCABC (Write Content Adaptive Brightness Control)

Inst / Para

Write/Read

RDCABD

Write

D7

0

D6

1

D5

0

D4

1

1st Parameter

Read

CEON

SREON

EN_LEVEL[1:0]

D3

D3

0

0

1

0

D1

1

D0

(Default)

0

(56h)

CABC_MODE[1:0]

00h

- CEON: Color and Skin Enhancement ON/OFF.

‘0’ = OFF

    ‘1’ = ON

- SREON: Sunlight Readability Enhancement ON/OFF

‘0’ = OFF

    ‘1’ = ON

Description

-EN_LEVEL: Enhancement mode Selection

CEON

SREON

EN_LEVEL[1:0]

Function

0

1

1

1

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

0

0

0

1

0

0

1

1

0

0

1

1

0

1

0

1

Enhancement OFF

CE LOW

CE MIDDLE

CE HIGH

SRE LOW

SRE MIDDLE

SRE HIGH

SRE USER DEFINE

Note: “X” is don’t care.

-CABC_MODE:    CABC mode selection

CABC_MODE[1:0]

00b

01b

10b

11b

Function

Off

User Interface Mode

Still Picture

Moving Image

Restriction

-

Register

Availability

Status

Sleep Out, Display On

Sleep In

Availability

Yes

Yes

V0.22                                                                          Page  155  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006

                                                                               ST7102

6.2.36  WRCABCMB (5EH) Write CABC Minimum Brightness

5EH

WRCABCMB (Write CABC Minimum Brightness)

Inst / Para

Write/Read

WRCABCMB

1st Parameter

Write

Write

D7

0

D6

1

D5

0

D4

1

D3

1

D3

1

D1

1

D0

(Default)

0

(5Eh)

CMB7

CMB6

CMB5

CMB4

CMB3

CMB2

CMB1

CMB0

00h

- This command is used to set the minimum brightness value of the display for CABC function.

Description

- In principle relationship is that 00h value means the lowest brightness for CABC and FFh value means the highest brightness

for CABC.

Restriction

-

Register

Availability

Status

Sleep Out, Display On

Sleep In

Availability

Yes

Yes

V0.22                                                                          Page  156  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006

                                                                               ST7102

6.2.37

RDCABCMB (5FH) Read CABC Minimum Brightness

5FH

RDCABCMB (Read CABC Minimum Brightness)

Inst / Para

Write/Read

RDCABCMB

Write

D7

0

D6

1

D5

0

D4

1

D3

1

D3

1

D1

1

D0

(Default)

1

(5Fh)

1st Parameter

Read

CMB7

CMB6

CMB5

CMB4

CMB3

CMB2

CMB1

CMB0

00h

- This command returns the minimum brightness value of CABC function.

Description

- In principle the relationship is that 00h value means the lowest brightness and FFh value means the highest brightness.

- See command “Write CABC Minimum Brightness (5EH)”.

Restriction

-

Register

Availability

Status

Sleep Out, Display On

Sleep In

Availability

Yes

Yes

V0.22                                                                          Page  157  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006

                                                                               ST7102

6.2.38

RDDID1 (DAH) Read Display Identification Information

DAH

RDDID1 (Read Display Identification Information 1)

Inst / Para

Write/Read

RDDID1

1st Parameter

Write

Read

D7

0

D6

0

D5

0

D4

0

D3

0

D3

1

D1

0

D0

(Default)

1

(DAH)

ID1[7:0]

FFh

- Read Display Identification Information

Description

ID1:LCD module's manufacturer ID (FFh: not programmed)

Restriction

-

Register

Availability

Status

Sleep Out, Display On

Sleep In

Availability

Yes

Yes

V0.22                                                                          Page  158  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006

                                                                               ST7102

6.2.39

RDDID2 (DBH) Read Display Identification Information

DBH

RDDID2 (Read Display Identification Information 2)

Inst / Para

Write/Read

RDDID2

1st Parameter

Write

Read

D7

0

D6

0

D5

0

D4

0

D3

0

D3

1

D1

0

D0

(Default)

1

(DBH)

ID2[7:0]

FFh

- Read Display Identification Information

Description

ID2:LCD module/driver version ID (FFh: not programmed)

Restriction

-

Register

Availability

Status

Sleep Out, Display On

Sleep In

Availability

Yes

Yes

V0.22                                                                          Page  159  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006

                                                                               ST7102

6.2.40

RDDID3 (DCH) Read Display Identification Information

DCH

RDDID 3 (Read Display Identification Information 3)

Inst / Para

Write/Read

RDDID3

1st Parameter

Write

Read

D7

0

D6

0

D5

0

D4

0

D3

0

ID3[7:0]

D3

1

D1

0

D0

(Default)

1

(DCH)

FFh

- Read Display Identification Information

Description

ID3:LCD module/driver ID (FFh: not programmed)

Restriction

-

Register

Availability

Status

Sleep Out, Display On

Sleep In

Availability

Yes

Yes

V0.22                                                                          Page  160  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006

                                                                               ST7102

6.2.41

RDDDBS (A1H) : Read DDB Start

A1H

RDDDBS (Read DDB Start)

Inst / Para

Write/Read

RDDDBS

1st Parameter

2nd Parameter

3rd Parameter

4th Parameter

5th parameter

6th Parameter

7th Parameter

8th Parameter

9th Parameter

10th Parameter

Write

Read

Read

Read

Read

Read

Read

Read

Read

Read

Read

D7

1

D6

0

D5

1

D4

0

D3

0

D3

0

D1

0

D0

(Default)

1

(A1H)

SID[7:0]

SID[15:8]

MID[7:0]

MID[15:8]

RID[7:0]

RID[15:8]

DDB_ID_1[7:0]

DDB_ID_2[7:0]

DDB_ID_3[7:0]

DDB_ID_4[7:0]

FFh

FFh

FFh

FFh

FFh

FFh

FFh

FFh

FFh

FFh

- This command returns supplier identification and display module / revision information.

Note:

1.  This information is not the same what DAh/DBh/DCh commands are returning.

2.  Parameter 8th is an “Exit code”, this means that there is no more data in the DDB block.

Description

This read sequence can be interrupted by any command and it can be continued by “Read DDB Continue(A8h)” command

when the first parameter, what has been transferred, is the parameter, which has not been sent e.g. RDDDBS => 1st parameter

has been sent => 2nd parameter has been sent => interrupt => RDDDBC => 3rd parameter of the RDDDBS has been sent.

Restriction

-

Register

Availability

Status

Sleep Out, Display On

Sleep In

Availability

Yes

Yes

V0.22                                                                          Page  161  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006

                                                                               ST7102

6.2.42

RDDDBC (A8H) : Read DDB Continue

A8H

RDDDBC (Read DDB Continue)

Inst / Para

Write/Read

Write

Read

Read

Read

Read

Read

Read

Read

D7

1

D6

0

D5

1

D4

0

D3

1

D3

0

D1

0

D0

(Default)

0

(A8H)

DDB1[7:0]

DDB2[7:0]

DDB3[7:0]

DDB4[7:0]

DDB5[7:0]

DDB6[7:0]

DDB7[7:0]

FFh

FFh

FFh

FFh

FFh

FFh

FFh

- This command returns supplier’s identification and display module model/revision information from the point where RDDDBS

command was interrupt by an other command e.g. RDDDBS was interrupt after 3rd parameter (DDB3). The first parameter

(DDB1), what RDDDBC is returning, is DDB4[7:0].

See also section “6.2.36 Read DDB Start (A1h)”.

RDDDBC

1st Parameter

2nd Parameter

3rd Parameter

4th Parameter

5th parameter

6th Parameter

7th Parameter

Description

Restriction

-

Register

Availability

Status

Sleep Out, Display On

Sleep In

Availability

Yes

Yes

V0.22                                                                          Page  162  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006

                                                                               ST7102

6.2.43

RDFCS (AAH) : Read First Checksum

AAH

RDFCS (Read First Checksum)

Inst / Para

Write/Read

RDFCS

1st Parameter

Write

Read

D7

1

D6

0

D5

1

D4

0

D3

1

D3

0

D1

1

D0

(Default)

0

(AAH)

FCS[7:0]

00h

- Read the first checksum that has been calculated from user commands after write access to these commands has been

Description

done.

Restriction

-only in sleep out mode

Register

Availability

Status

Sleep Out, Display On

Sleep In

Availability

Yes

Yes

V0.22                                                                          Page  163  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006

                                                                               ST7102

6.2.44

RDCCS (AFH) : Read Continue Checksum

AFH

RDCCS (Read Continue Checksum)

Inst / Para

Write/Read

RDCCS

1st Parameter

Write

Read

D7

1

D6

0

D5

1

D4

0

D3

1

D3

1

D1

1

D0

(Default)

1

(AFH)

CCS[7:0]

00h

-Read the continue checksum that has been calculated continuously after the first checksum has calculated from user

Description

commands after write access to these commands has been done.

Restriction

-

Register

Availability

Status

Sleep Out, Display On

Sleep In

Availability

Yes

Yes

V0.22                                                                          Page  164  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006

                                                                               ST7102

6.2.45

RDICID (F4H) : Read Sitronix IC ID

F4H

RDICID (Read Sitronix IC ID Code)

Inst / Para

Write/Read

D7

D6

D5

D4

D3

D3

D1

D0

(Default)

RDICID

1st Parameter

2nd Parameter

Write

Read

Read

1

0

0

1

1

0

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

0

0

0

0

1

0

1

1

(F4H)

71h

23h

Description

- Read Sitronix IC ID Code

Restriction

-

Register

Availability

Status

Sleep Out, Display On

Sleep In

Availability

Yes

Yes

V0.22                                                                          Page  165  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006

                                                                               ST7102

6.2.46  MIPIEXTFMAT (F9H) : MIPI Extension Format

F9H

MIPIEXTFMAT (MIPI Extension Format)

Inst / Para

Write/Read

D7

D6

D5

D4

D3

D3

MIPIEXTFMAT

Write

1st Parameter

Write

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

0

0

D1

0

D0

(Default)

1

(F9H)

PIXEL_EXTEN[1:0]

00h

-The PIXEL_EXTEN is used for pixel extension format.

PIXEL_EXTEN[1:0]

5-6-5 Format

6-6-6 Format

8-8-8 Format

R[7:0] = {R[4:0] , 000b }

R[7:0] = {R[5:0] , 00b }

R[7:0] = R[7:0]

00

G[7:0] = {G[5:0] , 00b }

G[7:0] = {G[5:0] , 00b }

G[7:0] = G[7:0]

B[7:0] = {B[4:0] , 000b }

B[7:0] = {B[5:0] , 00b }

B[7:0] = B[7:0]

R[7:0] = {R[4:0] , 111b}

R[7:0] = {R[5:0] , 11b}

R[7:0] = R[7:0]

01

G[7:0] = {G[5:0] , 11b}

G[7:0] = {G[5:0] , 11b}

G[7:0] = G[7:0]

Description

B[7:0] = {B[4:0] , 111b}

B[7:0] = {B[5:0] , 11b}

B[7:0] = B[7:0]

R[7:0] = {R[4:0] ,R[4:2]}

R[7:0] = {R[5:0] ,R[5:4]}

R[7:0] = R[7:0]

10

G[7:0] = {G[5:0] , G[5:4]}

G[7:0] = {G[5:0] , G[5:4]}

G[7:0] = G[7:0]

B[7:0] = {B[4:0] , B[4:2]}

B[7:0] = {B[5:0] , B[5:4]}

B[7:0] = B[7:0]

R[7:0] = {R[4:0] , G[5:3]}

R[7:0] = {R[5:0] , G[5:4]}

R[7:0] = R[7:0]

11

G[7:0] = {G[5:0] , G[5:4]}

G[7:0] = {G[5:0] , G[5:4]}

G[7:0] = G[7:0]

B[7:0] = {B[4:0] , G[5:3]}

B[7:0] = {B[5:0] , G[5:4]}

B[7:0] = B[7:0]

Restriction

-

Register

Availability

Status

Sleep Out, Display On

Sleep In

Availability

Yes

Yes

V0.22                                                                          Page  166  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006

                                                                               ST7102

7  ELECTRICAL CHARACTERISTICS

7.1

Absolute Maximum Ratings

Item

Supply Voltage (Analog)

Symbol

IOVCC

Range

Unit

- 0.3 ~ +2.1

Supply Voltage (I/O)

DPHYVCC

- 0.3 ~ +2.1

Driver Supply Voltage

AVDD-VSS

-0.3 ~ +6.6

Driver Supply Voltage

AVEE-VSS

- 6.6 ~ +0.3

Driver Supply Voltage

VGH-VGL

-0.3 ~ +30.0

Logic Input Voltage Range

Logic Output Voltage Range

Operating Temperature Range

Storage Temperature Range

VIN

VO

TOPR

TSTG

-0.3 ~ DPHYVCC + 0.3

-0.3 ~ DPHYVCC + 0.3

-30 ~ +75

-40 ~ +110

V

V

V

V

V

V

V

℃

℃

V0.22                                                                          Page  167  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006
                                                                               ST7102

7.2

DC Characteristics

7.2.1

Basic Characteristics

Parameter

Symbol

Condition

Specifica

tion

MIN.

Unit

Note

TYP.

MAX.

Power & Operation Voltage

IOVCC

1.65

1.8

3.3

Power supply Voltage

DPHYVCC

1.65

1.8

3.3

AVDD

AVEE

VGH

VGL

Gate Driver High Voltage

Gate Driver Low Voltage

Logic-High Input Voltage

VIH

Logic-Low Input Voltage

VIL

Input / Output

Logic-High Output Voltage

VOH

IOH = -1.0mA

4.5

4.5

7

-7

0.7

IOVCC

VSS

0.8

IOVCC

V

V

V

V

V

V

Note 2

5.5

5.5

6.3

6.3

18

-18

IOVCC

V

Note 1

0.3IOVCC

V

Note 1

IOVCC

V

Note 1

Logic-Low Output Voltage

VOL

IOL = +1.0mA

VSS

0.2 IOVCC  V

Note 1

Input Leakage Current

IIL

VIN=IOVCC/VSS

-1

+1

uA  Note 1

VCOM amplitude

VCOM

-2.75

-0.2

V

Source Driver

VCOM Voltage

Gamma Amplitude

(Positive)

Gamma Amplitude

(Negative)

Notes:

VGPAMP

VGNAMP

1. TA= -30 to 70℃ (to +75℃ no damage).

3

-6

6

-3

V

V

2. When evaluating the maximum and minimum of VGH/VGL. IOVCC/AVDD/AVEE is typical value.

3. VGH-VGL<28V.

4 . AVDD-VGPAMP>300mV        AVEE-VGNAMP>-300mV

V0.22                                                                          Page  168  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006

                                                                               ST7102

7.2.2

Current Consumption

(Ta = 25℃)

Parameter

Symbol

Test Condition

Specification

MIN.

TYP.

MAX.

IOVCC = 1.8

AVDD = 5.9

AVEE = -5.9

Deep standby mode

No load on panel

Power supply

voltage

IOVCC = 1.8

Sleep in mode

No load on panel

AVDD = 5.9

AVEE = -5.9

-

-

-

-

-

-

TBD

TBD

TBD

TBD

TBD

TBD

Unit

uA

uA

uA

mA

mA

mA

V0.22                                                                          Page  169  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006

                                                                               ST7102

7.2.3

MIPI DC Characteristic

Figure 76 MIPI Signaling Voltage Levels

Parameter

Symbol

Specification

MIN

TYP

MAX

Operation Voltage for MIPI Receiver

Low power mode operating voltage

VLPH

-

TBD

MIPI Characteristics for High Speed Receiver

Single-ended input low voltage

VILHS

TBD

Single-ended input high voltage

VIHHS

-

Common-mode voltage

VCMRXDC  TBD

-

-

-

-

-

TBD

TBD

Unit

V

mV

mV

mV

Differential input impedance

ZID

TBD

TBD

TBD

ohm

MIPI Characteristics for Low Power Mode

Pad signal voltage range

Logic 0 input threshold

Logic 1 input threshold

VI

VIL

VIH

TBD

TBD

TBD

Output low level

VOL

TBD

-

-

-

-

TBD

TBD

-

TBD

mV

mV

mV

mV

Output high level

VOH

TBD

TBD

TBD

V

V0.22                                                                          Page  170  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

VOH,MAXGNDVIH,MINVIL,MAXVIHHSVILHSVCMRXDC,MAXVCMRXDC,MINVOH,MAXVIH,MINVIL,MAXLP-RX Input HighLP-RX Input LowLP-RX ThresholdLP-TX Ouput HighLP-TX Ouput LowVOH,MINVOL,MAXVOL,MINLow PowerTransmitterLow PowerReceiverHigh SpeedReceiverHS-RXCommon ModeInput RangeHS-RXInput Range深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006

                                                                               ST7102

7.3

AC Characteristics

7.3.1

MIPI Timing

Figure 77 High Speed Mode – Clock Channel Timing

Signal

Symbol

Parameter

Unit

Description

MIN

TYP  MAX

Specification

DSI-CLK+/-

2xUIINST

Double UI instantaneous

TBD

DSI-CLK+/-

UIINSTA ,UIINSTB

UI instantaneous half

TBD

fDSI-CLK+/-

FDSICLK

DSI-CLK+/- frequency

TBD

DSI-Dn+/-

DSI-Dn+/-

TDS

TDH

Data to clock setup time

TBD

Data to clock hold time

TBD

-

-

-

-

-

TBD

TBD

ns

ns

TBD  MHz

-

-

UI

UI

V0.22                                                                          Page  171  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

DSI-CLK-DSI-CLK+2xUIINSTUIINSTAUIINSTB DSI-CLK-DSI-CLK+TDSDSI-D0-DSI-D0+ TDH TDHTDS 深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006

                                                                               ST7102

Figure 78 High-Speed Data Transmission

Parameter

Symbol

Specification

MIN

TYP

MAX

-

-

-

-

-

-

-

Time to drive LP-00 to prepare for HS transmission

THS-PREPARE

TBD

Time from start of t HS-TRAIL or t CLK-TRAIL period to start of LP-11 state

TEOT

Time to enable data receiver line termination measured from when Dn crosses VILMAX

THS-TERM-EN

Time to drive flipped differential state after last payload data bit of a HS transmission

Time-out at RX to ignore transition period of EoT

Time to drive LP-11 after HS burst

Length of any Low-Power state period

Sync sequence period

THS-TRAIL

THS-SKIP

THS-EXIT

TLPX

-

-

TBD

TBD

TBD

TBD

THS-SYNC

-

TBD

Minimum lead HS-0 drive period before the Sync sequence

THS-ZERO

TBD

Time interval during which the HS receiver should ignore any Clock Lane HS transitions,

starting from the beginning of TCLK-PREPAR

Time interval  during  which  the  HS  receiver  shall ignore  any  Data Lane  HS  transitions,

starting from the beginning of THS-PREPAR.

The HS receiver shall ignore any Data Lane transitions before the minimum value, and

the HS receiver shall respond to any Data Lane transitions after the maximum value.

TCLK-SETTLE

TBD

THS-SETTLE

TBD

-

-

-

Un

it

ns

ns

ns

ns

ns

ns

ns

ns

ns

TBD

TBD

TBD

-

TBD

-

-

-

-

TBD

ns

TBD

ns

V0.22                                                                          Page  172  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

DSI-CLK-DSI-CLK+VIHLPRX(Min)VIHLPRX(Max)DSI-D0+DSI-D0-TLPXTHS-PREPARETHS-ZEROTHS-SYNCTHS-TERM-ENTHS-SETTLETHS-SKIPTEOTTHS-TRAILTHS-EXITLow Power Mode,Disable RX Line TerminationHigh Speed Mode, Enable Rx Line TerminationLow Power Mode,Disable Rx LineTerminationLP-11LP-01LP-00LP-11DisconnectTerminatorCapture1stData Bit深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006

                                                                               ST7102

Figure 79 Switching the Clock Lane between Clock Transmission and Low-Power Mode

Parameter

Symbol

Specification

MIN

TYP

MAX

Time  that  the  transmitter  shall  continue  sending  HS  clock  after  the  last

associated Data Lane has transitioned to LP mode

Detection time that the clock has stopped toggling

Time to drive LP-00 to prepare for HS clock transmission

T CLK-POST

T CLK-MISS

T CLK-PREPARE

Minimum lead HS-0 drive period before starting Clock

T CLK-PREPARE +T CLK-ZERO

Time to enable Clock Lane receiver line termination measured from when

Dn cross VIL,MAX

Minimum time that the HS clock must be set prior to any associated date

lane beginning the transmission from LP to HS mode

Time  to  drive  HS  differential  state  after  last  payload  clock  bit  of  a  HS

T CLK-TERM-EN

T CLK-PRE

T CLK-TRAIL

transmission burst

Note: 3-Lane = (1000 / fmipi * 8 * 52 -THS_TRAIL- 60) / (1000/fmipi)

          4-Lane = (1000 / fmipi * 8 * 39 -THS_TRAIL- 60) / (1000/fmipi)

          Example: 3-Lane, 600Mbps : n = (1000 / 600 * 8 * 52 - THS_TRAIL- 60) / (1000/600) = 340

                            4-Lane, 600Mbps : n = (1000 / 600 * 8 * 39 - THS_TRAIL- 60) / (1000/600) = 236

TBD

-

TBD

TBD

-

TBD

TBD

-

-

-

-

-

-

-

Unit

ns

ns

ns

ns

-

TBD

TBD

-

TBD

ns

-

-

UI

ns

V0.22                                                                          Page  173  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

DSI-CLK-DSI-CLK+VIHLPRX(Min)VIHLPRX(Max)VIHLPRX(Min)VIHLPRX(Max)DSI-D0-DSI-D0+TEOTTCLK-MISSTCLK-SETTLETCLK-TERM-ENTHS-SKIPTCLK-POSTTCLK-TRAILTHS-EXITTLPXTCLK-PREPARETCLK-ZEROTCLK-PRETLPXTHS-PREPAREDisconnectTerminatorHS-0/1HS-0LP-11LP-01LP-00HS-0HS-0/1深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006

                                                                               ST7102

Figure 80 Bus Turn-around Procedure

Parameter

Symbol

Specification

MIN

TYP

MAX

Length of any Low-Power state period : Master side

TLPX

TBD

Length of any Low-Power state period : Slave side

TLPX

Ratio of TLPX (MASTER)/ TLPX (SLAVE) between Master and Slave side

Ratio TLPX

Time-out before new TX side start driving

Time to drive LP-00 by new TX

Time to drive LP-00 after Turnaround Request

T TA-SURE

T TA-GET

T TA-GO

TBD

TBD

TBD

-

-

-

-

-

-

TBD

TBD

TBD

TBD

TBD

TBD

-

-

Unit

ns

ns

ns

ns

ns

V0.22                                                                          Page  174  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

DSI-D0-DSI-D0+TLPXMTLPXMTLPXMTTA-SURETLPXDTLPXDTTA-GETDLP-11LP-10LP-00LP-10LP-00LP-00LP-00LP-10LP-11MPU is ControllingControl ChangeDisplay Module is Controlling深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006

                                                                               ST7102

7.3.2

MIPI Interface Timing

Parameter

Symbol

Min.

Typ..

Max.

Unit

Horizontal Sync. Width

Horizontal Sync. Back Porch

Horizontal Sync. Front Porch

Vertical Sync. Width

Vertical Sync. Back Porch

Vertical Sync. Front Porch

Vertical Frequency

HPW

HBP

HFP

VSW

VBP

VFP

TBD

TBD

TBD

TBD

TBD

TBD

-

-

-

-

-

-

-

TBD

-

-

-

-

-

-

-

Byte Clock

Byte Clock

Byte Clock

Line

Line

Line

Hz

V0.22                                                                          Page  175  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006

                                                                               ST7102

7.3.3

RGB timing

T rgbf
T rgbr

T SYNCS

HSYNC
VSYNC

ENABLE

DOTCLK

Data Bus
Write

T ENS

T ENH

V IH
V IL

PWDL

T rgbr

V IL

V IH
V IL

PWDH

V IH

T CYCD

T PDS

T PDH

Write Data

V IH
V IL

V IH

T rgbf

V IH

V IL

V IH
V IL

Signal

Symbol

Parameter

MIN

MAX

Unit

Description

IOVCC=1.8V, Ta=25℃

TSYNCS

VSYNC, HSYNC Setup Time

TBD

HSYNC,

VSYNC

ENABLE

TENS

TENH

Enable Setup Time

Enable Hold Time

PWDH

DOTCLK High-level Pulse Width

DOTCLK

PWDL

DOTCLK Low-level Pulse Width

DB

TCYCD

TPDS

TPDH

DOTCLK Cycle Time

PD Data Setup Time

PD Data Hold Time

-

-

-

-

-

-

-

-

ns

ns

ns

ns

ns

ns

ns

ns

TBD

TBD

TBD

TBD

TBD

TBD

TBD

V0.22                                                                          Page  176  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006

                                                                               ST7102

7.3.4

SPI9 & SPI16 Timing

Signal

Symbol

Parameter

Min

Max

Unit

Description

IOVCC=1.8V, Ta=25℃

TCSS

TCSH

TCSS

TSCC

Chip select setup time (write)

TBD

Chip select hold time (write)

TBD

Chip select setup time (read)

TBD

Chip select hold time (read)

TCHW

Chip select “H” pulse width

TSCYCW

Serial clock cycle (Write)

TSHW

TSLW

SCL “H” pulse width (Write)

SCL “L” pulse width (Write)

TSCYCR

Serial clock cycle (Read)

SCL “H” pulse width (Read)

SCL “L” pulse width (Read)

Data setup time

Data hold time

TSHR

TSLR

TSDS

TSDH

TACC

TOH

TBD

TBD

TBD

TBD

TBD

TBD

TBD

TBD

TBD

TBD

Access time

TBD

TBD

Output disable time

TBD

TBD

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

ns

ns

For maximum CL=30pF

For minimum CL=8pF

CSX

SCL

SDA

(DIN)

DOUT

V0.22                                                                          Page  177  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

CSXVIHVILTCHWTCSHTOHTCSSSCLSDASDOTSCCTSCYCW/TSCYCRTACCVIHVILVIHVILVIHVILVIHVILTSDSTSDHTSHW/TSHRTSLW/TSLR深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006

                                                                               ST7102

7.3.5

SPI8 (4 line) Timing

Signal

Symbol

Parameter

MIN

MAX  Unit

Description

IOVCC=1.8V, Ta=25℃

TCSS

TCSH

TCSS

TSCC

TCHW

Chip select setup time (write)

Chip select hold time (write)

Chip select setup time (read)

Chip select hold time (read)

Chip select “H” pulse width

TSCYCW

Serial clock cycle (Write)

TSHW

TSLW

SCL “H” pulse width (Write)

SCL “L” pulse width (Write)

TSCYCR

Serial clock cycle (Read)

TSHR

TSLR

TDCS

TDCH

TSDS

TSDH

TACC

TOH

SCL “H” pulse width (Read)

SCL “L” pulse width (Read)

D/CX setup time

D/CX hold time

Data setup time

Data hold time

Access time

Output disable time

CSX

SCL

D/CX

SDA

(DIN)

DOUT

TBD

TBD

TBD

TBD

TBD

TBD

TBD

TBD

TBD

TBD

TBD

TBD

TBD

TBD

TBD

TBD

TBD

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

ns

ns

ns

ns

50

50

-write command & data

ram

-read command & data

ram

For maximum CL=30pF

For minimum CL=8pF

V0.22                                                                          Page  178  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

CSXVIHVILTCHWTCSHTOHTCSSSCLSDASDOTSCCTSCYCW/TSCYCRTACCVIHVILVIHVILVIHVILTSDSTSDHTSHW/TSHRTSLW/TSLRD/CXVIHVILTDCSTDCH深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006

                                                                               ST7102

7.3.6

Touch SPI Timing

Signal

Symbol

Parameter

MIN

MAX

Unit

Description

IOVCC=1.8V, Ta=25℃

SCK

SCK

MOSI

MISO

SS

fSCK

tCYC

tDS

tDH

tDD

tSS_SCK

tSCK_SS

tR

SCK frequency

-

TBD

Mhz

SCK cycle time

Data setup time prior SCK rising

Data hold time after SCK rising

TBD

TBD

TBD

-

-

-

MISO data output delay from SCK falling

-

TBD

SS falling to 1st SCK falling

SCK rising to SS rising

CS recovery time

TBD

TBD

TBD

-

-

-

ns

ns

ns

ns

ns

ns

us

V0.22                                                                          Page  179  of 299                                                              2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006

                                                                             ST7102

7.3.7

Touch I2C timing

Signal

Symbol

Parameter

MIN

MAX

Unit

Description

IOVCC=1.8V, Ta=25℃

SCL

SDA

fSCL

tLOW

tHIGH

SCL clock frequency

SCL clock low period

SCL clock high period

tSU;Data

Data set-up time

tHD;Data

Data hold time

tSU;STA

Setup time for a repeated START condition

SDA

tHD;STA

Start condition hold time

tSU;STO

Setup time for STOP condition

tBUF

Bus free time between a STOP and START

-

TBD

kHz

TBD

TBD

TBD

TBD

TBD

TBD

TBD

TBD

-

-

-

-

-

-

-

-

ns

ns

ns

ns

ns

ns

ns

us

V0.22                                                                        Page  180  of  183                                                                      2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

tSU;STAtHIGHtLOWtHD;STAtBUFSCLSDAtrtftHD;DATtSU;DATSDAtSU;STO深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006

                                                                             ST7102

7.3.8

Reset Timing

Figure 81 Reset Operation

Figure 82 Reset Noise Rejected Diagram

Reset Timing Characteristics IOVCC=1.8v Ta=25℃

Parameter

Symbol

Specification

Min.

Typ.

Max.

Reset low width

Trst_width

TBD

Reset time

Trst_time

TBD

OP noise reject

Trst_op_nrj

-

-

-

-

-

-

TBD

us

Unit

ms

ms

- During the reset period, the display will be blanked, then return to default condition for IC initialization

V0.22                                                                        Page  181  of  183                                                                      2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

DisplaystatusTrst_widthTrst_timeDuring resetInitial condition(Default for H/W reset)RESXNormal operationShorter than 10us will be rejectedTrst_op_nrj深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006

                                                                             ST7102

7.3.9

Abnormal Timing:

ST7102 provides abnormal power drop detection function, but there has a premise that external power must drop at low ramp, or the

discharge function will not work. The following diagram is show how much time that external power drop from 90% to 10% is suitable.

Figure 83 Reset Noise Rejected Diagram

Reset Timing Characteristics IOVCC=1.8v, AVDD=5.8v, AVEE=-5.8v Ta=25℃

External

power

Power drop time from 90% to 10%

Min.

Typ.

Max.

IOVCC

100

AVDD

100

AVEE

100

Unit

ms

ms

ms

Note: Due to IOVCC is critical for whole chip, keep IOVCC as stable as possible

V0.22                                                                        Page  182  of  183                                                                      2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

IOVCC/AVDD/AVEEDisplaystatusSleep Out ModeAbnormal signalSleep In ModeAbnormal Sequence ~ 10ms90%10%>100ms深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006

                                                                             ST7102

8  REVISION HISTORY

Version

Date

Description

V0.0

V0.1

2023/10

First issue

2024/01

Modify Interface Select P13

V0.21

2024/10

Addition Some PAD description. P12~P17

Addition RGB application2 Table P93

V0.22                                                                        Page  183  of  183                                                                      2024/10

Sitronix Confidential    The information contained herein is the exclusive property of Sitronix and shall not be distributed, reproduced, or
disclosed in whole or in part without prior written permission of Sitronix.

深圳市双禹盛泰科技有限公司 联系电话： 0755-2772 1006

