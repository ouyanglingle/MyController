#include <Arduino.h>
#include <WiFi.h>
#include <time.h>

#include <TFT_eSPI.h>
#include <Ticker.h>

#include "menu.h"
#include "key.h"
#include "pid.h"
#include "mypicture.h"

// 全局显示对象
TFT_eSPI tft = TFT_eSPI();          // 主显示对象
TFT_eSprite bf = TFT_eSprite(&tft); // 主显示全缓冲区（240x240）

typedef struct MenuItem MenuItem;

enum TITLE_CLASS
{
    PRIMARY = 0,  // 一级标题
    SECONDARY,    // 二级标题
    NORMAL_TEXT,  // 普通文本
    SYS_FUNC_TEXT // 系统功能文本(如返回，打断等)
};

struct MenuItem
{
    const char *name;         // 菜单项名称
    void (*function)();       // 菜单项对应的函数
    MenuItem *subMenu;        // 子菜单
    int subMenuCount;         // 子菜单数量
    TITLE_CLASS titleClass;   // 标题&文本类型
    const uint16_t *bmp_icon; // 菜单项图标
};

void Menu_Return();
void DrawMainMenuBOX();

// 定义子菜单项
MenuItem SettingMenu[] PROGMEM = {
    {"Example 1", NULL, NULL, 0, SECONDARY, NULL},
    {"Example 2", NULL, NULL, 0, NORMAL_TEXT, NULL},
    {"Example 3", NULL, NULL, 0, NORMAL_TEXT, NULL},
    {"Example 4", NULL, NULL, 0, NORMAL_TEXT, NULL},
    {"Example 5", NULL, NULL, 0, NORMAL_TEXT, NULL},
    {"Example 6", NULL, NULL, 0, NORMAL_TEXT, NULL},
    {"Example 7", NULL, NULL, 0, NORMAL_TEXT, NULL},
    {"Example 8", NULL, NULL, 0, NORMAL_TEXT, NULL},
    {"Example 9", NULL, NULL, 0, NORMAL_TEXT, NULL},
    {"Example 10", NULL, NULL, 0, NORMAL_TEXT, NULL},
    {"Example 11", NULL, NULL, 0, NORMAL_TEXT, NULL},
    {"Example 12", NULL, NULL, 0, NORMAL_TEXT, NULL},
    {"Return", Menu_Return, NULL, 0, SYS_FUNC_TEXT, NULL},
};
// 主菜单项(1.3寸240x240屏幕最多水平放置3个100x100像素的图标)
MenuItem MainMenu[] PROGMEM = {
    {"SETTING1", NULL, SettingMenu, sizeof(SettingMenu) / sizeof(SettingMenu[0]), PRIMARY, SETTING1_PNG},
    {"SETTING2", NULL, NULL, 0, PRIMARY, SETTING2_PNG},
    {"SETTING3", NULL, NULL, 0, PRIMARY, SETTING3_PNG},
    {"SETTING3", NULL, NULL, 0, PRIMARY, SETTING3_PNG},
    {"SETTING2", NULL, NULL, 0, PRIMARY, SETTING2_PNG},
    {"SETTING1", NULL, NULL, 0, PRIMARY, SETTING1_PNG},
};
// ================================
//        这些个变量比较重要
// ================================

int nowMainMenuIndex = 0;             // 当前选中的主菜单项索引, 上电默认为第一个项目
int nowSubMenuIndex = 0;              // 当前选中的子菜单项索引
MenuLevel_E nowMenuLevel = MAIN_MENU; // 当前菜单层级,  上电默认为主菜单
uint16_t nowMenuCNT;                  // 当前菜单项数量
uint8_t MaxVisibleMainMenuCNT = 3;    // 最大可见主菜单数量（主菜单有图片,水平排列,还有当前页面显示不下的）
uint8_t MaxVisibleSubMenuCNT = 11;    // 最大可见子菜单数量（子菜单无图片,竖直排列,还有当前页面显示不下的）
int menuOffset = 0;                   // 菜单编号偏移量，用来移动
int maxOffset;                        // 边界

