/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : MeteoNet STM32N6 — 27 features, 6 sorties
  *                   Capteurs : HTS221 (Temp+Hum) + LPS22HH (Pression)
  *                   Réseau   : Dense(27→64→32→16→6) sigmoid
  ******************************************************************************
  * Copyright (c) 2026 STMicroelectronics. All rights reserved.
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include <string.h>
#include <math.h>

/* USER CODE BEGIN Includes */
#include "meteo.h"
#include "meteo_data.h"
#include "meteo_data_params.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define N_FEATURES  27
#define N_OUTPUTS    6
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
COM_InitTypeDef        BspCOMInit;
CACHEAXI_HandleTypeDef hcacheaxi;
I2C_HandleTypeDef      hi2c1;
I2C_HandleTypeDef      hi2c2;
TIM_HandleTypeDef      htim6;
UART_HandleTypeDef     huart4;

/* USER CODE BEGIN PV */
extern UART_HandleTypeDef hcom_uart[];

/* ── Capteurs ─────────────────────────────────────────────── */
float last_temp = 20.0f;
float last_rhum = 70.0f;
float last_pres = 1013.0f;

int  hts221_init(void);
void hts221_read_temp(void);
void hts221_read_hum(void);
int  pressure_init(void);
void pressure_read(void);

/* ── IA ───────────────────────────────────────────────────── */
static ai_handle   ai_meteo = AI_HANDLE_NULL;
static ai_buffer   ai_input[AI_METEO_IN_NUM];
static ai_buffer   ai_output[AI_METEO_OUT_NUM];
static float       in_data[AI_METEO_IN_1_SIZE];   /* 27 floats */
static float       out_data[AI_METEO_OUT_1_SIZE];  /*  6 floats */
static uint8_t     activation_buffer[AI_METEO_DATA_ACTIVATIONS_SIZE];

/* ── Labels des 6 sorties ─────────────────────────────────── */
static const char *LABELS[N_OUTPUTS] = {
    "Pluie      ",
    "Beau temps ",
    "Nuageux    ",
    "Brouillard ",
    "Vent fort  ",
    "Gel        "
};

/* ── Historique 3 jours (J-1, J-2, J-3) ──────────────────── */
static float hist_temp[3] = {20.0f, 20.0f, 20.0f};
static float hist_hum[3]  = {70.0f, 70.0f, 70.0f};
static float hist_pres[3] = {1013.0f, 1013.0f, 1013.0f};
static uint8_t hist_init  = 0;

/* ── Mois courant (1-12) — à mettre à jour selon votre RTC ── */
static uint8_t mois_courant = 4;  /* Avril par défaut */

/* ============================================================
 * SCALER — valeurs générées par le notebook (étape 12)
 * REMPLACER par les vraies valeurs après entraînement !
 * ============================================================ */
static const float SCALER_MEAN[N_FEATURES] = {
    12.027117f, 17.021980f, 7.241065f, 73.637453f, 92.273973f, 55.000934f,
    1018.042248f, 9.780915f, 14.457153f, -0.004328f, -0.013076f, 0.041843f,
    12.031445f, 73.650529f, 1018.000405f, 12.013169f, 73.739103f, 1018.009714f,
    12.043213f, 73.646326f, 1018.064477f, 12.023910f, 73.675695f, 1018.017455f,
    0.024792f, -0.012102f, -0.005701f
};


static const float SCALER_STD[N_FEATURES] = {
    7.570319f, 8.401702f, 6.832124f, 10.221421f, 7.130148f, 15.282062f,
    7.402988f, 3.505532f, 7.381430f, 1.956011f, 7.379057f, 4.238561f,
    7.556106f, 10.191839f, 7.397784f, 7.574693f, 10.197281f, 7.404213f,
    7.596477f, 10.122340f, 7.379851f, 7.448834f, 9.096466f, 6.777525f,
    3.416645f, 0.704853f, 0.709227f
};
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void PeriphCommonClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_ICACHE_Init(void);
static void MX_CACHEAXI_Init(void);
static void MX_TIM6_Init(void);
static void MX_UART4_Init(void);
static void MX_I2C1_Init(void);
static void MX_I2C2_Init(void);

