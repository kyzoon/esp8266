#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "esp_log.h"

#include "driver/gpio.h"
/* hw_timer 驱动 */
#include "driver/hw_timer.h"

static const char *TAG = "hw_timer_example";

// 仅一次触发，不会重载
#define TEST_ONE_SHOT	false
// 重载模式
#define TEST_RELOAD	true

/**
 * 说明:
 * 本实例展示如何使用 hw_timer 定时器
 * 使用 hw_timer 产生不同频率的波形
 *
 * GPIO 配置状态:
 * GPIO12: 输出
 * GPIO15: 输出
 *
 * 测试:
 * 连接 GPIO12 至 LED1
 * 连接 GPIO15 至 LED2
 * 在 GPIO12/15 两个 IO 口产生不同频率的波形输出, 由此来控制 LED
 */

void hw_timer_callback1(void *arg)
{
	static int state = 0;

	gpio_set_level(GPIO_NUM_12, (state++) % 2);
}

void hw_timer_callback2(void *arg)
{
	static int state = 0;

	gpio_set_level(GPIO_NUM_15, (state++) % 2);
}

void hw_timer_callback3(void *arg)
{
	gpio_set_level(GPIO_NUM_12, 0);
	gpio_set_level(GPIO_NUM_15, 1);
}

void app_main(void)
{
	gpio_config_t io_conf;
	
	io_conf.intr_type = GPIO_INTR_DISABLE;
	io_conf.mode = GPIO_MODE_OUTPUT;
	io_conf.pin_bit_mask = GPIO_Pin_12 | GPIO_Pin_15;
	io_conf.pull_down_en = 0;
	io_conf.pull_up_en = 0;
	gpio_config(&io_conf);
	
	ESP_LOGI(TAG, "Initialize hw_timer for callback1");
	// 初始化 hw_timer
	hw_timer_init(hw_timer_callback1, NULL);
	ESP_LOGI(TAG, "Set hw_timer timing time 100us with reload");
	// 设置 100us 报警定时器，并使能 hw_timer，重载模式
	hw_timer_alarm_us(100, TEST_RELOAD);
	// FreeRTOS 延时，让 100us 的定时器运行 1 秒，应该有 5000 个方波
	vTaskDelay(1000 / portTICK_RATE_MS);
	ESP_LOGI(TAG, "Deinitialize hw_timer for callback1");
	
	// 重置 hw_timer
	hw_timer_deinit();
	
	ESP_LOGI(TAG, "Initialize hw_timer for callback2");
	// 初始化 hw_timer
	hw_timer_init(hw_timer_callback2, NULL);
	ESP_LOGI(TAG, "Set hw_timer timing time 1ms with reload");
	// 设置 1ms 报警定时器，并使能 hw_timer，重载模式
	hw_timer_alarm_us(1000, TEST_RELOAD);
	// FreeRTOS 延时，让 1ms 的定时器运行 1 秒，应该有 500  个方波
	vTaskDelay(1000 / portTICK_RATE_MS);
	
	ESP_LOGI(TAG, "Set hw_timer timing time 10ms with reload");
	// 设置 10ms 报警定时器，并使能 hw_timer，重载模式
	hw_timer_alarm_us(10000, TEST_RELOAD);
	
	// FreeRTOS 延时，让 10ms 的定时器运行 2 秒，应该有 100 个方波
	vTaskDelay(2000 / portTICK_RATE_MS);
	ESP_LOGI(TAG, "Set hw_timer timing time 100ms with reload");
	// 设置 100ms 报警定时器，并使能 hw_timer，重载模式
	hw_timer_alarm_us(100000, TEST_RELOAD);
	// FreeRTOS 延时，让 10ms 的定时器运行 3 秒 15 个方波
	vTaskDelay(3000 / portTICK_RATE_MS);
	ESP_LOGI(TAG, "Cancel timing");
	// 禁止 hw_timer
	hw_timer_disarm();
	// 重置 hw_timer
	hw_timer_deinit();
	
	ESP_LOGI(TAG, "Initialize hw_timer for callback3");
	// 初始化 hw_timer
	hw_timer_init(hw_timer_callback3, NULL);
	
	ESP_LOGI(TAG, "Set hw_timer timing time 1ms with one-shot");
	// 设置 1ms 报警定时器，并使能 hw_timer，单次模式
	hw_timer_alarm_us(1000, TEST_ONE_SHOT);
}


