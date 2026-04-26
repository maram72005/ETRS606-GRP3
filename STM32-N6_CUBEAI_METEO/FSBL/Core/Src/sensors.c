/**
 ******************************************************************************
 * @file    sensors.c
 * @brief   Acquisition des capteurs — HTS221 (Temp+Hum) + LPS22HH (Pression)
 *
 *          === POUR CHANGER UN CAPTEUR ===
 *          Modifier uniquement ce fichier.
 *          L'interface (sensors.h) et main.c restent inchangés.
 ******************************************************************************
 */
#include "sensors.h"
#include "hts221_polling.h"
#include "pressure_polling.h"

/* ── Instance globale ─────────────────────────────────────────────────────── */
SensorData_t g_sensors = {
    .temp = 20.0f,
    .hum  = 70.0f,
    .pres = 1013.0f
};

/* ── Variables partagées avec les drivers (définies ici, extern dans les drivers) ── */
float last_temp = 20.0f;
float last_rhum = 70.0f;
float last_pres = 1013.0f;

/**
 * @brief Initialise tous les capteurs
 * @retval 0 si tous OK, masque de bits d'erreur sinon (bit0=HTS221, bit1=LPS22HH)
 */
int sensors_init(void)
{
    int err = 0;

    /* --- Capteur Température + Humidité : HTS221 --- */
    if (hts221_init() != 0)
        err |= (1 << 0);

    /* --- Capteur Pression : LPS22HH --- */
    if (pressure_init() != 0)
        err |= (1 << 1);

    return err;
}

/**
 * @brief Lit tous les capteurs et met à jour g_sensors
 *        Inclut le clamp des valeurs physiquement impossibles
 */
void sensors_read(void)
{
    /* --- Lecture hardware --- */
    hts221_read_temp();
    hts221_read_hum();
    pressure_read();

    /* --- Clamp sécurité --- */
    if (last_temp < -40.0f) last_temp = -40.0f;
    if (last_temp >  60.0f) last_temp =  60.0f;
    if (last_rhum <   0.0f) last_rhum =   0.0f;
    if (last_rhum > 100.0f) last_rhum = 100.0f;
    if (last_pres < 900.0f) last_pres = 900.0f;
    if (last_pres > 1100.0f) last_pres = 1100.0f;

    /* --- Mise à jour de la structure globale --- */
    g_sensors.temp = last_temp;
    g_sensors.hum  = last_rhum;
    g_sensors.pres = last_pres;
}
