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

#define UMBRAL_LDR 1500  //Variar esta constante segun el brillo de tu pantalla
#define POSICION_INICIAL_SERVO 8  //Angulo de 90 grados | variar estas contantes segun la altura o diseño de tu teclado
#define POSICION_DESTINO_CLICK_SERVO 4 //Angulo de 70 grados | variar estas contantes segun la altura o diseño de tu teclado

typedef enum
{
	LEERADC = 1,
	CONVERTIR,
	PWMOUTPUT
}Adcpwm;

typedef struct
{
	adv valorAdc;
	dcv dutyCyclePwm;
	Adcpwm curr_state;
	adc configCanalAdc;
	ADC_Type* base;
	ncadc numeroCanal;
	ngadc numeroGrupo;
}SensorPwm;

volatile dcv updatedDutycycle = 0;
char buffer[25]; //Variable para pruebas
Adcpwm Next_state;

void configPwm(void);
void outputPwm(dcv dutyCyclePwm);
void configAdc(adc* configuracionCanal, ADC_Type* base, ncadc numeroCanal, ngadc numeroGrupo);
adv readAdc(adc* configuracionCanal, ADC_Type* base, ngadc numeroCanalGrupo);
void adctopwm(SensorPwm* s);
void controlPersonaje(SensorPwm* s);
void initSensorPwm(SensorPwm* s, ADC_Type* base, ncadc numeroCanal, ngadc numeroGrupo);
boolean objetoDetectado(adv valorAdc);

void initSensorPwm(SensorPwm* s, ADC_Type* base, ncadc numeroCanal, ngadc numeroGrupo)
{
	s->dutyCyclePwm = 0;
	s->valorAdc = 0;
	s->curr_state = LEERADC;
	s->base = base;
	s->numeroCanal = numeroCanal;
	s->numeroGrupo = numeroGrupo;
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
		PRINTF("ADC16_DoAutoCalibration() Done.\r\n");
	}
	else
	{
		PRINTF("ADC16_DoAutoCalibration() Failed.\r\n");
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

void adctoPWM(SensorPwm* s) //Calibrar a tu conveniencia
{
	dcv duty;

	if(objetoDetectado(s->valorAdc))
	{
		duty = POSICION_DESTINO_CLICK_SERVO;
		GPIO_WritePinOutput(PTE, 21 , 1);
	}

	else
	{
		duty = POSICION_INICIAL_SERVO;
		GPIO_WritePinOutput(PTE , 21 , 0);
	}

	s->dutyCyclePwm = duty;

}

boolean objetoDetectado(adv valorAdc)
{
	if(valorAdc >= UMBRAL_LDR)
		return 1;
	else
		return 0;

}

void controlPersonaje(SensorPwm* s)
{

	switch(s->curr_state)
	{

	case LEERADC:
		s->valorAdc = readAdc(&s->configCanalAdc, s->base, s->numeroGrupo);
		Next_state = CONVERTIR;
		//sprintf(buffer,"Valor ADC: %d\n", s->valorAdc);
		//PRINTF(buffer);
		break;

	case CONVERTIR:
		adctoPWM(s);
		Next_state = PWMOUTPUT;
		break;

	case PWMOUTPUT:
		//sprintf(buffer,"Ciclo de Trabajo: %d\n", s->dutyCyclePwm);
		//PRINTF(buffer);
		outputPwm(s->dutyCyclePwm);
		Next_state = LEERADC;
		break;

	default:
		break;


	}

	s->curr_state = Next_state;

}




#endif /* ADCTOPWM_H_ */
