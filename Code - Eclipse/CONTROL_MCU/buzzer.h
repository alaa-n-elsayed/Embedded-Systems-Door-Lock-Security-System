/******************************************************************************
 *
 * Module: BUZZER
 *
 * File Name: buzzer.h
 *
 * Description: Header file for the BUZZER AVR driver
 *
 * Author: Alaa Elsayed
 *
 *******************************************************************************/

#ifndef BUZZER_H_
#define BUZZER_H_

#include "std_types.h"

/*******************************************************************************
 *                                Definitions                                  *
 *******************************************************************************/

/* Buzzer HW Ports and Pins IDs */
#define BUZZER_PORT_ID				PORTA_ID
#define BUZZER_PIN_ID				PIN0_ID

/*******************************************************************************
 *                      Functions Prototypes                                   *
 *******************************************************************************/

/*
 * Description :
 * 				Initialize Buzzer by adjusting the direction of its Pin as Output
 */
void Buzzer_Init(void);

/*
 * Description :
 * 				Turn On the Buzzer
 */
void Buzzer_On(void);

/*
 * Description :
 * 				Turn Off the Buzzer
 */
void Buzzer_Off(void);

#endif /* BUZZER_H_ */
