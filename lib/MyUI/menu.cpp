#include <Arduino.h>
#include <WiFi.h>
#include <time.h>

#include <TFT_eSPI.h>
#include <Ticker.h>

#include "menu.h"
#include "key.h"
#include "pid.h"
#include "mypicture.h"

#include "esp_system.h"

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
int animationStep = 10;               // 控制动画速度，值越大移动越快
int targetMenuOffset = 0;             // 目标偏移量
bool isAnimating = false;             // 动画状态标志
int currentAnimationProgress = 0;     // 当前动画进度
int lastMenuOffset = 0;               // 记录上一次的 menuOffset 值

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
        // bf.drawString(F("ESP32S3 Controller"), 0.5 * TFT_WIDTH - 216 / 2, 10);
        bf.pushImage(0, 0, HOME_WIDTH, HOME_HEIGHT, HOME); // 绘制主菜单标题图
        bf.setCursor(0, 70);
        bf.setTextSize(1);
        bf.printf("%.2f\n", temperatureRead());
        bf.setTextSize(2);
        nowMenuCNT = sizeof(MainMenu) / sizeof(MainMenu[0]);
        maxOffset = max(0, nowMenuCNT - MaxVisibleMainMenuCNT);

        if (isAnimating)
        {
            currentAnimationProgress += animationStep;
            if (currentAnimationProgress >= 100)
            {
                currentAnimationProgress = 100;
                isAnimating = false;
            }
        }
        // 计算实际偏移量
        float animOffset = 0;
        if (menuOffset < lastMenuOffset)
        {
            animOffset = (100 - currentAnimationProgress) / 100.0 * 80; // 向右滚动
        }
        else
        {
            animOffset = -(100 - currentAnimationProgress) / 100.0 * 80; // 向左滚动
        }

        // 修改后的绘制循环
        for (int i = 0; i < MaxVisibleMainMenuCNT; i++)
        {
            int index_to_draw = targetMenuOffset + i;
            if (index_to_draw >= sizeof(MainMenu) / sizeof(MenuItem))
                break;
            // 计算带动画的位置
            int base_x = 10 + i * 80;
            int final_x = base_x;
            if (isAnimating)
            {
                final_x = base_x - animOffset; // 调整方向
            }
            // 绘制图标
            if (MainMenu[index_to_draw].bmp_icon != NULL)
                bf.pushImage(final_x, png_y_pos, 60, 60, MainMenu[index_to_draw].bmp_icon);
            // 绘制标题
            bf.setTextSize(1);
            int text_x = final_x + (60 - bf.textWidth(MainMenu[index_to_draw].name)) * 0.5;
            bf.drawString(MainMenu[index_to_draw].name, text_x, png_y_pos - 20);
            bf.setTextSize(2);
        }
        //=========== 绘制复选框 ============
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
    // 保存旧的menuOffset用于判断方向
    int oldOffset = menuOffset;
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
                lastMenuOffset = menuOffset;
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
                lastMenuOffset = menuOffset;
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
                lastMenuOffset = menuOffset;
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
                lastMenuOffset = menuOffset;
                menuOffset--;
            }
        }
    }

    // 在Draw_Menu();之前添加动画触发判断
    if (menuOffset != oldOffset)
    {
        targetMenuOffset = menuOffset;
        isAnimating = true;
        currentAnimationProgress = 0;
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
float SelectBoxPID_posX_PID[] = {0.2, 0.05, 0.1}; // PID 参数（想弹起来，调调i和d）
pid_type_def SelectBoxPID_posX = {0};             // PID 结构体

int tarMenuItemPOS_y = 10;
float nowMenuItemPOS_y = 0;                         // 当前实际绘制的位置（由 PID 控制）
float SelectBoxPID_posY_PID[] = {0.2, 0.026, 0.22}; // PID 参数（想弹起来，调调i和d）
pid_type_def SelectBoxPID_posY = {0};               // PID 结构体
enum BOX_CLASS
{
    None = 0,      // 无效果
    NeonPulse,     // 霓虹光晕脉冲
    Hologram,      // 全息投影风格
    GravityRipple, // 引力波纹
    GlitchEffect,  // 像素故障特效
    RadarScan,     // 星际雷达扫描
    AuroraRibbon,  // 极光飘带
};
extern void drawBox(BOX_CLASS mode);
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
        drawBox(NeonPulse);
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
        bf.drawRect(10 - 3, nowMenuItemPOS_y - 3, RectWidth + 5, 22, CURSOR_COLOR);
        bf.drawRect(10 - 2, nowMenuItemPOS_y - 2, RectWidth + 3, 20, CURSOR_COLOR);
        break;
    }
    }
}

