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

static const char *s_tag = "AS2301";

static void as2301_read(void)
{
	for(;;)
	{
		gpio_set_level(GPIO_NUM_5, 0);
		vTaskDelay(1 / portTICK_RATE_MS);
		gpio_set_level(GPIO_NUM_5, 1);
		vTaskDelay(1 / portTICK_RATE_MS);
	}
}

void gpio_set_input_mode(void)
{
	gpio_config_t io_conf;
	
	// 设置 GPIO15/16 为输出模式
	// 禁止中断
	io_conf.intr_type = GPIO_INTR_DISABLE;
	// 设置为输出模式
	io_conf.mode = GPIO_MODE_INPUT;
	// 位掩码选择 GPIO15/16，置1
	io_conf.pin_bit_mask = GPIO_Pin_5;
	// 禁止下拉
	io_conf.pull_down_en = 0;
	// 禁止上拉
	io_conf.pull_up_en = 0;

	gpio_config(&io_conf);
}

void gpio_set_output_mode(void)
{
	gpio_config_t io_conf;
	
	// 设置 GPIO15/16 为输出模式
	// 禁止中断
	io_conf.intr_type = GPIO_INTR_DISABLE;
	// 设置为输出模式
	io_conf.mode = GPIO_MODE_OUTPUT;
	// 位掩码选择 GPIO15/16，置1
	io_conf.pin_bit_mask = GPIO_Pin_5;
	// 禁止下拉
	io_conf.pull_down_en = 0;
	// 禁止上拉
	io_conf.pull_up_en = 0;

	gpio_config(&io_conf);

    gpio_set_level(GPIO_NUM_5, 1);
}
static int atry[40] = { 0 };
static int btry[40] = { 0 };
#if 0
uint8_t am2301_get_bit(void)
{
	// int retry = 0;
	atry = 0;
	btry = 0;

	// 等待低电平
	while(gpio_get_level(GPIO_NUM_5) && atry < 100)
	{
		++atry;
		os_delay_us(1);
	}
	// 等待高电平
	while(!gpio_get_level(GPIO_NUM_5) && btry < 100)
	{
		++btry;
		os_delay_us(1);
	}
	os_delay_us(40);	// 等待 40us
	if(gpio_get_level(GPIO_NUM_5))
	{
		return 1;
	}
	else
	{
		return 0;
	}
}
#endif

static uint8_t t[5000] = {0};
void app_main(void)
{
	gpio_config_t io_conf;
	
	// 设置 GPIO15/16 为输出模式
	// 禁止中断
	io_conf.intr_type = GPIO_INTR_DISABLE;
	// 设置为输出模式
	io_conf.mode = GPIO_MODE_OUTPUT;
	// 位掩码选择 GPIO15/16，置1
	io_conf.pin_bit_mask = GPIO_Pin_5;
	// 禁止下拉
	io_conf.pull_down_en = 0;
	// 禁止上拉
	io_conf.pull_up_en = 0;

	gpio_config(&io_conf);

    gpio_set_level(GPIO_NUM_5, 1);

	int i = 0;
	uint64_t data = 0;
	uint8_t bit = 0;
	uint8_t last_bit = 0;
	int pos = 0;
	uint8_t retry[120] = {0};

	for(;;)
	{
        // 每分钟读取 1 次温湿度
		vTaskDelay(5 * 1000 / portTICK_RATE_MS);
		data = 0;
		memset(atry, 0, 40);
		memset(btry, 0, 40);

        // as2301_read();
		gpio_set_level(GPIO_NUM_5, 0);
		os_delay_us(1000);
		gpio_set_level(GPIO_NUM_5, 1);
		os_delay_us(10);
		gpio_set_input_mode();
		// os_delay_us(160);

		// last_bit = gpio_get_level(GPIO_NUM_5);
		for(i = 0; i < 5000; ++i)
		{
			t[i] = gpio_get_level(GPIO_NUM_5);
			// if(last_bit != bit)
			// {
			// 	retry[pos++] = bit;
			// 	last_bit = bit;
			// }
		}
		// for(i = 0; i < 120; ++i)
		// {
		// 	printf("%d", retry[i]);
		// }
		for(i = 0; i < 5000; ++i)
		{
			printf("%d", t[i]);
		}
		printf("\n");

		gpio_set_output_mode();

		continue;

		// 等待低电平
		while(gpio_get_level(GPIO_NUM_5))
		{
			os_delay_us(1);
		}
		for(i = 0; i < 40; ++i)
		{
			// data += am2301_get_bit();
			// bit = 0;
			// bit = am2301_get_bit();

			// atry[i] = 0;
			// btry[i] = 0;

			// 等待高电平
			while(!gpio_get_level(GPIO_NUM_5) && atry[i] < 1000)
			{
				++atry[i];
				os_delay_us(1);
			}
			// 等待低电平
			while(gpio_get_level(GPIO_NUM_5) && btry[i] < 1000)
			{
				++btry[i];
				os_delay_us(1);
			}
			// os_delay_us(40);	// 等待 40us
			// if(gpio_get_level(GPIO_NUM_5))
			// {
			// 	// bit = 1;
			// 	data += 1;
			// }

			// // printf("%d - %d, %d\n", bit, atry, btry);
			// // data += bit;
			// data <<= 1;
		}
		// ESP_LOGI(s_tag, "%lld", data);
		data = 0x11001100;
		printf("%lld\n", data);
		for(i = 0; i < 40; ++i)
		{
			// ESP_LOGI(s_tag, "%d ", atry[i]);
			printf("%03d ", atry[i]);
		}
		printf("\n");
		for(i = 0; i < 40; ++i)
		{
			// ESP_LOGI(s_tag, "%d", btry[i]);
			printf("%03d ", btry[i]);
		}
		printf("\n");

		gpio_set_output_mode();
		// vTaskDelay(5 / portTICK_RATE_MS);
		// gpio_set_level(GPIO_NUM_5, 1);
		// vTaskDelay(1 / portTICK_RATE_MS);
		// vTaskSuspend
	}
}