

## ESP8266 学习笔记 4 —— GPIO 实例

之后实例均都基于 RTOS_SDK：

想来想去，虽然一开始是想从 NONOS_SDK 开始做实例，因为使用 RTOS_SDK 版本，中间总是有一层 OS 层，不利于学习和理解。但是，既然官方都已经不再继续维护 NONOS_SDK 了，那就干脆，直接从 RTOS_SDK 开始吧！虽然可能会比较难理解。

------

### 1. 相关资料

在开始以下的实例搜索之前，需要先准备硬件相关的资料。原理图肯定是必不可少的。

以下资料均是在官网上找到的：

* 模块规格说明书：esp-wroom-02d_esp-wroom-02u_datasheet_cn.pdf
* 使用手册1：ESP8266-DevKitC_getting_started_guide__CN.pdf
* 使用手册2：ESP8266-DevKitC_getting_started_guide__CN_2.pdf
* 原理图和Layout：ESP8266-DevKitS-V1.0_reference_design.zip
* RTSO_SDK参考手册：docs-espressif-com-esp8266-rtos-sdk-en-latest.pdf
* 技术参考手册：esp8266-technical_reference_cn.pdf
* 硬件设计指南：esp8266_hardware_design_guidelines_cn.pdf

###2. GPIO 实例

参考 RTOS_SDK/examples/peripherals/gpio

#### i. 流程

* 配置 GPIO 引脚：GPIO4/5，GPIO15/16
* 配置 GPIO 中断触发类型：GPIO4 上升及下降沿触发中断，GPIO5 上升沿触发中断
* 安装 GPIO ISR 中断服务程序 —— <font color="red">使用中断第一步</font>
* 注册 GPIO 中断服务程序 —— <font color="red">使用中断第二步</font>
* 创建 FreeRTOS 队列和任务
* 循环：
  * 延时等待，CPU 可以处理其它任务
    * 读取队列，没有内容即超时返回
    * 读取队列，有内容即处理 —— 打印输出内容
  * 改变 GPIO15/16 输出电平，触发中断
  * ISR处理，消息插入队列

#### ii. 主程序分析

