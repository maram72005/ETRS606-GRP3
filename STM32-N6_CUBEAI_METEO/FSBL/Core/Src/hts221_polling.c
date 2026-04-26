#include <stdio.h>
#include <string.h>
#include "hts221_reg.h"
#include "stm32n6xx_hal.h"
#include "main.h"
#include "stm32n6xx_nucleo.h"

extern I2C_HandleTypeDef hi2c1;
extern float last_temp;
extern float last_rhum;
extern UART_HandleTypeDef hcom_uart[];

#define HTS221_ADDR  (0x5F << 1)

static stmdev_ctx_t hts_ctx;
typedef struct { float x0, y0, x1, y1; } lin_t;
static lin_t hts_hum_cal;
static lin_t hts_temp_cal;

/* Fonctions de com I2C */
static int32_t hts_write(void *h, uint8_t reg, const uint8_t *buf, uint16_t len) {
  if (len > 1) reg |= 0x80;
  return HAL_I2C_Mem_Write((I2C_HandleTypeDef *)h, HTS221_ADDR, reg, I2C_MEMADD_SIZE_8BIT, (uint8_t *)buf, len, 1000);
}

static int32_t hts_read(void *h, uint8_t reg, uint8_t *buf, uint16_t len) {
  if (len > 1) reg |= 0x80;
  return HAL_I2C_Mem_Read((I2C_HandleTypeDef *)h, HTS221_ADDR, reg, I2C_MEMADD_SIZE_8BIT, buf, len, 1000);
}

/* Interpolation lineaire precise */
static float lin_interp(lin_t *l, int16_t x) {
  if (l->x1 == l->x0) return l->y0;
  return ((l->y1 - l->y0) * (float)(x - l->x0) / (l->x1 - l->x0)) + l->y0;
}

int hts221_init(void) {
  hts_ctx.write_reg = hts_write;
  hts_ctx.read_reg  = hts_read;
  hts_ctx.handle    = &hi2c1;

  HAL_Delay(200);

  uint8_t id = 0;
  hts221_device_id_get(&hts_ctx, &id);
  if (id != HTS221_ID) return -1;

  /* ── Power ON en premier — charge l'OTP dans les registres ─────── */
  hts221_power_on_set(&hts_ctx, PROPERTY_ENABLE);
  HAL_Delay(20);   /* attente chargement OTP */

  /* ── Lecture registre par registre ─────────────────────────────── */
  uint8_t r[16] = {0};
  for (int i = 0; i < 16; i++) {
    HAL_I2C_Mem_Read(&hi2c1, HTS221_ADDR,
                     0x30 + i, I2C_MEMADD_SIZE_8BIT,
                     &r[i], 1, 1000);
  }

  /* ── Dump brut ───────────────────────────────────────────────────  */
  char dbg[120];
  snprintf(dbg, sizeof(dbg),
      "[RAW] %02X %02X %02X %02X %02X %02X %02X %02X\r\n",
      r[0],r[1],r[2],r[3],r[4],r[5],r[6],r[7]);
  HAL_UART_Transmit(&hcom_uart[COM1], (uint8_t*)dbg, strlen(dbg), 1000);

  snprintf(dbg, sizeof(dbg),
      "[RAW] %02X %02X %02X %02X %02X %02X %02X %02X\r\n",
      r[8],r[9],r[10],r[11],r[12],r[13],r[14],r[15]);
  HAL_UART_Transmit(&hcom_uart[COM1], (uint8_t*)dbg, strlen(dbg), 1000);

  /* ── Humidité ────────────────────────────────────────────────────  */
  hts_hum_cal.y0 = (float)r[0x00] / 2.0f;
  hts_hum_cal.y1 = (float)r[0x01] / 2.0f;
  hts_hum_cal.x0 = (float)(int16_t)((r[0x07] << 8) | r[0x06]);
  hts_hum_cal.x1 = (float)(int16_t)((r[0x0B] << 8) | r[0x0A]);

  /* ── Température ─────────────────────────────────────────────────  */
  uint16_t T0_x8 = r[0x02] | ((r[0x05] & 0x03) << 8);
  uint16_t T1_x8 = r[0x03] | ((r[0x05] & 0x0C) << 6);
  hts_temp_cal.y0 = (float)T0_x8 / 8.0f;
  hts_temp_cal.y1 = (float)T1_x8 / 8.0f;
  hts_temp_cal.x0 = (float)(int16_t)((r[0x0D] << 8) | r[0x0C]);
  hts_temp_cal.x1 = (float)(int16_t)((r[0x0F] << 8) | r[0x0E]);

  /* ── Affichage calibration ───────────────────────────────────────  */
  snprintf(dbg, sizeof(dbg),
      "[CAL] hum  x0=%.0f y0=%.1f x1=%.0f y1=%.1f\r\n",
      hts_hum_cal.x0, hts_hum_cal.y0,
      hts_hum_cal.x1, hts_hum_cal.y1);
  HAL_UART_Transmit(&hcom_uart[COM1], (uint8_t*)dbg, strlen(dbg), 1000);

  snprintf(dbg, sizeof(dbg),
      "[CAL] temp x0=%.0f y0=%.1f x1=%.0f y1=%.1f\r\n",
      hts_temp_cal.x0, hts_temp_cal.y0,
      hts_temp_cal.x1, hts_temp_cal.y1);
  HAL_UART_Transmit(&hcom_uart[COM1], (uint8_t*)dbg, strlen(dbg), 1000);

  /* ── Sanity check ────────────────────────────────────────────────  */
  if (hts_hum_cal.x1  <= hts_hum_cal.x0 ||
      hts_temp_cal.x1 <= hts_temp_cal.x0) {
    HAL_UART_Transmit(&hcom_uart[COM1],
        (uint8_t*)"[CAL] ERREUR calibration incoh\r\n", 31, 1000);
    return -2;
  }

  /* ── Configuration ODR ───────────────────────────────────────────  */
  hts221_data_rate_set(&hts_ctx, HTS221_ODR_1Hz);
  hts221_block_data_update_set(&hts_ctx, PROPERTY_ENABLE);
  HAL_Delay(100);

  HAL_UART_Transmit(&hcom_uart[COM1],
      (uint8_t*)"[HTS221] OK\r\n", 13, 1000);
  return 0;
}
void hts221_read_temp(void) {
  hts221_status_reg_t st;
  hts221_status_get(&hts_ctx, &st);

  if (st.t_da || 1) { // On force la lecture pour tester la variation
    int16_t raw = 0;
    hts221_temperature_raw_get(&hts_ctx, &raw);
    last_temp = lin_interp(&hts_temp_cal, raw);
    //printf("Temp: %.2f C\r\n", last_temp);
  }
}

void hts221_read_hum(void) {
  hts221_status_reg_t st;
  hts221_status_get(&hts_ctx, &st);

  int16_t raw = 0;
  hts221_humidity_raw_get(&hts_ctx, &raw);
  last_rhum = lin_interp(&hts_hum_cal, raw);
}
