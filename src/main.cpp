

#include <menu.h>
#include "mypicture.h"

void setup()
{
    Serial.begin(115200, SERIAL_8N1, -1, -1);
    pinMode(0, OUTPUT);
    pinMode(7, OUTPUT);
    digitalWrite(7, LOW);
    digitalWrite(0, HIGH);
    Key_Init();
    Menu_Init();
}

uint8_t j = 0;

void loop()
{
    Menu_Key_Handle();
}