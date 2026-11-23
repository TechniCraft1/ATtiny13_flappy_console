# Setup
To upload a Sketch to the ATtiny 13 using the Arduino IDE, follow this tutorial: https://www.hackster.io/taunoerik/programming-attiny13-with-arduino-uno-07beba.
Then open the sketch (flappy_v3.ino) using the Arduino IDE, choose the ATtiny 13 microcontroller and go to the "Tools" tab. 
After that, set the BOD to "disabled", the programmer to "Arduino as ISP" and the clock to "1.2MHz internal osc.".

The gerber files are provided in the gerber.zip file and work with any major PCB service.
ATTENTION: On some SSD1306 boards the VCC and GND lines are flipped, make sure, that you have the same display shown in the video!

# LIST OF PARTS
- bc547b or bc547c
- SSD 1306 128x64 i²c OLED display
- ATTiny13
- 6 mm standard SMD/THT push button
- resistor: 15 - 25 k
- capacitor: 100nF
- Arduino UNO or ISP Programmer (Programming only)
- CR2032 battery and standard THT holder
- custom PCB