/* USER CODE BEGIN PFP */
static int  ai_init(void);
static void ai_run(void);
/* USER CODE END PFP */

/* USER CODE BEGIN 0 */

/* ============================================================
 * ai_init — Initialisation du réseau de neurones
 * ============================================================ */
static int ai_init(void)
{
    ai_error err;
    const ai_handle activations[] = { activation_buffer };
    const ai_handle weights[]     = { (ai_handle)s_meteo_weights_array_u64 };

    err = ai_meteo_create_and_init(&ai_meteo, activations, weights);
    if (err.type != AI_ERROR_NONE) {
        char buf[60];
        snprintf(buf, sizeof(buf), "[AI] ERREUR init type=%d code=%d\r\n",
                 err.type, err.code);
        HAL_UART_Transmit(&hcom_uart[COM1], (uint8_t*)buf, strlen(buf), 1000);
        return -1;
    }

    ai_input[0]       = *ai_meteo_inputs_get(ai_meteo, NULL);
    ai_output[0]      = *ai_meteo_outputs_get(ai_meteo, NULL);
    ai_input[0].data  = AI_HANDLE_PTR(in_data);
    ai_output[0].data = AI_HANDLE_PTR(out_data);

    HAL_UART_Transmit(&hcom_uart[COM1],
        (uint8_t*)"[AI] Init OK — 27 entrees, 6 sorties\r\n", 38, 1000);
    return 0;
}

/* ============================================================
 * ai_run — Inférence + affichage des 6 sorties
 * ============================================================ */
