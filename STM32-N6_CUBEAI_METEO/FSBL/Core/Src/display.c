/**
 ******************************************************************************
 * @file    display.c
 * @brief   Formatage et envoi UART — toute la logique d'affichage ici
 *          Pour changer le format d'affichage : modifier uniquement ce fichier
 ******************************************************************************
 */
#include "display.h"
#include "stm32n6xx_hal.h"
#include "stm32n6xx_nucleo.h"
#include <stdio.h>
#include <string.h>

extern UART_HandleTypeDef hcom_uart[];

/* ── Macro interne pour envoyer une chaîne ───────────────────────────────── */
#define UART_PRINT(str) \
    HAL_UART_Transmit(&hcom_uart[COM1], (uint8_t*)(str), strlen(str), 1000)

/**
 * @brief Affiche la bannière de démarrage
 */
void display_banner(void)
{
    UART_PRINT("\r\n========================================\r\n");
    UART_PRINT("   METEO AI — STM32N6 — 6 classes\r\n");
    UART_PRINT("========================================\r\n");
}

/**
 * @brief Affiche les mesures brutes des capteurs
 */
void display_sensors(const SensorData_t *s)
{
    char buf[80];
    snprintf(buf, sizeof(buf),
        "Temp: %.2f C | Hum: %.2f %% | Pres: %.2f hPa\r\n",
        s->temp, s->hum, s->pres);
    UART_PRINT(buf);
}

/**
 * @brief Affiche les résultats de l'inférence IA
 * @param predictions  Tableau de N probabilités (0.0 à 1.0)
 * @param labels       Tableau de N chaînes de libellés
 * @param n            Nombre de classes
 */
void display_ai_results(const float *predictions, const char **labels, uint8_t n)
{
    char buf[64];

    UART_PRINT("=== METEO AI ===\r\n");

    for (uint8_t i = 0; i < n; i++) {
        float pct = predictions[i] * 100.0f;
        snprintf(buf, sizeof(buf), "  %s : %5.1f%%%s\r\n",
                 labels[i], pct, (pct >= 50.0f) ? " <<<" : "");
        UART_PRINT(buf);
    }

    UART_PRINT("================\r\n");
}
