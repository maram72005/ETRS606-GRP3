/**
 ******************************************************************************
 * @file    pressure_polling.h
 * @brief   Interface du driver LPS22HH (Pression atmosphérique)
 ******************************************************************************
 */
#ifndef PRESSURE_POLLING_H
#define PRESSURE_POLLING_H

int  pressure_init(void);
void pressure_read(void);

#endif /* PRESSURE_POLLING_H */
