/**
 * 说明:
 * 本实例展示如何使用 PWM 实现呼吸灯
 * 使用 GPIO4 作 PWM 输出，控制 LED 显示呼吸灯效果
 *
 * GPIO 配置状态:
 * GPIO4: PWM 通道 0 输出
  *
 * 测试:
 * 连接 GPIO4 至 LED
 * 在 GPIO4 产生不同占空比的固定频率方波，控制 LED 显示
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

#include "driver/gpio.h"
/* pwm 驱动 */
#include "driver/pwm.h"

#define PWM_CH_NUM          (1)
#define PWM_PERIOD          (500)

/**
 * 呼吸灯占空比步长计算
 * 实例：实现 2 秒/次的呼吸灯效果 
 * 假设每 20 ms 调整一次占空比参数
 * 每次呼吸会改变占空比次数：2000ms / 20ms = 100 次
 * 因此：增加占空比 50 次，减小占空比 50 次
 * PWM 周期为 500 us
 * 占空比变化步长： 500us / 50 = 10us
 * (此时占空比范围：0% ~ 100%)
 * 注意：占空比应该为高电平时长占整个周期时长的百分比，而实际设置的却是高电平工作时长，可能不太好理解。
 */
// #define BREATH_LEN_STEP     (10)
/* 占空比范围：0% ~ 50% */
// #define BREATH_LEN_STEP     (5)
/* 占空比范围：0% ~ 30% */
/* 30% 占空比的呼吸灯效果，个人感觉最好看，步长 10 和 5 都太亮，对比起来暗灯的时间就变短了，效果不明显，还刺眼 */
#define BREATH_LEN_STEP     (3)

// static const char *s_tag = "breath_led";

const uint32_t pwm_chs[PWM_CH_NUM] = { GPIO_NUM_4 };
uint32_t pwm_duties[PWM_CH_NUM] = { 0 };
int16_t pwm_phases[PWM_CH_NUM] = { 0 };

void app_main(void)
{
    int16_t i = 0;

    // 初始化 PWM ，1 个通道，输出至 GPIO4，初始占空比为 0
    pwm_init(PWM_PERIOD, pwm_duties, PWM_CH_NUM, pwm_chs);
    // 设置通道 0 的相位为 0
    pwm_set_phase(0, 0);
    pwm_start();

    for(;;)
    {
        vTaskDelay(20 / portTICK_RATE_MS);

        // 0 到 49，共 50 次，占空比增加
        if(50 > i)
        {
            pwm_duties[0] += BREATH_LEN_STEP;
        }
        // 50 到 99，共 50 次，占空比减少
        else if(100 > i)
        {
            pwm_duties[0] -= BREATH_LEN_STEP;
        }
        // ESP_LOGI(s_tag, "%d -> %d", i, pwm_duties[0]);
        pwm_set_duty(0, pwm_duties[0]);
        // 一定要在此处加 pwm_start()，让设置生效
        pwm_start();
        ++i;

        // 复位，重新开始增加/减少占空比
        if(100 == i)
        {
            i = 0;
        }
    }
}
