/**
 ******************************************************************************
 * @file    sensors.h
 * @brief   Interface d'acquisition des capteurs météo
 *          Pour changer un capteur : modifier uniquement sensors.c
 ******************************************************************************
 */
#ifndef SENSORS_H
#define SENSORS_H

#include <stdint.h>

/* ── Structure globale des mesures ───────────────────────────────────────── */
typedef struct {
    float temp;   /* Température (°C)    */
    float hum;    /* Humidité relative (%) */
    float pres;   /* Pression ramenée mer (hPa) */
} SensorData_t;

/* Instance globale accessible depuis tous les fichiers */
extern SensorData_t g_sensors;

/* ── API publique ─────────────────────────────────────────────────────────
 * Pour remplacer un capteur :
 *   1. Modifier l'implémentation dans sensors.c
 *   2. Ne pas toucher à ce .h ni à main.c
 * ──────────────────────────────────────────────────────────────────────── */
int  sensors_init(void);   /* Retourne 0 si OK, code erreur sinon */
void sensors_read(void);   /* Met à jour g_sensors                */

#endif /* SENSORS_H */
