/*********
PEKAWAY VAN PI 
esp32-c3 shunt 

This codes received the size of the battery in AH (utf-8) and also the SOC(utf-8) via BLE and will then calculate the new SOC according the current.
You can discharge and discharge. 
The status is readable via BLE.  -> please check the BLE PYTHON SCRIPTS. 

The the SOC is stored all 60S to spiffs file system as backup.

*********/




#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <Wire.h>
#include <INA.h>  // Zanshin INA Library
#include "SPIFFS.h"

#define uS_TO_S_FACTOR 1000000  /* Conversion factor for micro seconds to seconds */

const uint32_t SERIAL_SPEED{115200};     ///< Use fast serial speed
const uint32_t SHUNT_MICRO_OHM{375};  ///< Shunt resistance in Micro-Ohm, e.g. 100000 is 0.1 Ohm //75MV 200A
const uint16_t MAXIMUM_AMPS{1022};          ///< Max expected amps, clamped from 1A to a max of 1022A
uint8_t        devicesFound{0};          ///< Number of INAs found

unsigned long currentTime = millis();
unsigned long previousTime = 0;
const long timeoutTime = 2000;
int16_t receivedValue = 0;


INA_Class      INA;                      ///< INA class instantiation to use EEPROM
// INA_Class      INA(0);                ///< INA class instantiation to use EEPROM
// INA_Class      INA(5);                ///< INA class instantiation to use dynamic memory rather
//                                            than EEPROM. Allocate storage for up to (n) devices


#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

//BLE server name
#define bleServerName "Pekaway Shunt"

//float SOC;
long long SOC;
long VOLT;
long AMPS;

long long AHmillimax;
//long long AHmillistoredonSpiffs;
long long AHmillinow;

long microampsmillis;

BLECharacteristic *pCharacteristic; //MAXAMPHOURS
BLECharacteristic *pCharacteristic2; //SOC

bool deviceConnected = false;

// Timer variables
unsigned long previousMillis = 0;  // Speichert den Zeitstempel der vorherigen Schleife
unsigned long currentMillis = 0;   // Speichert den Zeitstempel der aktuellen Schleife
unsigned long previousMillisSAVE = 0;








// See the following for generating UUIDs:
// https://www.uuidgenerator.net/
#define SERVICE_UUID "91bad492-b950-4226-aa2b-4ede9fa42f59"


  BLECharacteristic INAAMPSCharacteristics("d2e8ede8-9b31-4478-9fd6-75845cb68b1d", BLECharacteristic::PROPERTY_READ);
  BLECharacteristic INAVOLTSCharacterictics("ff100f8c-1266-4309-b472-76bc25e4a62f", BLECharacteristic::PROPERTY_READ);
  BLECharacteristic INASOCCharacterictics("459e9ea4-a335-4a3f-b838-46788fd6bbe4", BLECharacteristic::PROPERTY_READ);
  

 void initSPIFFS() {
  if (!SPIFFS.begin(true)) {
    Serial.println("An error has occurred while mounting SPIFFS");
  }
  Serial.println("SPIFFS mounted successfully");
}

// Read File from SPIFFS
String readFile(fs::FS &fs, const char * path){
  Serial.printf("Reading file: %s\r\n", path);

  File file = fs.open(path);
  if(!file || file.isDirectory()){
    Serial.println("- failed to open file for reading");
    return String();
  }
  
  String fileContent;
  while(file.available()){
    fileContent = file.readStringUntil('\n');
    break;     
  }
  return fileContent;
}


void readAHMAXFromFile() {
    // Open the file for reading
    File file = SPIFFS.open("/dataAHMAX.txt", FILE_READ);
    if (!file) {
        Serial.println("Failed to open file for reading");
        return;
    }

    // Read data from the file and print it
    while (file.available()) {
        // Read a line from the file
        String line = file.readStringUntil('\n');
        AHmillimax=line.toInt();
        AHmillimax=AHmillimax*60*60*1000*1000;//milliampere, hours to millisecond
        Serial.println(line);
         Serial.println(AHmillimax);
    }

    // Close the file
    file.close();
}

void readAHNOWFromFile() {
    // Open the file for reading
    File file = SPIFFS.open("/dataAHNOW.txt", FILE_READ);
    if (!file) {
        Serial.println("Failed to open file for reading");
        return;
    }

    // Read data from the file and print it
    while (file.available()) {
        // Read a line from the file
        String line = file.readStringUntil('\n');
        Serial.print("read AHNOWFROM FILE - String:");
        Serial.println(line);
        AHmillinow=atoll(line.c_str());
        // Print the line to the Serial monitor
        Serial.print("AHmillinow as a VALUE long long :");
        Serial.println(AHmillinow);
    }

    // Close the file
    file.close();
}