/// @brief 初始化菜单
/// @param void
void Menu_Init(void)
{
    extern pid_type_def SelectBoxPID_posX;
    extern pid_type_def SelectBoxPID_posY;
    extern float SelectBoxPID_posX_PID[];
    extern float SelectBoxPID_posY_PID[];
    PID_init(&SelectBoxPID_posX, PID_DELTA, SelectBoxPID_posX_PID, 10, 0.2);
    PID_init(&SelectBoxPID_posY, PID_DELTA, SelectBoxPID_posY_PID, 6, 0.2);
    tft.init();
    tft.fillScreen(BF_BG_COLOR);
    bf.createSprite(TFT_WIDTH, TFT_HEIGHT);
    bf.setTextColor(BF_FG_COLOR, BF_BG_COLOR);
    bf.setTextSize(2); // 设置字体大小,
    bf.alphaBlend(true, BF_FG_COLOR, BF_BG_COLOR);
}

int png_x_pos = 0;
int png_y_pos = 165;
/// @brief 在屏幕上将菜单结构体数组绘制到屏幕上，一是会绘制当前可见的菜单项，二是绘制需要选择的复选框，他们分别用index_to_draw和nowMainMenuIndex记录
/// @param  void
void Draw_Menu(void)
{
    bf.fillSprite(BF_BG_COLOR); // 清屏
    switch (nowMenuLevel)
    {
    case MAIN_MENU:
    {
        bf.drawString(F("ESP32S3 Controller"), 0.5 * TFT_WIDTH - 216 / 2, 10);
        nowMenuCNT = sizeof(MainMenu) / sizeof(MainMenu[0]);
        maxOffset = max(0, nowMenuCNT - MaxVisibleMainMenuCNT);

        for (int i = 0; i < MaxVisibleMainMenuCNT; i++)
        {
            int index_to_draw = menuOffset + i;

            if (index_to_draw >= nowMenuCNT) // 超出边界，跳出循环，不再绘制
                break;
            png_x_pos = 10 + i * 80;
            // 绘制图标
            if (MainMenu[index_to_draw].bmp_icon != NULL)
                bf.pushImage(png_x_pos, png_y_pos, 60, 60, MainMenu[index_to_draw].bmp_icon);
            // 在屏幕绘制标题
            bf.setTextSize(1);
            int text_x = png_x_pos + (60 - bf.textWidth(MainMenu[index_to_draw].name)) * 0.5;
            bf.drawString(MainMenu[index_to_draw].name, text_x, png_y_pos - 20);
            bf.setTextSize(2);
        }
        //========== 绘制复选框 ===========
        DrawMainMenuBOX();
        // ========== 绘制水平进度条 ==========

        if (MaxVisibleSubMenuCNT > nowMenuCNT)
        {
            float visibleRatio = (float)MaxVisibleMainMenuCNT / nowMenuCNT; // 可见区域占总菜单的比例
            int scrollbarWidth = TFT_WIDTH * visibleRatio;                  // 滚动条的宽度
            scrollbarWidth = constrain(scrollbarWidth, 20, TFT_WIDTH);      // 确保滚动条至少有20像素宽
            int scrollbarPos = map(nowMainMenuIndex, 0, nowMenuCNT, 0, TFT_WIDTH - scrollbarWidth);
            bf.fillRect(0, TFT_HEIGHT - 8, TFT_WIDTH, 5, TFT_DARKGREY);                      // 背景条（高度为5像素）
            bf.fillRoundRect(scrollbarPos, TFT_HEIGHT - 10, scrollbarWidth, 9, 2, TFT_BLUE); // 填充条
        }
        break;
    }
    case SECOND_MENU:
    {
        nowMenuCNT = MainMenu[nowMainMenuIndex].subMenuCount;
        maxOffset = max(0, nowMenuCNT - MaxVisibleSubMenuCNT);

        for (int i = 0; i < MaxVisibleSubMenuCNT; i++)
        {
            int index_to_draw = menuOffset + i;
            if (index_to_draw >= nowMenuCNT)
                break;

            int sub_text_y = 10 + i * 20; // 每个标题高度为20像素

            // 绘制标题
            switch (MainMenu[nowMainMenuIndex].titleClass)
            {
            case SECONDARY:
                bf.setTextColor(TFT_PURPLE, TFT_DARKGREY);
                break;
            case NORMAL_TEXT:
                bf.setTextColor(BF_FG_COLOR, BF_BG_COLOR);
                break;
            case SYS_FUNC_TEXT:
                bf.setTextColor(TFT_WHITE, TFT_PURPLE);
                break;
            default:
                bf.setTextColor(BF_FG_COLOR, BF_BG_COLOR);
            }
            bf.drawString(MainMenu[nowMainMenuIndex].subMenu[index_to_draw].name, 10, sub_text_y);
        }
        // 绘制复选框
        DrawMainMenuBOX();
        // 绘制竖直进度条
        if (MaxVisibleSubMenuCNT < nowMenuCNT) // 当可见的子菜单数量小于实际子菜单数量时，才绘制进度条
        {
            float visibleRatio = (float)MaxVisibleSubMenuCNT / nowMenuCNT; // 可见区域占总菜单的比例
            int scrollbarHeight = TFT_HEIGHT * visibleRatio;               // 滚动条的高度
            scrollbarHeight = constrain(scrollbarHeight, 20, TFT_HEIGHT);  // 确保滚动条至少有20像素高
            // 计算滚动条的顶部位置
            int scrollbarPos = map(nowSubMenuIndex, 0, nowMenuCNT, 0, TFT_HEIGHT - scrollbarHeight);
            bf.fillRoundRect(TFT_WIDTH - 5, 0, 5, TFT_HEIGHT, 2, TFT_DARKGREY);             // 绘制背景
            bf.fillRoundRect(TFT_WIDTH - 5, scrollbarPos, 5, scrollbarHeight, 2, TFT_BLUE); // 绘制滚动条
        }
        break;
    }
    }

    bf.pushSprite(0, 0);
}

