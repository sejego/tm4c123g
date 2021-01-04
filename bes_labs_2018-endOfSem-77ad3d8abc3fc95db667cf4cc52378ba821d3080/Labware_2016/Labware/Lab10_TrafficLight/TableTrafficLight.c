// ***** 0. Documentation Section *****
// TableTrafficLight.c for Lab 10
// Runs on LM4F120/TM4C123
// Index implementation of a Moore finite state machine to operate a traffic light.  
// Daniel Valvano, Jonathan Valvano
// January 15, 2016

// east/west red light connected to PB5
// east/west yellow light connected to PB4
// east/west green light connected to PB3
// north/south facing red light connected to PB2
// north/south facing yellow light connected to PB1
// north/south facing green light connected to PB0
// pedestrian detector connected to PE2 (1=pedestrian present)
// north/south car detector connected to PE1 (1=car present)
// east/west car detector connected to PE0 (1=car present)
// "walk" light connected to PF3 (built-in green LED)
// "don't walk" light connected to PF1 (built-in red LED)

// ***** 1. Pre-processor Directives Section *****
#include "TExaS.h"
#include "tm4c123gh6pm.h"

// ***** 2. Global Declarations Section *****

#define goW 0
#define waitW 1
#define goN 2
#define waitN 3
#define walk 4
#define flash1 5
#define flash2 6
#define flash3 7
#define flash4 8
#define waitPed 9


// FUNCTION PROTOTYPES: Each subroutine defined
void DisableInterrupts(void); // Disable interrupts
void EnableInterrupts(void);  // Enable interrupts

unsigned long S;
unsigned long Input;

void Port_Init(void){ volatile unsigned long delay;
	SYSCTL_RCGC2_R |= 0x32;      // 1)F B E
  delay = SYSCTL_RCGC2_R;      // 2) no need to unlock
	
  GPIO_PORTE_AMSEL_R &= ~0x07; // 3) disable analog function on PE2-0
  GPIO_PORTE_PCTL_R &= ~0x000000FF; // 4) enable regular GPIO ???
  GPIO_PORTE_DIR_R &= ~0x07;   // 5) inputs on PE2-0
  GPIO_PORTE_AFSEL_R &= ~0x07; // 6) regular function on PE2-0
  GPIO_PORTE_DEN_R |= 0x07;    // 7) enable digital on PE2-0
	
  GPIO_PORTB_AMSEL_R &= ~0x3F; // 3) disable analog function on PB5-0
  GPIO_PORTB_PCTL_R &= ~0x00FFFFFF; // 4) enable regular GPIO
  GPIO_PORTB_DIR_R |= 0x3F;    // 5) outputs on PB5-0
  GPIO_PORTB_AFSEL_R &= ~0x3F; // 6) regular function on PB5-0
  GPIO_PORTB_DEN_R |= 0x3F;    // 7) enable digital on PB5-0
	
	GPIO_PORTF_AMSEL_R &= ~0x0A; // 3) disable analog function on PF3, PF1
  GPIO_PORTF_PCTL_R &= ~0x00FFFFFF; // 4) enable regular GPIO
  GPIO_PORTF_DIR_R |= 0x0A;    // 5) outputs on PF3,PF1
  GPIO_PORTF_AFSEL_R &= ~0x0A; // 6) regular function on PB5-0
  GPIO_PORTF_DEN_R |= 0x0A;    // 7) enable digital on PB5-0
	
}

void SysTick_Init(void){
  NVIC_ST_CTRL_R = 0;               // disable SysTick during setup
  NVIC_ST_CTRL_R = 0x00000005;      // enable SysTick with core clock
}

void SysTick_Wait(unsigned long delay){
  NVIC_ST_RELOAD_R = delay-1;  // number of counts to wait
  NVIC_ST_CURRENT_R = 0;       // any value written to CURRENT clears
  while((NVIC_ST_CTRL_R&0x00010000)==0){ // wait for count flag
  }
}

void SysTick_Wait10ms(unsigned long delay){
  unsigned long i;
  for(i=0; i<delay; i++){
    SysTick_Wait(800000);  // wait 10ms
  }
}
#define SENSOR  (*((volatile unsigned long *)0x4002401C)) //tut ostorozhno
#define LIGHT   (*((volatile unsigned long *)0x400050FC)) //tut vse ok
#define W_LIGHT (*((volatile unsigned long *)0x40025028)) // 28 -> tolko PF 3, 1
	
#define NVIC_ST_CTRL_R      (*((volatile unsigned long *)0xE000E010))
#define NVIC_ST_RELOAD_R    (*((volatile unsigned long *)0xE000E014))
#define NVIC_ST_CURRENT_R   (*((volatile unsigned long *)0xE000E018))


