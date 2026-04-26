/**
 ******************************************************************************
 * @file    hts221_polling.h
 * @brief   Interface du driver HTS221 (Température + Humidité)
 ******************************************************************************
 */
#ifndef HTS221_POLLING_H
#define HTS221_POLLING_H

int  hts221_init(void);
void hts221_read_temp(void);
void hts221_read_hum(void);

#endif /* HTS221_POLLING_H */
