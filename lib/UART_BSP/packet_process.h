#ifndef PACKET_PROCESS_H
#define PACKET_PROCESS_H

#include <Arduino.h>
#include <stdint.h>
#include <string.h>
// 环形缓冲区实现

// 指令配置参数（可以拓展，所以保留了）
typedef struct
{
    uint8_t header;     // 包头标识（默认0xAA）
    uint8_t min_length; // 最小指令长度
} CommandConfig;

// 循环缓冲区结构体
typedef struct
{
    uint8_t *buffer;      // 数据存储指针
    uint8_t buf_size;    // 缓冲区总大小
    uint8_t read_index;  // 读位置索引
    uint8_t write_index; // 写位置索引
    CommandConfig config; // 协议配置参数
} CommandBuffer_T;

/// @brief 初始化缓冲区，并为其分配外部提供的缓冲区。
/// @param cbuf 
/// @param ext_buf 
/// @param buf_size 
/// @param config // 指令配置参数
void CommandBuffer_Init(CommandBuffer_T *cbuf, uint8_t *ext_buf, uint8_t buf_size, CommandConfig config);

/// @brief 写入数据到缓冲区
/// @param cbuf 
/// @param data 
/// @param length 
/// @return 是否成功将数据写入缓冲区
uint8_t CommandBuffer_Write(CommandBuffer_T *cbuf, const uint8_t *data, uint8_t length);

/// @brief 解析完整指令
/// @param cbuf 
/// @param output 
/// @param max_output 
/// @return 表示成功解析出该指令包的长度
uint8_t CommandBuffer_Parse(CommandBuffer_T *cbuf, uint8_t *output, uint8_t max_output);
#endif
/******************** 使用示例 ​********************/
// #include <packet_process.h>
// // 为Serial分配解析数据包的缓冲区
// #define BUFFER_SIZE 64
// uint8_t serialBuffer[BUFFER_SIZE];
// CommandBuffer_T cmdBuffer;
// void processCommand(uint8_t *cmd, uint8_t len);
// void setup()
// {
// 	Serial.begin(115200);
// 	// 初始化指令配置
// 	CommandConfig config = {
// 		.header = 0xAA, // 包头标识
// 		.min_length = 4 // 最小指令长度
// 	};
// 	CommandBuffer_Init(&cmdBuffer, serialBuffer, BUFFER_SIZE, config);
// }
// void loop()
// {
// 	// Arduino固件只能主机收串口数据
// 	while (Serial.available() > 0)
// 	{
// 		uint8_t data = Serial.read();
// 		CommandBuffer_Write(&cmdBuffer, &data, 1); // 单字节写入
// 	}
// 	// 使用了一个独立的缓冲区 command[32] 来存储解析出的指令数据。
// 	// 解析结果不会直接覆盖 serialBuffer，而是存储在 command 数组中。
// 	// 这种方式适合需要将解析结果与原始缓冲区分离的场景。
// 	// 当 serialBuffer 中可能包含多种协议的数据包时，
// 	// 这种方式可以先将符合某种协议的指令提取到独立缓冲区中进行处理，而不会影响其他协议的数据。
// 	uint8_t command[32];
// 	uint8_t cmdLen = CommandBuffer_Parse(&cmdBuffer, command, sizeof(command));

// 	if (cmdLen > 0)
// 	{
// 		processCommand(command, cmdLen); // 处理有效指令
// 	}
// }
// /**
//  * 指令处理函数
//  * @param cmd 指令数据指针
//  * @param len 指令长度
//  */
// void processCommand(uint8_t *cmd, uint8_t len)
// {
// 	// 回显原始指令（调试用）
// 	Serial.print("[RX] ");
// 	for (int i = 0; i < len; i++)
// 	{
// 		Serial.print(cmd[i], HEX);
// 		Serial.print(" ");
// 	}
// 	Serial.println();
// 	// 基本格式验证
// 	if (cmd[0] != 0xAA || len != cmd[1])
// 	{
// 		Serial.println("Format Error");
// 		return;
// 	}
//     // 遍历指令内容（从第2字节开始）
// 	for (uint8_t i = 2; i < len - 1;)
// 	{
// 		// 安全边界检查
// 		if (i + 1 >= len - 1)
// 			break;
// 		uint8_t ledID = cmd[i++];
// 		uint8_t state = cmd[i++];
// 		// 执行控制操作
// 		switch (ledID)
// 		{
// 		case 0x01: // 红灯
// 			Serial.print("RED: ");
// 			Serial.println(state ? "ON" : "OFF");
// 			break;
// 		case 0x02: // 绿灯
// 			Serial.print("GREEN: ");
// 			Serial.println(state ? "ON" : "OFF");
// 			break;
// 		case 0x03: // 蓝灯
// 			Serial.print("BLUE: ");
// 			Serial.println(state ? "ON" : "OFF");
// 			break;
// 		default:
// 			Serial.print("Unknown LED ID: 0x");
// 			Serial.println(ledID, HEX);
// 		}
// 	}
// }