static void ai_run(void)
{
    float t = last_temp;
    float h = last_rhum;
    float p = last_pres;

    /* ── Initialisation historique au premier appel ── */
    if (!hist_init) {
        for (int i = 0; i < 3; i++) {
            hist_temp[i] = t;
            hist_hum[i]  = h;
            hist_pres[i] = p;
        }
        hist_init = 1;
    }

    float t1 = hist_temp[0], h1 = hist_hum[0], p1 = hist_pres[0]; /* J-1 */
    float t2 = hist_temp[1], h2 = hist_hum[1], p2 = hist_pres[1]; /* J-2 */
    float t3 = hist_temp[2], h3 = hist_hum[2], p3 = hist_pres[2]; /* J-3 */

    float temp_3j = (t + t1 + t2) / 3.0f;
    float hum_3j  = (h + h1 + h2) / 3.0f;
    float pres_3j = (p + p1 + p2) / 3.0f;
    float pi      = 3.14159265f;

    /* ── Construction des 27 features brutes ── */
    float raw[N_FEATURES] = {
        t,                                          /* 0  temp_mean  */
        t + 3.0f,                                   /* 1  temp_max   */
        t - 3.0f,                                   /* 2  temp_min   */
        h,                                          /* 3  hum_mean   */
        (h + 5.0f > 100.0f ? 100.0f : h + 5.0f),  /* 4  hum_max    */
        (h - 5.0f <   0.0f ?   0.0f : h - 5.0f),  /* 5  hum_min    */
        p,                                          /* 6  pressure   */
        6.0f,                                       /* 7  amp_temp   */
        t + 0.033f * h,                             /* 8  humidex    */
        t - t1,                                     /* 9  delta_temp */
        h - h1,                                     /* 10 delta_hum  */
        p - p1,                                     /* 11 delta_pres */
        t1, h1, p1,                                 /* 12-14 lag J-1 */
        t2, h2, p2,                                 /* 15-17 lag J-2 */
        t3, h3, p3,                                 /* 18-20 lag J-3 */
        temp_3j, hum_3j, pres_3j,                  /* 21-23 moy 3j  */
        p - pres_3j,                                /* 24 grad_pres  */
        sinf(2.0f * pi * mois_courant / 12.0f),    /* 25 sin_mois   */
        cosf(2.0f * pi * mois_courant / 12.0f)     /* 26 cos_mois   */
    };

    /* ── Normalisation Z-score ── */
    for (int i = 0; i < N_FEATURES; i++) {
        in_data[i] = (raw[i] - SCALER_MEAN[i]) / SCALER_STD[i];
    }

    /* ── Inférence ── */
    ai_i32 n = ai_meteo_run(ai_meteo, ai_input, ai_output);
    if (n <= 0) {
        HAL_UART_Transmit(&hcom_uart[COM1],
            (uint8_t*)"[AI] ERREUR inférence\r\n", 22, 1000);
        return;
    }

    /* ── Affichage de toutes les sorties ── */
    char buf[64];
    HAL_UART_Transmit(&hcom_uart[COM1],
        (uint8_t*)"=== METEO AI ===\r\n", 18, 1000);

    for (int i = 0; i < N_OUTPUTS; i++) {
        float pct = out_data[i] * 100.0f;
        snprintf(buf, sizeof(buf), "  %s : %5.1f%%%s\r\n",
                 LABELS[i], pct, (pct >= 50.0f) ? " <<<" : "");
        HAL_UART_Transmit(&hcom_uart[COM1], (uint8_t*)buf, strlen(buf), 1000);
    }

    HAL_UART_Transmit(&hcom_uart[COM1],
        (uint8_t*)"================\r\n", 18, 1000);

    /* ── LEDs : LED2 = pluie, LED1 = beau temps ── */
    if      (out_data[0] >= 0.5f) { BSP_LED_On(LED2); BSP_LED_Off(LED1); }
    else if (out_data[1] >= 0.5f) { BSP_LED_On(LED1); BSP_LED_Off(LED2); }
    else                           { BSP_LED_Off(LED1); BSP_LED_Off(LED2); }

    /* ── Décalage de l'historique ── */
    hist_temp[2] = hist_temp[1]; hist_hum[2] = hist_hum[1]; hist_pres[2] = hist_pres[1];
    hist_temp[1] = hist_temp[0]; hist_hum[1] = hist_hum[0]; hist_pres[1] = hist_pres[0];
    hist_temp[0] = t;            hist_hum[0]  = h;           hist_pres[0] = p;
}

/* USER CODE END 0 */

/* ============================================================
 * main
 * ============================================================ */