/// @brief 主要执行由按钮引起的菜单状态改变，比如进入菜单，退出菜单，通过函数指针调用函数
/// @param  void
void Menu_Key_Handle(void)
{
    // 获取按键状态
    KEY_STATE UP_KEY_STATE = getKeyState(RIGHT_PIN);
    KEY_STATE DOWN_KEY_STATE = getKeyState(LEFT_PIN);
    KEY_STATE ENTER_KEY_STATE = getKeyState(ENTER_PIN);

    if (ENTER_KEY_STATE == KEY_PRESS)
    {
        if (nowMenuLevel == MAIN_MENU && MainMenu[nowMainMenuIndex].subMenu != NULL) // 如果有子菜单，进入子菜单
        {
            nowMenuLevel = SECOND_MENU; // 改变菜单层级
            menuOffset = 0;             // 重置偏移量
        }
        else if (nowMenuLevel == SECOND_MENU)
        {
            if (MainMenu[nowMainMenuIndex].subMenu[nowSubMenuIndex].function != NULL)
                MainMenu[nowMainMenuIndex].subMenu[nowSubMenuIndex].function();
        }
    }
    else if (UP_KEY_STATE == KEY_PRESS)
    {
        if (nowMenuLevel == MAIN_MENU)
        {
            nowMainMenuIndex++;
            if (nowMainMenuIndex >= nowMenuCNT)
                nowMainMenuIndex = nowMenuCNT - 1;

            // 自动滚动屏幕
            if (nowMainMenuIndex >= menuOffset + MaxVisibleMainMenuCNT)
            {
                menuOffset++;
            }
        }
        else if (nowMenuLevel == SECOND_MENU)
        {
            nowSubMenuIndex++;
            if (nowSubMenuIndex >= MainMenu[nowMainMenuIndex].subMenuCount)
                nowSubMenuIndex = MainMenu[nowMainMenuIndex].subMenuCount - 1;

            // 自动滚动屏幕
            if (nowSubMenuIndex >= menuOffset + MaxVisibleSubMenuCNT)
            {
                menuOffset++;
            }
        }
    }
    else if (DOWN_KEY_STATE == KEY_PRESS)
    {
        if (nowMenuLevel == MAIN_MENU)
        {
            nowMainMenuIndex--;
            if (nowMainMenuIndex < 0)
                nowMainMenuIndex = 0;

            // 自动滚动屏幕
            if (nowMainMenuIndex < menuOffset)
            {
                menuOffset--;
            }
        }
        else if (nowMenuLevel == SECOND_MENU)
        {
            nowSubMenuIndex--;
            if (nowSubMenuIndex < 0)
                nowSubMenuIndex = 0;

            // 自动滚动屏幕
            if (nowSubMenuIndex < menuOffset)
            {
                menuOffset--;
            }
        }
    }
    Draw_Menu();
}

