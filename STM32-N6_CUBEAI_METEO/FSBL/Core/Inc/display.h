/**
 ******************************************************************************
 * @file    display.h
 * @brief   Interface d'affichage UART — formatage des données météo
 ******************************************************************************
 */
#ifndef DISPLAY_H
#define DISPLAY_H

#include "sensors.h"
#include <stdint.h>

/* ── API publique ─────────────────────────────────────────────────────────── */
void display_init(void);
void display_sensors(const SensorData_t *s);
void display_ai_results(const float *predictions, const char **labels, uint8_t n);
void display_banner(void);

#endif /* DISPLAY_H */
