/*
 * adctopwm.h
 *
 *  Created on: 23 mar. 2020
 *      Author: Diego Moreno
 */

#ifndef ADCTOPWM_H_
#define ADCTOPWM_H_

#define BOARD_TPM_BASEADDR TPM2
#define BOARD_TPM_CHANNEL 1U  //PTB3
#define TPM_SOURCE_CLOCK CLOCK_GetFreq(kCLOCK_PllFllSelClk)
#define FRECUENCIA_PWM 50U //para un periodo de 20 ms

#define POSICION_INICIAL_SERVO 5  //Angulo de 0 grados | variar estas contantes segun la altura o diseño de tu teclado
#define POSICION_DESTINO_CLICK_SERVO 6 //Angulo de 40 grados | variar estas contantes segun la altura o diseño de tu teclado

typedef enum  //Maquina de estados para el sistema
{
	LEERADC = 1,
	CONVERTIR,
	PWMOUTPUT
}Adcpwm;

typedef struct //Tipo de dato para utilizar un sensor a traves del modulo ADC
{
	adv valorAdc;
	adc configCanalAdc;
	ADC_Type* base;
	adv umbral;
	ncadc numeroCanal;
	ngadc numeroGrupo;
}SensorPwm;


char buffer[25]; //Variable para pruebas
Adcpwm curr_state = LEERADC;
Adcpwm Next_state = LEERADC;
dcv dutyCyclePwm;

void configPwm(void);
void outputPwm(dcv dutyCyclePwm);
void configAdc(adc* configuracionCanal, ADC_Type* base, ncadc numeroCanal, ngadc numeroGrupo);
adv readAdc(adc* configuracionCanal, ADC_Type* base, ngadc numeroCanalGrupo);
void adctopwm(SensorPwm* s1, SensorPwm* s2);
void controlPersonaje(SensorPwm* s1, SensorPwm* s2);
void initSensorPwm(SensorPwm* s, ADC_Type* base, ncadc numeroCanal, ngadc numeroGrupo, adv umbral);
boolean objetoDetectado(SensorPwm* s);

void initSensorPwm(SensorPwm* s, ADC_Type* base, ncadc numeroCanal, ngadc numeroGrupo, adv umbral)
{
	s->valorAdc = 0;
	s->base = base;
	s->numeroCanal = numeroCanal;
	s->numeroGrupo = numeroGrupo;
	s->umbral =umbral;
	configAdc(&s->configCanalAdc, base, numeroCanal, numeroGrupo);
}

void configPwm(void)
{
	tpm_config_t tpmInfo;
	tpm_pwm_level_select_t pwmLevel = kTPM_HighTrue;
	tpm_chnl_pwm_signal_param_t tpmParam;

	tpmParam.chnlNumber = (tpm_chnl_t)BOARD_TPM_CHANNEL;
	tpmParam.level = pwmLevel;
	tpmParam.dutyCyclePercent = 0;

	CLOCK_SetTpmClock(1U);

	TPM_GetDefaultConfig(&tpmInfo);
	/* Initialize TPM module */
	tpmInfo.prescale = kTPM_Prescale_Divide_128; //Para que la frecuencia minima sea de 50 hz
	TPM_Init(BOARD_TPM_BASEADDR, &tpmInfo);

	TPM_SetupPwm(BOARD_TPM_BASEADDR, &tpmParam, 1U, kTPM_EdgeAlignedPwm, FRECUENCIA_PWM, TPM_SOURCE_CLOCK);

	TPM_StartTimer(BOARD_TPM_BASEADDR, kTPM_SystemClock);

}

