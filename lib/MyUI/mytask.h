#ifndef MYTASK_H
#define MYTASK_H 

#include "WiFi.h"
#include <WiFiClient.h>
#include <WiFiServer.h>

extern IPAddress IP;

void ShowJoystikValue();
void TCP_Init();
void TCP_tick();

void DebugReceive();

#endif