```C
/* main/user_main.c */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* RTOS_SDK 是基于 FreeRTOS */
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

/* GPIO 驱动 */
#include "driver/gpio.h"

/* ESP 日志打印输出 */
#include "esp_log.h"
/* ESP 头文件 */
#include "esp_system.h"

static const char *TAG = "main";

/**
 * 说明:
 * 本实例展示如何配置 GPIO 以及如何使用 GPIO 中断
 *
 * GPIO 配置状态:
 * GPIO15: 输出
 * GPIO16: 输出
 * GPIO4:  输入，上拉，上升及下降沿触发中断
 * GPIO5:  输入，上拉，上升沿触发中断
 *
 * 测试:
 * 连接 GPIO15 至 GPIO4
 * 连接 GPIO16 至 GPIO5
 * 在 GPIO15/16 两个 IO 口产生脉冲, 由此来触发 GPIO4/5 中断
 */

static xQueueHandle gpio_evt_queue = NULL;

static void gpio_isr_handler(void *arg)
{
	/* 接收中断源 GPIO 引脚号 */
	/* 即为 gpio_isr_handler_add() 注册函数的第三个参数 */
	uint32_t gpio_num = (uint32_t) arg;
	/* 插入队列 */
	/* ISR 版本无延时参数 */
	xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

static void gpio_task_example(void *arg)
{
	uint32_t gpio_num;

	for(;;)
	{
		/* 读取队列，如果有内容则取出，会删除队列中此内容，然后处理
		   如果没有内容，则在 portMAX_DELAY 之后超时 */
		if(xQueueReceive(gpio_evt_queue, &gpio_num, portMAX_DELAY))
		{
			ESP_LOGI(TAG, "GPIO[%d] intr, val: %d", gpio_num, gpio_get_level(gpio_num));
		}
	}
}

void app_main(void)
{
	gpio_config_t io_conf;
	
	// 设置 GPIO15/16 为输出模式
	// 禁止中断
	io_conf.intr_type = GPIO_INTR_DISABLE;
	// 设置为输出模式
	io_conf.mode = GPIO_MODE_OUTPUT;
	// 位掩码选择 GPIO15/16，置1
	io_conf.pin_bit_mask = GPIO_Pin_16 | GPIO_Pin_15;
	// 禁止下拉
	io_conf.pull_down_en = 0;
	// 禁止上拉
	io_conf.pull_up_en = 0;
	gpio_config(&io_conf);

	// 设置 GPIO4/5 为输入模式
	// 上升沿触发中断模式
	io_conf.intr_type = GPIO_INTR_POSEDGE;
	// 位掩码选择 GPIO4/5，置1
	io_conf.pin_bit_mask = GPIO_Pin_5 | GPIO_Pin_4;
	// 设置为输入模式
	io_conf.mode = GPIO_MODE_INPUT;
	// 使能上拉
	io_conf.pull_up_en = 1;
	gpio_config(&io_conf);

	// 设置 GPIO4 中断，上升及下降沿触发
	gpio_set_intr_type(GPIO_NUM_4, GPIO_INTR_ANYEDGE);
	
	// 先 安装 GPIO ISR 中断服务程序，参数 0 并没有实际意义
	gpio_install_isr_service(0);
	// 再 设置 GPIO4 中断服务程序
	gpio_isr_handler_add(GPIO_NUM_4, gpio_isr_handler, (void *) GPIO_NUM_4);
	// 设置 GPIO5 中断服务程序
	gpio_isr_handler_add(GPIO_NUM_5, gpio_isr_handler, (void *) GPIO_NUM_5);
	
	// 删除 GPIO4 中断服务程序
	// 此处代码只是演示怎么删除 GPIO 中断服务程序，无实际意义，可以删除或注释掉
	// gpio_isr_handler_remove(GPIO_NUM_4);
	// 设置 GPIO4 中断服务程序
	// gpio_isr_handler_add(GPIO_NUM_4, gpio_isr_handler, (void *) GPIO_NUM_4);

	// 创建队列，用于处理 GPIO ISR 产生的事件
	gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
	// 创建 GPIO 任务
	xTaskCreate(gpio_task_example, "gpio_task_example", 2048, NULL, 10, NULL);
    
	int cnt = 0;

	for(;;)
	{
		ESP_LOGI(TAG, "cnt: %d", cnt++);
        // 添加一个延时等待，让 CPU 空闲，可以去处理其它任务
		vTaskDelay(1000 / portTICK_RATE_MS);
		// 设置 GPIO15/16 输出电平高低，根据 cnt 计数器进行高低电平翻转，生成脉冲
		gpio_set_level(GPIO_NUM_15, cnt % 2);
		gpio_set_level(GPIO_NUM_16, cnt % 2);
	}
}
```

#### iii. 打印输出

