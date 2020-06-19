# ESP8266 学习笔记1 —— Ubuntu环境搭建

### 0. 环境

VMWare

Ubuntu16-x64

### 1. Ubuntu编译环境搭建

官方给出的可能的依赖：

```shell
$ sudo apt-get install gcc git wget make libncurses-dev flex bison gperf python python-serial

# 会用到的工具
sudo pip install pyserial
```

下载工具链Toolchain：[xtensa-lx106](https://dl.espressif.com/dl/xtensa-lx106-elf-linux64-1.22.0-100-ge567ec7-5.2.0.tar.gz)

```
$ mkdir -p ~/esp
$ cd ~/esp
$ tar -zxvf xtensa-lx106-elf-linux64-1.22.0-100-ge567ec7-5.2.0.tar.gz
# 添加环境变量
$ export PATH=$PATH:$HOME/esp/xtensa-lx106-elf/bin
# 检查工具链
$ xtensa-lx106-elf-gcc -v
......
gcc version 5.2.0 (crosstool-NG crosstool-ng-1.22.0-100-ge567ec7)
```

