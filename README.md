# ESP8266 学习笔记


###1. 目录结构说明

* doc - 官方等参考文件
* notes - 笔记
* project - 实例工程，不包含 SDK 内容
* res - 软件等资源

另外两个目录，为解压后的 SDK 和 toolchain ，在 res 下均可找到，因此不重复上传。

* ESP8266_RTOS_SDK-3.1.2
* xtensa-lx106-elf

### 2. chenv.sh

一个为了方便自已写的配置环境的脚本文件，方便快速配置好环境变量。即添加了 `PATH` 和 `IDF_PATH` 两个变量

使用如下：

```shell
$ source chenv.sh
```

