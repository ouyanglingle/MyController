#ifndef INPUT_H
#define INPUT_H

#include "Arduino.h"
// 按钮的引脚编号
#define UP_PIN 1
#define DOWN_PIN 2
#define ENTER_PIN 10
// 摇杆的引脚编号
//       ^ y
//       |
//       |
//-------|------> x
#define JOYSTICK_X_PIN 20
#define JOYSTICK_Y_PIN 19

#define RED_LED 35 // 低电平点亮
#define GREEN_LED 36

enum KEY_STATE
{
    KEY_RELEASE = 0, // 未按下
    KEY_PRESS,       // 按下
    KEY_LONG_PRESS,  // 一直长按
};

typedef struct
{
    uint8_t pin;               // 按键引脚编号
    uint8_t last_state;        // 上一次读取的按键状态
    uint8_t stable_state;      // 消抖后的稳定状态
    uint8_t last_stable_state; // 上一次的稳定状态（用于检测松开事件）
    uint32_t last_check_time;  // 上一次状态变化的时间戳
    uint8_t debouncing;        // 是否正在消抖
} Debounce_def;

enum JOYSTICK_STATE
{
    IDLE,
    CENTER,  // 摇杆处于中心位置
    XLEFT,   // 向左推
    XRIGHT,  // 向右推
    YBEFORE, // 向前推
    YAFTER,  // 向后推
};

typedef struct
{
    int16_t x_value;
    int16_t y_value;
} Joystick_def;

extern Joystick_def joystick;

void Key_Init(void);
KEY_STATE getKeyState(uint8_t pin);

void Joystick_Init();

void JoystickStateUpdate();



#endif