int main(void)
{
    /* USER CODE BEGIN 1 */
    /* USER CODE END 1 */

    HAL_Init();
    SystemClock_Config();
    PeriphCommonClock_Config();

    MX_GPIO_Init();
    MX_ICACHE_Init();
    MX_CACHEAXI_Init();
    MX_TIM6_Init();
    MX_UART4_Init();
    MX_I2C1_Init();
    MX_I2C2_Init();

    /* USER CODE BEGIN 2 */
    BSP_LED_Init(LED1);
    BSP_LED_Init(LED2);

    /* ── COM1 ─────────────────────────────────────────────── */
    BspCOMInit.BaudRate   = 115200;
    BspCOMInit.WordLength = COM_WORDLENGTH_8B;
    BspCOMInit.StopBits   = COM_STOPBITS_1;
    BspCOMInit.Parity     = COM_PARITY_NONE;
    BspCOMInit.HwFlowCtl  = COM_HWCONTROL_NONE;
    if (BSP_COM_Init(COM1, &BspCOMInit) != BSP_ERROR_NONE) {
        BSP_LED_On(LED2);
        while (1) {}
    }

    HAL_UART_Transmit(&hcom_uart[COM1],
        (uint8_t*)"\r\n=== METEO AI STM32N6 — 6 classes ===\r\n",
        39, 1000);

    /* ── Capteurs ─────────────────────────────────────────── */
    if (hts221_init() != 0) {
        HAL_UART_Transmit(&hcom_uart[COM1],
            (uint8_t*)"[HTS221] ERREUR init\r\n", 22, 1000);
    }
    if (pressure_init() != 0) {
        HAL_UART_Transmit(&hcom_uart[COM1],
            (uint8_t*)"[LPS22HH] ERREUR init\r\n", 23, 1000);
    }

    /* ── IA ───────────────────────────────────────────────── */
    ai_init();

    HAL_UART_Transmit(&hcom_uart[COM1],
        (uint8_t*)"=====================================\r\n", 38, 1000);
    /* USER CODE END 2 */

    /* ── Boucle principale ────────────────────────────────── */
    while (1)
    {
        /* USER CODE BEGIN WHILE */

        /* Lecture des capteurs */
        hts221_read_temp();
        hts221_read_hum();
        pressure_read();
        /*for (int i = 0; i < 3; i++) {
            hist_temp[i] = last_temp;
            hist_hum[i]  = last_rhum;
            hist_pres[i] = last_pres;
        }*/
        hist_init = 1;
        if (last_rhum < 0.0f)   last_rhum = 0.0f;
        if (last_rhum > 100.0f) last_rhum = 100.0f;
        if (last_temp < -40.0f) last_temp = -40.0f;
        if (last_temp > 60.0f)  last_temp = 60.0f;
        if (last_pres < 900.0f) last_pres = 900.0f;
        if (last_pres > 1100.0f) last_pres = 1100.0f;

        /* Affichage des mesures brutes */
        char buf[80];
        snprintf(buf, sizeof(buf),
            "Temp: %.2f C | Hum: %.2f %% | Pres: %.2f hPa\r\n",
            last_temp, last_rhum, last_pres);
        HAL_UART_Transmit(&hcom_uart[COM1], (uint8_t*)buf, strlen(buf), 1000);

        /* Inférence et affichage des 6 classes */
        ai_run();

        HAL_Delay(2000);

        /* USER CODE END WHILE */
        /* USER CODE BEGIN 3 */
    }
    /* USER CODE END 3 */
}

/* USER CODE BEGIN CLK 1 */
/* USER CODE END CLK 1 */

