#ifndef BLUETOOTH_H
#define BLUETOOTH_H

#include <Arduino.h>
#include <NimBLEDevice.h>

#include "env.h"

class Bluetooth {
  public:
    Bluetooth();

    void begin();
    void send(const String& message);

  private:
    NimBLEServer* _server;
    NimBLECharacteristic* _rxCharacteristic;
    NimBLECharacteristic* _txCharacteristic;

    class RxCallbacks : public NimBLECharacteristicCallbacks {
      public:
        explicit RxCallbacks(Bluetooth* bluetooth);

        void onWrite(
          NimBLECharacteristic* characteristic,
          NimBLEConnInfo& connInfo
        ) override;

      private:
        Bluetooth* _bluetooth;
    };

    class ServerCallbacks : public NimBLEServerCallbacks{
      public:
        explicit ServerCallbacks(Bluetooth* bluetooth);

        void onConnect(
          NimBLEServer* server,
          NimBLEConnInfo& connInfo
        ) override;

        void onDisconnect(
          NimBLEServer* server,
          NimBLEConnInfo& connInfo,
          int reason
        ) override;

      private:
        Bluetooth* _bluetooth;
    };
};

#endif