```shell
I (368) gpio: GPIO[15]| InputEn: 0| OutputEn: 1| OpenDrain: 0| Pullup: 0| Pulldown: 0| Intr:0	# GPIO15 输出
I (368) gpio: GPIO[16]| InputEn: 0| OutputEn: 1| OpenDrain: 0| Pullup: 0| Pulldown: 0| Intr:0	# GPIO16 输出
I (388) gpio: GPIO[4]| InputEn: 1| OutputEn: 0| OpenDrain: 0| Pullup: 1| Pulldown: 0| Intr:1	# GPIO4 输入，上拉，中断使能
I (398) gpio: GPIO[5]| InputEn: 1| OutputEn: 0| OpenDrain: 0| Pullup: 1| Pulldown: 0| Intr:1	# GPIO5 输入，上拉，中断使能
I (418) main: cnt: 0						# 初始情况：GPIO15 连接至 GPIO4，GPIO16 连接至 GPIO5，GPIO4/5 均为 0
											# 第 0 次：cnt 已经为 1
											# 之后，进行 vTaskDelay 时延，处理子任务 gpio_task_example()
											# 子任务读取队列，无消息，直至超时后，返回主程序
											# GPIO15/16 输出 1，GPIO4/5 输入上升沿电平，触发GPIO4/5中断
                                            # 中断处理函数将消息插入队列，然后返回至程序
                                            # 进入下一个循环
I (1418) main: cnt: 1						# 第 1 次：cnt 已经为 2
											# 之后，进入 vTaskDelay 时延，处理子任务 gpio_task_example()
											# 此时可以读取两个消息：4 和 5
I (1418) main: GPIO[4] intr, val: 1			# GPIO15 连接至 GPIO4，GPIO15 电平先变化，所以中断先触发
I (1418) main: GPIO[5] intr, val: 1			# GPIO16 连接至 GPIO5，GPIO15 电平先变化，所以中断先触发
											# GPIO15/16 输出 0， GPIO4/5 输入下降沿电平，仅触发GPIO4中断
											# 中断处理函数将消息插入队列，然后返回至程序
I (2418) main: cnt: 2						# 第 2 次：cnt 已经为 3
											# 之后，进入 vTaskDelay 时延，处理子任务 gpio_task_example()
											# 此时可以读取一个消息：4
I (2418) main: GPIO[4] intr, val: 0
I (3418) main: cnt: 3						# 之后随着电平翻转，依次处理中断，输出内容
I (3418) main: GPIO[4] intr, val: 1
I (3418) main: GPIO[5] intr, val: 1
I (4418) main: cnt: 4
I (4418) main: GPIO[4] intr, val: 0
I (5418) main: cnt: 5
I (5418) main: GPIO[4] intr, val: 1
I (5418) main: GPIO[5] intr, val: 1
I (6418) main: cnt: 6
I (6418) main: GPIO[4] intr, val: 0
I (7418) main: cnt: 7
```

图解如下所示：

<img src="E:\esp\notes\images\GPIO实例图解.png" alt="GPIO实例图解" style="zoom:80%;" />

**目前还有个疑问：为什么初始化的 GPIO 口为低电?**

### 3. 程序框架

```C
// 任务函数
static void task_fun(void *arg)
{
	...
}

// 离程序入口
void app_main(void)
{
	// 配置硬件，如 GPIO，中断 等

    // 创建 FreeRTOS 任务以及其它
	xTaskCreate(task_fun, "task_fun", 2048, NULL, 10, NULL);
    
	for(;;)
	{
		// 主线程代码
        ...
	}
}
```

### 4. GPIO API

#### i.头文件：`esp266/include/driver/gpio.h`

#### ii.函数概览：

```C
esp_err_t gpio_config(const gpio_config_t *gpio_cfg);
esp_err_t gpio_set_intr_type(gpio_num_t gpio_num, gpio_int_type_t intr_type);
esp_err_t gpio_set_level(gpio_num_t gpio_num, uint32_t level);
int gpio_get_level(gpio_num_t gpio_num);
esp_err_t gpio_set_direction(gpio_num_t gpio_num, gpio_mode_t mode);
esp_err_t gpio_set_pull_mode(gpio_num_t gpio_num, gpio_pull_mode_t pull);
esp_err_t gpio_wakeup_enable(gpio_num_t gpio_num, gpio_int_type_t intr_type);
esp_err_t gpio_wakeup_disable(gpio_num_t gpio_num);
esp_err_t gpio_isr_register(void (*fn))void *, void *arg, int no_use, gpio_isr_handle_t *handle_no_use);
esp_err_t gpio_pullup_en(gpio_num_t gpio_num);
esp_err_t gpio_pullup_dis(gpio_num_t gpio_num);
esp_err_t gpio_pulldown_en(gpio_num_t gpio_num);
esp_err_t gpio_pulldown_dis(gpio_num_t gpio_num);
esp_err_t gpio_install_isr_service(int no_use);
void gpio_uninstall_isr_service(void);
esp_err_t gpio_isr_handler_add(gpio_num_t gpio_num, gpio_isr_t isr_handler, void *args);
esp_err_t gpio_isr_handler_remove(gpio_num_t gpio_num);
```

#### iii.函数说明：

```C
esp_err_t gpio_config(const gpio_config_t *gpio_cfg)
```
`GPIO` 通用配置。配置 `GPIO` 模式，上拉，下拉，中断类型。

