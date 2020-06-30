/**
 * 说明:
 * 本实例展示如何使用 IIC
 * 使用 IIC 控制 AT24C32 EEPROM 读写
 *
 * GPIO 配置状态:
 * GPIO14 作为主机 SDA 连接至 AT24C32 SDA
 * GPIO2  作为主机 SCL 连接到 AT24C32 SCL
 * 不必要增加外部上拉电阻，驱动程序将使能内部上拉电阻
 *
 * 测试:
 * 如果连接上 AT24C32 EEPROM，测试读写数据
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

#include "driver/i2c.h"
#include "driver/gpio.h"


static const char *TAG = "AT24C32";

#define I2C_PORT_2_AT24C32			I2C_NUM_0        /*!< 主机设备 IIC 端口号 */

/*
默认 AT24C32 芯片上 A0，A1，A2 引脚为低电平
测量了 DS3231 模块上的 AT24C23 芯片 A0，A1，A2 引脚都为高电平，所以地址为 0xAE
*/
#define AT24C32_ADDR				0xAE             /*!< 从机 AT24C32 地址 */
#define WRITE_BIT                   I2C_MASTER_WRITE /*!< I2C 主机写操作位 */
#define READ_BIT                    I2C_MASTER_READ  /*!< I2C 主机读操作位 */
#define ACK_CHECK_EN                0x1              /*!< I2C 主机确认接收从机 ACK 信号 */
#define ACK_CHECK_DIS               0x0              /*!< I2C 主机不接收从机 ACK 信号  */
#define ACK_VAL                     0x0              /*!< I2C ACK值 */
#define NACK_VAL                    0x1              /*!< I2C NACK值 */
#define LAST_NACK_VAL               0x2              /*!< I2C 末尾ACK值 */

// 测试读写数据，跨页情况
#define AT24C32_TEST_DATA_LEN		(66)

/* IIC 主机初始化：主机模式，GPIO */
static esp_err_t i2c_module_init(void)
{
	i2c_config_t conf;

	// 主机模式
	conf.mode = I2C_MODE_MASTER;
	// GPIO14 -> SDA
	conf.sda_io_num = GPIO_NUM_14;
	// SDA 引脚上拉
	conf.sda_pullup_en = 0;
	// GPIO2 -> SCL
	conf.scl_io_num = GPIO_NUM_2;
	// SCL 引脚上拉
	conf.scl_pullup_en = 0;

	// 设置 IIC 工作模式
	ESP_ERROR_CHECK(i2c_driver_install(I2C_PORT_2_AT24C32, conf.mode));
	// 设置 IIC 引脚配置
	ESP_ERROR_CHECK(i2c_param_config(I2C_PORT_2_AT24C32, &conf));

	return ESP_OK;
}

static esp_err_t at24c32_init(i2c_port_t i2c_num)
{
	vTaskDelay(100 / portTICK_RATE_MS);

	i2c_module_init();	// 初始化 IIC 接口

	return ESP_OK;
}

static esp_err_t at24c32_write_in_page(i2c_port_t i2c_num, uint16_t reg_address,
									   uint8_t *data, size_t data_len)
{
	int ret = 0;
	// 创建 IIC 命令连接
	i2c_cmd_handle_t cmd = i2c_cmd_link_create();

	// 创建一个命令队列，并装载一个开始信号
	i2c_master_start(cmd);
	// 装载一个从机地址及写指令，ACK 应答使能
	i2c_master_write_byte(cmd, AT24C32_ADDR | WRITE_BIT, ACK_CHECK_EN);
	// 装载从机寄存器地址，ACK应答使能
	i2c_master_write_byte(cmd, (uint8_t)(reg_address >> 8), ACK_CHECK_EN);
	i2c_master_write_byte(cmd, (uint8_t)(reg_address & 0xFF), ACK_CHECK_EN);
	// 装载写至从机寄存器的数据，ACK应答使能
	i2c_master_write(cmd, data, data_len, ACK_CHECK_EN);
	// 装载停止信号
	i2c_master_stop(cmd);

	// 发送命令队列中的数据
	ret = i2c_master_cmd_begin(i2c_num, cmd, 1000 / portTICK_RATE_MS);
	// 释放命令连接
	i2c_cmd_link_delete(cmd);

	return ret;
}

/* 支持跨页写数据 */
static esp_err_t at24c32_write(i2c_port_t i2c_num, uint16_t reg_address,
							  uint8_t *data, size_t data_len)
{
	int ret = 0;
	uint16_t cur_addr = reg_address;
	uint8_t *cur_data = data;
	size_t cur_len = 0;
	size_t last_len = data_len;
	int first_flag = 1;

	cur_len = 32 - ((int)cur_addr % 32);
	last_len = data_len - cur_len;

	for(;;)
	{
		ret = at24c32_write_in_page(i2c_num, cur_addr, cur_data, cur_len);
		if(ESP_OK != ret)
		{
			return ret;
		}

		if(0 == last_len)
		{
			break;
		}

		// 下一页写数据地址
		if(1 == first_flag)
		{
			first_flag = 0;
			cur_addr += cur_len;
			cur_data += cur_len;
		}
		else
		{
			cur_addr += 32;
			cur_data += 32;
		}
		// 下一页写数据长度
		if(last_len > 32)
		{
			cur_len = 32;
			last_len -= 32;
		}
		else
		{
			cur_len = last_len;
			last_len = 0;
		}

		// 跨页写数据时，当前页写完后，需要增加适当的延时，让写入执行完成，再继续写下一页
		vTaskDelay(20 / portTICK_RATE_MS);
	}

	return ret;
}

