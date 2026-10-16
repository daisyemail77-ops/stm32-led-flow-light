/**
 * 任务二(1)：STM32 USART1 中断方式发送 + 中断接收
 *
 * 与任务一的区别：
 *   - 任务一：阻塞发送 HAL_UART_Transmit（CPU 干等）
 *   - 本程序：中断发送 HAL_UART_Transmit_IT（CPU 不等，发完进中断）
 *
 * 功能：
 *   1. 连续发送 "hello windows! "
 *   2. 上位机发 '#' → 暂停；发 '*' → 恢复
 */
#include "stm32f1xx_hal.h"

/* =============== 全局变量 =============== */
UART_HandleTypeDef huart1;
volatile uint8_t g_send = 1;
volatile uint8_t tx_done = 1;   /* 1=上次发送已完成，可以发下一次 */
uint8_t rx_byte;

const uint8_t tx_msg[] = "hello windows! ";
#define TX_LEN  14

/* =============== 系统时钟 ===============
 * 硬件运行：HSE 8MHz × PLL9 = 72MHz
 * 软件仿真：Keil 仿真模型不支持 HSE（HSERDY 永远不就绪，会卡死），
 *           编译时在 Options→C/C++→Define 里加 USE_SIMULATION 宏，
 *           则改用 HSI 8MHz/2 × PLL16 = 64MHz，仿真即可正常运行。
 */
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

#ifdef USE_SIMULATION
    /* ===== 仿真模式：HSI 内部时钟（Keil 模型支持） =====
     * HSI 8MHz → /2 = 4MHz → ×16 = 64MHz
     * APB2(USART1) = 64MHz，波特率 115200 误差 0.08%，正常 */
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
    /* ===== 硬件模式：HSE 8MHz × PLL9 = 72MHz ===== */
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

/* =============== USART1 初始化：115200 8N1 =============== */
void USART1_UART_Init(void)
{
    huart1.Instance          = USART1;
    huart1.Init.BaudRate     = 115200;
    huart1.Init.WordLength   = UART_WORDLENGTH_8B;
    huart1.Init.StopBits     = UART_STOPBITS_1;
    huart1.Init.Parity        = UART_PARITY_NONE;
    huart1.Init.Mode         = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    HAL_UART_Init(&huart1);
}

/* =============== MSP：GPIO + USART1 中断 =============== */
void HAL_UART_MspInit(UART_HandleTypeDef *huart)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    if (huart->Instance == USART1) {
        __HAL_RCC_USART1_CLK_ENABLE();
        __HAL_RCC_GPIOA_CLK_ENABLE();

        GPIO_InitStruct.Pin   = GPIO_PIN_9;
        GPIO_InitStruct.Mode  = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

        GPIO_InitStruct.Pin  = GPIO_PIN_10;
        GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
        GPIO_InitStruct.Pull = GPIO_PULLUP;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

        /* 发送和接收共用 USART1 全局中断 */
        HAL_NVIC_SetPriority(USART1_IRQn, 0, 0);
        HAL_NVIC_EnableIRQ(USART1_IRQn);
    }
}

/* =============== 中断服务函数 =============== */
void USART1_IRQHandler(void)
{
    HAL_UART_IRQHandler(&huart1);
}

/* =============== 接收回调：# 暂停 / * 恢复 =============== */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1) {
        if (rx_byte == '#')      g_send = 0;
        else if (rx_byte == '*') g_send = 1;
        HAL_UART_Receive_IT(&huart1, &rx_byte, 1);
    }
}

/* =============== 发送完成回调：标记发完 =============== */
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1) {
        tx_done = 1;
    }
}

/* =============== SysTick =============== */
void SysTick_Handler(void)
{
    HAL_IncTick();
}

/* =============== 主函数 =============== */
int main(void)
{
    HAL_Init();
    SystemClock_Config();
    USART1_UART_Init();

    HAL_UART_Receive_IT(&huart1, &rx_byte, 1);
    HAL_UART_Transmit(&huart1, (uint8_t *)"STM32 USART1 IT ready!\r\n", 24, 100);

    while (1) {
        if (g_send && tx_done) {
            tx_done = 0;
            /* 启动一次中断发送（非阻塞，立即返回） */
            HAL_UART_Transmit_IT(&huart1, (uint8_t *)tx_msg, TX_LEN);
        }
        /* 这里 CPU 可以干别的事，等 TxCpltCallback 置 tx_done=1 再发下一次 */
    }
}
