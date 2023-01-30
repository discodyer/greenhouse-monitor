# greenhouse-monitor

绿色农业

## 产品介绍

本产品是一个环境采集装置，包含了一系列的传感器用于检测环境状态信息，并可以将传感器信息上报给后台进行分析。

本产品还可以接收后台传达的指令或根据当前状态进行温室环境温度和湿度的自动控制等操作。

## 硬件方案

芯片方案：

1. 华为海思Hi3861芯片
2. 乐鑫ESP32系列芯片

### 华为海思Hi3861介绍

华为海思 Hi3861 芯片，它包括一个主频为 160MHz 的 32 位 RISC-V 内核、352KB SRAM 和 288KB ROM。  
该模块还集成了 2MB 闪存和集成 Wi-Fi 控制器，支持 IEEE 802.11b/g/n，在 2.4GHz 频谱上的最大数据速率为 72.2Mbps。  
集成到 SoC 中的外设包括 SDIO、SPI、I2C、GPIO、UART、ADC、PWM 和 I2S。

芯片将运行基于华为开源的 LiteOS 内核的 Open Harmony 轻量系统（mini system）系统。  
支持最新版本的 Open Harmony 3.0 LTS 或 3.1 beta 版本。  

![润和 Hi3861 开发板](https://static.sitestack.cn/projects/openharmony-1.0-zh-cn/quick-start/figures/Hi3861-WLAN%E6%A8%A1%E7%BB%84%E5%A4%96%E8%A7%82%E5%9B%BE.png)

### 乐鑫ESP32系列芯片介绍

ESP32是一系列低成本，低功耗的单片机微控制器，集成了Wi-Fi和双模蓝牙。 ESP32系列采用Tensilica Xtensa LX6微处理器，包括双核心和单核变体，内置天线开关，RF变换器，功率放大器，低噪声接收放大器，滤波器和电源管理模块。  
处理器采用 Xtensa 双核心 (或者宏内核) 32位 LX6 微处理器, 工作时脉 80/160/240 MHz, 运算能力高达 600 DMIPS  
具有 448 KB ROM (64KB+384KB)， 520 KB SRAM。支持 IEEE 802.11 b/g/n，蓝牙 v4.2 BR/EDR/BLE，同时外设接口丰富，支持I2C, SPI, CAN等。  
ESP32有云端一体的全链路开发框架，支持 Arduino framework  

![ESP32 微控制器](https://upload.wikimedia.org/wikipedia/commons/3/33/Espressif_ESP-WROOM-32_Wi-Fi_%26_Bluetooth_Module.jpg)

### 传感器及外部设备

传感器需要做到实时检测大棚内环境温度湿度和光照强度，可以实现远程开启补光灯及通风电机等，实现控制大棚内的环境。

- 温湿度传感器 DHT11
- 光敏传感器
- 二氧化碳含量传感器
- 土壤湿度传感器
- 水泵
- 风扇
- 灯泡

## 设备与云端进行通信

设备与IoT云间的通讯协议包含了MQTT，LwM2M/CoAP，HTTP/HTTP2，Modbus，OPC-UA，OPC-DA。而我们设备端与云端通讯主要用的协议是MQTT。
