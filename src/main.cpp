#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#define LED_PIN 8
#define ADC_PIN 3
#define N_MUESTRAS 32

#define SERVICE_UUID "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define RX_UUID      "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define TX_UUID      "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

BLECharacteristic *txChar;
bool conectado = false;

void enviar(const String &msg) {
  if (!conectado) return;
  txChar->setValue(msg.c_str());
  txChar->notify();
}

float medirVoltios() {
  uint32_t suma = 0;
  for (int i = 0; i < N_MUESTRAS; i++) suma += analogReadMilliVolts(ADC_PIN);
  return (suma / N_MUESTRAS) / 1000.0;
}

class ServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer *s) override {
    conectado = true;
    Serial.println("Celular conectado");
  }
  void onDisconnect(BLEServer *s) override {
    conectado = false;
    Serial.println("Celular desconectado");
    BLEDevice::startAdvertising();
  }
};

class RxCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *c) override {
    String rx = c->getValue().c_str();
    rx.trim();
    if (rx.equalsIgnoreCase("on"))  { digitalWrite(LED_PIN, LOW);  enviar("LED on\n"); }
    else if (rx.equalsIgnoreCase("off")) { digitalWrite(LED_PIN, HIGH); enviar("LED off\n"); }
  }
};

void setup() {
  Serial.begin(115200);
  delay(2000);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);

  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);

  BLEDevice::init("C3-SuperMini");
  BLEServer *server = BLEDevice::createServer();
  server->setCallbacks(new ServerCallbacks());
  BLEService *service = server->createService(SERVICE_UUID);

  txChar = service->createCharacteristic(TX_UUID, BLECharacteristic::PROPERTY_NOTIFY);
  txChar->addDescriptor(new BLE2902());

  BLECharacteristic *rxChar = service->createCharacteristic(
      RX_UUID, BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR);
  rxChar->setCallbacks(new RxCallbacks());

  service->start();
  BLEAdvertising *adv = BLEDevice::getAdvertising();
  adv->addServiceUUID(SERVICE_UUID);
  adv->setScanResponse(true);
  BLEDevice::startAdvertising();

  Serial.println("Listo, conectate desde nRF Connect");
}

void loop() {
  static uint32_t t0 = 0;
  if (millis() - t0 > 500) {          // cada 0.5 s
    t0 = millis();
    float v = medirVoltios();
    String msg = "V=" + String(v, 3);
    Serial.println(msg);              // también en la PC
    enviar(msg);                      // al celular
  }
}