struct State {
  unsigned long Out;
	unsigned long Out2;
  unsigned long Time; 
  unsigned long Next[8];					//tut ostorozno, eto 8 stadij kuda mozhno dalwe poiti
};
typedef const struct State STyp;

// according to the table of FSM composed on a paper
/*
STyp FSM[10]={
 {0x0C,0x02,70,{goW,goW,waitW,waitW,waitW,waitW,waitW,waitW}},
 {0x14,0x02,20,{goN,goN,goN,goN,walk,walk,walk,walk}}, 	//ostorozno walk 3 sprava
 {0x21,0x02,70,{goN,waitN,goN,waitN,waitN,waitN,waitN,waitN}},
 {0x22,0x02,20,{goW,goW,goW,goW,walk,walk,walk,goW}}, 	//ostorozhno WALK 4 sprava
 {0x24,0x08,70,{walk, flash1, flash1, flash1, flash1, flash1, walk, flash1}},
 {0x24,0x02,70,{goW,goW,goN,goW,goW,goN,walk,goW}},		//ostorozhno s walk
 {0x24,0x00,20,{flash2,flash2,flash2,flash2,flash2,flash2,flash2}},
 {0x24,0x02,20,{flash3,flash3,flash3,flash3,flash3,flash3,flash3}},
 {0x24,0x00,20,{flash4,flash4,flash4,flash4,flash4,flash4,flash4}},
 {0x24,0x02,20,{waitPed,waitPed,waitPed,waitPed,waitPed,waitPed,waitPed,waitPed}},
};

STyp FSM[10]={
 {0x0C,0x02,70,{goW,goW,waitW,waitW,waitW,waitW,waitW,waitW}},
 {0x14,0x02,20,{goN,goN,goN,goN,walk,walk,walk,goN}}, 	//ostorozno walk 3 sprava
 {0x21,0x02,70,{goN,waitN,goN,waitN,waitN,waitN,waitN,waitN}},
 {0x22,0x02,20,{goW,goW,goW,goW,walk,walk,walk,walk}}, 	//ostorozhno WALK 4 sprava
 {0x24,0x08,70,{walk,flash1,flash1,flash1,flash1,flash1,walk,flash1}},
 {0x24,0x02,20,{goW,goW,goN,goW,goW,goN,walk,goW}},		//ostorozhno s walk
 {0x24,0x00,20,{flash2,flash2,flash2,flash2,flash2,walk,flash2}},
 {0x24,0x02,20,{flash3,flash3,flash3,flash3,flash3,walk,flash3}},
 {0x24,0x00,20,{flash4,flash4,flash4,flash4,flash4,walk,flash4}},
 {0x24,0x02,20,{waitPed,waitPed,waitPed,waitPed,waitPed,waitPed,walk,waitPed}},
};*/

STyp FSM[10]={
 {0x0C,0x02,70,{goW,goW,waitW,waitW,waitW,waitW,waitW,waitW}},
 {0x14,0x02,20,{goN,goN,goN,goN,walk,walk,walk,goN}}, 	
 {0x21,0x02,70,{goN,waitN,goN,waitN,waitN,waitN,waitN,waitN}},
 {0x22,0x02,20,{goW,goW,goW,goW,walk,walk,walk,walk}}, 	
 {0x24,0x08,70,{walk, flash1, flash1, flash1, walk, flash1, flash1, flash1}},
 {0x24,0x00,70,{flash2,flash2,flash2,flash2,flash2,flash2,flash2,flash2}},		
 {0x24,0x02,20,{flash3,flash3,flash3,flash3,flash3,flash3,flash3}},
 {0x24,0x00,20,{flash4,flash4,flash4,flash4,flash4,flash4,flash4}},
 {0x24,0x02,20,{waitPed,waitPed,waitPed,waitPed,waitPed,waitPed,waitPed}},
 {0x24,0x02,20,{walk,goW,goN,goW,walk,goN,goW,goW}},
};
// ***** 3. Subroutines Section *****

int main(void){ 
  TExaS_Init(SW_PIN_PE210, LED_PIN_PB543210,ScopeOff); // activate grader and set system clock to 80 MHz
	//PLL_Init();
  SysTick_Init();
	Port_Init();
  S = goW;
	
  EnableInterrupts();
  while(1){
		LIGHT = FSM[S].Out;  // set lights
		W_LIGHT = FSM[S].Out2; 	//set walk lights
    SysTick_Wait10ms(FSM[S].Time);
    Input = SENSOR;     // read sensors
    S = FSM[S].Next[Input]; 
     
  }
}

