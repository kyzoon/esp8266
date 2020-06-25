/**
 * 说明:
 * 本实例展示如何使用 IIC
 * 使用 IIC 控制 MPU6050 六轴传感器
 *
 * GPIO 配置状态:
 * GPIO14 作为主机 SDA 连接至 MPU6050 SDA
 * GPIO2  作为主机 SCL 连接到 MPU6050 SCL
 * 不必要增加外部上拉电阻，驱动程序将使能内部上拉电阻
 *
 * 测试:
 * 如果连接上传感器，则读取数据
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


static const char *TAG = "DS3231";

#define I2C_PORT_2_DS3231			I2C_NUM_0        /*!< 主机设备 IIC 端口号 */

#define DS3231_ADDR					0x68             /*!< 从机 MPU6050 地址 */
#define WRITE_BIT                   I2C_MASTER_WRITE /*!< I2C 主机写操作位 */
#define READ_BIT                    I2C_MASTER_READ  /*!< I2C 主机读操作位 */
#define ACK_CHECK_EN                0x1              /*!< I2C 主机确认接收从机 ACK 信号 */
#define ACK_CHECK_DIS               0x0              /*!< I2C 主机不接收从机 ACK 信号  */
#define ACK_VAL                     0x0              /*!< I2C ACK值 */
#define NACK_VAL                    0x1              /*!< I2C NACK值 */
#define LAST_NACK_VAL               0x2              /*!< I2C 末尾ACK值 */

/**
 * MPU6050 寄存器地址
 */
#define REG_SEC					0x00
#define REG_MIN					0x01
#define REG_HOUR				0x02
#define REG_DAY					0x03
#define REG_DATE				0x04
#define REG_MONTH				0x05
#define REG_YEAR				0x06
#define REG_A1_SEC				0x07
#define REG_A1_MIN				0x08
#define REG_A1_HOUR				0x09
#define REG_A1_DATE				0x0A
#define REG_A2_MIN				0x0B
#define REG_A2_HOUR				0x0C
#define REG_A2_DATE				0x0D
#define REG_CTRL				0x0E
#define REG_CTRL_STATUS			0x0F
#define REG_AGING_OFFSET		0x10
#define REG_TEMP_MSB			0x11
#define REG_TEMP_LSB			0x12

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
	ESP_ERROR_CHECK(i2c_driver_install(I2C_PORT_2_DS3231, conf.mode));
	// 设置 IIC 引脚配置
	ESP_ERROR_CHECK(i2c_param_config(I2C_PORT_2_DS3231, &conf));

	return ESP_OK;
}

