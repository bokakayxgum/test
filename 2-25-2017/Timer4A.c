// Timer4A.c
// Runs on LM4F120/TM4C123
// Use Timer0A in periodic mode to request interrupts at a particular
// period.
// Daniel Valvano
// September 11, 2013

/* This example accompanies the book
   "Embedded Systems: Introduction to ARM Cortex M Microcontrollers"
   ISBN: 978-1469998749, Jonathan Valvano, copyright (c) 2015
   Volume 1, Program 9.8

  "Embedded Systems: Real Time Interfacing to ARM Cortex M Microcontrollers",
   ISBN: 978-1463590154, Jonathan Valvano, copyright (c) 2014
   Volume 2, Program 7.5, example 7.6

 Copyright 2015 by Jonathan W. Valvano, valvano@mail.utexas.edu
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
#include <stdint.h>
#include "tm4c123gh6pm.h"



void DisableInterrupts(void); // Disable interrupts
void EnableInterrupts(void);  // Enable interrupts
long StartCritical (void);    // previous I bit, disable interrupts
void EndCritical(long sr);    // restore I bit to previous value
void WaitForInterrupt(void);  // low power mode
void (*PeriodicTask2)(void);   // user function


uint32_t  COUNT = 0;

#define PD0       (*((volatile uint32_t *)0x40007004))
#define PD1       (*((volatile uint32_t *)0x40007008))

// ***************** Timer4A_Init ****************
//Vector Number: 86, IRQ: 70, Uses EN2
// Activate TIMER0 interrupts to run user task periodically
// Inputs:  task is a pointer to a user function
//          period in secs converted to units (1/clockfreq), 32 bits
//          at 80MHz bus speed, 1 cycle = 12.5ns
//Outputs: none
void Timer4A_Init(void(*task)(void), uint32_t period, unsigned long priority){long sr;
  period = ((period*1000000000)/12.5);        //convert seconds to cycles
  
  sr = StartCritical(); 
  SYSCTL_RCGCTIMER_R |= 0x10;   // 0) activate TIMER4
  PeriodicTask2 = task;          // user function
  TIMER4_CTL_R = 0x00000000;    // 1) disable TIMER0A during setup
  TIMER4_CFG_R = 0x00000000;    // 2) configure for 32-bit mode
  TIMER4_TAMR_R = 0x00000002;   // 3) configure for periodic mode, default down-count settings
//  TIMER4_TAILR_R = period-1;    // 4) reload value

  TIMER4_TAILR_R = 80000;    // 4) reload value

  TIMER4_TAPR_R = 0;            // 5) bus clock resolution
  TIMER4_ICR_R = 0x00000001;    // 6) clear TIMER0A timeout flag
  TIMER4_IMR_R = 0x00000001;    // 7) arm timeout interrupt
//  NVIC_PRI17_R = (NVIC_PRI17_R&0xFF00FFFF)|0x00200000; // 8) priority 
  NVIC_PRI17_R = (NVIC_PRI17_R&0xFF00FFFF)|(priority << 20); // 8) priority 
  // interrupts enabled in the main program after all devices initialized
// vector number 35, interrupt number 19
//  NVIC_EN0_R = 1<<19;           // 9) enable IRQ 19 in NVIC
  NVIC_EN2_R = 1<<(70-64);           // 9) enable IRQ 70 in NVIC
  TIMER4_CTL_R = 0x00000001;    // 10) enable TIMER0A

//  
//  SYSCTL_RCGCGPIO_R |= 0x08;        // 1) activate port D
//  while((SYSCTL_PRGPIO_R&0x08)==0){};   // allow time for clock to stabilize
//                                    // 2) no need to unlock PD3-0
//  GPIO_PORTD_AMSEL_R &= ~0x03;      // 3) disable analog functionality on PD0-1
//  GPIO_PORTD_PCTL_R &= ~0xFF;       // 4) GPIO
//  GPIO_PORTD_DIR_R |= 0x03;         // 5) make PD0-1 out
//  GPIO_PORTD_AFSEL_R &= ~0x03;      // 6) regular port function
//  GPIO_PORTD_DEN_R |= 0x03;         // 7) enable digital I/O on PD0-1

  EndCritical(sr);
  
  EnableInterrupts();
  
}

void Timer4A_Handler(void){
  TIMER4_ICR_R = TIMER_ICR_TATOCINT;// acknowledge timer0A timeout 
  
  COUNT++;
  (*PeriodicTask2)();                // execute user task

}

uint32_t Timer4A_ReturnCount(void){

  return COUNT;
  
}

void Timer4A_ResetCount(void){
  
  COUNT = 0;
  
}

// REturns the current time in 
//
unsigned long Timer4A_getTime(void){
  return 0;
}
