## ESP8266 学习笔记 9 —— IIC
作者：Preston_zhu<br>
日期：2020.6.24

### 0. IIC 硬件描述
ESP8266 有 4 个PWM 输出接口/通道。如下表

### 1. PWM 实例

参考 RTOS_SDK/examples/peripherals/i2c
在原实例上做了修改，方便理解内容

#### i. 流程

* 初始化 PWM，设定 PWM 周期，占空比，通道数和输出引脚
* 设置 PWM 反转输出通道
* 设置 PWM 输出相位
* 开启 PWM 输出
* 输出 5s PWM，停止 PWM 5s，然后再开启/停止，一直重复 ...

#### ii. 主程序分析

```C
```

#### iii. 打印输出

```shell
I (383) reset_reason: RTC reset 1 wakeup 0 store 0, reason is 1
I (483) gpio: GPIO[14]| InputEn: 0| OutputEn: 1| OpenDrain: 1| Pullup: 0| Pulldown: 0| Intr:0
I (483) gpio: GPIO[2]| InputEn: 0| OutputEn: 1| OpenDrain: 1| Pullup: 0| Pulldown: 0| Intr:0
I (503) main: *******************
I (503) main: WHO_AM_I: 0x68
I (503) main: TEMP: 28.41
I (513) main: Accel X: 7938
I (513) main: Accel Y: 14390
I (523) main: Accel Z: -310
I (523) main: Gyros X: -45
I (533) main: Gyros Y: -20
I (533) main: Gyros Z: -10
I (543) main: error_count: 0

I (3553) main: *******************
I (3553) main: WHO_AM_I: 0x68
I (3553) main: TEMP: 28.41
I (3553) main: Accel X: 7938
I (3563) main: Accel Y: 14390
I (3563) main: Accel Z: -310
I (3573) main: Gyros X: -45
I (3573) main: Gyros Y: -20
I (3583) main: Gyros Z: -10
I (3583) main: error_count: 0
```

#### iv. 图解
间隔周期 5s
![pwm1](images/pwm1.png)
通道 3 波形迟通道 2 波形 138us

#### v. PWM 应用代码流程
![hw_timer应用流程.png](images\hw_timer应用流程.png)

##### vi. 疑问
**既然 PWM 是由软件实现，PWM 输出口可以指定 GPIO 引脚，为什么官方文档里还是提及 PWM 有4个通道？？**

### 3. API

#### i.头文件：`esp266/include/driver/i2c.h`

#### ii.函数概览：

```C
esp_err_t i2c_driver_install(i2c_port_t i2c_num, i2c_mode_t mode);
esp_err_t i2c_driver_delete(i2c_port_t i2c_num);
esp_err_t i2c_param_config(i2c_port_t i2c_num, const i2c_config_t *i2c_conf);
esp_err_t i2c_set_pin(i2c_port_t i2c_num, int sda_io_num, int scl_io_num,
					  gpio_pullup_t sda_pullup_en, gpio_pullup_t scl_pullup_en,
					  i2c_mode_t mode);
i2c_cmd_handle_t i2c_cmd_link_create(void);
void i2c_cmd_link_delete(i2c_cmd_handle_t cmd_handle);
esp_err_t i2c_master_start(i2c_cmd_handle_t cmd_handle)
esp_err_t i2c_master_write_byte(i2c_cmd_handle_t cmd_handle, uint8_t data, bool ack_en)
esp_err_t i2c_master_write(i2c_cmd_handle_t cmd_handle, uint8_t *data,
						   size_t data_len, bool ack_en)
esp_err_t i2c_master_read_byte(i2c_cmd_handle_t cmd_handle, uint8_t *data,
							   i2c_ack_type_t ack)
esp_err_t i2c_master_read(i2c_cmd_handle_t cmd_handle, uint8_t *data,
						  size_t data_len, i2c_ack_type_t ack)
esp_err_t i2c_master_stop(i2c_cmd_handle_t cmd_handle)
esp_err_t i2c_master_cmd_begin(i2c_port_t i2c_num, i2c_cmd_handle_t cmd_handle,
							   TickType_t ticks_to_wait)
```

#### iii.函数说明：

```C
esp_err_t pwm_clear_channel_invert(uint16_t channel_mask)
```
清除 PWM 通道反转输出设置。此函数只有当 PWM 通道已经设置为反转输出时才有效
注意：需要在配置 PWM 之后，调用 `pwm_start()` 函数，使设置生效

返回值：

* `ESP_OK` - 成功
* `ESP_ERR_INVALID_ARG` - 参数错误

参数：

* `channel_mask` - 通道位掩码，设置为需要清除的通道
  * 例如：假设初始化了 8 个通道，且通道 0，1，2，3 均已经设置为反转输出通道，如果 `channel_mask` = 0x07，则通道 0，1，2 会设置成正常输出，通道 3 则保持反转输出
