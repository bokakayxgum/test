// ADC.h
// Runs on TM4C123
// Provide functions that initialize ADC0 SS3 to be triggered by
// software and trigger a conversion, wait for it to finish,
// and return the result.
// Code from ADCSWTrigger.h copied and modified
// Chui Mun Chan, Daniel Valvano
// January 30, 2017

/* This example accompanies the book
   "Embedded Systems: Real Time Interfacing to Arm Cortex M Microcontrollers",
   ISBN: 978-1463590154, Jonathan Valvano, copyright (c) 2015

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


// --------------ADC_Init-------------------------
// Take one sample from specified channel
// Input: Channel Number
// Output: one sample
void ADC_Init(uint8_t channelNum);
  
// --------------ADC_GetOneInit-------------------------
// Take one sample from specified channel
// Input: Channel Number
// Output: one sample
uint32_t ADC_GetOne(void);

// --------------ADC_Init------------------------
// Input: Channel Number, Sampling Rate , buffer pointer, number of Samples
// Output: None
void ADC_Collect_Init(unsigned int channelNum,unsigned int period);

//-----------------Reset Status---------------
void ADC_Reset(void);


// --------------ADC_Collect------------------------
// Get multiple x number of samples from specified channel
// Steps--
// Step 1: Run the calculation for period
// Step 2: ADC_Init
// Input: Channel Number, Sampling Rate, Number of Samples, Buffer Pointer
// Output: x number of samples
// void ADC_GetMany (unsigned int numberOfSamples);
//int ADC_Collect(unsigned int channelNum, unsigned int fs, unsigned short buffer[], unsigned int numberOfSamples);

// --------------ADC_Status------------------
// Return the status of ADC_GetMany
// Input: None
// Output: 0 = Done, 1 = Not Done
//int ADC_Status(void);

// --------------ADC_In------------------------
// Initialize the ADC_GetOne for channel 0 and run GetOne
// Input: none
// Output: 12-bit result of ADC conversion
uint32_t ADC_In(void);

// --------------ADC_Collect-------------------------------
//Input: Channel Number, Sampling Rate, and task
//Output: None
void ADC_Collect(unsigned int channelNum, unsigned int fs,void(*task)(unsigned long));

