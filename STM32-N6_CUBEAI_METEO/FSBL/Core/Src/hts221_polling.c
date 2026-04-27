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

static int32_t hts_write(void *h, uint8_t reg, const uint8_t *buf, uint16_t len) {
  if (len > 1) reg |= 0x80;
  return HAL_I2C_Mem_Write((I2C_HandleTypeDef *)h, HTS221_ADDR, reg,
                            I2C_MEMADD_SIZE_8BIT, (uint8_t *)buf, len, 1000);
}

static int32_t hts_read(void *h, uint8_t reg, uint8_t *buf, uint16_t len) {
  if (len > 1) reg |= 0x80;
  return HAL_I2C_Mem_Read((I2C_HandleTypeDef *)h, HTS221_ADDR, reg,
                           I2C_MEMADD_SIZE_8BIT, buf, len, 1000);
}

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
  if (id != HTS221_ID) {
    HAL_UART_Transmit(&hcom_uart[COM1],
        (uint8_t*)"[HTS221] ID KO\r\n", 16, 1000);
    return -1;
  }

  /* Power ON — charge l'OTP */
  hts221_power_on_set(&hts_ctx, PROPERTY_ENABLE);
  HAL_Delay(20);

  /* ── Calibration via les fonctions du driver (gère les offsets) ── */
  hts221_hum_adc_point_0_get(&hts_ctx, &hts_hum_cal.x0);
  hts221_hum_rh_point_0_get(&hts_ctx,  &hts_hum_cal.y0);
  hts221_hum_adc_point_1_get(&hts_ctx, &hts_hum_cal.x1);
  hts221_hum_rh_point_1_get(&hts_ctx,  &hts_hum_cal.y1);

  hts221_temp_adc_point_0_get(&hts_ctx, &hts_temp_cal.x0);
  hts221_temp_deg_point_0_get(&hts_ctx, &hts_temp_cal.y0);
  hts221_temp_adc_point_1_get(&hts_ctx, &hts_temp_cal.x1);
  hts221_temp_deg_point_1_get(&hts_ctx, &hts_temp_cal.y1);

  /* Affichage calibration */
  char dbg[80];
  snprintf(dbg, sizeof(dbg),
      "[CAL] hum  x0=%.0f y0=%.1f x1=%.0f y1=%.1f\r\n",
      hts_hum_cal.x0, hts_hum_cal.y0, hts_hum_cal.x1, hts_hum_cal.y1);
  HAL_UART_Transmit(&hcom_uart[COM1], (uint8_t*)dbg, strlen(dbg), 1000);

  snprintf(dbg, sizeof(dbg),
      "[CAL] temp x0=%.0f y0=%.1f x1=%.0f y1=%.1f\r\n",
      hts_temp_cal.x0, hts_temp_cal.y0, hts_temp_cal.x1, hts_temp_cal.y1);
  HAL_UART_Transmit(&hcom_uart[COM1], (uint8_t*)dbg, strlen(dbg), 1000);

  /* Sanity check */
  if (hts_hum_cal.x1 <= hts_hum_cal.x0 || hts_temp_cal.x1 <= hts_temp_cal.x0) {
    HAL_UART_Transmit(&hcom_uart[COM1],
        (uint8_t*)"[CAL] ERREUR calibration\r\n", 26, 1000);
    return -2;
  }

  /* ODR 7 Hz + BDU */
  hts221_data_rate_set(&hts_ctx, HTS221_ODR_7Hz);
  hts221_block_data_update_set(&hts_ctx, PROPERTY_ENABLE);
  HAL_Delay(200);

  HAL_UART_Transmit(&hcom_uart[COM1], (uint8_t*)"[HTS221] OK\r\n", 13, 1000);
  return 0;
}

void hts221_read_temp(void) {
  hts221_status_reg_t st;
  for (int retry = 0; retry < 20; retry++) {
    hts221_status_get(&hts_ctx, &st);
    if (st.t_da) break;
    HAL_Delay(50);
  }
  if (st.t_da) {
    int16_t raw = 0;
    hts221_temperature_raw_get(&hts_ctx, &raw);
    last_temp = lin_interp(&hts_temp_cal, raw);
  }
}

void hts221_read_hum(void) {
  hts221_status_reg_t st;
  for (int retry = 0; retry < 20; retry++) {
    hts221_status_get(&hts_ctx, &st);
    if (st.h_da) break;
    HAL_Delay(50);
  }
  if (st.h_da) {
    int16_t raw = 0;
    hts221_humidity_raw_get(&hts_ctx, &raw);
    last_rhum = lin_interp(&hts_hum_cal, raw);
  }
}