#################################################
```C
esp_err_t i2c_driver_install(i2c_port_t i2c_num, i2c_mode_t mode)
```
I2C driver install.
Return
• ESP_OK Success
• ESP_ERR_INVALID_ARG Parameter error
• ESP_FAIL Driver install error
Parameters
• i2c_num: I2C port number
• mode: I2C mode( master or slave )

```C
esp_err_t i2c_driver_delete(i2c_port_t i2c_num)
```
I2C driver delete.
Return
• ESP_OK Success
• ESP_ERR_INVALID_ARG Parameter error
Parameters
• i2c_num: I2C port number

```C
esp_err_t i2c_param_config(i2c_port_t i2c_num, const i2c_config_t *i2c_conf )
```
I2C parameter initialization.
Note It must be used after calling i2c_driver_install
Return
• ESP_OK Success
• ESP_ERR_INVALID_ARG Parameter error
Parameters
• i2c_num: I2C port number
• i2c_conf: pointer to I2C parameter settings

```C
esp_err_t i2c_set_pin(i2c_port_t i2c_num, int sda_io_num, int scl_io_num, gpio_pullup_t sda_pullup_en, gpio_pullup_t scl_pullup_en, i2c_mode_t mode)
```
Configure GPIO signal for I2C sck and sda.
Return
• ESP_OK Success
• ESP_ERR_INVALID_ARG Parameter error
Parameters
• i2c_num: I2C port number
• sda_io_num: GPIO number for I2C sda signal
• scl_io_num: GPIO number for I2C scl signal
• sda_pullup_en: Whether to enable the internal pullup for sda pin
• scl_pullup_en: Whether to enable the internal pullup for scl pin
• mode: I2C mode

```C
i2c_cmd_handle_t i2c_cmd_link_create(void)
```
Create and init I2C command link.
Note Before we build I2C command link, we need to call i2c_cmd_link_create() to create a command link.
After we finish sending the commands, we need to call i2c_cmd_link_delete() to release and return the
resources.
Return i2c command link handler

```C
void i2c_cmd_link_delete(i2c_cmd_handle_t cmd_handle)
```
Free I2C command link.
Note Before we build I2C command link, we need to call i2c_cmd_link_create() to create a command link.
After we finish sending the commands, we need to call i2c_cmd_link_delete() to release and return the
resources.
Parameters
• cmd_handle: I2C command handle

```C
esp_err_t i2c_master_start(i2c_cmd_handle_t cmd_handle)
```
Queue command for I2C master to generate a start signal.
Note Only call this function in I2C master mode Call i2c_master_cmd_begin() to send all queued commands
Return
• ESP_OK Success
• ESP_ERR_INVALID_ARG Parameter error
Parameters
• cmd_handle: I2C cmd link

```C
esp_err_t i2c_master_write_byte(i2c_cmd_handle_t cmd_handle, uint8_t data, bool ack_en)
```
Queue command for I2C master to write one byte to I2C bus.
Note Only call this function in I2C master mode Call i2c_master_cmd_begin() to send all queued commands
Return
• ESP_OK Success
• ESP_ERR_INVALID_ARG Parameter error
Parameters
• cmd_handle: I2C cmd link
• data: I2C one byte command to write to bus
• ack_en: enable ack check for master

```C
esp_err_t i2c_master_write(i2c_cmd_handle_t cmd_handle, uint8_t *data, size_t data_len, bool
```
ack_en)
Queue command for I2C master to write buffer to I2C bus.
Note Only call this function in I2C master mode Call i2c_master_cmd_begin() to send all queued commands
Return
• ESP_OK Success
• ESP_ERR_INVALID_ARG Parameter error
Parameters
• cmd_handle: I2C cmd link
• data: data to send
• data_len: data length
• ack_en: enable ack check for master

```C
esp_err_t i2c_master_read_byte(i2c_cmd_handle_t cmd_handle, uint8_t *data, i2c_ack_type_t ack)
```
Queue command for I2C master to read one byte from I2C bus.
Note Only call this function in I2C master mode Call i2c_master_cmd_begin() to send all queued commands
Return
• ESP_OK Success
• ESP_ERR_INVALID_ARG Parameter error
Parameters
• cmd_handle: I2C cmd link
• data: pointer accept the data byte
• ack: ack value for read command

```C
esp_err_t i2c_master_read(i2c_cmd_handle_t cmd_handle, uint8_t *data, size_t data_len,
i2c_ack_type_t ack)
```
Queue command for I2C master to read data from I2C bus.
Note Only call this function in I2C master mode Call i2c_master_cmd_begin() to send all queued commands
Return
• ESP_OK Success
• ESP_ERR_INVALID_ARG Parameter error
Parameters
• cmd_handle: I2C cmd link
• data: data buffer to accept the data from bus
• data_len: read data length
• ack: ack value for read command

