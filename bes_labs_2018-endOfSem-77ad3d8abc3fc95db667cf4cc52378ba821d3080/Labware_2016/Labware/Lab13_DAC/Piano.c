// Piano.c
// Runs on LM4F120 or TM4C123, 
// edX lab 13 
// There are four keys in the piano
// Daniel Valvano
// December 29, 2014

// Port E bits 3-0 have 4 piano keys

#include "Piano.h"
#include "..//tm4c123gh6pm.h"


// **************Piano_Init*********************
// Initialize piano key inputs
// Input: none
// Output: none
void Piano_Init(void){unsigned volatile delay; 
  SYSCTL_RCGC2_R |= 0x00000010;			// PORT E clock set
	delay=SYSCTL_RCGC2_R;							// set delay
	GPIO_PORTE_AMSEL_R &=~0x0F;				// no analog
	GPIO_PORTE_PCTL_R &= ~0x00000000;	//regular function
	//GPIO_PORTA_CR_R = 0x0C;
	GPIO_PORTE_DIR_R |= 0x00;					// make PE0-3 in
	//GPIO_PORTE_DR8R_R |= 0x0F;				// 8mA current on out ???
	GPIO_PORTE_AFSEL_R &= ~0x0F;			// no alt function
	GPIO_PORTE_DEN_R |= 0x0F;					// enable digital i/o
}
// **************Piano_In*********************
// Input from piano key inputs
// Input: none 
// Output: 0 to 15 depending on keys
// 0x01 is key 0 pressed, 0x02 is key 1 pressed,
// 0x04 is key 2 pressed, 0x08 is key 3 pressed
unsigned long Piano_In(void){
  unsigned long Out;
	Out = GPIO_PORTE_DATA_R;
	Out &= 0x0F;
  return Out; // out da key
}
