#include <stdio.h>
#include "lps22hh_reg.h"
#include "stm32n6xx_hal.h"

extern I2C_HandleTypeDef hi2c1;
extern float last_pres;

// --- CONFIGURATION ---
#define LPS22HH_ADDR  (0x5D << 1)
#define MY_ALTITUDE   270.0f  // Altitude en mètres (Chambéry ~270m)

static stmdev_ctx_t pres_ctx;

/* --- Fonctions de communication I2C --- */
static int32_t pres_write(void *h, uint8_t reg, const uint8_t *buf, uint16_t len) {
  return HAL_I2C_Mem_Write((I2C_HandleTypeDef *)h, LPS22HH_ADDR, reg, I2C_MEMADD_SIZE_8BIT, (uint8_t *)buf, len, 1000);
}

static int32_t pres_read(void *h, uint8_t reg, uint8_t *buf, uint16_t len) {
  return HAL_I2C_Mem_Read((I2C_HandleTypeDef *)h, LPS22HH_ADDR, reg, I2C_MEMADD_SIZE_8BIT, buf, len, 1000);
}

/* --- Initialisation du capteur --- */
int pressure_init(void) {
  pres_ctx.write_reg = pres_write;
  pres_ctx.read_reg  = pres_read;
  pres_ctx.handle    = &hi2c1;
  pres_ctx.mdelay    = HAL_Delay;

  uint8_t id = 0;
  lps22hh_device_id_get(&pres_ctx, &id);

  if (id != LPS22HH_ID) {
      printf("[LPS22HH] Erreur : ID non reconnu (0x%02X)\r\n", id);
      return -1;
  }

  /* Reset logiciel du capteur */
  lps22hh_reset_set(&pres_ctx, PROPERTY_ENABLE);
  HAL_Delay(50);

  /* Bloquer la mise à jour des données pendant la lecture (évite les valeurs incohérentes) */
  lps22hh_block_data_update_set(&pres_ctx, PROPERTY_ENABLE);

  /* Configuration du débit de sortie à 10 Hz */
  lps22hh_data_rate_set(&pres_ctx, LPS22HH_10_Hz);

  printf("[LPS22HH] Initialise OK (Altitude: %.1fm)\r\n", MY_ALTITUDE);
  return 0;
}

/* --- Lecture et calcul de la pression corrigée --- */
void pressure_read(void) {
  lps22hh_all_sources_t st;

  /* Vérification de la disponibilité des données */
  lps22hh_all_sources_get(&pres_ctx, &st);

  if (st.status.p_da) {
    uint32_t raw_p = 0;
    lps22hh_pressure_raw_get(&pres_ctx, &raw_p);

    /* 1. Conversion LSB vers hPa (Pression mesurée au sol) */
    float pres_absolute = lps22hh_from_lsb_to_hpa(raw_p);

    /* 2. Correction Barométrique pour Chambéry
       À 27°C-30°C, l'air est moins dense, on utilise 8.36
       pour transformer la pression locale en pression "Niveau de la Mer" */
    last_pres = pres_absolute + (MY_ALTITUDE / 8.36f);

    /* Affichage pour ton terminal de debug */
    printf("[STATION CHAMBERY] Sol: %.2f hPa | Mer: %.2f hPa\r\n", pres_absolute, last_pres);
  }
}
