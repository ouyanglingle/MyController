#pragma once
#ifndef MENU_H
#define MENU_H

#include <TFT_eSPI.h>
#include <key.h>
#include "pid.h"
#include <menutask.h>
#include <esp_heap_caps.h>

#define BF_BG_COLOR TFT_BLACK        // 主菜单背景颜色
#define BF_FG_COLOR TFT_GOLD         // 主菜单字体颜色
#define CURSOR_COLOR TFT_RED         // 复选框颜色
#define PROGRESS_BAR_COLOR TFT_GREEN // 进度条颜色

extern TFT_eSprite bf;

typedef enum
{
    MAIN_MENU,
    SECOND_MENU,
} MenuLevel_E;

void Menu_Init(void);
void Draw_Menu(void);
void Menu_Key_Handle(void);

#endif
