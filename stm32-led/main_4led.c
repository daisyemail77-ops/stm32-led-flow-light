/*****************************************************************************
 * 文件名：main_4led.c
 * 功  能：在 3 个外部 LED 的基础上，把最小系统板板载 PC13 LED 也加入流水灯
 *         —— 4 个 LED 轮流闪烁，每个点亮 1 秒
 * 方  式：寄存器直接操作
 *
 * 引脚分配：
 *   外部红灯 LED1 -> PA0   （GPIOA）
 *   外部绿灯 LED2 -> PB0   （GPIOB）
 *   外部蓝灯 LED3 -> PC14  （GPIOC）
 *   板载 LED4    -> PC13  （GPIOC，最小系统板出厂已焊接，丝印标注 PC13）
 *
 * 极性说明（重要）：
 *   外部 3 个 LED：GPIO -> 电阻 -> LED -> GND，高电平点亮；
 *   板载 PC13 LED：Blue Pill 板上接法为 3.3V -> 电阻 -> LED -> PC13，
 *                  即 PC13 输出低电平时灯亮（低电平点亮），与外部灯相反！
 *   代码中用 LED4_ON / LED4_OFF 两个宏封装了这种相反极性。
 *
 * 编译环境：Keil MDK5，芯片选择 STM32F103C8
 *****************************************************************************/

/* ================= 一、寄存器地址定义 ================= */
#define RCC_BASE        0x40021000UL
/* APB2 外设时钟使能寄存器，偏移 0x18；bit2/3/4 分别使能 GPIOA/B/C 时钟 */
#define RCC_APB2ENR     (*(volatile unsigned int *)(RCC_BASE + 0x18))

#define GPIOA_BASE      0x40010800UL
#define GPIOB_BASE      0x40010C00UL
#define GPIOC_BASE      0x40011000UL

#define GPIO_CRL        0x00UL   /* 配置低寄存器（引脚 0~7） */
#define GPIO_CRH        0x04UL   /* 配置高寄存器（引脚 8~15） */
#define GPIO_ODR        0x0CUL   /* 输出数据寄存器 */

#define GPIOA_CRL       (*(volatile unsigned int *)(GPIOA_BASE + GPIO_CRL))
#define GPIOB_CRL       (*(volatile unsigned int *)(GPIOB_BASE + GPIO_CRL))
#define GPIOC_CRL       (*(volatile unsigned int *)(GPIOC_BASE + GPIO_CRL))
#define GPIOC_CRH       (*(volatile unsigned int *)(GPIOC_BASE + GPIO_CRH))
#define GPIOA_ODR       (*(volatile unsigned int *)(GPIOA_BASE + GPIO_ODR))
#define GPIOB_ODR       (*(volatile unsigned int *)(GPIOB_BASE + GPIO_ODR))
#define GPIOC_ODR       (*(volatile unsigned int *)(GPIOC_BASE + GPIO_ODR))

/* ================= 二、引脚号定义 ================= */
#define LED_RED_PIN     0       /* PA0  外部红灯 */
#define LED_GREEN_PIN   0       /* PB0  外部绿灯 */
#define LED_BLUE_PIN    14      /* PC14 外部蓝灯 */
#define LED_BOARD_PIN   13      /* PC13 板载 LED */

/* ================= 三、点亮/熄灭封装宏 ================= */
/* 外部灯：输出高电平点亮 */
#define LED1_ON()   (GPIOA_ODR |=  (1 << LED_RED_PIN))      /* 红灯亮 */
#define LED1_OFF()  (GPIOA_ODR &= ~(1 << LED_RED_PIN))      /* 红灯灭 */
#define LED2_ON()   (GPIOB_ODR |=  (1 << LED_GREEN_PIN))    /* 绿灯亮 */
#define LED2_OFF()  (GPIOB_ODR &= ~(1 << LED_GREEN_PIN))    /* 绿灯灭 */
#define LED3_ON()   (GPIOC_ODR |=  (1 << LED_BLUE_PIN))     /* 蓝灯亮 */
#define LED3_OFF()  (GPIOC_ODR &= ~(1 << LED_BLUE_PIN))     /* 蓝灯灭 */
/* 板载 PC13 灯：低电平点亮（极性相反） */
#define LED4_ON()   (GPIOC_ODR &= ~(1 << LED_BOARD_PIN))    /* PC13 输出低 -> 板载灯亮 */
#define LED4_OFF()  (GPIOC_ODR |=  (1 << LED_BOARD_PIN))    /* PC13 输出高 -> 板载灯灭 */

