/**
 * 实验5：定时器 PWM 呼吸灯（STM32F103C8T6 + HAL 库）
 *
 * 任务1：TIM2 定时中断（1s 溢出一次），中断里翻转 PA0 红灯
 *        → LED 以 2s 频率周期性地亮-灭（亮 1s、灭 1s）
 * 任务2：TIM3_CH3（PB0）PWM 模式驱动绿灯呼吸灯
 *        PWM 频率 2kHz，占空比 0%→100%→0% 渐变，一个呼吸周期约 2s
 *
 * 编译说明：
 *   硬件烧录：直接编译（HSE 8MHz × PLL9 = 72MHz）
 *   软件仿真：Options→C/C++→Define 加 USE_SIMULATION（HSI 8MHz × PLL16 = 64MHz）
 *             仿真结束后烧录真实硬件前，必须删除 USE_SIMULATION 再编译
 */
#include "stm32f1xx_hal.h"

/* =============== 定时器时钟 ===============
 * TIM2/TIM3 挂 APB1：APB1 预分频=2（≠1）时，定时器时钟 = APB1×2
 * 硬件 72MHz；仿真（USE_SIMULATION）64MHz */
#ifdef USE_SIMULATION
#define TIM_CLK  64000000UL
#else
#define TIM_CLK  72000000UL
#endif

/* =============== 全局句柄 =============== */
TIM_HandleTypeDef htim2;   /* 任务1：定时中断（1s 翻转） */
TIM_HandleTypeDef htim3;   /* 任务2：PWM 呼吸灯 */

/* =============== 系统时钟 ===============
 * 硬件：HSE 8MHz × PLL9 = 72MHz
 * 仿真：HSI 8MHz/2 × PLL16 = 64MHz（Keil 仿真模型不支持 HSE）
 */
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

#ifdef USE_SIMULATION
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState       = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState   = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource  = RCC_PLLSOURCE_HSI_DIV2;
    RCC_OscInitStruct.PLL.PLLMUL     = RCC_PLL_MUL16;
    HAL_RCC_OscConfig(&RCC_OscInitStruct);

    RCC_ClkInitStruct.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                       RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource    = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider   = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider  = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider  = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2);
#else
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState       = RCC_HSE_ON;
    RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
    RCC_OscInitStruct.PLL.PLLState   = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource  = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLMUL     = RCC_PLL_MUL9;
    HAL_RCC_OscConfig(&RCC_OscInitStruct);

    RCC_ClkInitStruct.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                       RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource    = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider   = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider  = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider  = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2);
#endif
}

/* =============== 任务1：TIM2 定时中断（1s） =============== */
void TIM2_Base_Init(void)
{
    htim2.Instance = TIM2;
    htim2.Init.Prescaler     = TIM_CLK / 10000 - 1;   /* 计数时钟 10kHz */
    htim2.Init.CounterMode   = TIM_COUNTERMODE_UP;
    htim2.Init.Period        = 10000 - 1;             /* 计 10000 个 → 1s */
    htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    HAL_TIM_Base_Init(&htim2);
    HAL_TIM_Base_Start_IT(&htim2);
}

/* =============== 任务2：TIM3_CH3 (PB0) PWM 呼吸灯 =============== */
void TIM3_PWM_Init(void)
{
    TIM_OC_InitTypeDef sConfigOC = {0};

    htim3.Instance = TIM3;
    htim3.Init.Prescaler     = TIM_CLK / 1000000 - 1; /* 计数时钟 1MHz */
    htim3.Init.CounterMode   = TIM_COUNTERMODE_UP;
    htim3.Init.Period        = 500 - 1;               /* 1MHz/500 = 2kHz PWM */
    htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    HAL_TIM_PWM_Init(&htim3);

    sConfigOC.OCMode     = TIM_OCMODE_PWM1;           /* PWM 模式 1 */
    sConfigOC.Pulse      = 0;                         /* 初始占空比 0% */
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_3);
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_3);
}

/* =============== MSP：GPIO 复用 + NVIC =============== */
void HAL_TIM_Base_MspInit(TIM_HandleTypeDef *htim)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    if (htim->Instance == TIM2) {
        __HAL_RCC_TIM2_CLK_ENABLE();
        __HAL_RCC_GPIOA_CLK_ENABLE();

        /* PA0 推挽输出：任务1 定时翻转 */
        GPIO_InitStruct.Pin   = GPIO_PIN_0;
        GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

        HAL_NVIC_SetPriority(TIM2_IRQn, 1, 0);
        HAL_NVIC_EnableIRQ(TIM2_IRQn);
    }
}

void HAL_TIM_PWM_MspInit(TIM_HandleTypeDef *htim)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    if (htim->Instance == TIM3) {
        __HAL_RCC_TIM3_CLK_ENABLE();
        __HAL_RCC_GPIOB_CLK_ENABLE();

        /* PB0 复用推挽输出：TIM3_CH3 */
        GPIO_InitStruct.Pin   = GPIO_PIN_0;
        GPIO_InitStruct.Mode  = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    }
}

/* =============== 中断服务函数 =============== */
void TIM2_IRQHandler(void)
{
    HAL_TIM_IRQHandler(&htim2);
}

/* =============== 更新中断回调：1s 翻转 PA0 =============== */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM2) {
        HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_0);
    }
}

/* =============== SysTick（HAL_Delay 时基） =============== */
void SysTick_Handler(void)
{
    HAL_IncTick();
}

/* =============== 呼吸灯步进延时 ===============
 * 硬件：SysTick 精确，HAL_Delay(2) = 2ms/步 → 周期约 2s
 * 仿真：Keil 指令模拟有开销，HAL_Delay(1) 实际约 2ms/步 → 周期也约 2s
 */
#ifdef USE_SIMULATION
#define BREATH_STEP_MS  1
#else
#define BREATH_STEP_MS  2
#endif

/* =============== 主函数 =============== */
int main(void)
{
    HAL_Init();
    SystemClock_Config();

    TIM2_Base_Init();   /* 任务1：红灯 2s 周期亮灭 */
    TIM3_PWM_Init();    /* 任务2：绿灯呼吸灯 */

    while (1) {
        /* 渐亮：占空比 0% → 100%（CCR 0 → 499） */
        for (uint16_t i = 0; i <= 499; i++) {
            __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, i);
            HAL_Delay(BREATH_STEP_MS);
        }
        /* 渐灭：占空比 100% → 0%（CCR 499 → 0） */
        for (uint16_t i = 499; i > 0; i--) {
            __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, i);
            HAL_Delay(BREATH_STEP_MS);
        }
        /* 一个呼吸周期 = 998 步 × 步进 ≈ 2s */
    }
}