void writeAHNOWToFile() {
  SPIFFS.remove("/dataAHNOW.txt");
  File file = SPIFFS.open("/dataAHNOW.txt", FILE_WRITE );//delete before open
        
        
        
        if (file) {
            // Write the received string to the file
            file.println(AHmillinow);
            file.close();
            Serial.println("AHmillinow (amillimperemillisecondes):");
            Serial.print(AHmillinow);
            Serial.println("   stored on SPIFFS");
        } else {
            Serial.println("Failed to open file for writing");
        }

}


//send string SOC and AMPHOURS
class MyCallbacks : public BLECharacteristicCallbacks {
   void onWrite(BLECharacteristic *pCharacteristic) {
        BLEUUID characteristicUUID = pCharacteristic->getUUID();
        if (characteristicUUID.equals(BLEUUID("38f2bd70-d659-4970-86c1-061e24700a6e"))) {
            // Handling write operation for MAX AMPHOURS characteristic
            Serial.println("Received MAX AMPHOURS");

            // Get the value written to the characteristic
            std::string value(pCharacteristic->getValue().c_str());

            // Convert the string value to a C string
            const char* receivedString = value.c_str();
            long long  MAX = atoi(receivedString);
            AHmillimax=0;
            AHmillimax=MAX*60*60*1000*1000;//milliampere, hours to millisecond

            // Print the received string
            Serial.print("Received String: ");
            Serial.println(receivedString);
             Serial.print("in ampere milliseconds: ");
            Serial.println(AHmillimax);

          
        

            SPIFFS.remove("/dataAHMAX.txt");
            // Open SPIFFS file for writing
            File file = SPIFFS.open("/dataAHMAX.txt", FILE_WRITE);
            if (file) {
                // Write the received string to the file
                file.println(receivedString);
                file.close();
                Serial.println("String stored on SPIFFS");
            } else {
                Serial.println("Failed to open file for writing");
            }
        } else if (characteristicUUID.equals(BLEUUID("63d58a25-c22b-4586-b297-f1e310b7b0bc"))) {
            // Handling write operation for SOC AMPHOURS characteristic
            Serial.println("Received SOC AMPHOURS");

            // Get the value written to the characteristic
            std::string value(pCharacteristic->getValue().c_str());

            // Convert the string value to a C string
            const char* receivedString = value.c_str();
            long long SOCsend = atoi(receivedString);
            // Print the received string
            Serial.print("Received String: ");
            Serial.println(receivedString);
            
            AHmillinow=0;
            AHmillinow = AHmillimax / 100 * SOCsend;

            Serial.print("SOC NOW in Amperemillisec ");
            Serial.println(AHmillinow);
            SPIFFS.remove("/dataAHNOW.txt");
            // Open SPIFFS file for writing
            File file = SPIFFS.open("/dataAHNOW.txt", FILE_WRITE);
            if (file) {
                // Write the received string to the file
                file.println(AHmillinow);
                file.close();
                Serial.println("String stored on SPIFFS");
            } else {
                Serial.println("Failed to open file for writing");
            }
        }
    }
};



//restart BLE Advertising after disconnect
class MyServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
    }
    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      // Restart advertising after disconnection
      pServer->startAdvertising();
    }
};




