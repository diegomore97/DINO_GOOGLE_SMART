/*
 * sistemaPrincipal.h
 *
 *  Created on: 7 abr. 2020
 *      Author: Diego Moreno
 */

#ifndef SISTEMAPRINCIPAL_H_
#define SISTEMAPRINCIPAL_H_

#include <fsl_gpio.h>
#include "fsl_debug_console.h"
#include "fsl_pit.h"
#include "fsl_uart.h"
#include "fsl_adc16.h"
#include "fsl_tpm.h"
#include <stdio.h>
#include "tipoVariables.h"
#include "adctopwm.h"

#define PIT_SOURCE_CLOCK CLOCK_GetFreq(kCLOCK_BusClk)

void configPits(void);
void prepararSistema(void);

volatile uint32_t flagPIT0 = 0;
SensorPwm ldr1, ldr2;


void configPits(void)
{
	pit_config_t My_PIT;

	PIT_GetDefaultConfig(&My_PIT);

	PIT_Init(PIT, &My_PIT);

	/* Set timer period for channel 0 */
	//PIT_SetTimerPeriod(PIT, kPIT_Chnl_0, QUARTER_USEC_TO_COUNT(90U, PIT_SOURCE_CLOCK));
	PIT_SetTimerPeriod(PIT, kPIT_Chnl_0,MSEC_TO_COUNT(50, PIT_SOURCE_CLOCK));

	/* Enable timer interrupts for channel 0 */
	PIT_EnableInterrupts(PIT, kPIT_Chnl_0, kPIT_TimerInterruptEnable);

	PIT_StopTimer(PIT, kPIT_Chnl_0);

	EnableIRQ(PIT_IRQn);

}

void prepararSistema(void)
{

	//configPits();   //Timer0

	configPwm();    //PWM

	initSensorPwm(&ldr1, ADC0, 0U, 0U, 1500); //Canal de ADC que se sensara    | CONFIGURAR ADC0 GRUPO 0 CANAL 0 PTE20  | UMBRAL DE 1500
	initSensorPwm(&ldr2, ADC0, 3U, 0U, 1000 ); //Canal de ADC que se sensara   | CONFIGURAR ADC0 GRUPO 3 CANAL 0 PTE22 | UMBRAL DE 1000

	PRINTF("SISTEMA DINOSAURIO DE GOOGLE INICIADO\n\n");

}

#endif /* SISTEMAPRINCIPAL_H_ */