返回值：

* `ESP_OK` - 成功
* `ESP_ERR_INVALID_ARG` - 参数错误

参数：

* `gpio_cfg` - GPIO 配置结构体指针



```C
esp_err_t gpio_set_intr_type(gpio_num_t gpio_num, gpio_int_type_t intr_type)
```
配置 `GPIO` 中断触发类型。

返回值：

* `ESP_OK` - 成功

* `ESP_ERR_INVALID_ARG` - 参数错误

参数：

* `gpio_num` -  GPIO 引脚号，可引用宏 `GPIO_NUM_0` ~ `GPIO_NUM_16`
* `intr_type` - 中断类型，参考枚举 `gpio_int_type_t`



```C
esp_err_t gpio_set_level(gpio_num_t gpio_num, uint32_t level)
```
设置 `GPIO` 输出电平。

返回值：
* `ESP_OK` - 成功
* `ESP_ERR_INVALID_ARG` - `GPIO` 引脚号错误

参数：
* `gpio_num` - `GPIO` 引脚号，，可引用宏 `GPIO_NUM_0` ~ `GPIO_NUM_16`
* `level` - 输出电平：0 : 低电平；1 : 高电平



```C
int gpio_get_level(gpio_num_t gpio_num)
```
获取 `GPIO` 输入电平。
注意：如果引脚未配置为输入（或输入/输出）形式，则返回值总是 0
返回值：
* 0 - `GPIO` 输入电平为 0
* 1 - `GPIO` 输入电平为 1

参数：
* `gpio_num` - `GPIO` 引脚号，可引用宏 `GPIO_NUM_0` ~ `GPIO_NUM_16`



```C
esp_err_t gpio_set_direction(gpio_num_t gpio_num, gpio_mode_t mode)
```
设置 `GPIO` 输入/输出方向，例如：仅为输入，或仅为输出
返回值：

* `ESP_OK` - 成功
* `ESP_ERR_INVALID_ARG` - `GPIO` 错误

参数：
* `gpio_num` - `GPIO` 引脚号，可引用宏 `GPIO_NUM_0` ~ `GPIO_NUM_16`
* `mode` - `GPIO` 输入输出方向，参考枚举 `gpio_mode_t`



```C
esp_err_t gpio_set_pull_mode(gpio_num_t gpio_num, gpio_pull_mode_t pull)
```
配置 `GPIO` 上拉/下拉寄存器
注意：ESP8266 GPIO 除 RTC GPIO 引脚外，其它 GPIO 不能下拉，而 RTC GPIO 引脚则不能被上拉？？
返回值：

* `ESP_OK` - 成功
* `ESP_ERR_INVALID_ARG` - 参数错误

参数：
* `gpio_num` - `GPIO` 引脚号，可引用宏 `GPIO_NUM_0` ~ `GPIO_NUM_16`
* `pull` - `GPIO` 上拉/下拉模式，参考枚举 `gpio_pull_mode_t`



```C
esp_err_t gpio_wakeup_enable(gpio_num_t gpio_num, gpio_int_type_t intr_type)
```
使能 `GPIO` `wake-up` 唤醒功能

注意：`RTC IO` 引脚不能用作 `wake-up` 唤醒功能

返回值：
* `ESP_OK` - 成功
* `ESP_ERR_INVALID_ARG` - 参数错误

参数：
* `gpio_num` - `GPIO` 引脚号，可引用宏 `GPIO_NUM_0` ~ `GPIO_NUM_16`
* `intr_type` - `GPIO` `wake-up` 唤醒类型。仅 `GPIO_INTR_LOW_LEVEL` 或 `GPIO_INTR_HIGH_LEVEL` 可用，此处利用了中断类型的枚举



```C
esp_err_t gpio_wakeup_disable(gpio_num_t gpio_num)
```
禁止 `GPIO` `wake-up` 唤醒功能
注意：`RTC IO` 引脚不能用作 `wake-up` 唤醒功能
返回值：

* `ESP_OK` - 成功
* `ESP_ERR_INVALID_ARG` - 参数错误

参数：

