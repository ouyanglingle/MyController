#include "mytask.h"

#include <menu.h>
void ShowJoystikValue()
{
    bf.fillScreen(BF_BG_COLOR);

    while (1)
    {
        bf.fillScreen(BF_BG_COLOR);
        bf.drawRect(0, 0, 127, 127, TFT_RED);
        bf.drawRect(1, 1, 125, 125, TFT_RED);
        bf.fillCircle(map(joystick.x_value, -255, 255, 0, 127), map(joystick.y_value, -255, 255, 127, 0), 5, TFT_GREEN);
        bf.pushSprite(0, 0);

        if (getKeyState(ENTER_PIN) == KEY_PRESS)
        {
            break;
        }
    }
}

#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiServer.h>

#define AP_SSID "ESP32S3_CTRL"
#define AP_PASS "12345678"
#define PORT 6700

WiFiServer server(PORT);
PROGMEM IPAddress IP;
void TCP_Init()
{
    // 创建AP热点
    WiFi.softAP(F(AP_SSID), F(AP_PASS));
    IP = WiFi.softAPIP();
    bf.drawString(F("AP IP: "), 0, 65);
    bf.setCursor(30, 65);
    bf.print(IP);
    bf.setCursor(0, 85);
    server.begin(); // 启动TCP服务器
    bf.println(F("TCP Server Started"));
    bf.pushSprite(0, 0);
    delay(2000);
}

/*
server.available()：监听并返回一个已连接的 WiFiClient 对象，若无客户端连接则返回空对象。
WiFiClient client：定义一个客户端对象，用于后续与该客户端进行通信。
*/
void TCP_tick()
{
    bf.setTextSize(1);
    while (1)
    {
        bf.fillScreen(BF_BG_COLOR);
        WiFiClient client = server.available();
        if (client)
        {
            while (client.connected())
            {
                bf.setCursor(0, 75);
                bf.print(client.localIP());
                bf.setCursor(0, 85);
                bf.print(client.localPort());

                if (client.available())
                {
                    bf.fillScreen(BF_BG_COLOR);
                    static uint8_t data[64] = {0};
                    // String data = client.readStringUntil('\n');
                    // data.trim();
                    bf.setCursor(0, 0);
                    Serial.write(data, client.read(data, 100));
                    Serial.println();
                    bf.print(client.read(data, 100));
                    // bf.drawString(data, 0, 0);
                }
                bf.pushSprite(0, 0);
                digitalWrite(36, HIGH);
            }
        }
        else
            bf.setCursor(0, 230);
        bf.println(F("Client Lost!"));

        digitalWrite(36, !digitalRead(36));

        bf.pushSprite(0, 0);
        if (getKeyState(ENTER_PIN) == KEY_PRESS)
        {
            client.stop();
            break;
        }
    }
}

#include "packet_process.h"

// 为Serial2分配解析数据包的缓冲区
#define BUFFER_SIZE 64
uint8_t serial2Buffer[BUFFER_SIZE];
CommandBuffer_T cmdBuffer;
/**
 * 指令处理函数
 * @param cmd 指令数据指针
 * @param len 指令长度
 */
void processCommand(uint8_t *cmd, uint16_t len)
{
    // 回显原始指令（调试用）
    // Serial.print("[RX] ");
    // for (int i = 0; i < len; i++)
    // {
    //     Serial.print(cmd[i], HEX);
    //     Serial.print(" ");
    // }
    // Serial.println();
    // 基本格式验证
    if (cmd[0] != 0xAA || len != cmd[1])
    {
        Serial.println("Format Error");
        return;
    }
    // 遍历指令内容（从第2字节开始）
    for (uint16_t i = 2; i < len - 1; i++)
    {
        // 安全边界检查
        if (i + 1 >= len - 1)
            break;

        switch (cmd[2])
        {
            bf.setCursor(10, 10);
            bf.print(F("circle_sta: "));
            bf.setCursor(10, 25);
        case 1:
            bf.print(F("find_circle"));
            break;
        case 2:
            bf.print(F("Sigan_line"));
            break;
        case 3:
            bf.print(F("will_go_circle"));
            break;
        case 4:
            bf.print(F("in_circle"));
            break;
        case 5:
            bf.print(F("go_str"));
            break;
        case 6:
            bf.print(F("out_circle"));
            break;
        }

        switch (cmd[3])
        {
            bf.setCursor(10, 35);
            bf.print(F("circle_sta: "));
            bf.setCursor(10, 45);
        case 1:
            bf.print(F("Left"));
            break;
        case 2:
            bf.print(F("Right"));
            break;
        }
    }
}

void DebugReceive()
{

    // 初始化指令配置
    CommandConfig config = {
        .header = 0xAA, // 包头标识
        .min_length = 4 // 最小指令长度
    };
    CommandBuffer_Init(&cmdBuffer, serial2Buffer, BUFFER_SIZE, config);

    while (1)
    {
        // Arduino固件只能主机收串口数据
        while (Serial2.available() > 0)
        {
            uint8_t data = Serial2.read();
            CommandBuffer_Write(&cmdBuffer, &data, 1); // 单字节写入
        }
        // 使用了一个独立的缓冲区 command[32] 来存储解析出的指令数据。
        // 解析结果不会直接覆盖 serialBuffer，而是存储在 command 数组中。
        // 这种方式适合需要将解析结果与原始缓冲区分离的场景。
        // 当 serialBuffer 中可能包含多种协议的数据包时，
        // 这种方式可以先将符合某种协议的指令提取到独立缓冲区中进行处理，而不会影响其他协议的数据。
        uint8_t command[20];
        uint16_t cmdLen = CommandBuffer_Parse(&cmdBuffer, command, sizeof(command));

        if (cmdLen > 0)
        {
            bf.fillScreen(BF_BG_COLOR);
            processCommand(command, cmdLen); // 处理有效指令
        }
        bf.pushSprite(0, 0);
        if (getKeyState(ENTER_PIN) == KEY_PRESS)
        {
            break;
        }
    }
}