/**
  * @brief System Clock Configuration
  */
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    if (HAL_PWREx_ConfigSupply(PWR_EXTERNAL_SOURCE_SUPPLY) != HAL_OK)
        Error_Handler();

    RCC_OscInitStruct.OscillatorType      = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState            = RCC_HSI_ON;
    RCC_OscInitStruct.HSIDiv              = RCC_HSI_DIV1;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL1.PLLState       = RCC_PLL_NONE;
    RCC_OscInitStruct.PLL2.PLLState       = RCC_PLL_NONE;
    RCC_OscInitStruct.PLL3.PLLState       = RCC_PLL_NONE;
    RCC_OscInitStruct.PLL4.PLLState       = RCC_PLL_NONE;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
        Error_Handler();

    HAL_RCC_GetClockConfig(&RCC_ClkInitStruct);
    if ((RCC_ClkInitStruct.CPUCLKSource == RCC_CPUCLKSOURCE_IC1) ||
        (RCC_ClkInitStruct.SYSCLKSource == RCC_SYSCLKSOURCE_IC2_IC6_IC11)) {
        RCC_ClkInitStruct.ClockType    = (RCC_CLOCKTYPE_CPUCLK | RCC_CLOCKTYPE_SYSCLK);
        RCC_ClkInitStruct.CPUCLKSource = RCC_CPUCLKSOURCE_HSI;
        RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
        if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct) != HAL_OK)
            Error_Handler();
    }

    RCC_OscInitStruct.OscillatorType     = RCC_OSCILLATORTYPE_NONE;
    RCC_OscInitStruct.PLL1.PLLState      = RCC_PLL_ON;
    RCC_OscInitStruct.PLL1.PLLSource     = RCC_PLLSOURCE_HSI;
    RCC_OscInitStruct.PLL1.PLLM         = 2;
    RCC_OscInitStruct.PLL1.PLLN         = 25;
    RCC_OscInitStruct.PLL1.PLLFractional = 0;
    RCC_OscInitStruct.PLL1.PLLP1        = 1;
    RCC_OscInitStruct.PLL1.PLLP2        = 1;
    RCC_OscInitStruct.PLL2.PLLState      = RCC_PLL_ON;
    RCC_OscInitStruct.PLL2.PLLSource     = RCC_PLLSOURCE_HSI;
    RCC_OscInitStruct.PLL2.PLLM         = 4;
    RCC_OscInitStruct.PLL2.PLLN         = 75;
    RCC_OscInitStruct.PLL2.PLLFractional = 0;
    RCC_OscInitStruct.PLL2.PLLP1        = 2;
    RCC_OscInitStruct.PLL2.PLLP2        = 1;
    RCC_OscInitStruct.PLL3.PLLState      = RCC_PLL_NONE;
    RCC_OscInitStruct.PLL4.PLLState      = RCC_PLL_NONE;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
        Error_Handler();

    RCC_ClkInitStruct.ClockType      = RCC_CLOCKTYPE_CPUCLK | RCC_CLOCKTYPE_HCLK
                                     | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1
                                     | RCC_CLOCKTYPE_PCLK2  | RCC_CLOCKTYPE_PCLK5
                                     | RCC_CLOCKTYPE_PCLK4;
    RCC_ClkInitStruct.CPUCLKSource   = RCC_CPUCLKSOURCE_IC1;
    RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_IC2_IC6_IC11;
    RCC_ClkInitStruct.AHBCLKDivider  = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV1;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV1;
    RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV1;
    RCC_ClkInitStruct.APB5CLKDivider = RCC_APB5_DIV1;
    RCC_ClkInitStruct.IC1Selection.ClockSelection  = RCC_ICCLKSOURCE_PLL2;
    RCC_ClkInitStruct.IC1Selection.ClockDivider    = 1;
    RCC_ClkInitStruct.IC2Selection.ClockSelection  = RCC_ICCLKSOURCE_PLL1;
    RCC_ClkInitStruct.IC2Selection.ClockDivider    = 6;
    RCC_ClkInitStruct.IC6Selection.ClockSelection  = RCC_ICCLKSOURCE_PLL1;
    RCC_ClkInitStruct.IC6Selection.ClockDivider    = 3;
    RCC_ClkInitStruct.IC11Selection.ClockSelection = RCC_ICCLKSOURCE_PLL1;
    RCC_ClkInitStruct.IC11Selection.ClockDivider   = 3;
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct) != HAL_OK)
        Error_Handler();
}

/**
  * @brief Peripherals Common Clock Configuration
  */
void PeriphCommonClock_Config(void)
{
    RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};
    PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_TIM;
    PeriphClkInitStruct.TIMPresSelection     = RCC_TIMPRES_DIV1;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
        Error_Handler();
}

/**
  * @brief CACHEAXI Initialization
  */
static void MX_CACHEAXI_Init(void)
{
    hcacheaxi.Instance = CACHEAXI;
    if (HAL_CACHEAXI_Init(&hcacheaxi) != HAL_OK)
        Error_Handler();
}

/**
  * @brief ICACHE Initialization
  */
static void MX_ICACHE_Init(void)
{
    if (HAL_ICACHE_ConfigAssociativityMode(ICACHE_1WAY) != HAL_OK)
        Error_Handler();
    if (HAL_ICACHE_Enable() != HAL_OK)
        Error_Handler();
}

/**
  * @brief GPIO Initialization
  */
static void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOH_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitStruct.Pin       = VCP_RX_Pin;
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull      = GPIO_NOPULL;
    GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF3_LPUART1;
    HAL_GPIO_Init(VCP_RX_GPIO_Port, &GPIO_InitStruct);

    GPIO_InitStruct.Pin  = GPIO_PIN_10 | UCPD1_VSENSE_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

/**
  * @brief TIM6 Initialization
  */
