#include <menu.h>

extern void TCP_Init();

void setup()
{
    Serial.begin(115200, SERIAL_8N1, RX, TX);
    Serial2.begin(115200, SERIAL_8N1, 18, 17);
    Key_Init();
    Menu_Init();
    TCP_Init();
    Draw_Menu();
}
void loop()
{
    Menu_Key_Handle();
}
