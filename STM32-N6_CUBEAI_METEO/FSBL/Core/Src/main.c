/**
 ******************************************************************************
 * @file    main.c
 * @brief   MeteoNet STM32N6 — Point d'entrée
 *          Architecture : periph_init → sensors_init → boucle (sensors_read,
 *                         ai_run, display)
 *
 *          main() ne contient QUE 3 appels haut niveau :
 *            1. periph_init()    — matériel
 *            2. sensors_init()   — capteurs
 *            3. ai_init()        — réseau de neurones
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "periph_init.h"
#include "sensors.h"
#include "display.h"
#include <string.h>
#include <math.h>

/* USER CODE BEGIN Includes */
#include "meteo.h"
#include "meteo_data.h"
#include "meteo_data_params.h"
/* USER CODE END Includes */

/* ── Constantes ──────────────────────────────────────────────────────────── */
#define N_FEATURES  27
#define N_OUTPUTS    6

/* ── Variables IA ────────────────────────────────────────────────────────── */
static ai_handle   ai_meteo = AI_HANDLE_NULL;
static ai_buffer   ai_input[AI_METEO_IN_NUM];
static ai_buffer   ai_output[AI_METEO_OUT_NUM];
static float       in_data[AI_METEO_IN_1_SIZE];
static float       out_data[AI_METEO_OUT_1_SIZE];
static uint8_t     activation_buffer[AI_METEO_DATA_ACTIVATIONS_SIZE];

/* ── Labels des 6 sorties ────────────────────────────────────────────────── */
static const char *LABELS[N_OUTPUTS] = {
    "Pluie      ",
    "Beau temps ",
    "Nuageux    ",
    "Brouillard ",
    "Vent fort  ",
    "Gel        "
};

/* ── Historique 3 jours ──────────────────────────────────────────────────── */
static float   hist_temp[3] = {20.0f, 20.0f, 20.0f};
static float   hist_hum[3]  = {70.0f, 70.0f, 70.0f};
static float   hist_pres[3] = {1013.0f, 1013.0f, 1013.0f};
static uint8_t hist_init    = 0;

/* ── Mois courant (1-12) ─────────────────────────────────────────────────── */
static uint8_t mois_courant = 4;  /* Avril — à mettre à jour via RTC */

/* ── Scaler Z-score (généré par le notebook) ─────────────────────────────── */
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

/* ── Prototypes privés ───────────────────────────────────────────────────── */
void SystemClock_Config(void);
void PeriphCommonClock_Config(void);
static int  ai_init(void);
static void ai_run(void);

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
        /* Utilise display pour signaler l'erreur */
        display_ai_results(NULL, NULL, 0);
        return -1;
    }

    ai_input[0]       = *ai_meteo_inputs_get(ai_meteo, NULL);
    ai_output[0]      = *ai_meteo_outputs_get(ai_meteo, NULL);
    ai_input[0].data  = AI_HANDLE_PTR(in_data);
    ai_output[0].data = AI_HANDLE_PTR(out_data);

    return 0;
}

/* ============================================================
 * ai_run — Construction features + inférence + affichage
 * ============================================================ */
