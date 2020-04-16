# ECG Generator
Open source low-cost ECG Generator board to generate and simulate the ECG signal of 3-leads RA,LA,RL

#### Source Files description
* Prototype  
  * prototyping by arduino
    * Code by platformio
    * Simulation by proteus
    * References
    * Signal Data Array c
  * prototyping by ESP8266
    * Code by platformio
* PCB Board
* Firmware

#### Work reports and descriptions
* April 16, 2020: The prototype by Arduino ( the Code and the simulation ) done and based the article/work of James P. Lynch "Adafruit Menta ECG Simulator". There is a problem with the simulation, the voltage output from the circuit was in Volt, not Millivolt. The next work is to test it in real, __I will back to this work when I have a real oscilloscope__. If the work is done by Arduino, I will transfer it to work with ESP8266 or ESP32, I will low the cost from 60$ to just 10 $.  

![Screenshot](/Prototype/prototyping_by_arduino/Simulation_by_proteus/1.png)

#### Tools 
* Platformio for VS code IDE 
* Proteus Design Suite 8.9 
* Arduino boards for proteus 8.9 from the engineering projects website
* Engauge Digitizer
* Geany IDE

#### References
* Arduino Menta ECG Simulator project  
https://github.com/lynchzilla/ecg_simulator

