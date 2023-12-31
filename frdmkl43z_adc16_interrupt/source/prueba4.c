/*
 * Copyright (c) 2013 - 2015, Freescale Semiconductor, Inc.
 * Copyright 2016-2018 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "fsl_debug_console.h"
#include "board.h"
#include "fsl_adc16.h"

#include "pin_mux.h"
#include "clock_config.h"
#include "SD2_board.h"
#include "key.h"
/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define ADC16_BASE          ADC0
#define DEMO_ADC16_CHANNEL_GROUP 0U
#define DEMO_ADC16_USER_CHANNEL  23U /* PTE30, ADC0_SE23 */

#define ADC16_IRQn             ADC0_IRQn

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/*******************************************************************************
 * Variables
 ******************************************************************************/
const uint32_t g_Adc16_12bitFullRange = 4096U;
const float k_instr = 1.0f;
volatile uint32_t last_conversion_value;
volatile uint32_t tara;
volatile bool new_measurement_available = false;
volatile uint32_t timer = 0;
volatile uint32_t time_of_last_measurement = 0;

/*******************************************************************************
 * Code
 ******************************************************************************/

void ADC0_IRQHandler(void)
{
    /* Read conversion result to clear the conversion completed flag. */
    uint32_t g_Adc16ConversionValue = ADC16_GetChannelConversionValue(ADC16_BASE, DEMO_ADC16_CHANNEL_GROUP);
    last_conversion_value = g_Adc16ConversionValue;
    new_measurement_available = true;
}

int main(void)
{
    adc16_config_t adc16ConfigStruct;
    adc16_channel_config_t adc16ChannelConfigStruct;

    BOARD_InitPins();
    BOARD_BootClockRUN();
    BOARD_InitDebugConsole();
    EnableIRQ(ADC16_IRQn);

    board_init();
    key_init();

    SysTick_Config(SystemCoreClock / 1000U);

    PRINTF("\r\nBALANZA DIGITAL - Made by Rafagani and Co.\r\n");

    ADC16_GetDefaultConfig(&adc16ConfigStruct);

    adc16ConfigStruct.referenceVoltageSource = kADC16_ReferenceVoltageSourceValt;
    adc16ConfigStruct.clockSource = kADC16_ClockSourceAlt1; // BUSCLK / 2
    adc16ConfigStruct.enableAsynchronousClock = true;
    adc16ConfigStruct.clockDivider = kADC16_ClockDivider8;
    adc16ConfigStruct.resolution = kADC16_ResolutionSE12Bit;
    adc16ConfigStruct.longSampleMode = kADC16_LongSampleCycle24;
    adc16ConfigStruct.enableHighSpeed = false;
    adc16ConfigStruct.enableLowPower = false;
    adc16ConfigStruct.enableContinuousConversion = true;

    ADC16_Init(ADC16_BASE, &adc16ConfigStruct);
    ADC16_EnableHardwareTrigger(ADC16_BASE, false); /* Make sure the software trigger is used. */

    adc16ChannelConfigStruct.channelNumber                        = DEMO_ADC16_USER_CHANNEL;
    adc16ChannelConfigStruct.enableInterruptOnConversionCompleted = true; /* Enable the interrupt. */

	// Hardware Average Enable
	ADC0->SC3 |= 1 << 2; // Hardware average function enabled.

	// Hardware Average Select
	ADC0->SC3 |= 0B11; // 32 samples averaged

    ADC16_SetChannelConfig(ADC16_BASE, DEMO_ADC16_CHANNEL_GROUP, &adc16ChannelConfigStruct);

    while (1){
        if(key_getPressEv(BOARD_SW_ID_3))
        {
        	DisableIRQ(ADC16_IRQn);

            tara = last_conversion_value;
            PRINTF("Se tara la balanza. ADC mide: %d\r\n", tara);

            EnableIRQ(ADC16_IRQn);
        }
        if(new_measurement_available) {
            DisableIRQ(ADC16_IRQn);

            uint32_t diff = timer - time_of_last_measurement;
            time_of_last_measurement = timer;

            PRINTF("Peso: %d Tiempo de Conversion: %d ms\r\n",(int)((last_conversion_value - tara) * k_instr), diff);
            new_measurement_available = false;
            
            EnableIRQ(ADC16_IRQn);
        }
    }
}

void SysTick_Handler(void)
{
    timer++;
	key_periodicTask1ms();
}