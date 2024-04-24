/*********
PEKAWAY VAN PI 
esp32-c3 shunt 

This code receives the size of the battery in AH (utf-8) and also the SOC(utf-8) via BLE and then calculates the new SOC according to the current. You can discharge and recharge.
The status is readable via BLE. -> please check the BLE PYTHON SCRIPTS.

The SOC is stored every 60 seconds to SPIFFS file system as backup.

You can only set up the battery 5 minutes after power up -> just for safety

*********/

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <Wire.h>
#include <INA.h>  // Zanshin INA Library
#include "SPIFFS.h"


//PIXEL LED
#include <Adafruit_NeoPixel.h> // Include the Neopixel library
#define LED_PIN     4          // Define the GPIO pin for the Neopixel
#define LED_COUNT   1          // Define the number of Neopixels
#define BRIGHTNESS_SCALE 255   // Max brightness for Neopixels
#define BLINK_INTERVAL 1000 // Blink interval in milliseconds
#define BLINK_DURATION 120000 // Blink duration in milliseconds
unsigned long blinkStartTime = 0; // Variable to store the start time of blinking

Adafruit_NeoPixel pixels(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800); // Initialize Neopixel object

#define uS_TO_S_FACTOR 1000000  /* Conversion factor for microseconds to seconds */

const uint32_t SERIAL_SPEED{115200};     ///< Use fast serial speed
const uint32_t SHUNT_MICRO_OHM{375};  ///< Shunt resistance in Micro-Ohm, e.g. 100000 is 0.1 Ohm //75MV 200A
const uint16_t MAXIMUM_AMPS{1022};          ///< Max expected amps, clamped from 1A to a max of 1022A
uint8_t        devicesFound{0};          ///< Number of INAs found

unsigned long currentTime = millis();
unsigned long previousTime = 0;
const long timeoutTime = 2000;
int16_t receivedValue = 0;

INA_Class      INA;                      ///< INA class instantiation to use EEPROM

#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

// BLE server name
#define bleServerName "Pekaway Shunt"

long long SOC;
long VOLT;
long AMPS;

long long AHmillimax;
long long AHmillinow;
long long AHmillinowlast = 0; 
long long AHmillinowChangeToStore;


long microampsmillis;

BLECharacteristic *pCharacteristic; //MAXAMPHOURS
BLECharacteristic *pCharacteristic2; //SOC

bool deviceConnected = false;

// Timer variables
unsigned long previousMillis = 0;  // Stores the timestamp of the previous loop
unsigned long previousMillisSAVE = 0;
bool setupclosed = false; 

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/
#define SERVICE_UUID "91bad492-b950-4226-aa2b-4ede9fa42f59"

BLECharacteristic INAAMPSCharacteristics("d2e8ede8-9b31-4478-9fd6-75845cb68b1d", BLECharacteristic::PROPERTY_READ);
BLECharacteristic INAVOLTSCharacterictics("ff100f8c-1266-4309-b472-76bc25e4a62f", BLECharacteristic::PROPERTY_READ);
BLECharacteristic INASOCCharacterictics("459e9ea4-a335-4a3f-b838-46788fd6bbe4", BLECharacteristic::PROPERTY_READ);


void setPixelColor(){
   // Calculate current brightness based on current value
   

    float currentMicroAmps = INA.getBusMicroAmps(0); // Get current in microamperes
    float currentAmps = currentMicroAmps / 1000000.0; // Convert microamperes to amperes

     uint8_t brightness = map(abs(currentAmps), 0, 200, 0, BRIGHTNESS_SCALE);

    

    // Get current value
    float current = INA.getBusMicroAmps(0) / 1000.0; // Convert to milliamps

    brightness = brightness*10;

    // Set LED color based on current value
    if (current < 0) { // Positive current (charging)
        pixels.setPixelColor(0, pixels.Color(0, brightness, 0)); // Green color
    } else if (current > 0) { // Negative current (discharging)
        pixels.setPixelColor(0, pixels.Color(brightness, 0, 0)); // Red color
    } else { // No current
        pixels.setPixelColor(0, pixels.Color(0, 0, 0)); // Turn off LED
    }

    pixels.show(); // Update Neopixel
}

void startBlinking() {
    blinkStartTime = millis(); // Record the start time of blinking
}

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
        Serial.print("From File as String: ");
        Serial.println(line);
        Serial.print("From File as LONG LONG: ");
         Serial.println(AHmillimax);
    }

    // Close the file
    file.close();
}

