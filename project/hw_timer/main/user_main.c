#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "esp_log.h"

#include "driver/gpio.h"
/* hw_timer 驱动 */
#include "driver/hw_timer.h"

static const char *TAG = "hw_timer_example";

// 仅一次触发
#define TEST_ONE_SHOT	false
// 重载模式
#define TEST_RELOAD		true

/**
 * 说明:
 * 本实例展示如何使用 HW_Timer 定时器
 * 使用 HW_TImer 产生不同频率的波形
 *
 * GPIO 配置状态:
 * GPIO15: 输出
 *
 * 测试:
 * 连接 GPIO15 至 LED1
 * 在 GPIO15 产生不同频率的方形波形输出, 由此来控制 LED
 */

void hw_timer_callback1(void *arg)
{
	static int state = 0;

	gpio_set_level(GPIO_NUM_15, (state ++) % 2);
}

void hw_timer_callback2(void *arg)
{
	static int state = 0;

	gpio_set_level(GPIO_NUM_15, (state ++) % 2);
}

void hw_timer_callback3(void *arg)
{
	gpio_set_level(GPIO_NUM_15, 0);
}

void app_main(void)
{
	gpio_config_t io_conf;
	
	io_conf.intr_type = GPIO_INTR_DISABLE;
	io_conf.mode = GPIO_MODE_OUTPUT;
	io_conf.pin_bit_mask = GPIO_Pin_15;
	io_conf.pull_down_en = 0;
	io_conf.pull_up_en = 0;
	gpio_config(&io_conf);
	
	// 输出周期 200us(5000Hz) 的方波 1s
	ESP_LOGI(TAG, "Initialize hw_timer for callback1");
	// 初始化 HW_Timer，注册回调函数
	hw_timer_init(hw_timer_callback1, NULL);
	ESP_LOGI(TAG, "Set hw_timer timing time 100us with reload");
	// 设置定时器报警中断 100us，重载模式，启动定时器
	hw_timer_alarm_us(100, TEST_RELOAD);
	// 延时 1s，即让定时器运行 1s
	vTaskDelay(1000 / portTICK_RATE_MS);

	ESP_LOGI(TAG, "-");
	ESP_LOGI(TAG, "Deinitialize hw_timer for callback1");
	// 重置定时器
	hw_timer_deinit();

	// 输出周期 2ms 的方波 1s
	ESP_LOGI(TAG, "-");
	ESP_LOGI(TAG, "Initialize hw_timer for callback2");
	// 初始化定时器，注册回调函数
	hw_timer_init(hw_timer_callback2, NULL);
	ESP_LOGI(TAG, "Set hw_timer timing time 1ms with reload");
	// 设置定时器报警中断 1ms，重载模式，重新启动定时器
	hw_timer_alarm_us(1000, TEST_RELOAD);
	// 延时 1s，即让定时器运行 1s
	vTaskDelay(1000 / portTICK_RATE_MS);
	
	// 输出周期 20ms 的方波 2s
	ESP_LOGI(TAG, "-");
	ESP_LOGI(TAG, "Set hw_timer timing time 10ms with reload");
	// 设置定时器报警中断 10ms，重载模式，重新启动定时器
	hw_timer_alarm_us(10000, TEST_RELOAD);
	// 延时 2s，即让定时器运行 2s
	vTaskDelay(2000 / portTICK_RATE_MS);
	
	// 输出周期 200ms 的方波 3s
	ESP_LOGI(TAG, "-");
	ESP_LOGI(TAG, "Set hw_timer timing time 100ms with reload");
	// 设置定时器报警中断 100ms，重载模式，重新启动定时器
	hw_timer_alarm_us(100000, TEST_RELOAD);
	// 延时 3s，即让定时器运行 3s
	vTaskDelay(3000 / portTICK_RATE_MS);

	ESP_LOGI(TAG, "-");
	ESP_LOGI(TAG, "Cancel timing");
	// 禁止/停止定时器
	hw_timer_disarm();
	// 重置定时器
	hw_timer_deinit();
	
	// 定时器 1ms 后关闭 LED1
	ESP_LOGI(TAG, "-");
	ESP_LOGI(TAG, "Initialize hw_timer for callback3");
	// 初始化定时器
	hw_timer_init(hw_timer_callback3, NULL);
	ESP_LOGI(TAG, "Set hw_timer timing time 1ms with one-shot");
	// 设置定时器报警中断 1ms，单次模式，启动定时器
	hw_timer_alarm_us(1000, TEST_ONE_SHOT);   
}