/* ================= 四、软件延时函数 ================= */
/* 内部 8MHz HSI 时钟，内层 5000 次空循环约 1ms（Keil -O0 优化下微调后的值） */
void delay_ms(unsigned int ms)
{
    volatile unsigned int i, j;
    for (i = 0; i < ms; i++) {
        for (j = 0; j < 5000; j++) {
            /* 空循环延时 */
        }
    }
}

/* ================= 五、主函数 ================= */
int main(void)
{
    /* ---------- 第一步：使能 GPIOA / GPIOB / GPIOC 时钟 ---------- */
    RCC_APB2ENR |= (1 << 2) | (1 << 3) | (1 << 4);  /* 一次打开三个端口时钟 */

    /* ---------- 第二步：配置引脚为通用推挽输出 ---------- */
    /* PA0、PB0：引脚 0~7，配置在 CRL；推挽输出 50MHz（MODE=11, CNF=00） */
    GPIOA_CRL &= ~(0xF << (4 * LED_RED_PIN));
    GPIOA_CRL |=  (0x3 << (4 * LED_RED_PIN));
    GPIOB_CRL &= ~(0xF << (4 * LED_GREEN_PIN));
    GPIOB_CRL |=  (0x3 << (4 * LED_GREEN_PIN));

    /* PC13、PC14：引脚 8~15，配置在 CRH，引脚号先减 8
     * PC13/PC14/PC15 为低速引脚，MODE 用 10（2MHz） */
    GPIOC_CRH &= ~(0xF << (4 * (LED_BOARD_PIN - 8)));  /* 清零 PC13 的 bit20~23 */
    GPIOC_CRH |=  (0x2 << (4 * (LED_BOARD_PIN - 8)));  /* PC13 推挽输出 2MHz */
    GPIOC_CRH &= ~(0xF << (4 * (LED_BLUE_PIN - 8)));   /* 清零 PC14 的 bit24~27 */
    GPIOC_CRH |=  (0x2 << (4 * (LED_BLUE_PIN - 8)));   /* PC14 推挽输出 2MHz */

    /* ---------- 第三步：初始化，4 个 LED 全部熄灭 ---------- */
    LED1_OFF();
    LED2_OFF();
    LED3_OFF();
    LED4_OFF();

    /* ---------- 第四步：主循环，四灯轮流点亮，每灯 1 秒 ---------- */
    while (1) {
        /* 第 1 秒：外部红灯 PA0 亮，其余灭 */
        LED1_ON();  LED2_OFF(); LED3_OFF(); LED4_OFF();
        delay_ms(1000);

        /* 第 2 秒：外部绿灯 PB0 亮，其余灭 */
        LED1_OFF(); LED2_ON();  LED3_OFF(); LED4_OFF();
        delay_ms(1000);

        /* 第 3 秒：外部蓝灯 PC14 亮，其余灭 */
        LED1_OFF(); LED2_OFF(); LED3_ON();  LED4_OFF();
        delay_ms(1000);

        /* 第 4 秒：板载 PC13 LED 亮，其余灭（用板载灯验证烧录成功） */
        LED1_OFF(); LED2_OFF(); LED3_OFF(); LED4_ON();
        delay_ms(1000);
    }
    /* 循环：红 -> 绿 -> 蓝 -> 板载 -> 红 -> ... 四灯流水 */
}