void configAdc(adc* configuracionCanal, ADC_Type* base, ncadc numeroCanal, ngadc numeroCanalGrupo)
{
	adc16_config_t adc16ConfigStruct;

	ADC16_GetDefaultConfig(&adc16ConfigStruct);

#ifdef BOARD_ADC_USE_ALT_VREF
	adc16ConfigStruct.referenceVoltageSource = kADC16_ReferenceVoltageSourceValt;
#endif
	ADC16_Init(base, &adc16ConfigStruct);
	ADC16_EnableHardwareTrigger(base, false); /* Make sure the software trigger is used. */
#if defined(FSL_FEATURE_ADC16_HAS_CALIBRATION) && FSL_FEATURE_ADC16_HAS_CALIBRATION
	if (kStatus_Success == ADC16_DoAutoCalibration(base))
	{
		//PRINTF("ADC16_DoAutoCalibration() Done.\r\n");
	}
	else
	{
		//PRINTF("ADC16_DoAutoCalibration() Failed.\r\n");
	}
#endif /* FSL_FEATURE_ADC16_HAS_CALIBRATION */

	(*configuracionCanal).channelNumber = numeroCanal;
	(*configuracionCanal).enableInterruptOnConversionCompleted = false;
#if defined(FSL_FEATURE_ADC16_HAS_DIFF_MODE) && FSL_FEATURE_ADC16_HAS_DIFF_MODE
	(*configuracionCanal).enableDifferentialConversion = false;
#endif /* FSL_FEATURE_ADC16_HAS_DIFF_MODE */

	ADC16_SetChannelConfig(base, numeroCanalGrupo, configuracionCanal);
}

adv readAdc(adc* configuracionCanal, ADC_Type* base, ngadc numeroCanalGrupo)
{
	ADC16_SetChannelConfig(base, numeroCanalGrupo, configuracionCanal);

	while (0U == (kADC16_ChannelConversionDoneFlag & ADC16_GetChannelStatusFlags(base, numeroCanalGrupo)));

	return ADC16_GetChannelConversionValue(base, numeroCanalGrupo);

}

void outputPwm(uint8_t dutyCyclePwm)
{
	TPM_UpdatePwmDutycycle(BOARD_TPM_BASEADDR, (tpm_chnl_t)BOARD_TPM_CHANNEL, kTPM_EdgeAlignedPwm, dutyCyclePwm);

}

void adctoPWM(SensorPwm* s1, SensorPwm* s2)         //Calibrar a tu conveniencia en las constantes
{
	dcv duty;

	if(objetoDetectado(s1) || objetoDetectado(s2)) //Si hay un obstaculo a cualquier altura, SALTAR!
	{
		duty = POSICION_DESTINO_CLICK_SERVO;       //Mover servomotor 45 grados
		GPIO_WritePinOutput(PTE, 21 , 1);          //Encender un led
	}

	else
	{
		duty = POSICION_INICIAL_SERVO;            //Mover servomotor a 0 grados
		GPIO_WritePinOutput(PTE , 21 , 0);        //Apagar el foco
	}

	dutyCyclePwm = duty;                         //Ciclo de trabajo entre el 0 y el 10%

}

boolean objetoDetectado(SensorPwm* s)
{
	if(s->valorAdc >= s->umbral)                //Hay un obstaculo
		return 1;
	else
		return 0;

}

void controlPersonaje(SensorPwm* s1, SensorPwm* s2)
{

	switch(curr_state)
	{

	case LEERADC:
		s1->valorAdc = readAdc(&s1->configCanalAdc, s1->base, s1->numeroGrupo);   //Sensor Nivel Tierra
		s2->valorAdc = readAdc(&s2->configCanalAdc, s2->base, s2->numeroGrupo);   //Sensor Nivel medio
		Next_state = CONVERTIR;
		//sprintf(buffer,"Valor ADC: %d\n", s->valorAdc);
		//PRINTF(buffer);
		break;

	case CONVERTIR:
		adctoPWM(s1, s2);                                                        //Verificar si hay obstaculos
		Next_state = PWMOUTPUT;
		break;

	case PWMOUTPUT:
		//sprintf(buffer,"Ciclo de Trabajo: %d\n", s->dutyCyclePwm);
		//PRINTF(buffer);
		outputPwm(dutyCyclePwm);                                                //Mover el servomotor
		Next_state = LEERADC;
		break;

	default:
		break;


	}

	curr_state = Next_state;

}




#endif /* ADCTOPWM_H_ */
