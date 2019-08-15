# Arduino core for ESP32 WiFi chip

## Contents
- [Using Instructions](#installation-instructions)
- [Libraries Instructions](#Libraries-Instructions)

## Using Instructions  
The purpose of this article is to show how to use seeeduino V4 to control Spartan Edge Accelerator Board by arduino IDE  
- Spartan Edge Accelerator Board  
It can be used independently as an Arduino compatible board, or plugged into an Arduino as an auxiliary accelerator board.  
(picture)  
- Seeeduino V4  
The Seeeduino v4.0 is an ATMEGA328 Microcontroller development board. The ATMEGA328P-MU is a high performance, low power AVR 8-Bit Microcontroller. The Seeeduino v4.0 has 14 digital input/output pins (6 of which can be used as PWM outputs) and 6 analog pins.  
(pic)
- Installation Instructions  
	+ Install arduino IDE
	you can see the [web](https://www.arduino.cc/).
    + Using Arduino IDE Boards Manager  
 Starting with 1.6.4, Arduino allows installation of third-party platform packages using Boards Manager. We have packages available for Windows, Mac OS, and Linux (32 and 64 bit).  
 Enter https://dl.espressif.com/dl/package_esp32_index.json into Additional Board Manager URLs field  
 If you want to see the details, you can refer to this [website](https://github.com/espressif/arduino-esp32/blob/master/docs/arduino-ide/boards_manager.md).  
 
## Libraries Instructions  
### [spartan_edge_ioex](https://github.com/SU1JUN4KANG1/spartan_edge_ioex)
This library makes it easier for you to control spartan-edge-accelerator board  
#### 1.Flash MicroPython Firmware  
- Check the fifth pin in SW4 on the Slave side
- Download the [github repository](https://github.com/PYNQ-X1024/MicroPython_ESP32_on_X1024.git)  
- Go to the dir (DOWMLOADPATH)/MicroPython_ESP32_on_X1024/MicroPython_BUILD/firmware/esp32_sea  
- Execute the command  
___../flash.sh -p /dev/ttyUSB0 -b 460800___  
And you should press BOOT key for a while  
Parameter specification  
	+ -p Uart device  
	+ -b  Baudrate  
 
#### 2.Loading the FPGA_LOGIC automatically
- Installation uPyCraft program  
Install uPyCraft,you can dowmload it form this [web](https://randomnerdtutorials.com/install-upycraft-ide-windows-pc-instructions/).
- Use USB type-c cable to connect Spartan Board to PC
- Put the SW1 on USB siede  
- install the CP2102 dirver 
- Add Python program  
Click ___Tools->Serial___  and select appropriate ___TTY/COM___ port, if you can see ___">>>"___ in terminal, you're successful. 
if you can't, try again  
put the file ___boot.py___ to /device in uPyCraft IDE.
You cam find ___boot.py___ in <u>https://github.com/PYNQ-X1024/MicroPython_ESP32_on_X1024/tree/master/test</u>  
- SD card by ready  
open the [web](https://github.com/PYNQ-X1024/MicroPython_ESP32_on_X1024/tree/master/test/sd_card)  
and copy all file to your SD card 
``` c
The file description is as follows
/sd_card root_dir
│-board_config.json       You can configure which FPGA logic to load when powered on, as well as other configurations
│
└─overlay
        color_detect.bit  To detect color blocks, RPi MIPI camera v1.3 version is required.
        hdmi_v1.bit       Displays moving color stripes on HDMI
        mipi_camera.bit   The RPi MIPI camera v1.3 is required to capture images and display them on the HDMI display.
        spi2gpio.bit      SPI interface GPIO extension logic, which supporting the operation of ADC/DAC/ rgb-led.
```  
#### 3.Using esp32 with arduino by spartan_edge_ioex Library  
- you can dowmload it by [link](https://github.com/SU1JUN4KANG1/spartan_edge_ioex). 
And move it to ___arduino\libraries___,  
- Example Instructions
   + 01LED_AND_BUTTON  
 Using button USER1 to control LED1/2 reverse
   + 02RGB_LED  
 Set the RGBLED's R,G,B value,which will change the led color
   + 03ADC_AND_DAC  
 Read ADC information from FPGA and output by Serial, adn write DAC value to FPGA
   + 04SWITCH  
 Check the SW4 and ouput infomation by Serial
   + 05SWITCH_FPGA_BIT  
 Using i2c to send infomation to ESP32 to select the fpga-logic-bit.
 
Opening the .ino file and dowmload it by arduino IDE，you can check the board functionality  
