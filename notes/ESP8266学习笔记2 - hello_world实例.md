# ESP8266 学习笔记1

### 1. 硬件信息

ESP8266-DevKitC 和 LoLin NodeMcu

| #         | ESP8266-DevKitC | LoLin NodeMcu |
| --------- | --------------- | ------------- |
| 模组型号  | ESP-WROOM-2D    | ESP-12F       |
| SPI Flash | 2MB Flash       | 4MB Flash     |
| 晶振      | 26MHz           |               |

### 2. 编译 ESP8266_RTOS_SDK (now is V3.1.2)

资源下载: [Espressif](https://www.espressif.com/zh-hans/support/download/all)

```shell
# ~/esp
$ unzip ESP8266_RTOS_SDK-3.1.2.zip
# 设置 idf 环境变量
$ export IDF_PATH=$HOME/esp/ESP8266_RTOS_SDK-3.1.2
# 利用 SDK 模板新创项目
$ mkdir hello_world
$ cp -r ESP8266_RTOS_SDK-3.1.2/examples/get-started/project_template/* hello_world/
$ cd hello_world
$ make menuconfig
# 配置内容主要确认以下：
# Serial flasher config --->
# (/dev/ttyUSB0) Default serial port 前边是硬件的连接的串口
# Flash size (2 MB) ---> 2 MB 是所使用硬件的 SPI Flash 的大小
# 其它暂时默认即可，一般不需要修改
$ make
... ...
# 不出意外，应该可以正常编译完成
```

其实也建议使用 git 去下载，这要，可以随时保证代码是最新版本

```shell
git clone --rescursive https://github.com/espressif/ESP8266_RTOS_SDK.git
```

### 3. 烧录

a. ESP8266-DevKitC

评估板使用中有如下内容：

![image-20200618153225123](E:\esp\notes\images\image-20200618153225123.png)

![image-20200617210541002](E:\esp\notes\images\image-20200617210541002.png)

所以，可以有两种方式下载，如果简单点，就将 Bit2 拨到 ON 

注意确保 /dev/ttyUSB0 或者对应的设备，可以找到

```shell
# 擦除 flash 可以使用 make erase_flash
# 烧录
$ make flash
... ...
# 运行软件及查看结果
$ make monitor
... ...
# 输出的很多日志内容中，可以到以下内容，即为 user_main.c 中的内容
SDK version：
... ...
# monitor 终端使用快捷键
# Quit: Ctrl+] | Menu: Ctrl+T | Help: Ctrl+T + Ctrl+H
```



------

#### ** /dev/ttyUSB0 无权限问题

通过将当前用户添加至 dialout 用户组解决此问题

```shell
$ sudo usermod -a -G dialout 用户名
```

然后注销再重新登录

#### ** python3 环境切换

```shell
$ sudo update-alternatives --install /usr/bin/python python /usr/bin/python2 100
$ sudo update-alternatives --install /usr/bin/python python /usr/bin/python3 150
$ sudo update-alternatives --config python
# 然后选择 python3
```

####** 在 Windows 中使用 SSCOM 查看打印输出

注意：波特率设置为 74880

将 Bit2 拨回 OFF，连接上硬件后，按一下 EN 键，即可看到打印

#### ** monitor 问题

遇到错误：

```shell
... ...
ESP8266_RTOS_SDK-3.1.2/tools/idf_monitor.py
... ...
python3.7/site-packages/serial/tools/miniterm.py
... ...
TypeError: a bytes-like object is required, not 'int'
```

[miniterm error in make monitor](https://github.com/espressif/esp-idf/issues/954) 

其中有说明，idf_monitor 只支持 python2，不支持3，也暂时没有计划什么时候会支持 python3，建议通过 SDK 配置 menuconfig 进行修改 python 支持

```shell
$ make menuconfig
# SDK tool configuration --->
# (python2) Python 2 interpreter 如果环境 python 为 3， 则此处改成 python2
```

之后使用 `make flash` 和 `make monitor` 均正常

####** ESP8266 启动时 rst cause

在 ESP8266 启动时, `ROM CODE` 会读取 `GPIO` 状态和 `rst cause` 状态, 进而决定 ESP8266 工作模式.

ESP8266 启动时会有如下打印:

```shell
ets Jan 8 2013, rst cause:1, boot mode:(5,7)
```

其中 `rst cause` 说明如下:

| 值   | 枚举定义                 | 意义                       |
| ---- | ------------------------ | -------------------------- |
| 0    | NO_MEAN                  | 无意义                     |
| 1    | VBAT_REST                | 上电复位(电源重启)         |
| 2    | EXT_SYS_RESET            | 外部复位 (deep-sleep 醒来) |
| 3    | SW_RESET                 | 软件复位                   |
| 4    | WDT_RESET                | 硬件看门狗复位             |
| 5    | DEEPSLEEP_TIMER_RESET    | /                          |
| 6    | DEEPSLEEP_POWER_ON_RESET | /                          |

> Notes:
> 软件 `WDT`  重启或者软件复位都会维持上次重启状态. 比如第一次是电源重启, `rst cause` 为 1, 软件复位后 `rst cause` 仍然为 1.

#### ** ESP8266 上电 boot mode

ESP8266 上电时会判断 `boot strapping` 管脚的状态，并决定 `boot mode`.
例如上电打印:

```shell
ets Jan 8 2013,rst cause:1, boot mode:(3,2)
```


其中 `boot mode` 说明如下:

* 第一个值代表当前 boot 模式
* 第二个值代表 SDIO/UART 判断

`boot mode` 由 `strapping` 管脚的 3 位值 `[GPIO15, GPIO0, GPIO2]` 共同决定，如下表所示:

| boot mode | Strapping 管脚的 3 位值<br>[GPIO15, GPIO0, GPIO2] | SDIO/UART 判断 | 意义                        |
| --------- | ------------------------------------------------- | -------------- | --------------------------- |
| 0         | [0, 0, 0]                                         | -              | remap boot                  |
| 1         | [0, 0, 1]                                         | -              | UART boot                   |
| 2         | [0, 1, 0]                                         | -              | jump boot                   |
| 3         | [0, 1, 1]                                         | -              | fast flash boot             |
| 4         | [1, 0, 0]                                         | 2              | SDIO lowspeed V1 UART boot  |
| 5         | [1, 0, 1]                                         | 2              | SDIO lowspeed v2 UART boot  |
| 6         | [1, 1, 0]                                         | 2              | SDIO highspeed v1 UART boot |
| 7         | [1, 1, 1]                                         | 2              | SDIO highspeed v2 UART boot |
| 4-7       | [1,0,0], [1,0,1], [1,1,0], [1,1,1]                | 非 2           | SDIO boot                   |

> Note:
>
> `boot mode` 4~7 为 `SDIO` 的不同的协议标准，包括低速 (lowspeed) 和高速 (highspeed)， 版本号(V1, V2)等， 但并非所有 MCU 都会同时支持这些标准。

#### ** 参考

[问题参考：ESP8266 常见固件烧写失败原因和解决方法](https://blog.csdn.net/espressif/article/details/102650210?utm_medium=distribute.pc_relevant_t0.none-task-blog-BlogCommendFromMachineLearnPai2-1.nonecase&depth_1-utm_source=distribute.pc_relevant_t0.none-task-blog-BlogCommendFromMachineLearnPai2-1.nonecase)