// AI给设计的复选框样式
void drawBox(BOX_CLASS mode)
{
    switch (mode)
    {
    case NeonPulse:
    {
        float pulse = 0.5 * (1 + sin(millis() / 300.0));
        for (int i = 5; i > 0; i--)
        {
            uint16_t blendColor = bf.color565(32 * i * pulse, 64 * i * pulse, 96 * i * pulse);
            bf.drawRoundRect(nowMenuItemPOS_x - i, png_y_pos - i, 60 + 2 * i, 60 + 2 * i, 3 + i, blendColor);
        }
        break; // 明确结束当前case
    }
    case Hologram:
    {
        int scanLine = (millis() / 50) % 64;
        for (int y = 0; y < 60; y += 4)
        {
            for (int x = 0; x < 60; x += 4)
            {
                uint16_t dotColor = (abs(y - scanLine) < 2) ? 0x07FF : 0xF81F;
                bf.drawPixel(nowMenuItemPOS_x + x, png_y_pos + y, dotColor);
            }
        }
        bf.drawRoundRect(nowMenuItemPOS_x - 2 + scanLine % 3, png_y_pos - 2 + scanLine % 3, 64, 64, 8, 0x07FF);
        bf.drawRoundRect(nowMenuItemPOS_x + scanLine % 2, png_y_pos + scanLine % 2, 60, 60, 6, 0xF81F);
        break; // 明确结束当前case
    }
    case GravityRipple:
    {
        float wave = sin(millis() / 150.0);
        for (int r = 34; r < 40; r++)
        {
            for (int a = 0; a < 360; a += 10)
            {
                int distort = 2 * sin(r + millis() / 100.0);
                int x = nowMenuItemPOS_x + 30 + (r + distort) * cos(a * DEG_TO_RAD);
                int y = png_y_pos + 30 + (r + distort) * sin(a * DEG_TO_RAD);
                bf.drawPixel(x, y, 0x7FF);
            }
        }
        break; // 明确结束当前case
    }
    case GlitchEffect:
    {
        int glitch = random(0, 10);
        bf.drawRect(nowMenuItemPOS_x, png_y_pos, 60, 60, TFT_WHITE);
        if (glitch > 7)
        {
            int shift = random(-3, 3);
            bf.drawRect(nowMenuItemPOS_x + shift, png_y_pos, 60, 60, 0x07E0);
            bf.drawRect(nowMenuItemPOS_x - shift, png_y_pos, 60, 60, 0xF800);
        }
        break; // 明确结束当前case
    }
    case RadarScan:
    {
        int scanAngle = (millis() / 50) % 360;
        bf.drawCircle(nowMenuItemPOS_x + 30, png_y_pos + 30, 31, 0x5E7F);
        bf.drawCircle(nowMenuItemPOS_x + 30, png_y_pos + 30, 28, 0x5E7F);
        float rad = scanAngle * DEG_TO_RAD;
        int x2 = 30 + 30 * cos(rad);
        int y2 = 30 + 30 * sin(rad);
        bf.drawLine(nowMenuItemPOS_x + 30, png_y_pos + 30, nowMenuItemPOS_x + x2, png_y_pos + y2, 0x5E7F);
        bf.fillCircle(nowMenuItemPOS_x + 30, png_y_pos + 30, 3, 0x5E7F);
        break; // 明确结束当前case
    }
    case AuroraRibbon:
    {
        int ribbonPos = (millis() / 50) % 80;
        bf.drawFastVLine(nowMenuItemPOS_x - 10 + ribbonPos, png_y_pos - 10, 80, 0x7C1F);
        bf.drawFastVLine(nowMenuItemPOS_x - 8 + ribbonPos, png_y_pos - 10, 80, 0x7FE0);
        bf.drawFastVLine(nowMenuItemPOS_x - 6 + ribbonPos, png_y_pos - 10, 80, 0xF81F);
        break; // 明确结束当前case
    }
    case None:
    {
        bf.drawRect(nowMenuItemPOS_x - 3, png_y_pos - 3, 65, 65, CURSOR_COLOR);
        bf.drawRect(nowMenuItemPOS_x - 2, png_y_pos - 2, 63, 63, CURSOR_COLOR);
        break; // 明确结束当前case
    }
    }
}