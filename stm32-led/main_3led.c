/*****************************************************************************
 * 文件名：main_3led.c
 * 功  能：STM32F103C8T6 最小系统核心板 + 3 个外部 LED（红/绿/蓝）
 *         使用 GPIOA、GPIOB、GPIOC 三种端口控制，轮流闪烁，间隔 1 秒
 * 方  式：寄存器直接操作（不依赖标准外设库，纯寄存器编程）
 *
 * 引脚分配：
 *   红灯 LED1 -> PA0  （GPIOA 端口，引脚 0）
 *   绿灯 LED2 -> PB0  （GPIOB 端口，引脚 0）
 *   蓝灯 LED3 -> PC14 （GPIOC 端口，引脚 14）
 *
 * 硬件接线：GPIO 引脚 -> 限流电阻(330Ω~1kΩ) -> LED 阳极；LED 阴极 -> GND
 *           （引脚输出高电平时 LED 点亮，即"高电平点亮"接法）
 *
 * 编译环境：Keil MDK5，芯片选择 STM32F103C8
 *****************************************************************************/

/* ================= 一、寄存器地址定义 ================= */

/* RCC（复位与时钟控制）外设基地址 */
#define RCC_BASE        0x40021000UL
/* APB2 外设时钟使能寄存器，偏移地址 0x18（见参考手册 6.3.7 节）
 * 相关位：bit2=IOPAEN（GPIOA 时钟）bit3=IOPBEN（GPIOB 时钟）bit4=IOPCEN（GPIOC 时钟）
 * 写 1 使能对应端口时钟，写 0 关闭 */
#define RCC_APB2ENR     (*(volatile unsigned int *)(RCC_BASE + 0x18))

/* GPIO 各端口基地址（见参考手册第 8 章） */
#define GPIOA_BASE      0x40010800UL    /* GPIOA 基地址 */
#define GPIOB_BASE      0x40010C00UL    /* GPIOB 基地址 */
#define GPIOC_BASE      0x40011000UL    /* GPIOC 基地址 */

/* GPIO 寄存器偏移地址（各端口通用，见参考手册 8.2 节寄存器描述） */
#define GPIO_CRL        0x00UL   /* 端口配置低寄存器：控制引脚 0~7 */
#define GPIO_CRH        0x04UL   /* 端口配置高寄存器：控制引脚 8~15 */
#define GPIO_IDR        0x08UL   /* 端口输入数据寄存器（只读） */
#define GPIO_ODR        0x0CUL   /* 端口输出数据寄存器 */
#define GPIO_BSRR       0x10UL   /* 端口位设置/清除寄存器 */
#define GPIO_BRR        0x14UL   /* 端口位清除寄存器 */

/* 用宏把"基地址 + 偏移"直接定义成可读写的寄存器变量 */
#define GPIOA_CRL       (*(volatile unsigned int *)(GPIOA_BASE + GPIO_CRL))
#define GPIOB_CRL       (*(volatile unsigned int *)(GPIOB_BASE + GPIO_CRL))
#define GPIOC_CRL       (*(volatile unsigned int *)(GPIOC_BASE + GPIO_CRL))
#define GPIOC_CRH       (*(volatile unsigned int *)(GPIOC_BASE + GPIO_CRH))
#define GPIOA_ODR       (*(volatile unsigned int *)(GPIOA_BASE + GPIO_ODR))
#define GPIOB_ODR       (*(volatile unsigned int *)(GPIOB_BASE + GPIO_ODR))
#define GPIOC_ODR       (*(volatile unsigned int *)(GPIOC_BASE + GPIO_ODR))

/* ================= 二、引脚号定义 ================= */
#define LED_RED_PIN     0       /* 红灯接 PA0 */
#define LED_GREEN_PIN   0       /* 绿灯接 PB0 */
#define LED_BLUE_PIN    14      /* 蓝灯接 PC14 */

/* ================= 三、软件延时函数 ================= */
/* STM32F103C8 上电默认使用内部 8MHz HSI 时钟，未配置 PLL */
/* 本函数用空循环"数指令周期"实现毫秒级延时：
 * 内层 5000 次循环约耗时 1ms（Keil -O0 优化下微调后的值，原 8000 实际偏慢） */
void delay_ms(unsigned int ms)
{
    volatile unsigned int i, j;   /* volatile：防止编译器把循环优化掉 */
    for (i = 0; i < ms; i++) {
        for (j = 0; j < 5000; j++) {
            /* 空语句，占用 CPU 周期实现延时 */
        }
    }
}