* `gpio_num` - `GPIO` 引脚号，可引用宏 `GPIO_NUM_0` ~ `GPIO_NUM_16`



```C
esp_err_t gpio_isr_register(void (*fn))void *, void *arg, int no_use, gpio_isr_handle_t *handle_no_use)
```
注册 `GPIO` 中断处理函数，即 `ISR`。中断发生时，`ISR` 处理函数总是会被调用。参考 `gpio_install_isr_service()` 和 `gpio_isr_handler_add()`，使用驱动库支持 `GPIO` 中断服务。

返回值：

* `ESP_OK` - 成功
* `ESP_ERR_INVALID_ARG` -  `GPIO` 错误
* `ESP_ERR_NOT_FOUND` -  未找到指定的空闲中断，注册失败

参数：
* `fn` - 中断处理函数
* `no_use` - 仅为与 esp32 进行兼容，没有实际意义，为 0
* `arg` - 传至中断处理函数的参数
* `handle_no_use` - 返回的处理函数指针，仅为与 esp32 进行兼容，没有实际意义，为 NULL



```C
esp_err_t gpio_pullup_en(gpio_num_t gpio_num)
```
使能 `GPIO` 引脚上拉
注意：ESP8266 GPIO 除 RTC GPIO 引脚外，其它 GPIO 不能下拉，而 RTC GPIO 引脚则不能被上拉？？
返回值：

* `ESP_OK` - 成功
* `ESP_ERR_INVALID_ARG` - 参数错误

参数：
* `gpio_num` - `GPIO` 引脚号，可引用宏 `GPIO_NUM_0` ~ `GPIO_NUM_16`



```C
esp_err_t gpio_pullup_dis(gpio_num_t gpio_num)
```
禁止 `GPIO` 引脚上拉
注意：ESP8266 GPIO 除 RTC GPIO 引脚外，其它 GPIO 不能下拉，而 RTC GPIO 引脚则不能被上拉？？
返回值：

* `ESP_OK` - 成功
* `ESP_ERR_INVALID_ARG` - 参数错误

参数：

* `gpio_num` - `GPIO` 引脚号，可引用宏 `GPIO_NUM_0` ~ `GPIO_NUM_16`



```C
esp_err_t gpio_pulldown_en(gpio_num_t gpio_num)
```
使能 `GPIO` 引脚下拉
注意：ESP8266 GPIO 除 RTC GPIO 引脚外，其它 GPIO 不能下拉，而 RTC GPIO 引脚则不能被上拉？？
返回值：

* `ESP_OK` - 成功

* `ESP_ERR_INVALID_ARG` - 参数错误

参数：

* `gpio_num` - `GPIO` 引脚号，可引用宏 `GPIO_NUM_0` ~ `GPIO_NUM_16`



```C
esp_err_t gpio_pulldown_dis(gpio_num_t gpio_num)
```
禁止 `GPIO` 引脚下拉
注意：ESP8266 GPIO 除 RTC GPIO 引脚外，其它 GPIO 不能下拉，而 RTC GPIO 引脚则不能被上拉？？
返回值：

* `ESP_OK` - 成功
* `ESP_ERR_INVALID_ARG` - 参数错误

参数：

* `gpio_num` - `GPIO` 引脚号，可引用宏 `GPIO_NUM_0` ~ `GPIO_NUM_16`



```C
esp_err_t gpio_install_isr_service(int no_use)
```
安装驱动库的 `GPIO ISR` 处理服务，以允许处理每一个 `GPIO` 引脚的中断

使用此函数，将为所有 `GPIO` 中断注册一个全局的 `ISR` 服务。而各个引脚的中断处理程序则通过 `gpio_isr_hander_add()` 函数注册

返回值：
* `ESP_OK` - 成功
* `ESP_ERR_NO_MEM` - 内存空间不足，无法注册服务
* `ESP_ERR_INVALID_STATE` - `GPIO ISR` 处理服务已经注册
* `ESP_ERR_NOT_FOUND` - 未找到指定的空闲中断，注册失败
* `ESP_ERR_INVALID_ARG` - `GPIO` 错误

