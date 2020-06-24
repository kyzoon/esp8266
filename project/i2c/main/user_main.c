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

#define I2C_PORT_2_MPU6050					I2C_NUM_0        /*!< 主机设备 IIC 端口号 */

#define MPU6050_SENSOR_ADDR                 0x68             /*!< 从机 MPU6050 地址 */
#define MPU6050_CMD_START                   0x41             /*!< 设置 MPU6050 测量模式指令 */
#define MPU6050_WHO_AM_I                    0x75             /*!< 读取 MPU6050 WHO_AM_I 寄存器指令 */
#define WRITE_BIT                           I2C_MASTER_WRITE /*!< I2C 主机写操作位 */
#define READ_BIT                            I2C_MASTER_READ  /*!< I2C 主机读操作位 */
#define ACK_CHECK_EN                        0x1              /*!< I2C 主机确认接收从机 ACK 信号 */
#define ACK_CHECK_DIS                       0x0              /*!< I2C 主机不接收从机 ACK 信号  */
#define ACK_VAL                             0x0              /*!< I2C ACK值 */
#define NACK_VAL                            0x1              /*!< I2C NACK值 */
#define LAST_NACK_VAL                       0x2              /*!< I2C 末尾ACK值 */

/**
 * MPU6050 寄存器地址
 */
#define SMPLRT_DIV      0x19
#define CONFIG          0x1A
#define GYRO_CONFIG     0x1B
#define ACCEL_CONFIG    0x1C
#define ACCEL_XOUT_H    0x3B
#define ACCEL_XOUT_L    0x3C
#define ACCEL_YOUT_H    0x3D
#define ACCEL_YOUT_L    0x3E
#define ACCEL_ZOUT_H    0x3F
#define ACCEL_ZOUT_L    0x40
#define TEMP_OUT_H      0x41
#define TEMP_OUT_L      0x42
#define GYRO_XOUT_H     0x43
#define GYRO_XOUT_L     0x44
#define GYRO_YOUT_H     0x45
#define GYRO_YOUT_L     0x46
#define GYRO_ZOUT_H     0x47
#define GYRO_ZOUT_L     0x48
#define PWR_MGMT_1      0x6B
#define WHO_AM_I        0x75

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

/**
 * @brief 通过 IIC 总线写数据至 MPU6050
 *
 * 1. IIC 主机发送数据
 * ___________________________________________________________________________________________________
 * | start | slave_addr + wr_bit + ack | write reg_address + ack | write data_len byte + ack  | stop |
 * --------|---------------------------|-------------------------|----------------------------|------|
 *
 * @param i2c_num       - 端口号
 * @param reg_address   - 从机设备寄存器地址
 * @param data          - 发送数据
 * @param data_len      - 发送数据长度（字节）
 *
 * @return
 *     - ESP_OK Success         : 成功
 *     - ESP_ERR_INVALID_ARG    : 参数错误
 *     - ESP_FAIL               : 发送指令错误，从机未应答
 *     - ESP_ERR_INVALID_STATE  : IIC 驱动服务未安装或者当前非主机模式
 *     - ESP_ERR_TIMEOUT        : 总线忙，操作超时
 */
static esp_err_t mpu6050_write(i2c_port_t i2c_num, uint8_t reg_address, uint8_t *data, size_t data_len)
{
	int ret = 0;
	i2c_cmd_handle_t cmd = i2c_cmd_link_create();

	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, MPU6050_SENSOR_ADDR << 1 | WRITE_BIT, ACK_CHECK_EN);
	i2c_master_write_byte(cmd, reg_address, ACK_CHECK_EN);
	i2c_master_write(cmd, data, data_len, ACK_CHECK_EN);
	i2c_master_stop(cmd);

	ret = i2c_master_cmd_begin(i2c_num, cmd, 1000 / portTICK_RATE_MS);
	i2c_cmd_link_delete(cmd);

	return ret;
}

/**
 * @brief 通过 IIC 总线从 MPU6050 读取数据
 *
 * 1. IIC 主机发送待读取寄存器地址到从设备
 * ______________________________________________________________________
 * | start | slave_addr + wr_bit + ack | write reg_address + ack | stop |
 * --------|---------------------------|-------------------------|------|
 *
 * 2. IIC 主机读取从机返回的数据
 * ___________________________________________________________________________________
 * | start | slave_addr + wr_bit + ack | read data_len byte + ack(last nack)  | stop |
 * --------|---------------------------|--------------------------------------|------|
 *
 * @param i2c_num       - 端口号
 * @param reg_address   - 从机设备寄存器地址
 * @param data          - 读取到的数据
 * @param data_len      - 读取到的数据长度（字节）
 *
 * @return
 *     - ESP_OK Success         : 成功
 *     - ESP_ERR_INVALID_ARG    : 参数错误
 *     - ESP_FAIL               : 发送指令错误，从机未应答
 *     - ESP_ERR_INVALID_STATE  : IIC 驱动服务未安装或者当前非主机模式
 *     - ESP_ERR_TIMEOUT        : 总线忙，操作超时
 */
static esp_err_t mpu6050_read(i2c_port_t i2c_num, uint8_t reg_address, uint8_t *data, size_t data_len)
{
    int ret;

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, MPU6050_SENSOR_ADDR << 1 | WRITE_BIT, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, reg_address, ACK_CHECK_EN);
    i2c_master_stop(cmd);
    ret = i2c_master_cmd_begin(i2c_num, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);

    if(ret != ESP_OK)
    {
        return ret;
    }

    cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, MPU6050_SENSOR_ADDR << 1 | READ_BIT, ACK_CHECK_EN);
    i2c_master_read(cmd, data, data_len, LAST_NACK_VAL);
    i2c_master_stop(cmd);
    ret = i2c_master_cmd_begin(i2c_num, cmd, 1000 / portTICK_RATE_MS);
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
    cmd_data = 0x00;	// 设置 PWR_MGMT_1 寄存器，复位 MPU6050 TODO:???
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