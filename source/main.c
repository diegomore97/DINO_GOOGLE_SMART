//INTEGRANTES DE EQUIPO:
  //Vazquez Barba Christian
  //Chavez Padilla Alejandro
  //Moreno Arroyo Diego
  //Ortega Gonzalez Misael

#include "board.h"
#include "pin_mux.h"
#include "fsl_pit.h"

#include "gsc_sch_core.h"
#include "gsc_sch_core_tick_isr.h"
#include "core_cm0plus.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/*******************************************************************************
 * Variables
 ******************************************************************************/
volatile unsigned int sys_tick_counter = 0;

/* TODO: insert other include files here. */

/* TODO: insert other definitions and declarations here. */

/*
 * @brief   Application entry point.
 */


int main(void)
{
	/* Board pin, clock, debug console init */
	BOARD_InitPins();
	BOARD_BootClockRUN();
	BOARD_InitDebugConsole();

	/* SysTick Configuration */
	SysTick_Config(48000000U/1000U); //This only applies for ARM Cores with SysTick capability

	prepararSistema(); //Inicializar modulos y variables para el reproductor de musica

	/* Scheduler Initialization and tasks initialization  */
	gsc_sch_core_Init();

	/* Execute Scheduler */
	gsc_sch_core_exec();
}

void SysTick_Handler(void)
 {
 	sys_tick_counter++;
 	gsc_sch_core_tick_isr();
 }

