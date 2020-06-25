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


static const char *TAG = "main";

#define I2C_PORT_2_MPU6050          I2C_NUM_0        /*!< 主机设备 IIC 端口号 */

#define MPU6050_SENSOR_ADDR         0x68             /*!< 从机 MPU6050 地址 */
#define MPU6050_CMD_START           0x41             /*!< 设置 MPU6050 测量模式指令 */
#define MPU6050_WHO_AM_I            0x75             /*!< 读取 MPU6050 WHO_AM_I 寄存器指令 */
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
#define SMPLRT_DIV                  0x19
#define CONFIG                      0x1A
#define GYRO_CONFIG                 0x1B
#define ACCEL_CONFIG                0x1C
#define ACCEL_XOUT_H                0x3B
#define ACCEL_XOUT_L                0x3C
#define ACCEL_YOUT_H                0x3D
#define ACCEL_YOUT_L                0x3E
#define ACCEL_ZOUT_H                0x3F
#define ACCEL_ZOUT_L                0x40
#define TEMP_OUT_H                  0x41
#define TEMP_OUT_L                  0x42
#define GYRO_XOUT_H                 0x43
#define GYRO_XOUT_L                 0x44
#define GYRO_YOUT_H                 0x45
#define GYRO_YOUT_L                 0x46
#define GYRO_ZOUT_H                 0x47
#define GYRO_ZOUT_L                 0x48
#define SIG_PATH_RST                0x68
#define PWR_MGMT_1                  0x6B
#define WHO_AM_I                    0x75

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
	ESP_ERROR_CHECK(i2c_driver_install(I2C_PORT_2_MPU6050, conf.mode));
	// 设置 IIC 引脚配置
	ESP_ERROR_CHECK(i2c_param_config(I2C_PORT_2_MPU6050, &conf));

	return ESP_OK;
}

/* IIC 主机发送数据 */
static esp_err_t mpu6050_write(i2c_port_t i2c_num, uint8_t reg_address,
							   uint8_t *data, size_t data_len)
{
	int ret = 0;
	// 创建 IIC 命令连接
	i2c_cmd_handle_t cmd = i2c_cmd_link_create();

	// 创建一个命令队列，并装载一个开始信号
	i2c_master_start(cmd);
	// 装载一个从机地址及写指令，ACK 应答使能
	i2c_master_write_byte(cmd, MPU6050_SENSOR_ADDR << 1 | WRITE_BIT, ACK_CHECK_EN);
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
static esp_err_t mpu6050_read(i2c_port_t i2c_num, uint8_t reg_address,
							  uint8_t *data, size_t data_len)
{
	int ret;
	// 创建 IIC 命令连接
	i2c_cmd_handle_t cmd = i2c_cmd_link_create();

	// 给从机发送读数据地址，通知其准备数据
	// 创建一个命令队列，并装载一个开始信号
	i2c_master_start(cmd);
	// 装载一个从机地址及写指令，ACK 应答使能
	i2c_master_write_byte(cmd, MPU6050_SENSOR_ADDR << 1 | WRITE_BIT, ACK_CHECK_EN);
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
	i2c_master_write_byte(cmd, MPU6050_SENSOR_ADDR << 1 | READ_BIT, ACK_CHECK_EN);
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

/* 初始化 MPU6050 */
static esp_err_t mpu6050_init(i2c_port_t i2c_num)
{
	uint8_t cmd_data;

	vTaskDelay(100 / portTICK_RATE_MS);

	i2c_module_init();	// 初始化 IIC 接口

	// 对 MPU6050 进行必要的配置
	cmd_data = 0x00;	// 设置 PWR_MGMT_1 寄存器，唤醒 MPU6050
	ESP_ERROR_CHECK(mpu6050_write(i2c_num, PWR_MGMT_1, &cmd_data, 1));
	cmd_data = 0x07;    // 设置 SMPRT_DIV 寄存器, 设置陀螺仪输出速率分频：8 分频
	ESP_ERROR_CHECK(mpu6050_write(i2c_num, SMPLRT_DIV, &cmd_data, 1));
	cmd_data = 0x06;    // 设置 CONFIG 寄存器，设置数字低通过滤器 (DLPF) 
	ESP_ERROR_CHECK(mpu6050_write(i2c_num, CONFIG, &cmd_data, 1));
	cmd_data = 0x18;    // 设置 GYRO_CONFIG 寄存器，设置陀螺仪测量范围: +/- 1000dps
	ESP_ERROR_CHECK(mpu6050_write(i2c_num, GYRO_CONFIG, &cmd_data, 1));
	cmd_data = 0x01;    // 设置 ACCEL_CONFIG 寄存器，设置加速计测量范围：+/-2g
	ESP_ERROR_CHECK(mpu6050_write(i2c_num, ACCEL_CONFIG, &cmd_data, 1));

	return ESP_OK;
}

static void i2c_task_example(void *arg)
{
	uint8_t sensor_data[14];
	uint8_t who_am_i = 0;
	double temp = 0;
	static uint32_t error_count = 0;
	int ret = 0;

	// 初始化 MPU6050
	mpu6050_init(I2C_PORT_2_MPU6050);

	while(1)
	{
		who_am_i = 0;
		// 读取 WHO_AM_I 寄存器，验证 MPU6050 连接与数据读取
		mpu6050_read(I2C_PORT_2_MPU6050, WHO_AM_I, &who_am_i, 1);
		if(0x68 != who_am_i)
		{
			error_count++;
		}

		temp = 0;
		memset(sensor_data, 0, 14);
		// 读取 MPU6050 加速计、温度传感器与陀螺仪数据
		ret = mpu6050_read(I2C_PORT_2_MPU6050, ACCEL_XOUT_H, sensor_data, 14);

		if(ret == ESP_OK)
		{
			ESP_LOGI(TAG, "*******************");
			ESP_LOGI(TAG, "WHO_AM_I: 0x%02x", who_am_i);
			temp = 36.53 + ((double)(int16_t)((sensor_data[6] << 8) | sensor_data[7]) / 340);
			ESP_LOGI(TAG, "TEMP: %d.%d", (uint16_t)temp, (uint16_t)(temp * 100) % 100);

			ESP_LOGI(TAG, "Accel X: %d", (int16_t)((sensor_data[0] << 8) | sensor_data[1]));
			ESP_LOGI(TAG, "Accel Y: %d", (int16_t)((sensor_data[2] << 8) | sensor_data[3]));
			ESP_LOGI(TAG, "Accel Z: %d", (int16_t)((sensor_data[4] << 8) | sensor_data[5]));

			ESP_LOGI(TAG, "Gyros X: %d", (int16_t)((sensor_data[8] << 8) | sensor_data[9]));
			ESP_LOGI(TAG, "Gyros Y: %d", (int16_t)((sensor_data[10] << 8) | sensor_data[11]));
			ESP_LOGI(TAG, "Gyros Z: %d", (int16_t)((sensor_data[12] << 8) | sensor_data[13]));

			ESP_LOGI(TAG, "error_count: %d\n", error_count);
		}
		else
		{
			ESP_LOGE(TAG, "No ack, sensor not connected...skip...\n");
		}

		vTaskDelay(3000 / portTICK_RATE_MS);
	}

	i2c_driver_delete(I2C_PORT_2_MPU6050);
}

void app_main(void)
{
	xTaskCreate(i2c_task_example, "i2c_task_example", 2048, NULL, 10, NULL);
}