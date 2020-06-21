## ESP8266 学习笔记 6 —— PWM 实例
作者：Preston_zhu<br>
日期：2020.6.21

### 0. PWM 硬件描述
ESP8266 有 4 个PWM 输出接口/通道。如下表
| 管脚名称 | 管脚编号 | IO | 功能名称 |
| ---- | ---- | ---- | ---- |
| MTDI | 10 | IO12 | PWM0 |
| MTDO | 13 | IO15 | PWM1 |
| MTMS | 9 | IO14 | PWM2 |
| GPIO4 | 16 | IO4 | PWM3 |
注意：此处资料与实例中有差别？？
PWM 功能由 FRC1 在软件上实现。(推断 PWM 与输出引脚无关，因为是软件实现)

> FRC1 是一个 23bit 的硬件定时器

PWM 特性：
* 使用 NMI (NOn Maskable Interrupt) 中断，NMI 拥有最高中断优先级，可保证 PWM 输出波形的准确度
* 可扩展最多 8 路 PWM 信号
* &gt;14 bit 分辨率，最小分辨率 45ns

> 注意：
> * PWM 驱动接口不能与硬件定时器 hw_timer 接口函数同时使用，因为二者共用同一个硬件定时器
> * 如果使用 PWM 驱动，请勿调用 `wifi_set_sleep_type(LIGT_SLEEP)` 函数将自动睡眠模式设置为 Light Sleep 模式。因为 Light Sleep 模式在睡眠基本会停 CPU，停 CPU 期间不能响应 NMI 中断，会影响 PWM 输出
> * 如需进入 Deep Sleep，请先将 PWM 关闭，再进行休眠

PWM 的时钟由高速系统时钟提供，其频率高达 80 MHz。 PWM 通过预分频器将时钟源 16 分频，其输入时间频率为 5 MHz。PWM 通过 FRC1 来产生粗调定时，结合高速系统时钟的微调，可将分辨率提高到 45ns

参数说明：
* 最小分辨率：45ns （近似对应于硬件 PWM 的输入时钟频率为22.72 MHz）： &gt;14 bit PWM @ 1kHz
* PWM 周期：1000us (1kHz) ~ 10000 us(100Hz)


### 1. PWM 实例

参考 RTOS_SDK/examples/peripherals/pwm
在原实例上做了修改，方便理解内容

#### i. 流程

* 初始化 PWM，设定 PWM 周期，占空比，通道数和输出引脚
* 设置 PWM 反转输出通道
* 设置 PWM 输出相位
* 开启 PWM 输出
* 输出 5s PWM，停止 PWM 5s，然后再开启/停止，一直重复 ...

#### ii. 主程序分析

```C
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
```

#### iii. 打印输出

```shell
```

#### iv. 图解
![breath_led1](images/breath_led1.png)
从图中可以看出，PWM 波形有作高低电平转换
![breath_led2](images/breath_led2.png)
在各个阶段选择性的展开了一些波形，并测试了一下参数，说明不同时间，PWM 输出波形的占空比变化情况。
如程度内容一致，先增加占空比，再减少占空比，如此反复。

#### v. 遇到的问题记录
1.
一开始上来就写占空比为0，因为是从0开始增加嘛！习惯性的就这样做了。但是由于对代码也不太熟悉，写了之后，灯显示却不对，后来只能返回到 PWM 的实例代码，一步一步调试过来。其实，可以一开始先设定一个固定有输出的效果，然后让基本的内容工作起来，再做进一步改变，会更好！

这里面，其实就有一个，关于思考方式的：正向逻辑与逆向逻辑。正常情况下，如何如何做，为正向。反过来的思考方式，为逆向。

我就是先想着逆向去直接实现，但对于 PWM 代码实现部分，其实并没有我想像的那么熟悉，所以直接做反而出了问题。还是应该正向的，比较保守的来做。

2. 
中间修改占空比参数时，一开始忘记了，一定要在修改 PWM 参数之后，再次调用 pwm_start() 让修改生效。怎么调都发现输出的占空比并未改变，也加重了上一个错误。后来突然想起 API 中有反复提到。

<font color="red">**注意：需要在配置 PWM 后，调用 `pwm_start()` 函数，使设置生效**</font>
