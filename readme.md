**ESP32-C3 based SHUNT** 

**Features:** 

esp32-c3
Step-down converter 6-32V input
BLE or Wifi -> PCB Antenna
INA226 sensor
WS2812b Status Led. 
USB-C for programming. 

Currently works with a measuring shunt 200A/75MV. 


**First software**
-> BLE Broadcast
-> read SOC, AMPS, VOLT
-> Save battery size in AH
-> Set SOC
->Readout via NRF Connect or Python scripts (see REPO)
-> Save the SOC in the SPIFFS file system


Errors and wish list: 
-> History data
-> Improvement of the BLE services
-> Wifi HTTP/MQTT version 
-> Housing
-> Serial data out
-> Power consumption reduction 



**Known problems:**
-DS18B20 connected to SPI FLash pin 24, this causes a problem and the ESP does not boot 
-> DeepSleep Flash problem with the ESP32-C3 USB-C port


**Hints:** 
- If the COM port is not found turn the USB-C cable. Not all pins are assigned. 








