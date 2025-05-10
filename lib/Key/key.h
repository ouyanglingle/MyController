#ifndef KEY_H
#define KEY_H

#include "Arduino.h"

#define RIGHT_PIN 8
#define LEFT_PIN 9
#define ENTER_PIN 10

enum KEY_STATE
{
    KEY_RELEASE = 0, // 未按下
    KEY_PRESS, // 按下
    KEY_LONG_PRESS, // 一直长按
};

typedef struct
{
    uint16_t pin;               // 按键引脚编号
    uint8_t last_state;         // 上一次读取的按键状态
    uint8_t stable_state;       // 消抖后的稳定状态
    uint8_t last_stable_state;  // 上一次的稳定状态（用于检测松开事件）
    uint32_t last_check_time;   // 上一次状态变化的时间戳
    uint8_t debouncing;         // 是否正在消抖
} DebounceInfo;


void Key_Init(void);

KEY_STATE getKeyState(uint8_t pin);

#endif