/// @brief 返回主菜单
void Menu_Return()
{
    nowMenuLevel = MAIN_MENU;
    menuOffset = 0;
}

int tarMenuItemPOS_x = 10;                        // 目标位置（当前高亮项的 x 坐标）
float nowMenuItemPOS_x = 0;                       // 当前实际绘制的位置（由 PID 控制）
float SelectBoxPID_posX_PID[] = {0.2, 0.05, 0.3}; // PID 参数（想弹起来，调调i和d）
pid_type_def SelectBoxPID_posX = {0};             // PID 结构体

int tarMenuItemPOS_y = 10;
float nowMenuItemPOS_y = 0;                       // 当前实际绘制的位置（由 PID 控制）
float SelectBoxPID_posY_PID[] = {0.2, 0.026, 0.22}; // PID 参数（想弹起来，调调i和d）
pid_type_def SelectBoxPID_posY = {0};             // PID 结构体

void DrawMainMenuBOX()
{
    switch (nowMenuLevel)
    {
    case MAIN_MENU:
    {
        int selected_index_in_view = nowMainMenuIndex - menuOffset; // 把全局菜单索引转换成当前屏幕上可视区域内的相对位置索引。

        if (selected_index_in_view >= 0 && selected_index_in_view < MaxVisibleMainMenuCNT) // 防止当前选中的项被滚出屏幕外了
        {
            tarMenuItemPOS_x = 10 + selected_index_in_view * 80;
        }
        PID_calc(&SelectBoxPID_posX, nowMenuItemPOS_x, tarMenuItemPOS_x);

        nowMenuItemPOS_x += SelectBoxPID_posX.out; // 使用 PID 的输出来更新实际位置
        // 判断位置是否稳定(没写)
        bf.drawRect(nowMenuItemPOS_x - 3, png_y_pos - 3, 65, 65, CURSOR_COLOR);
        bf.drawRect(nowMenuItemPOS_x - 2, png_y_pos - 2, 63, 63, CURSOR_COLOR);
        break;
    }
    case SECOND_MENU:
    {
        int selected_index_in_view = nowSubMenuIndex - menuOffset;
        int RectWidth = 0;
        if (selected_index_in_view >= 0 && selected_index_in_view < MaxVisibleSubMenuCNT)
        {
            RectWidth = bf.textWidth(MainMenu[nowMainMenuIndex].subMenu[nowSubMenuIndex].name);
            tarMenuItemPOS_y = 10 + selected_index_in_view * 20;
        }
        PID_calc(&SelectBoxPID_posY, nowMenuItemPOS_y, tarMenuItemPOS_y);
        nowMenuItemPOS_y += SelectBoxPID_posY.out; // 使用 PID 的输出来更新实际位置
        // 判断位置是否稳定(没写)
        bf.drawRect(10 - 3, nowMenuItemPOS_y - 3, RectWidth + 3, 22, CURSOR_COLOR);
        bf.drawRect(10 - 2, nowMenuItemPOS_y - 2, RectWidth + 1, 20, CURSOR_COLOR);
        break;
    }
    }
}
