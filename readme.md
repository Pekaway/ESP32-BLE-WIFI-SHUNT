**ESP32-C3 based SHUNT** 

**Features:** 

 - esp32-c3 
 - Step-down converter 6-32V input 
 - BLE or Wifi -> PCB Antenna
 - INA226 sensor     
 - WS2812b Status Led.   
 - USB-C for programming

. 

Currently works with a measuring shunt 200A/75MV. 


**First software**

 - BLE Broadcast
 - read SOC, AMPS, VOLT
 - save battery size in AH
 - set SOC
 - readout via NRF connect or Python scripts (see REPO)
 - save the SOC in the SPIFFS file system

Errors and wish list: 
- history data
- improvement of the BLE services
- Wifi HTTP/MQTT version 
-  housing
- serial data out
- oower consumption reduction 



**Known problems:**
- DS18B20 connected to SPI FLash pin 24, this causes a problem and the ESP does not boot 
- DeepSleep Flash problem with the ESP32-C3 USB-C port
- own consumption is not counted
- below 100mA the accuracy decreases


**Hints:** 
- If the COM port is not found turn the USB-C cable. Not all pins are assigned.
- 








