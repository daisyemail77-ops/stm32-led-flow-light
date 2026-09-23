# STM32F103C8T6 寄存器方式 LED 流水灯

STM32 最小系统核心板（Blue Pill）寄存器方式点灯实验：GPIOA / GPIOB / GPIOC 三端口控制 3 个外部 LED 轮流闪烁（间隔 1 秒），并将板载 PC13 LED 加入流水灯。

## 项目结构

```
STM32流水灯/
├── main_3led.c        # 版本一：3 个外部 LED（红PA0/绿PB0/蓝PC14）轮流闪烁
├── main_4led.c        # 版本二：加入板载 PC13 LED，4 灯流水
├── 实验报告.md         # 实验报告（Markdown）
├── 博客文章.md         # 博客文章（发 CSDN / 博客园）
└── Git提交教程.md      # Git 安装与代码提交步骤
```

## 硬件接线

```
PA0 ──→ 电阻(330Ω~1kΩ) ──→ 红灯 ──→ GND   高电平点亮
PB0 ──→ 电阻 ──→ 绿灯 ──→ GND             高电平点亮
PC14 ──→ 电阻 ──→ 蓝灯 ──→ GND            高电平点亮
PC13（板载 LED，无需接线）                低电平点亮
```

## 开发环境

- Keil MDK5，芯片 STM32F103C8
- 纯寄存器方式编程（不依赖标准外设库）
- 下载：ST-Link（SWD）或串口 ISP

## 编译运行

1. Keil 新建工程 → 选 STM32F103C8；
2. 添加 main_3led.c（或 main_4led.c）到工程；
3. F7 编译 → 下载到核心板；
4. 观察 LED 轮流闪烁。

## 参考手册

- 《STM32F10xxx 中文参考手册》RM0008（V10）
- STM32F103x8 数据手册（DS5319）