参数：
* `no_use` - 仅为了与 esp32 进行兼容，没有实际意义，为 0



```C
void gpio_uninstall_isr_service()
```
卸载驱动库的 `GPIO ISR` 处理服务，释放相关资源




```C
esp_err_t gpio_isr_handler_add(gpio_num_t gpio_num, gpio_isr_t isr_handler, void *args)
```
为对应的 `GPIO` 引脚注册中断处理函数

使用此函数之前，必需使用 `gpio_install_isr_service()` 函数安装驱动库 `GPIO ISR` 处理服务。此注册的中断处理函数将被 `ISR` 调用。调用中断处理函数，有一个”栈大小限制“参数（在 menuconfig 中有配置项 ”ISR stack size“），相比于全局 `GPIO` 中断处理函数，这个栈更小一些。

参数：

* `gpio_num` - `GPIO` 引脚号，可使用宏：`GPIO_NUM_0` ~ `GPIO_NUM_16`
* `isr_handler` - 中断处理函数
* `args` - 传至中断处理函数的参数，通常定义为对应的 `GPIO` 引脚号，用以区分中断源来自哪个 `GPIO` 引脚

返回值：

* `ESP_OK` - 成功
* `ESP_ERR_INVALID_STATE` - 错误状态，`ISR` 服务未初始化
* `ESP_ERR_INVALID_ARG` - 参数错误
参数：

* `gpio_num` - `GPIO` 引脚号，可引用宏 `GPIO_NUM_0` ~ `GPIO_NUM_16`
* `isr_handler` - 对应 `GPIO` 引脚的 `ISR`处理函数句柄
* `args` - 传向 `ISR` 处理函数的参数



```C
esp_err_t gpio_isr_handler_remove(gpio_num_t gpio_num)
```
删除对应 `GPIO` 引脚已注册的中断处理函数

* `gpio_num` - `GPIO` 引脚号，可使用宏：`GPIO_NUM_0` ~ `GPIO_NUM_16`

返回值：
* `ESP_OK` - 成功
* `ESP_ERR_INVALID_STATE` - 错误状态，`ISR` 服务未初始化
* `ESP_ERR_INVALID_ARG` - 参数错误
参数：

* `gpio_num` - `GPIO` 引脚号，可引用宏 `GPIO_NUM_0` ~ `GPIO_NUM_16`



---

结构体：
```C
struct gpio_config_t
	// 用于函数 gpio_config() 配置参数，配置 GPIO 引脚功能
    // 成员：
    // 设置位掩码，选择 GPIO 引脚，每一位代表一个 GPIO 引脚：0 ~ 16
	uint32_t pin_bit_mask
    // 设置 GPIO 模式：输入/输出模式
	gpio_mode_t mode
    // 设置 GPIO 上拉使能
	gpio_pullup_t pull_up_en
	// 设置 GPIO 下拉使能
	gpio_pulldown_t pull_down_en
	// 设置 GPIO 中断类型
	gpio_int_type_t intr_type
```

宏
```C
BIT(x)
GPIO_Pin_0
GPIO_Pin_1
GPIO_Pin_2
GPIO_Pin_3
GPIO_Pin_4
GPIO_Pin_5
GPIO_Pin_6
GPIO_Pin_7
GPIO_Pin_8
GPIO_Pin_9
GPIO_Pin_10
GPIO_Pin_11
GPIO_Pin_12
GPIO_Pin_13
GPIO_Pin_14
GPIO_Pin_15
GPIO_Pin_16
GPIO_Pin_All
GPIO_MODE_DEF_DISABLE
GPIO_MODE_DEF_INPUT
GPIO_MODE_DEF_OUTPUT
GPIO_MODE_DEF_OD
GPIO_PIN_COUNT
// 检查 GPIO 引脚号否合法
GPIO_IS_VALID_GPIO(gpio_num)
// 检查 RTC GPIO 引脚号是否合法
RTC_GPIO_IS_VALID_GPIO(gpio_num)
```

类型定义
```C
// GPIO ISR 函数指针
typedef void (*gpio_isr_t)(void *)
// GPIO ISR 函数句柄指针
typedef void *gpio_isr_handle_t
```