static void ai_run(void)
{
    float t = g_sensors.temp;
    float h = g_sensors.hum;
    float p = g_sensors.pres;

    /* Initialisation historique au premier appel */
    if (!hist_init) {
        for (int i = 0; i < 3; i++) {
            hist_temp[i] = t;
            hist_hum[i]  = h;
            hist_pres[i] = p;
        }
        hist_init = 1;
    }

    float t1 = hist_temp[0], h1 = hist_hum[0], p1 = hist_pres[0];
    float t2 = hist_temp[1], h2 = hist_hum[1], p2 = hist_pres[1];
    float t3 = hist_temp[2], h3 = hist_hum[2], p3 = hist_pres[2];

    float temp_3j = (t + t1 + t2) / 3.0f;
    float hum_3j  = (h + h1 + h2) / 3.0f;
    float pres_3j = (p + p1 + p2) / 3.0f;
    float pi      = 3.14159265f;

    /* Construction des 27 features */
    float raw[N_FEATURES] = {
        t,
        t + 3.0f,
        t - 3.0f,
        h,
        (h + 5.0f > 100.0f ? 100.0f : h + 5.0f),
        (h - 5.0f <   0.0f ?   0.0f : h - 5.0f),
        p,
        6.0f,
        t + 0.033f * h,
        t - t1,
        h - h1,
        p - p1,
        t1, h1, p1,
        t2, h2, p2,
        t3, h3, p3,
        temp_3j, hum_3j, pres_3j,
        p - pres_3j,
        sinf(2.0f * pi * mois_courant / 12.0f),
        cosf(2.0f * pi * mois_courant / 12.0f)
    };

    /* Normalisation Z-score */
    for (int i = 0; i < N_FEATURES; i++)
        in_data[i] = (raw[i] - SCALER_MEAN[i]) / SCALER_STD[i];

    /* Inférence */
    ai_i32 n = ai_meteo_run(ai_meteo, ai_input, ai_output);
    if (n <= 0) return;

    /* Affichage via display.c */
    display_ai_results(out_data, LABELS, N_OUTPUTS);

    /* LEDs : LED2 = pluie, LED1 = beau temps */
    if      (out_data[0] >= 0.5f) { BSP_LED_On(LED2); BSP_LED_Off(LED1); }
    else if (out_data[1] >= 0.5f) { BSP_LED_On(LED1); BSP_LED_Off(LED2); }
    else                           { BSP_LED_Off(LED1); BSP_LED_Off(LED2); }

    /* Décalage historique */
    hist_temp[2] = hist_temp[1]; hist_hum[2] = hist_hum[1]; hist_pres[2] = hist_pres[1];
    hist_temp[1] = hist_temp[0]; hist_hum[1] = hist_hum[0]; hist_pres[1] = hist_pres[0];
    hist_temp[0] = t;            hist_hum[0]  = h;           hist_pres[0] = p;
}

/* ============================================================
 *  MAIN — 3 appels d'initialisation, boucle simple
 * ============================================================ */
int main(void)
{
    HAL_Init();
    SystemClock_Config();
    PeriphCommonClock_Config();

    /* ── 1. Périphériques ─────────────────────────────────── */
    if (periph_init() != 0)
        while (1) {}   /* Erreur COM : LED2 allumée par periph_init */

    display_banner();

    /* ── 2. Capteurs ──────────────────────────────────────── */
    sensors_init();    /* Les erreurs par capteur sont loguées dans sensors.c */

    /* ── 3. Réseau de neurones ────────────────────────────── */
    ai_init();

    /* ── Boucle principale ────────────────────────────────── */
    while (1)
    {
        sensors_read();
        display_sensors(&g_sensors);
        ai_run();
        HAL_Delay(2000);
    }
}

/* ── Fonctions horloge (inchangées) ──────────────────────────────────────── */
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    if (HAL_PWREx_ConfigSupply(PWR_EXTERNAL_SOURCE_SUPPLY) != HAL_OK) Error_Handler();

    RCC_OscInitStruct.OscillatorType      = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState            = RCC_HSI_ON;
    RCC_OscInitStruct.HSIDiv              = RCC_HSI_DIV1;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL1.PLLState       = RCC_PLL_NONE;
    RCC_OscInitStruct.PLL2.PLLState       = RCC_PLL_NONE;
    RCC_OscInitStruct.PLL3.PLLState       = RCC_PLL_NONE;
    RCC_OscInitStruct.PLL4.PLLState       = RCC_PLL_NONE;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) Error_Handler();

    HAL_RCC_GetClockConfig(&RCC_ClkInitStruct);
    if ((RCC_ClkInitStruct.CPUCLKSource == RCC_CPUCLKSOURCE_IC1) ||
        (RCC_ClkInitStruct.SYSCLKSource == RCC_SYSCLKSOURCE_IC2_IC6_IC11)) {
        RCC_ClkInitStruct.ClockType    = (RCC_CLOCKTYPE_CPUCLK | RCC_CLOCKTYPE_SYSCLK);
        RCC_ClkInitStruct.CPUCLKSource = RCC_CPUCLKSOURCE_HSI;
        RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
        if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct) != HAL_OK) Error_Handler();
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
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) Error_Handler();

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
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct) != HAL_OK) Error_Handler();
}

void PeriphCommonClock_Config(void)
{
    RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};
    PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_TIM;
    PeriphClkInitStruct.TIMPresSelection     = RCC_TIMPRES_DIV1;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK) Error_Handler();
}

void Error_Handler(void)
{
    __disable_irq();
    BSP_LED_On(LED2);
    while (1) {}
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line) {}
#endif