/* 读数据，是否跨页，都不需要增加延时 */
static esp_err_t at24c32_read(i2c_port_t i2c_num, uint16_t reg_address,
								   uint8_t *data, size_t data_len)
{
	int ret;
	// 创建 IIC 命令连接
	i2c_cmd_handle_t cmd = i2c_cmd_link_create();

	// 给从机发送读数据地址，通知其准备数据
	// 创建一个命令队列，并装载一个开始信号
	i2c_master_start(cmd);
	// 装载一个从机地址及写指令，ACK 应答使能
	i2c_master_write_byte(cmd, AT24C32_ADDR | WRITE_BIT, ACK_CHECK_EN);
	// 装载从机寄存器地址，即待读取数据的寄存器地址，ACK应答使能
	i2c_master_write_byte(cmd, (uint8_t)(reg_address >> 8), ACK_CHECK_EN);
	i2c_master_write_byte(cmd, (uint8_t)(reg_address & 0xFF), ACK_CHECK_EN);

	// 装载停止信号
	i2c_master_stop(cmd);
	// 发送命令队列中的数据
	ret = i2c_master_cmd_begin(i2c_num, cmd, 1000 / portTICK_RATE_MS);
	// 释放命令连接
	i2c_cmd_link_delete(cmd);

	// 验证读取命令是否发送成功
	if(ret != ESP_OK)
	{
		return ret;
	}

	// 主机读取从机发送的数据
	cmd = i2c_cmd_link_create();
	// 创建一个命令队列，并装载一个开始信号
	i2c_master_start(cmd);
	// 装载一个从机地址及读指令，ACK 应答使能
	i2c_master_write_byte(cmd, AT24C32_ADDR | READ_BIT, ACK_CHECK_EN);
	// 装载读取命令，待读取数据缓存区和数据长度，用于从 IIC 总线读取数据，
	// 最后一个数据应答 NACK
	i2c_master_read(cmd, data, data_len, LAST_NACK_VAL);
	// 装载停止信号
	i2c_master_stop(cmd);
	// 发送命令队列中的数据
	// ** 这里不好理解 **
	// 命令队列中装载了读取的信息：缓存区和数据长度，
	// 这里才会真正处理读取数据操作。是阻塞运行。并将读取的数据存储至缓存区
	// 所以程序返回时，数据应该已经读取到缓存区了，可以再取出来用了
	// 多个数据的读取，被封装在底层驱动中了
	ret = i2c_master_cmd_begin(i2c_num, cmd, 1000 / portTICK_RATE_MS);
	// 释放命令连接
	i2c_cmd_link_delete(cmd);

	return ret;
}

static void i2c_task_example(void *arg)
{
	// uint8_t e2p_byte = 0;
	uint8_t e2p_wb[AT24C32_TEST_DATA_LEN];
	int temp = 0;
	int i = 0;

	// 初始化 AT24C32
	at24c32_init(I2C_PORT_2_AT24C32);

	for(;;)
	{
		// 写入并读取一个字节
		// ESP_LOGI(TAG, "Write 1 byte data");
		// e2p_byte = 0;
		// ESP_ERROR_CHECK(at24c32_write_in_page(I2C_PORT_2_AT24C32, 0x00, &temp, 1));
		// // 写之后立即读，需要增加延时，否则读取失败
		// vTaskDelay(20 / portTICK_RATE_MS);
		// at24c32_read(I2C_PORT_2_AT24C32, 0x00, &e2p_byte, 1);
		// ESP_LOGI(TAG, "Read E2PROM data at [0]: %X", e2p_byte);
		// ++temp;

		ESP_LOGI(TAG, "Write block data");
		// 跨页写入并读取数据
		for(i = 0; i < AT24C32_TEST_DATA_LEN; ++i)
		{
			e2p_wb[i] = temp++;
			printf("%02X ", e2p_wb[i]);
		}
		printf("\n");
		ESP_ERROR_CHECK(at24c32_write(I2C_PORT_2_AT24C32, 0x02, e2p_wb, AT24C32_TEST_DATA_LEN));

		// 写之后立即读，需要增加延时，否则读取失败
		vTaskDelay(20 / portTICK_RATE_MS);

		memset(e2p_wb, 0, AT24C32_TEST_DATA_LEN);
		// 跨页读，不需要延时
		ESP_ERROR_CHECK(at24c32_read(I2C_PORT_2_AT24C32, 0x02, e2p_wb, AT24C32_TEST_DATA_LEN));
		for(i = 0; i < AT24C32_TEST_DATA_LEN; ++i)
		{
			printf("%02X ", e2p_wb[i]);
		}
		printf("\n\n");

		vTaskDelay(5000 / portTICK_RATE_MS);
	}

	i2c_driver_delete(I2C_PORT_2_AT24C32);
}

void app_main(void)
{
	xTaskCreate(i2c_task_example, "i2c_task_example", 2048, NULL, 10, NULL);
}
