// TuningFork.c Lab 12
// Runs on LM4F120/TM4C123
// Use SysTick interrupts to create a squarewave at 440Hz.  
// There is a positive logic switch connected to PA3, PB3, or PE3.
// There is an output on PA2, PB2, or PE2. The output is 
//   connected to headphones through a 1k resistor.
// The volume-limiting resistor can be any value from 680 to 2000 ohms
// The tone is initially off, when the switch goes from
// not touched to touched, the tone toggles on/off.
//                   |---------|               |---------|     
// Switch   ---------|         |---------------|         |------
//
//                    |-| |-| |-| |-| |-| |-| |-|
// Tone     ----------| |-| |-| |-| |-| |-| |-| |---------------
//
// Daniel Valvano, Jonathan Valvano
// January 15, 2016

/* This example accompanies the book
   "Embedded Systems: Introduction to ARM Cortex M Microcontrollers",
   ISBN: 978-1469998749, Jonathan Valvano, copyright (c) 2015

 Copyright 2016 by Jonathan W. Valvano, valvano@mail.utexas.edu
    You may use, edit, run or distribute this file
    as long as the above copyright notice remains
 THIS SOFTWARE IS PROVIDED "AS IS".  NO WARRANTIES, WHETHER EXPRESS, IMPLIED
 OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF
 MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE.
 VALVANO SHALL NOT, IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL,
 OR CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
 For more information about my classes, my research, and my books, see
 http://users.ece.utexas.edu/~valvano/
 */


#include "TExaS.h"
#include "..//tm4c123gh6pm.h"
//#define SYSCTL_RCGC2_R          (*((volatile unsigned long *)0x400FE108))

// basic functions defined at end of startup.s
void DisableInterrupts(void); // Disable interrupts
void EnableInterrupts(void);  // Enable interrupts
void WaitForInterrupt(void);  // low power mode
unsigned long ct = 0;
unsigned long Inct = 0;

// input from PA3, output from PA2, SysTick interrupts
void Sound_Init(void){ unsigned volatile delay;
	SYSCTL_RCGC2_R |= 0x00000001;			// PORT A clock set
	delay=SYSCTL_RCGC2_R;							// set delay
	GPIO_PORTA_AMSEL_R &=~0x20;				// no analog
	GPIO_PORTA_PCTL_R &= ~0x00000000;	//regular function
	//GPIO_PORTA_CR_R = 0x0C;
	GPIO_PORTA_DIR_R |= 0x04;					// make PA2 out PA3 in
	GPIO_PORTA_DR8R_R |= 0x04;				// 8mA current on out
	GPIO_PORTA_AFSEL_R &= ~0x0C;			// no alt function 0x0C
	GPIO_PORTA_DEN_R |= 0x0C;					// enable digital i/o
	NVIC_ST_CTRL_R = 0;								// disable SysTick for init
	NVIC_ST_RELOAD_R = 90908;				// 2.272 ms at 80 MHz		?? maybe me a mistake, 181818/90909
	NVIC_ST_CURRENT_R =0;
	NVIC_SYS_PRI3_R = NVIC_SYS_PRI3_R&0x00FFFFFF; // priority 0
	NVIC_ST_CTRL_R = 0x00000007;			// enable with core clock and interrupts???
	//EnableInterrupts();
}

// called at 880 Hz
void SysTick_Handler(void){
	if(ct == 1){
		GPIO_PORTA_DATA_R ^= 0x04;	//toggle PA2
	}
 //GPIO_PORTA_DATA_R ^= 0x20;
}

int main(void){// activate grader and set system clock to 80 MHz
	unsigned long wut;
	TExaS_Init(SW_PIN_PA3, HEADPHONE_PIN_PA2,ScopeOn); 
  Sound_Init();         
  EnableInterrupts();   // enable after all initialization are done
  while(1){   // main program is free to perform other tasks
    // do not use WaitForInterrupt() here, it may cause the TExaS to crash
		wut = GPIO_PORTA_DATA_R;
		wut &= 0x08;
		if(wut==0x08 && Inct == 0){ //08
			ct++;
			Inct++;
		}
		if(wut == 0x00 && Inct == 1)
			Inct++;
		if(wut == 0x08 && Inct == 2){ //0c
			ct=0;
			GPIO_PORTA_DATA_R &=~0x04;
			Inct++;
		}
		if(wut == 0x00 && Inct == 3)
			Inct=0;
  }
}
