#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// ====== НАСТРОЙКИ ======
#define DEVICE_NAME "ESP32-Relay"

const int RELAY1_PIN = 2;  // D2
const int RELAY2_PIN = 4;  // D4
const int LED_PIN    = 15; // D15 — индикатор поиска/подключения (отдельный от реле пин!)

// Если реле включается по HIGH, а не по LOW — поменяйте местами значения
const int RELAY_ON  = LOW;
const int RELAY_OFF = HIGH;

const unsigned long LED_BLINK_INTERVAL_MS = 300; // скорость мигания при поиске
// =======================

unsigned long lastBlinkTime = 0;
bool ledState = false;

#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

BLECharacteristic *pTxCharacteristic;
bool deviceConnected = false;

bool relay1State = false;
bool relay2State = false;

void sendStatus() {
  String msg = String(relay1State ? "1:ON" : "1:OFF") + "," + String(relay2State ? "2:ON" : "2:OFF");
  pTxCharacteristic->setValue(msg.c_str());
  pTxCharacteristic->notify();
}

class ServerCallbacks: public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    deviceConnected = true;
    digitalWrite(LED_PIN, HIGH); // подключено — горит ровно
    delay(300);
    sendStatus();
  }
  void onDisconnect(BLEServer* pServer) {
    deviceConnected = false;
    pServer->getAdvertising()->start();
  }
};

class RxCallbacks: public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    String cmd = pCharacteristic->getValue().c_str();
    cmd.trim();
    cmd.toLowerCase();

    if (cmd == "1on")  { relay1State = true;  digitalWrite(RELAY1_PIN, RELAY_ON); }
    else if (cmd == "1off") { relay1State = false; digitalWrite(RELAY1_PIN, RELAY_OFF); }
    else if (cmd == "2on")  { relay2State = true;  digitalWrite(RELAY2_PIN, RELAY_ON); }
    else if (cmd == "2off") { relay2State = false; digitalWrite(RELAY2_PIN, RELAY_OFF); }
    else if (cmd == "status") { }

    sendStatus();
  }
};

void setup() {
  Serial.begin(115200);

  pinMode(RELAY1_PIN, OUTPUT);
  pinMode(RELAY2_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(RELAY1_PIN, RELAY_OFF);
  digitalWrite(RELAY2_PIN, RELAY_OFF);
  digitalWrite(LED_PIN, LOW);

  BLEDevice::init(DEVICE_NAME);
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new ServerCallbacks());

  BLEService *pService = pServer->createService(SERVICE_UUID);

  pTxCharacteristic = pService->createCharacteristic(
                        CHARACTERISTIC_UUID_TX,
                        BLECharacteristic::PROPERTY_NOTIFY);
  pTxCharacteristic->addDescriptor(new BLE2902());

  BLECharacteristic *pRxCharacteristic = pService->createCharacteristic(
                        CHARACTERISTIC_UUID_RX,
                        BLECharacteristic::PROPERTY_WRITE);
  pRxCharacteristic->setCallbacks(new RxCallbacks());

  pService->start();
  pServer->getAdvertising()->start();

  Serial.println("BLE запущен: " DEVICE_NAME);
}

void loop() {
  if (!deviceConnected) {
    unsigned long now = millis();
    if (now - lastBlinkTime >= LED_BLINK_INTERVAL_MS) {
      lastBlinkTime = now;
      ledState = !ledState;
      digitalWrite(LED_PIN, ledState ? HIGH : LOW);
    }
  }
  delay(20);
}