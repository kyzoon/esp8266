/**
 * 说明:
 * 本实例展示如何使用 PWM
 * 使用 4 路 PWM 输出波形
 *
 * GPIO 配置状态:
 * GPIO12: PWM 通道 0 输出
 * GPIO13: PWM 通道 1 输出
 * GPIO14: PWM 通道 2 输出
 * GPIO15: PWM 通道 3 输出
 *
 * 测试:
 * 连接 GPIO12/13/14/15 至 逻辑分析仪
 * 在 GPIO 产生相同频率的方形波形，各个波形可能相位或占空比不一样
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "esp_log.h"
#include "esp_system.h"
#include "esp_err.h"

/* pwm 驱动 */
#include "driver/gpio.h"
#include "driver/pwm.h"

// PWM 周期 500us(2Khz)
#define PWM_CHANNEL_NUM     (4)
#define PWM_PERIOD          (500)

static const char *TAG = "pwm_example";

// pwm 引脚号
const uint32_t pin_num[PWM_CHANNEL_NUM] = {
    GPIO_NUM_12,
    GPIO_NUM_13,
    GPIO_NUM_14,
    GPIO_NUM_15
};

// 占空比 duty 表
// 占空比：说明高电平在一个周期里的占比时间
uint32_t duties[PWM_CHANNEL_NUM] = {
    // 250, 250, 250, 250
    // 100, 200, 300, 400,
    250, 250, 250, 250, 250
};

// 相位 phase 表
// 相位：即同频率波形起点位置的时间差
int16_t phase[PWM_CHANNEL_NUM] = {
    // 0, 0, 50, -50
    // 0, 0, 90, -90
    0, 0, 0, 0, 0
};

void app_main()
{
    int16_t count = 0;

    // PWM 初始化
    pwm_init(PWM_PERIOD, duties, PWM_CHANNEL_NUM, pin_num);
    // 设置 PWM 通道 0 反转输出，所以 通道 1 可作为参考输出
    pwm_set_channel_invert(0x1 << 0);
    // 设置 PWM 相位
    // 即使用希望相位为 0，也必需要配置为 0
    pwm_set_phases(phase);
    // 开启 PWM
    pwm_start();

    // PWM 输出 5s，停止 5s，输出 5s，停止 5s ... ...
    while(1)
    {
        // PWM 运行 5s
        if(count == 5)
        {
            // 0x3 : b0011
            // PWM 通道 0，1 输出高电平
            // PWM 通道 2，3 输出低电平
            pwm_stop(0x3);
            ESP_LOGI(TAG, "PWM stop\n");
        }
        // 等待 5s，重新启动 PWM
        else if(count == 10)
        {
            pwm_start();
            ESP_LOGI(TAG, "PWM re-start\n");
            count = 0;
        }

        count++;
        vTaskDelay(1000 / portTICK_RATE_MS);
    }
}
