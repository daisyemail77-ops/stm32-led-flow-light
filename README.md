# STM32 学习实验集合（F103C8T6）

基于 STM32F103C8T6（Blue Pill 最小系统板）的系列实验代码，使用 Keil MDK5 开发。

## 实验列表

### 实验1：寄存器方式 LED 流水灯（stm32-led/）
- 寄存器直接操作 GPIOA/GPIOB/GPIOC，控制 3 个外部 LED 轮流闪烁（间隔 1s）
- 板载 PC13 LED 加入流水灯（低电平点亮）
- 完整 Keil 工程 + 3灯/4灯 两个版本源码

### 实验4：HAL 串口通信（experiment4-uart/）
- USART1：115200、8N1，HAL 库
- 任务一：阻塞发送 `hello windows!`，上位机发 `#` 暂停、`*` 恢复
- 任务二(1)：中断方式发送；任务二(3)：DMA 方式发送（DMA1_Channel4）
- 任务三：Keil 软件仿真 + 逻辑分析仪观察 USART1_SR 波形，实测一字节周期 89.28μs，实际波特率 ≈112kbps（设定 115200）
- 完整 Keil 工程 + HAL 驱动全套（含 DMA）

## 开发环境
- Keil MDK5 / STM32F1xx HAL 库 V1.8.7
- 烧录：FlyMcu（USART1）/ ST-Link
