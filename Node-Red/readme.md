**ESP32-C3 based SHUNT** 

**Node-Red Example** 
This example uses the Python scripts to set and read out the shunt. 

The flow can be easily integrated into the VAN PI OS. The values SOC, Volt and Current are written to the same global variables as the wired VAN PI shunt. 

The data is therefore displayed and correctly assigned after setting MainBattData to "VAN PI SHUNT". 

When searching, the name "Pekaway Shunt" is searched for and the corresponding MAC address is saved. If there are several shunts in the vicinity, this can lead to problems. 