void readAHNOWFromFile() {
  File file = SPIFFS.open("/dataAHNOW.txt", FILE_READ);
  if (!file) {
    Serial.println("Failed to open file for reading");
    return;
  }

  while (file.available()) {
    String line = file.readStringUntil('\n');
    Serial.print("read AHNOWFROM FILE - String:");
    Serial.println(line);
    AHmillinow = atoll(line.c_str());
    AHmillinowlast = AHmillinow;
    Serial.print("AHmillinow as a VALUE long long :");
    Serial.println(AHmillinow);
  }
  file.close();
}

void writeAHNOWToFile() {
  SPIFFS.remove("/dataAHNOW.txt");
  File file = SPIFFS.open("/dataAHNOW.txt", FILE_WRITE );
        
  if (file) {
    file.println(AHmillinow);
    file.close();
    Serial.println("AHmillinow (milliamperemillisecondes):");
    Serial.print(AHmillinow);
    Serial.println("   stored on SPIFFS");
  } else {
    Serial.println("Failed to open file for writing");
  }
}

class MyCallbacks : public BLECharacteristicCallbacks {
   void onWrite(BLECharacteristic *pCharacteristic) {
        BLEUUID characteristicUUID = pCharacteristic->getUUID();
        if (characteristicUUID.equals(BLEUUID("38f2bd70-d659-4970-86c1-061e24700a6e"))) {
           if (setupclosed != true) {
            Serial.println("Received MAX AMPHOURS");
            std::string value(pCharacteristic->getValue().c_str());
            const char* receivedString = value.c_str();
            long long MAX = atoi(receivedString);
            AHmillimax = 0;
            AHmillimax = MAX * 60 * 60 * 1000 * 1000; // Convert to milliampere, hours to milliseconds
            Serial.print("Received String: ");
            Serial.println(receivedString);
            Serial.print("in milliamperemilliseconds: ");
            Serial.println(AHmillimax);

            SPIFFS.remove("/dataAHMAX.txt");
            File file = SPIFFS.open("/dataAHMAX.txt", FILE_WRITE);
            if (file) {
                file.println(receivedString);
                file.close();
                Serial.println("String stored on SPIFFS");
            } else {
                Serial.println("Failed to open file for writing");
            }
           }
        } else if (characteristicUUID.equals(BLEUUID("63d58a25-c22b-4586-b297-f1e310b7b0bc"))) {
            if (setupclosed != true) {
            Serial.println("Received SOC percent");
            std::string value(pCharacteristic->getValue().c_str());
            const char* receivedString = value.c_str();
            long long SOCsend = atoi(receivedString);
            Serial.print("Received String: ");
            Serial.println(receivedString);
            AHmillinow = 0;
            AHmillinow = AHmillimax / 100 * SOCsend;
            Serial.print("SOC NOW in milliamperemillisec ");
            Serial.println(AHmillinow);
            SPIFFS.remove("/dataAHNOW.txt");
            File file = SPIFFS.open("/dataAHNOW.txt", FILE_WRITE);
            if (file) {
                file.println(AHmillinow);
                file.close();
                Serial.println("String stored on SPIFFS");
            } else {
                Serial.println("Failed to open file for writing");
            }
            }
        }
    }
};

class MyServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
    }
    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      pServer->startAdvertising();
    }
};

void setup() {
  Serial.begin(115200);
  setupclosed = false; 

  initSPIFFS();

  Serial.println("DATA from SPIFFS Storage: ");
  Serial.println("MAXAMPHOURS: ");  
  readAHMAXFromFile();
  readAHNOWFromFile();

  Serial.print("\n\nDisplay INA Readings V1.0.8\n");
  Serial.print(" - Searching & Initializing INA devices\n");

  devicesFound = INA.begin(MAXIMUM_AMPS, SHUNT_MICRO_OHM);
  while (devicesFound == 0) {
    Serial.println(F("No INA device found, retrying in 10 seconds..."));
    delay(10000);
    devicesFound = INA.begin(MAXIMUM_AMPS, SHUNT_MICRO_OHM);
  }
  Serial.print(F(" - Detected "));
  Serial.print(devicesFound);
  Serial.println(F(" INA devices on the I2C bus"));
  INA.setBusConversion(8500);
  INA.setShuntConversion(8500);
  INA.setAveraging(128);
  INA.setMode(INA_MODE_CONTINUOUS_BOTH);
  INA.alertOnBusOverVoltage(true, 16000);

  BLEDevice::init(bleServerName);
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  BLEService *inaService = pServer->createService(SERVICE_UUID);

  inaService->addCharacteristic(&INAAMPSCharacteristics);
  inaService->addCharacteristic(&INAVOLTSCharacterictics);
  inaService->addCharacteristic(&INASOCCharacterictics);

  MyCallbacks *callbacksAMPHOURS = new MyCallbacks();
  pCharacteristic = inaService->createCharacteristic(
    "38f2bd70-d659-4970-86c1-061e24700a6e",
    BLECharacteristic::PROPERTY_WRITE
  );
  pCharacteristic->setCallbacks(callbacksAMPHOURS);

  MyCallbacks *callbacksSOC = new MyCallbacks();
  pCharacteristic2 = inaService->createCharacteristic(
    "63d58a25-c22b-4586-b297-f1e310b7b0bc",
    BLECharacteristic::PROPERTY_WRITE
  );
  pCharacteristic2->setCallbacks(callbacksSOC);

  inaService->start();

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pServer->getAdvertising()->start();
  Serial.println("Waiting a client connection to notify...");
  AHmillinowlast = AHmillinow;
  AHmillinowChangeToStore = 360000000; // around 0,1AH  -> not store store only if there is a change. 

  startBlinking();

            pixels.clear(); // Clear any existing colors
            pixels.setPixelColor(0, pixels.Color(0, 0, 100));
            pixels.show();
}