/* IIC 主机发送数据 */
static esp_err_t ds3231_write(i2c_port_t i2c_num, uint8_t reg_address,
							   uint8_t *data, size_t data_len)
{
	int ret = 0;
	// 创建 IIC 命令连接
	i2c_cmd_handle_t cmd = i2c_cmd_link_create();

	// 创建一个命令队列，并装载一个开始信号
	i2c_master_start(cmd);
	// 装载一个从机地址及写指令，ACK 应答使能
	i2c_master_write_byte(cmd, DS3231_ADDR << 1 | WRITE_BIT, ACK_CHECK_EN);
	// 装载从机寄存器地址，ACK应答使能
	i2c_master_write_byte(cmd, reg_address, ACK_CHECK_EN);
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

/**
 * 1. IIC 主机发送待读取寄存器地址到从设备
 * 2. IIC 主机读取从机返回的数据
 */
static esp_err_t ds3231_read(i2c_port_t i2c_num, uint8_t reg_address,
							  uint8_t *data, size_t data_len)
{
	int ret;
	// 创建 IIC 命令连接
	i2c_cmd_handle_t cmd = i2c_cmd_link_create();

	// 给从机发送读数据地址，通知其准备数据
	// 创建一个命令队列，并装载一个开始信号
	i2c_master_start(cmd);
	// 装载一个从机地址及写指令，ACK 应答使能
	i2c_master_write_byte(cmd, DS3231_ADDR << 1 | WRITE_BIT, ACK_CHECK_EN);
	// 装载从机寄存器地址，即待读取数据的寄存器地址，ACK应答使能
	i2c_master_write_byte(cmd, reg_address, ACK_CHECK_EN);
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
	i2c_master_write_byte(cmd, DS3231_ADDR << 1 | READ_BIT, ACK_CHECK_EN);
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

static void ds3231_set_datetime(uint8_t sec, uint8_t min, uint8_t hour,
								uint8_t weekday,
								uint8_t day, uint8_t mon, uint8_t year)
{
	uint8_t datetime[7];

	datetime[0] = ((sec / 10) << 4) | (sec % 10);
	datetime[1] = ((min / 10) << 4) | (min % 10);
	datetime[2] = ((hour / 10) << 4) | (hour % 10);
	datetime[3] = ((weekday / 10) << 4) | (weekday % 10);
	datetime[4] = ((day / 10) << 4) | (day % 10);
	datetime[5] = ((mon / 10) << 4) | (mon % 10);
	datetime[6] = ((year / 10) << 4) | (year % 10);

	// 设置 DS3231 RTC 时间
	ESP_ERROR_CHECK(ds3231_write(I2C_PORT_2_DS3231, REG_SEC, datetime, 7));
}

static void ds3231_set_alarm1(uint8_t day, uint8_t hour, uint8_t min, uint8_t sec, uint8_t is_by_day)
{
	uint8_t datetime[4];

	datetime[0] = ((sec / 10) << 4) | (sec % 10);
	datetime[1] = ((min / 10) << 4) | (min % 10);
	datetime[2] = ((hour / 10) << 4) | (hour % 10);
	datetime[3] = ((day / 10) << 4) | (day % 10);

	if(1 == is_by_day)
	{
		datetime[3] |= 0x40;
	}

	// 设置 DS3231 RTC 时间
	ESP_ERROR_CHECK(ds3231_write(I2C_PORT_2_DS3231, REG_A1_SEC, datetime, 4));
}

static void ds3231_set_alarm2(uint8_t day, uint8_t hour, uint8_t min, uint8_t is_by_day)
{
	uint8_t datetime[3];

	datetime[0] = ((min / 10) << 4) | (min % 10);
	datetime[1] = ((hour / 10) << 4) | (hour % 10);
	datetime[2] = ((day / 10) << 4) | (day % 10);

	if(1 == is_by_day)
	{
		datetime[2] |= 0x40;
	}

	// 设置 DS3231 RTC 时间
	ESP_ERROR_CHECK(ds3231_write(I2C_PORT_2_DS3231, REG_A2_MIN, datetime, 3));
}

/* 初始化 DS3231 */
static esp_err_t ds3231_init(i2c_port_t i2c_num)
{
	uint8_t cmd_data = 0;

	vTaskDelay(100 / portTICK_RATE_MS);

	i2c_module_init();	// 初始化 IIC 接口

	// 配置 DS3231，RS: 8kHz, 中断使能，闹钟 1 和 2 中断使能
	cmd_data = 0x1F;
	ESP_ERROR_CHECK(ds3231_write(I2C_PORT_2_DS3231, REG_CTRL, &cmd_data, 1));
	// 配置 DS3231，清除 Alarm 1 和 Alarm 2 中断标志位
	cmd_data = 0x88;
	ESP_ERROR_CHECK(ds3231_write(I2C_PORT_2_DS3231, REG_CTRL_STATUS, &cmd_data, 1));

	ds3231_set_alarm1(4, 22, 54, 05, 1);
	ds3231_set_alarm2(4, 22, 54, 1);

	return ESP_OK;
}

static void i2c_task_example(void *arg)
{
	uint8_t datetime_data[19];
	static uint32_t error_count = 0;
	int temp = 0;
	int temp_flag = ' ';
	int ret = 0;
	int need_clean_alarm_flag = 0;

	// 初始化 MPU6050
	ds3231_init(I2C_PORT_2_DS3231);

	while(1)
	{
		memset(datetime_data, 0, 19);
		// 读取 DS3231 日期时间数据
		ret = ds3231_read(I2C_PORT_2_DS3231, REG_SEC, datetime_data, 19);
		if(ret == ESP_OK)
		{
			ESP_LOGI(TAG, "*******************");
			ESP_LOGI(TAG, "Datetime : 20%02X-%02X-%02X [%X] %02X:%02X:%02X",
					 datetime_data[6], datetime_data[5], datetime_data[4],
					 datetime_data[3],
					 datetime_data[2], datetime_data[1], datetime_data[0]);

			ESP_LOGI(TAG, "Alarm 1  : %s %02X %02X:%02X:%02X",
					 datetime_data[10] & 0x40 ? "Each Weekday:" : "Each Month Date:",
					 datetime_data[10] & 0xBF,
					 datetime_data[9], datetime_data[8], datetime_data[7]);

			ESP_LOGI(TAG, "Alarm 2  : %s %02X %02X:%02X",
					 datetime_data[13] & 0x40 ? "Each Weekday:" : "Each Month Date:",
					 datetime_data[13] & 0xBF,
					 datetime_data[12], datetime_data[11]);

			ESP_LOGI(TAG, "Control  : %02X", datetime_data[14]);
			ESP_LOGI(TAG, "Status   : %02X", datetime_data[15]);
			if(0 != (datetime_data[15] & 0x02))
			{
				ESP_LOGI(TAG, "         - Alarm 2 Trigger");
				datetime_data[15] &= 0xFD;
				need_clean_alarm_flag = 1;
			}
			if(0 != (datetime_data[15] & 0x01))
			{
				ESP_LOGI(TAG, "         - Alarm 1 Trigger");
				datetime_data[15] &= 0xFE;
				need_clean_alarm_flag = 1;
			}
			ESP_LOGI(TAG, "Aging    : %02X", datetime_data[16]);

			temp = (datetime_data[17] << 8) | datetime_data[18];
			if(0 != (temp & 0x8000))
			{
				temp_flag = '-';
				temp &= 0x7FFF;
			}
			temp >>= 6;
			ESP_LOGI(TAG, "Temp     :%c%d", temp_flag, temp);

			ESP_LOGI(TAG, "error_count: %d\n", error_count);

			if(1 == need_clean_alarm_flag)
			{
				ESP_ERROR_CHECK(ds3231_write(I2C_PORT_2_DS3231, REG_CTRL_STATUS, &datetime_data[15], 1));
			}
		}
		else
		{
			ESP_LOGE(TAG, "No ack, sensor not connected...skip...\n");
		}

		vTaskDelay(3000 / portTICK_RATE_MS);
	}

	i2c_driver_delete(I2C_PORT_2_DS3231);
}

void app_main(void)
{
	xTaskCreate(i2c_task_example, "i2c_task_example", 2048, NULL, 10, NULL);
}