void setup() {
  // Start serial communication 
  Serial.begin(115200);
 

  // Set ESP32 to light sleep mode for 30s current test
  //esp_sleep_enable_timer_wakeup(1 * 30 * 1000000);
  //esp_light_sleep_start();
//-> 0,011A light_sleep  0,035A running no improvment with deepsleep


  initSPIFFS();
 
 //socstringfromfile = readFile(SPIFFS, socPath);
  //Serial.println(socstringfromfile);
  Serial.println("DATA from SPIFFS Storage: ");
  Serial.println("MAXAMPHOURS: ");  
  readAHMAXFromFile();
  Serial.println("LAST AMP: ");
  readAHNOWFromFile();


 

  Serial.print("\n\nDisplay INA Readings V1.0.8\n");
  Serial.print(" - Searching & Initializing INA devices\n");
  /************************************************************************************************
  ** The INA.begin call initializes the device(s) found with an expected ±1 Amps maximum current **
  ** and for a 0.1Ohm resistor, and since no specific device is given as the 3rd parameter all   **
  ** devices are initially set to these values.                                                  **
  ************************************************************************************************/
  devicesFound = INA.begin(MAXIMUM_AMPS, SHUNT_MICRO_OHM);  // Expected max Amp & shunt resistance
  while (devicesFound == 0) {
    Serial.println(F("No INA device found, retrying in 10 seconds..."));
    delay(10000);                                             // Wait 10 seconds before retrying
    devicesFound = INA.begin(MAXIMUM_AMPS, SHUNT_MICRO_OHM);  // Expected max Amp & shunt resistance
  }                                                           // while no devices detected
  Serial.print(F(" - Detected "));
  Serial.print(devicesFound);
  Serial.println(F(" INA devices on the I2C bus"));
  INA.setBusConversion(8500);             // Maximum conversion time 8.244ms
  INA.setShuntConversion(8500);           // Maximum conversion time 8.244ms
  INA.setAveraging(128);                  // Average each reading n-times
  INA.setMode(INA_MODE_CONTINUOUS_BOTH);  // Bus/shunt measured continuously
  INA.alertOnBusOverVoltage(true, 16000);  // Trigger alert if over 5V on bus



  // Create the BLE Device
  BLEDevice::init(bleServerName);

  // Create the BLE Server
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *inaService = pServer->createService(SERVICE_UUID);

  //ADD VOLT AMPS SOC Prodcast
  inaService->addCharacteristic(&INAAMPSCharacteristics);
  inaService->addCharacteristic(&INAVOLTSCharacterictics);
  inaService->addCharacteristic(&INASOCCharacterictics);

  
  //ADD WRITE MAX AMPHOURS
// Create an instance of MyCallbacks for MAX AMPHOURS characteristic
MyCallbacks *callbacksAMPHOURS = new MyCallbacks();
pCharacteristic = inaService->createCharacteristic(
    "38f2bd70-d659-4970-86c1-061e24700a6e",
    BLECharacteristic::PROPERTY_WRITE
);
pCharacteristic->setCallbacks(callbacksAMPHOURS);

// Create an instance of MyCallbacks for SOC AMPHOURS characteristic
MyCallbacks *callbacksSOC = new MyCallbacks();
pCharacteristic2 = inaService->createCharacteristic(
    "63d58a25-c22b-4586-b297-f1e310b7b0bc",
    BLECharacteristic::PROPERTY_WRITE
);
pCharacteristic2->setCallbacks(callbacksSOC);



 
  // Start the service
  inaService->start();

  // Start advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);

  pServer->getAdvertising()->start();
  Serial.println("Waiting a client connection to notify...");


}

void loop() {
  static char     sprintfBuffer[100];  // Buffer to format output
  static char     busVChar[8], shuntChar[10], busMAChar[10], busMWChar[10], SOCchar[10];  // Output buffers


    dtostrf(INA.getBusMilliVolts(0) / 1000.0, 7, 4, busVChar);      // Convert floating point to char VOLT
    dtostrf(INA.getShuntMicroVolts(0) / 1000.0, 9, 4, shuntChar);  // Convert floating point to char mA
    dtostrf(INA.getBusMicroAmps(0) / 1000.0 *-1, 9, 4, busMAChar);     // Convert floating point to char
    dtostrf(INA.getBusMicroWatts(0) / 10000.0, 9, 4, busMWChar);    // Convert floating point to char
    sprintf(sprintfBuffer, "%2d %3d %s %sV %smV %smA %smW\n", 0 + 1, INA.getDeviceAddress(0),
    INA.getDeviceName(0), busVChar, shuntChar, busMAChar, busMWChar);

    dtostrf(SOC,6,2,SOCchar);
    INAAMPSCharacteristics.setValue(busMAChar);
    INAVOLTSCharacterictics.setValue(busVChar);
    INASOCCharacterictics.setValue(SOCchar);

             SOC = (float)AHmillinow/(float)AHmillimax*100.0; //just Long long to float 
              // SOC = AHmillinow/AHmillimax*100.0; //just Long long to float 


    unsigned long currentMillis = millis(); // Get the current time in milliseconds
     
    long long CurrentCapture = (INA.getBusMicroAmps(0) / 1000.00 *-1)*(currentMillis - previousMillis);
    AHmillinow = AHmillinow + CurrentCapture;

    previousMillis = currentMillis;

    // store SOC intervall
    if (currentMillis - previousMillisSAVE >= 60000) {
        // Update the previous time
        previousMillisSAVE = currentMillis;
        Serial.print("last current reading milliAMPS: ");
        Serial.println(INA.getBusMicroAmps(0) / 10000.00);
             
             
             
             Serial.print("SOC:");
             Serial.println(SOC);
                     // Call your function here
       writeAHNOWToFile();
       //Serial.println("runtime:");
       //Serial.println(millis());
    }




  }