void loop() {

// if (millis() - blinkStartTime < BLINK_DURATION) {
//         // Determine whether to turn on or off based on time
//         if ((millis() / BLINK_INTERVAL) % 2 == 0) {
//             // Turn Neopixel LED on (blue color)
//             pixels.clear(); // Clear any existing colors
//             pixels.setPixelColor(0, pixels.Color(10, 0, 0));
//             pixels.show();
//         } else {
//             // Turn Neopixel LED off
//             pixels.clear();
//             pixels.show();
//         }
//       } else {
//         // Blinking duration has elapsed, stop blinking
//         pixels.clear();
//         pixels.show();
//     }

  static char sprintfBuffer[100];
  static char busVChar[8], shuntChar[10], busMAChar[10], busMWChar[10], SOCchar[10];

  dtostrf(INA.getBusMilliVolts(0) / 1000.0, 7, 4, busVChar);
  dtostrf(INA.getShuntMicroVolts(0) / 1000.0, 9, 4, shuntChar);
  dtostrf(INA.getBusMicroAmps(0) / 1000.0 *-1, 9, 4, busMAChar);
  dtostrf(INA.getBusMicroWatts(0) / 10000.0, 9, 4, busMWChar);
  sprintf(sprintfBuffer, "%2d %3d %s %sV %smV %smA %smW\n", 0 + 1, INA.getDeviceAddress(0),
    INA.getDeviceName(0), busVChar, shuntChar, busMAChar, busMWChar);

  dtostrf(SOC,6,2,SOCchar);
  INAAMPSCharacteristics.setValue(busMAChar);
  INAVOLTSCharacterictics.setValue(busVChar);
  INASOCCharacterictics.setValue(SOCchar);

  SOC = (float)AHmillinow / (float)AHmillimax * 100.0;

  unsigned long currentMillis = millis();
  long long CurrentCapture = (INA.getBusMicroAmps(0) / 1000.00 *-1)*(currentMillis - previousMillis);
  AHmillinow = AHmillinow + CurrentCapture;

  previousMillis = currentMillis;

  if (currentMillis > 60000*2 && setupclosed==false) {
    setupclosed = true; 
    Serial.println("-----------------------Setup Closed---------------");
     pixels.clear(); // Clear any existing colors
     pixels.show();
  }

  if(SOC > 100)
    {
      SOC=100;
      AHmillinow = AHmillimax;
    }

      if(SOC < 0)
    {
      SOC=0;
      AHmillinow = 0;
    }



  if (currentMillis - previousMillisSAVE >= 30000) {
    previousMillisSAVE = currentMillis;
    Serial.print("last current reading milliAMPS: ");
    Serial.println(INA.getBusMicroAmps(0) / 10000.00);
    Serial.print("Runtime in Millis: ");
    Serial.println(currentMillis);
             
    Serial.print("SOC:");
    Serial.println(SOC);

    Serial.print("AHmillinowlast:");
    Serial.println(AHmillinowlast);

  //Store SOC in Spiffs 
  long long AHmilliChange = AHmillinowlast - AHmillinow; 
   if(AHmilliChange < 0)
   {
     AHmilliChange = AHmilliChange*-1; 
   }

    Serial.print("AHmilli change:");
    Serial.println(AHmilliChange);
    

    if(AHmilliChange > AHmillinowChangeToStore) {
       writeAHNOWToFile();
       AHmillinowlast = AHmillinow; 
      }
    else
     {
       Serial.println("- Not Stored - change smaller then 0,1AH");
     }
  }

   if (millis() - blinkStartTime > BLINK_DURATION)
   {
   setPixelColor();
   }
}
