#pragma once

#include "NimBLEServer.h"

class BleInterfaceable
{
public:
    virtual ~BleInterfaceable() = default;
    virtual void createServiceAndCharacteristics(NimBLEServer* server) = 0;
};