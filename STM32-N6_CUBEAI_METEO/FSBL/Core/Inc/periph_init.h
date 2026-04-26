/**
 ******************************************************************************
 * @file    periph_init.h
 * @brief   Initialisation de tous les périphériques matériels
 *          GPIO, UART, I2C, Cache, Timers — tout en un seul appel
 ******************************************************************************
 */
#ifndef PERIPH_INIT_H
#define PERIPH_INIT_H

/**
 * @brief  Initialise tous les périphériques (GPIO, I2C, UART, Cache, Timer)
 *         À appeler une seule fois au début de main()
 * @retval 0 si OK, -1 si erreur COM
 */
int periph_init(void);

#endif /* PERIPH_INIT_H */
