/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this list
 *   of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * o Neither the name of Freescale Semiconductor, Inc. nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "fsl_device_registers.h"
#include "stdio.h"

int button = 0;
int tick = 0;
int decay_one = 0;
int decay_two = 0;

unsigned short ADC0_read16b(void)
{
	ADC0_SC1A = 0x00000; //Write to SC1A to start conversion from ADC_0
	while(ADC0_SC2 & ADC_SC2_ADACT_MASK); // Conversion in progress
	while(!(ADC0_SC1A & ADC_SC1_COCO_MASK)); // Until conversion complete
	ADC0_SC1A = 0x1F; //disable module
	return ADC0_RA;
}

unsigned short ADC1_read16b(void)
{
	ADC1_SC1A = 0x00000; //Write to SC1A to start conversion from ADC_1
	while(ADC1_SC2 & ADC_SC2_ADACT_MASK); // Conversion in progress
	while(!(ADC1_SC1A & ADC_SC1_COCO_MASK)); // Until conversion complete
	ADC1_SC1A = 0x1F; //disable module
	return ADC1_RA;
}

int main(void)
{
	SIM_SCGC5 |= SIM_SCGC5_PORTA_MASK; /*Enable Port A Clock Gate Control*/
	SIM_SCGC5 |= SIM_SCGC5_PORTB_MASK; /*Enable Port B Clock Gate Control*/
	SIM_SCGC5 |= SIM_SCGC5_PORTC_MASK; /*Enable Port C Clock Gate Control*/
	SIM_SCGC5 |= SIM_SCGC5_PORTD_MASK; /*Enable Port D Clock Gate Control*/
	SIM_SCGC6 |= SIM_SCGC6_ADC0_MASK; /*Enable ADC0 Clock Gate Control*/
	SIM_SCGC3 |= SIM_SCGC3_ADC1_MASK; /*Enable ADC1 Clock Gate Control*/
	SIM_SCGC3 |= SIM_SCGC3_FTM3_MASK;  // FTM3 clock enable

	PORTA_GPCLR = 0x00020100; //enable port A pin 1
	PORTB_GPCLR = 0x040C0100; //enable port B pins 2-3 and 10
	PORTC_GPCLR = 0x05BF0100; //enable port C pins 0-5, 7-8, and 10
	PORTD_GPCLR = 0x00FF0100; //enable port D pins 0-7

	ADC0_CFG1 = 0x0C;  //set ADC to 16 bit and Bus Clock
	ADC0_SC1A = 0x1F;  //disable module

	ADC1_CFG1 = 0x0C;  //set ADC to 16 bit and Bus Clock
	ADC1_SC1A = 0x1F;  //disable module

	PORTC_PCR10 = 0x300; // Port C Pin 10 as FTM3_CH6 (ALT3)
	FTM3_MODE = 0x5; // Enable FTM3
	FTM3_MOD = 0xFFFF;
	FTM3_SC = 0x0D; // System clock / 32

	PORTA_PCR1 = 0x90100; //set port A pin 1 to interrupt on rising edge
	PORTA_ISFR = 0xFFFFFFFF; //clear pin A interrupt flags

	GPIOA_PDDR = 0x0;
	GPIOB_PDDR = 0x0400; //set Port B pins 2-3 to input and 10 to output
	GPIOC_PDDR = 0x1BF; //set Port C pins 0-5 and 7-10 to output
	GPIOD_PDDR = 0xFF;  //set Port D pins 0-7 to output

	NVIC_EnableIRQ(PORTA_IRQn); //unmasked port A pins for interrupts

	//game loop
	while(1){
		//press button to start game

		if(button == 1){
			button = 0;

			while(1){

				int play_one = ADC0_read16b();  //read photoresistors input
				int play_two = ADC1_read16b();

				tick++;					//note: tie always mean player one wins
				if(tick == 0x2FFFF){
					if(play_one > 0x0160){	//fill led if player 1 led is bright enough
						GPIOC_PDOR = (GPIOC_PDOR << 1) + 1;
						GPIOC_PDOR = ((GPIOC_PDOR & 0x40) << 1) | (GPIOC_PDOR & 0x1BF);
						decay_one = 0;
					}
					else{					//decay led if light not bright
						decay_one++;
						if(decay_one > 1){
							GPIOC_PDOR = GPIOC_PDOR >> 1;
							GPIOC_PDOR = ((GPIOC_PDOR & 0x40) >> 1) | (GPIOC_PDOR & 0x1BF);
							decay_one = 0;
						}
					}

					if(play_two > 0x0400){	//fill led if player 2 led is bright
						GPIOD_PDOR = (GPIOD_PDOR << 1) + 1;
						decay_two = 0;
					}
					else{		//decay led if light not bright
						decay_two++;
						if(decay_two > 1){
							GPIOD_PDOR = GPIOD_PDOR >> 1;
							decay_two = 0;
						}
					}

					tick = 0;
				}

				if(GPIOC_PDOR == 0x1BF){
					//Player one wins
					if(button == 1)
						button = 0;
					break;
				}
				else if(GPIOD_PDOR == 0xFF){
					//Player two wins
					if(button == 1)
						button = 0;
					break;
				}
			}
		}
	}
	/* Never leave main */
	return 0;
}

void PORTA_IRQHandler(void){
	NVIC_ClearPendingIRQ(PORTA_IRQn); /* Clear pending interrupts */

	button = 1;
	tick = 0;
	decay_one = 0;
	decay_two = 0;
	GPIOC_PDOR = 0;
	GPIOD_PDOR = 0;
	for(int i = 0; i < 0x2FF; i++){
		FTM3_C6SC = 0x1C; // Output-compare; Set output
		FTM3_C6V = FTM3_CNT + 466; // 700 us
		while(!(FTM3_C6SC & 0x80));
		FTM3_C6SC &= ~(1 << 7);
		FTM3_C6SC = 0x18; // Output-compare; Clear output
		FTM3_C6V = FTM3_CNT + 200; // 300 us
		while(!(FTM3_C6SC & 0x80));
		FTM3_C6SC &= ~(1 << 7);
	}

	PORTA_ISFR = 0xFFFFFFFF; //clear pin A interrupt flags
}


