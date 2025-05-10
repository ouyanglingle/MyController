#include <menutask.h>
#include <menu.h>
#include <USB.h>
#include <USBHIDMouse.h>

#define PLAYER_SIZE 12
#define GRAVITY 0.5       // 重力强度
#define JUMP_FORCE -8     // 跳跃力度
#define OBSTACLE_WIDTH 20 // 障碍物宽度
#define OBSTACLE_SPEED 3  // 障碍物移动速度（X轴递减值）