```C
esp_err_t i2c_master_stop(i2c_cmd_handle_t cmd_handle)
```
Queue command for I2C master to generate a stop signal.
Note Only call this function in I2C master mode Call i2c_master_cmd_begin() to send all queued commands
Return
• ESP_OK Success
• ESP_ERR_INVALID_ARG Parameter error
Parameters
• cmd_handle: I2C cmd link

```C
esp_err_t i2c_master_cmd_begin(i2c_port_t i2c_num,
							   i2c_cmd_handle_t cmd_handle,
							   TickType_t ticks_to_wait)
```
I2C master send queued commands. This function will trigger sending all queued commands. The task will be
blocked until all the commands have been sent out. The I2C APIs are not thread-safe, if you want to use one
I2C port in different tasks, you need to take care of the multi-thread issue.
Note Only call this function in I2C master mode
Return
• ESP_OK Success
• ESP_ERR_INVALID_ARG Parameter error
• ESP_FAIL Sending command error, slave doesn’t ACK the transfer.
• ESP_ERR_INVALID_STATE I2C driver not installed or not in master mode.
• ESP_ERR_TIMEOUT Operation timeout because the bus is busy.
Parameters
• i2c_num: I2C port number
• cmd_handle: I2C command handler
• ticks_to_wait: maximum wait ticks.

---

结构体
```C
struct i2c_config_t
	I2C initialization parameters.
	Public Members
	i2c_mode_t mode
	I2C mode
	gpio_num_t sda_io_num
	GPIO number for I2C sda signal
	gpio_pullup_t sda_pullup_en
	Internal GPIO pull mode for I2C sda signal
	gpio_num_t scl_io_num
	GPIO number for I2C scl signal
	gpio_pullup_t scl_pullup_en
	Internal GPIO pull mode for I2C scl signal
	uint32_t clk_stretch_tick
	Clock Stretch time, depending on CPU frequency
```

类型定义
```C
typedef void *i2c_cmd_handle_t
// IIC 指令句柄
```

枚举
```C
enum i2c_mode_t
	I2C_MODE_MASTER				// IIC 主机模式
	I2C_MODE_MAX
```

```C
enum i2c_rw_t
	I2C_MASTER_WRITE = 0		// IIC 写数据
	I2C_MASTER_READ				// IIC 读数据
```

```C
enum i2c_opmode_t
	I2C_CMD_RESTART = 0			// IIC 重新开始指令
	I2C_CMD_WRITE				// IIC 写指令
	I2C_CMD_READ				// IIC 读指令
	I2C_CMD_STOP				// IIC 停止指令
```

```C
enum i2c_port_t
	I2C_NUM_0 = 0           	// IIC 端口 0
	I2C_NUM_MAX
```

```C
enum i2c_ack_type_t
	I2C_MASTER_ACK = 0x0		// IIC 读取每字节数据都应答 ACK
	I2C_MASTER_NACK = 0x1		// IIC 读取每字节数据不应答 NACK
	I2C_MASTER_LAST_NACK = 0x2	// IIC 读取末尾字节数据不应答 NACK
	I2C_MASTER_ACK_MAX
```

### 4. 相位计算

周期T：period，单位：s

频率f：frequency = 1 / 周期

相位：Phase = 2&pound;f

相位范围应当为：(-180, 180)

相位差：针对同频率波形而言

简单的理解，就是两个波形（周期性）相对应部分的时间差，例如起始位置的时间差

实例中，设置了相位差分别为：0，0，50，-50，则对应计算 4 个通道输出波形时间差为：
* 通道 0：时间差 = 0s
* 通道 1：时间差 = 0s
* 通道 2：时间差 = 50 / 180 x 500us = 138us
* 通道 3：时间差 = 250us - 50 / 180 x 500us = 112us

这里有个疑问：

相位计算取的半个周期： 50 度 / 180 度，然后使用了一个周期的时间 500us，计算的值与实际的值一致。为什么不是用半个周期的时间 250us 呢？
猜想：应该是因为此方波没有负半轴的波形，相位只有 180 度

### 5. 其它实例测试
#### i. 特殊相位差实例

相位参数为 (0, 0, 90, -90) 
![pwm4](images/pwm4.png)
从图可知：
通道 2 波形与通道 3 和 4 均差半个波形，因为相位相差 90 度
通道 3 与 4 则波形相同，因为相位相关 180 度

#### ii. 不同占空比实例

占空比参数为 (100, 200, 300, 400)
![pwm5](images/pwm5.png)

#### iii. 下图为实例输出五个通道

具体参数为：
* GPIO：12/13/14/15/16  
* 占空比：(250, 250, 250, 250, 250)
* 相位：(0, 0, 0, 0, 0)
![pwm6](images/pwm6.png)
成功验证了多于 4 个 PWM 通道输出