static void MX_TIM6_Init(void)
{
    TIM_MasterConfigTypeDef sMasterConfig = {0};
    htim6.Instance               = TIM6;
    htim6.Init.Prescaler         = 100 - 1;
    htim6.Init.CounterMode       = TIM_COUNTERMODE_UP;
    htim6.Init.Period            = 65535;
    htim6.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    if (HAL_TIM_Base_Init(&htim6) != HAL_OK)
        Error_Handler();
    sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
    sMasterConfig.MasterSlaveMode     = TIM_MASTERSLAVEMODE_DISABLE;
    if (HAL_TIMEx_MasterConfigSynchronization(&htim6, &sMasterConfig) != HAL_OK)
        Error_Handler();
}

/**
  * @brief UART4 Initialization
  */
static void MX_UART4_Init(void)
{
    huart4.Instance                    = UART4;
    huart4.Init.BaudRate               = 115200;
    huart4.Init.WordLength             = UART_WORDLENGTH_8B;
    huart4.Init.StopBits               = UART_STOPBITS_1;
    huart4.Init.Parity                 = UART_PARITY_NONE;
    huart4.Init.Mode                   = UART_MODE_TX_RX;
    huart4.Init.HwFlowCtl              = UART_HWCONTROL_NONE;
    huart4.Init.OverSampling           = UART_OVERSAMPLING_16;
    huart4.Init.OneBitSampling         = UART_ONE_BIT_SAMPLE_DISABLE;
    huart4.Init.ClockPrescaler         = UART_PRESCALER_DIV1;
    huart4.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
    if (HAL_UART_Init(&huart4) != HAL_OK)                              Error_Handler();
    if (HAL_UARTEx_SetTxFifoThreshold(&huart4, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK) Error_Handler();
    if (HAL_UARTEx_SetRxFifoThreshold(&huart4, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK) Error_Handler();
    if (HAL_UARTEx_DisableFifoMode(&huart4) != HAL_OK)                Error_Handler();
}

/**
  * @brief I2C1 Initialization
  */
static void MX_I2C1_Init(void)
{
    hi2c1.Instance             = I2C1;
    hi2c1.Init.Timing          = 0x10C0ECFF;
    hi2c1.Init.OwnAddress1     = 0;
    hi2c1.Init.AddressingMode  = I2C_ADDRESSINGMODE_7BIT;
    hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c1.Init.OwnAddress2     = 0;
    hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
    hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c1.Init.NoStretchMode   = I2C_NOSTRETCH_DISABLE;
    if (HAL_I2C_Init(&hi2c1) != HAL_OK)                                Error_Handler();
    if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK) Error_Handler();
    if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK)           Error_Handler();
}

/**
  * @brief I2C2 Initialization
  */
static void MX_I2C2_Init(void)
{
    hi2c2.Instance             = I2C2;
    hi2c2.Init.Timing          = 0x10C0ECFF;
    hi2c2.Init.OwnAddress1     = 0;
    hi2c2.Init.AddressingMode  = I2C_ADDRESSINGMODE_7BIT;
    hi2c2.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c2.Init.OwnAddress2     = 0;
    hi2c2.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
    hi2c2.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c2.Init.NoStretchMode   = I2C_NOSTRETCH_DISABLE;
    if (HAL_I2C_Init(&hi2c2) != HAL_OK)                                Error_Handler();
    if (HAL_I2CEx_ConfigAnalogFilter(&hi2c2, I2C_ANALOGFILTER_ENABLE) != HAL_OK) Error_Handler();
    if (HAL_I2CEx_ConfigDigitalFilter(&hi2c2, 0) != HAL_OK)           Error_Handler();
}

/* USER CODE BEGIN 4 */
/* USER CODE END 4 */

/**
  * @brief  Error Handler
  */
void Error_Handler(void)
{
    /* USER CODE BEGIN Error_Handler_Debug */
    __disable_irq();
    BSP_LED_On(LED2);
    while (1) {}
    /* USER CODE END Error_Handler_Debug */
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line) {}
#endif