/* ================= 四、主函数 ================= */
int main(void)
{
    /* ---------- 第一步：使能 GPIOA / GPIOB / GPIOC 时钟 ---------- */
    /* 注意：STM32 任何外设使用前必须先在 RCC 里打开它的时钟，
     * 否则寄存器操作无效（读回来全是 0） */
    RCC_APB2ENR |= (1 << 2);    /* bit2 = 1：使能 GPIOA 时钟 */
    RCC_APB2ENR |= (1 << 3);    /* bit3 = 1：使能 GPIOB 时钟 */
    RCC_APB2ENR |= (1 << 4);    /* bit4 = 1：使能 GPIOC 时钟 */

    /* ---------- 第二步：配置引脚为"通用推挽输出" ---------- */
    /* CRL/CRH 寄存器每 4 位控制一个引脚，从低到高依次是：
     *   bit[4y+1:4y] = MODEy[1:0]（输出模式/速度）
     *   bit[4y+3:4y+2] = CNFy[1:0]（输出配置/输入方式）
     * 输出模式 MODE：01=10MHz 10=2MHz 11=50MHz，00=输入模式
     * 输出配置 CNF：00=通用推挽输出 01=通用开漏输出 */
    /* PA0（引脚 0，配置在 CRL 的 bit0~3） */
    GPIOA_CRL &= ~(0xF << (4 * LED_RED_PIN));   /* 先清零 PA0 的 4 位配置 */
    GPIOA_CRL |=  (0x3 << (4 * LED_RED_PIN));   /* MODE=11(50MHz) CNF=00(推挽) */
    /* PB0（引脚 0，配置在 CRL 的 bit4~7） */
    GPIOB_CRL &= ~(0xF << (4 * LED_GREEN_PIN)); /* 先清零 PB0 的 4 位配置 */
    GPIOB_CRL |=  (0x3 << (4 * LED_GREEN_PIN)); /* 推挽输出 50MHz */
    /* PC14（引脚 14 > 7，配置在 CRH，引脚号先减 8）
     * PC13/PC14/PC15 是低速引脚（数据手册规定最大输出 2MHz），
     * 所以 MODE 用 10（2MHz），点灯完全够用 */
    GPIOC_CRH &= ~(0xF << (4 * (LED_BLUE_PIN - 8)));   /* 清零 PC14 的 bit24~27 */
    GPIOC_CRH |=  (0x2 << (4 * (LED_BLUE_PIN - 8)));   /* 推挽输出 2MHz */

    /* ---------- 第三步：初始化，三个 LED 全部熄灭 ---------- */
    /* ODR 寄存器：对应位写 1 输出高电平，写 0 输出低电平 */
    GPIOA_ODR &= ~(1 << LED_RED_PIN);   /* PA0 输出低 -> 红灯灭 */
    GPIOB_ODR &= ~(1 << LED_GREEN_PIN); /* PB0 输出低 -> 绿灯灭 */
    GPIOC_ODR &= ~(1 << LED_BLUE_PIN);  /* PC14 输出低 -> 蓝灯灭 */

    /* ---------- 第四步：主循环，三灯轮流点亮，每灯 1 秒 ---------- */
    while (1) {
        /* 第 1 秒：红灯亮，绿灯蓝灯灭 */
        GPIOA_ODR |= (1 << LED_RED_PIN);     /* PA0 输出高 -> 红灯亮 */
        GPIOB_ODR &= ~(1 << LED_GREEN_PIN);  /* PB0 输出低 -> 绿灯灭 */
        GPIOC_ODR &= ~(1 << LED_BLUE_PIN);   /* PC14 输出低 -> 蓝灯灭 */
        delay_ms(1000);                      /* 延时 1 秒 */

        /* 第 2 秒：绿灯亮，红灯蓝灯灭 */
        GPIOA_ODR &= ~(1 << LED_RED_PIN);    /* 红灯灭 */
        GPIOB_ODR |= (1 << LED_GREEN_PIN);   /* PB0 输出高 -> 绿灯亮 */
        GPIOC_ODR &= ~(1 << LED_BLUE_PIN);   /* 蓝灯灭 */
        delay_ms(1000);                      /* 延时 1 秒 */

        /* 第 3 秒：蓝灯亮，红灯绿灯灭 */
        GPIOA_ODR &= ~(1 << LED_RED_PIN);    /* 红灯灭 */
        GPIOB_ODR &= ~(1 << LED_GREEN_PIN);  /* 绿灯灭 */
        GPIOC_ODR |= (1 << LED_BLUE_PIN);    /* PC14 输出高 -> 蓝灯亮 */
        delay_ms(1000);                      /* 延时 1 秒 */
    }
    /* 循环往复，三个 LED 轮流闪烁 */
}
