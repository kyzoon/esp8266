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

	// 安装 GPIO ISR 中断服务程序，参数 0 并没有实际意义
	gpio_install_isr_service(0);
	// 设置 GPIO4 中断服务程序
	gpio_isr_handler_add(GPIO_NUM_4, gpio_isr_handler, (void *) GPIO_NUM_4);
	// 设置 GPIO5 中断服务程序
	gpio_isr_handler_add(GPIO_NUM_5, gpio_isr_handler, (void *) GPIO_NUM_5);
	
	// 删除 GPIO4 中断服务程序
	// 此处代码只是演示怎么删除 GPIO 中断服务程序，无实际意义，可以删除或注释掉
	// gpio_isr_handler_remove(GPIO_NUM_4);
	// 设置 GPIO4 中断服务程序
	// gpio_isr_handler_add(GPIO_NUM_4, gpio_isr_handler, (void *) GPIO_NUM_4);

	// 创建队列，用于处理 GPIO ISR 发来的事件
	gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
	// 创建 GPIO 任务
	xTaskCreate(gpio_task_example, "gpio_task_example", 2048, NULL, 10, NULL);

	int cnt = 0;

	for(;;)
	{
		ESP_LOGI(TAG, "cnt: %d", cnt++);
		vTaskDelay(1000 / portTICK_RATE_MS);
		// 设置 GPIO15/16 输出电平高低，根据 cnt 计数器进行高低电平翻转，生成脉冲
		gpio_set_level(GPIO_NUM_15, cnt % 2);
		gpio_set_level(GPIO_NUM_16, cnt % 2);
	}
}


