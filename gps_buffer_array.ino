//=========================
// Libraries
//=========================
#include <BLEDevice.h>
#include <BLEServer.h>
#include <TinyGPSPlus.h>

TinyGPSPlus gps;
double latitude;
double longitude;
const int ledPin = 2;

//=========================
// Compiler Constants
//=========================
// SAMPLE Service
#define SAMPLE_SERVICE_UUID "cb0f22c6-1000-4737-9f86-1c33f4ee9eea"
#define SAMPLE_CHARACTERISTIC_UUID "cb0f22c6-1001-41a0-93d4-9025f8b5eafe"

//=========================
// BLE Server
//=========================
BLEServer *pServer;
// Characteristics: Fixed String
BLECharacteristic *fixedStringCharacteristic;

//=========================
// Global Variables
//=========================
bool client_is_connected = false;
String coordBuffer[500];
int size = -1;

//=========================
// Function Headers
//=========================
void setupBLEServer(void);
void setupSampleService(void);
void setupAdvertisementData(void);
void notifyData(void);


//=========================
// Classes
//=========================
// BLE Callbacks
class BaseBLEServerCallbacks : public BLEServerCallbacks
{
  void onConnect(BLEServer *pServer)
  {
    client_is_connected = true;
    Serial.println("Device connected");
  }

  void onDisconnect(BLEServer *pServer)
  {
    client_is_connected = false;
    Serial.println("Device disconnected");
    // Restart advertising
    pServer->getAdvertising()->start();
  }
};

class FixedStringCallback : public BLECharacteristicCallbacks
{
  void onRead(BLECharacteristic *pCharacteristic)
  {
    Serial.println("FixedStringCallback->onRead: Called");
    // Send fixed string
    pCharacteristic->setValue("KL10AE1994");
    Serial.println("Data Sent: KL10AE1994");
  }
};

void setup()
{
  // Setup USB Serial
  Serial.begin(9600);
  pinMode(ledPin, OUTPUT);

  // Setup BLE
  Serial.println("--Setting up BLE Server--");
  setupBLEServer();
  setupSampleService();
  setupAdvertisementData();
  Serial2.begin(9600, SERIAL_8N1, 17, 16);

  Serial.println("--Setup Complete--");
}

void loop()
{
  // Check if a client is connected to send data
  digitalWrite(ledPin, HIGH);
  // Turn the LED off by making the voltage LOW
  while (Serial2.available() > 0) {
    // Process incoming characters
    gps.encode(Serial2.read());
  }
  
  if (gps.location.isUpdated()) {
    Serial.print("Latitude: ");
    Serial.println(gps.location.lat(), 6); // Print latitude with 6 decimal places
    Serial.print("Longitude: ");
    Serial.println(gps.location.lng(), 6); // Print longitude with 6 decimal places
    if (client_is_connected)
    {
    // Optionally, notify data at desired intervals or based on specific conditions
      notifyData();
    }
    else{
      bufferData();
     }
  }

  digitalWrite(ledPin, LOW);

  delay(10000); // period of 10s
}

String getGpsCoord(){
  latitude = gps.location.lat();
  longitude = gps.location.lng();

  String stringCoord = String(latitude) + "," + String(longitude);
  return stringCoord;
  }

void notifyData(void)
{
  // Send coordinates as the notification
  String valueToSend;
  
  if(size >= 0){
    valueToSend = bufferToString();
    char buf[valueToSend.length() + 1];
    valueToSend.toCharArray(buf, sizeof(buf));

    fixedStringCharacteristic->setValue((uint8_t *)buf, sizeof(buf));
    fixedStringCharacteristic->notify();
    return;
    }

  
  valueToSend = getGpsCoord();

  char buf[valueToSend.length() + 1];

  valueToSend.toCharArray(buf, sizeof(buf));

  fixedStringCharacteristic->setValue((uint8_t *)buf, sizeof(buf));
  fixedStringCharacteristic->notify();
  Serial.println("Notified Data: ");
  Serial.println(buf);

}

void setupBLEServer(void)
{
  BLEDevice::init("ble_server_2");
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new BaseBLEServerCallbacks());

  BLEAddress mac_address = BLEDevice::getAddress();
  Serial.println("BLE server setup: SUCCESS");
  Serial.print("MAC: ");
  Serial.println(mac_address.toString().c_str());
}

void setupSampleService(void)
{
  BLEService *sampleService = pServer->createService(SAMPLE_SERVICE_UUID);

  // Fixed String Characteristic
  fixedStringCharacteristic = sampleService->createCharacteristic(
      SAMPLE_CHARACTERISTIC_UUID,
      BLECharacteristic::PROPERTY_READ |
          BLECharacteristic::PROPERTY_NOTIFY);
  fixedStringCharacteristic->setCallbacks(new FixedStringCallback());

  
  fixedStringCharacteristic->setValue("KL10AE1994");

  sampleService->start();
  Serial.println("Sample Service setup: SUCCESS");
}

void setupAdvertisementData(void)
{
  BLEAdvertising *pAdvertising = pServer->getAdvertising();
  pAdvertising->start();
  Serial.println("Advertisement setup: SUCCESS");
}

void bufferData(){
   String coord = getGpsCoord();
   if(size >= 500){
    Serial.println("Buffer Full");
    return;
    }
    size++;
    coordBuffer[size] = coord;
    Serial.println(coord);
    Serial.println("Buffer Successfull");
  }

String bufferToString(){
  String res = "";
  for(int i = 0; i < size; i++){
    res += coordBuffer[i];
    res += '+';
    }

  res += coordBuffer[size];
  size = -1;
  return res;
  }