枚举
```C
enum gpio_num_t
	// GPIO 引脚号，输入与输出
	GPIO_NUM_0 = 0
	GPIO_NUM_1 = 1
	GPIO_NUM_2 = 
	GPIO_NUM_3 = 3
	GPIO_NUM_4 = 4
	GPIO_NUM_5 = 5
	GPIO_NUM_6 = 6
	GPIO_NUM_7 = 7
	GPIO_NUM_8 = 8
	GPIO_NUM_9 = 9
	GPIO_NUM_10 = 10
	GPIO_NUM_11 = 11
	GPIO_NUM_12 = 12
	GPIO_NUM_13 = 13
	GPIO_NUM_14 = 14
	GPIO_NUM_15 = 15
	GPIO_NUM_16 = 16
	GPIO_NUM_MAX = 17
```

```C
enum gpio_int_type_t
	// 禁止 GPIO 中断
	GPIO_INTR_DISABLE = 0
	// GPIO 中断类型：上升沿触发
	GPIO_INTR_POSEDGE = 1
	// GPIO 中断类型：下降沿触发
	GPIO_INTR_NEGEDGE = 2
	// GPIO 中断类型：上升及下降沿触发
	GPIO_INTR_ANYEDGE = 3
	// GPIO 中断类型：输入低电平触发
	GPIO_INTR_LOW_LEVEL = 4
	// GPIO 中断类型：输入高电平触发
	GPIO_INTR_HIGH_LEVEL = 5
	GPIO_INTR_MAX
```

```C
enum gpio_mode_t
	// 禁止输入和输出模式
	GPIO_MODE_DISABLE = GPIO_MODE_DEF_DISABLE
	// 仅输入模式input only
	GPIO_MODE_INPUT = GPIO_MODE_DEF_INPUT
	// 仅输出模式output only mode
	GPIO_MODE_OUTPUT = GPIO_MODE_DEF_OUTPUT
	// 仅开漏输出模式
	GPIO_MODE_OUTPUT_OD = ((GPIO_MODE_DEF_OUTPUT) | (GPIO_MODE_DEF_OD))
```

```C
enum gpio_pull_mode_t
	// 仅上拉
	GPIO_PULLUP_ONLY
	// 仅下拉
	GPIO_PULLDOWN_ONLY
	// 浮动
	GPIO_FLOATING
```

```C
enum gpio_pullup_t
	// 禁止 GPIO 上拉寄存器
	GPIO_PULLUP_DISABLE = 0x0
	// 使能 GPIO 上拉寄存器
	GPIO_PULLUP_ENABLE = 0x1
```

```C
enum gpio_pulldown_t
	// 禁止 GPIO 下拉寄存器
	GPIO_PULLDOWN_DISABLE = 0x0
	// 使能 GPIO 下拉寄存器
	GPIO_PULLDOWN_ENABLE = 0x1
```

### 5. 日志 API（部分）

#### i.头文件：`log/include/esp_log.h`

#### ii.函数概览

```C
putchar_like_t esp_log_set_putchar(putchar_like_t func)
uint32_t esp_log_timestamp(void)
uint32_t esp_log_early_timestamp(void)
void esp_log_write(esp_log_level_t level, const char *tag, const char *format, ...)
void esp_early_log_write(esp_log_level_t level, const char *tag, const char *format, ...)
```

#### iii.函数说明：


```C
void esp_log_write(esp_log_level_t level, const char *tag, const char *format, ...)
```
输出消息到日志
此函数不应该被直接调用。作为代替，应该使用任一宏：`ESP_LOGE`, `ESP_LOGW`, `ESP_LOGI`, `ESP_LOGD,` `ESP_LOGV`
此函数，或这些宏，不应该被用于中断处理函数中



---

宏
```C
// Log Error level
ESP_LOGE(tag, format, ...)
// Log Warning level
ESP_LOGW(tag, format, ...)
// Log Info level
ESP_LOGI(tag, format, ...)
// Log Debug level
ESP_LOGD(tag, format, ...)
// Log Verbose level
ESP_LOGV(tag, format, ...)
```

