//Interpreter.C
//Description: Processess user input to perform various functions
//Input: string pointer from UART
//Output: none
//Alana Wilcoxson
//Febuary 4, 2017

#include  <stdlib.h>
#include  <stdint.h>
#include  <string.h>

#include "UART.h"
#include "OS.h"
#include "NumIn.h"
//#include "ST7735.h"
#include "Timer4A.h"
//#include "ADC.h"

void DisableInterrupts(void); // Disable interrupts
void EnableInterrupts(void);  // Enable interrupts
long StartCritical (void);    // previous I bit, disable interrupts
void EndCritical(long sr);    // restore I bit to previous value
void WaitForInterrupt(void);  // low power mode

// -----------|| GLOBALS ||-------------
#define DEBUG 0     //flag to run debugging functions; set to 1 to run debug functions
#define SPACE 32

char UserIn[20];    //input string from UART
Sema4Type userInFifo;   //1 means "Ready", 0 means "HasContents" 


char lcd[3] = "lcd";  //start of lcd command line
char clr[3] = "clr";  
char tlcd[4] = "tlcd";

char T[1] = "T";       //start of timer commands
char RC[2] = "RC";       //Reset COUNT in Timer4A handler
char GC[2] = "GC";      //get COUNT from Timer4A handler

char adc[3] = "adc";
char madc[4] = "madc";



//temp string for debug
char Command[] = "command";




//-------------------|| OutCRLF ||---------------------
// Output a CR,LF to UART to go to a new line
// Input: none
// Output: none
// Daniel Valvano
void OutCRLF(void){
  UART_OutChar(CR);
  UART_OutChar(LF);
}



//------------|| LCD_Commands ||-----------
//Interprets which of the following commands to run
//Input: userIn string should be in following format
//       lcd device line input
//       i.e "lcd 1 2 input"
void LCD_Commands(char* userIn){
  int device = userIn[4] - 48;
  int line = userIn[6] - 48;
  
  char input[20] = {0};   //auto null termination

  //Index 8 should be beginning of user input string
  short j = 8;

  for(short i = 0; userIn[j] > 0; i++){  
    input[i] = userIn[j];
    j++;
   }
  
   
  //ST7735_SplitScreen(device, line, input, 0);
  
  //DEBUG
   //will output user input to UART
   if(DEBUG){
     OutCRLF();
     UART_OutString(input);
   }
  
 }



//------------|| CLR_Commands ||-----------
//Interprets which device to clear on screen
//Input: userIn string should be in following format
 //       clr device: 0 = both, 1 = top, 2 = bottom 
//       i.e "clr 1" --> clear top screen 
void CLR_Commands(char* userIn){
  int device = userIn[4] - 48;

  //ST7735_ClearScreen(device);
  
}



//------------|| T4_Commands ||-----------
//Interprets how to set Timer4A
//Input: userIn string should be in following format
//       T period priority; period is in seconds
//       i.e "T 5 0" --> Set timer4A to 5 secs with top priority 
void T4_Commands(char* userIn){
//  unsigned long period = userIn[2] - 48;
//  unsigned long priority = userIn[4] - 48;
  
  //OS_AddPeriodicThread(period, priority);
    
}

//------------|| ADC_Commands ||-----------
//Interprets to grab ONE sample from ADC
//Input: userIn string should be in following format
//       adc channel: 0 to 10
//			 i.e. "adc 6 " -->Collect 1 sample from channeL
//void ADC_Commands(char* userIn){
//	int channel = 0;
//	if (userIn[5] == NULL){ // check if the adc input is 2 digit or 1 digit adc
//	  channel = userIn[4] - 48;
//	}
//	else{
//		channel = (userIn[5]-48)+10;
//	}
//	
//	int sample = -1;
//  //ADC_GetOneInit(channel);
//  sample = ADC_GetOne();
//	OutCRLF();
//	UART_OutString("ADC Result = ");
//	UART_OutUDec(sample);
//}

//------------|| MADC_Commands ||-----------
//Interprets to grab samples from ADC
//Input: userIn string should be in following format
//       madc channel samplngFreq size: 
//			 i.e. "adc 1 64000 50" -->Collects 50 samples from channel 1 at 64kHz
//       max samplingFreq: 100 000 Hz
//       max samples: 100



//void MADC_Commands(char* userIn){
//	unsigned short buffer[100] = {0};
//	int channel = 0;
//	short freqStart = 7;
//	if (userIn[6] == SPACE){
//	  channel = userIn[5] - 48;
//	}
//	else{
//		channel = userIn[6]+10-48;
//		freqStart = 8;
//	}
//	
//	//Extract the user input of frequency
//	//Convert to integer
//	char freqStr[7] = {0};
//  short count = 0;
//	for(short i = freqStart; userIn[i] != SPACE; i++ ){
//   freqStr[count] = userIn[i];
//   count++;
//	}
//  freqStr[count] = 0;   //null termination
//	int freqInt = atoi(freqStr);
//	//--------------------------------------
//	//Extract the user input of number of Samples
//	//Convert to integer	
//	char sampleStr[3] = {0};
//  short sampleStart = count + freqStart;
//	count = 0;
//	for(short i = sampleStart; userIn[i] != NULL; i++){
//   sampleStr[count] = userIn[i];
//   count++;
//	}
//	sampleStr[sampleStart] = 0;
//	int sampleInt = atoi(sampleStr);
	//----------------------------------------------
//	int ADC_invalid = ADC_Collect(channel,freqInt,buffer,sampleInt); // 0 = invalid
//	
//	if (ADC_invalid == 1){
//  while(ADC_Status()){
//	 }
//	ADC_Reset(); 
//	for(short i =0; i < sampleInt; i++){
//		 uint32_t adc_value = (uint32_t)buffer[i];
//		 OutCRLF();
//	   UART_OutUDec(adc_value);
//	 }
// }
//	else{
//	OutCRLF();
//	UART_OutString("Invalid input");
//	}
//}

//-------------------|| Interpreter_Run ||---------------------
// Processess user input to perform various functions
// Input: string pointer from UART
// Output: none
void Interpreter_Run(void){
  
  //OS_InitSemaphore(&userInFifo, 0);

while(1){   

    OS_Wait(&userInFifo);
  
    OutCRLF();
    UART_InString(UserIn,19);

      //check for numerical operations
      if(UserIn[0] >= 42 && UserIn[0] <= 47){
        UART_OutString(" = ");
        UART_OutUDec(NumIn(UserIn));
        OutCRLF();  
      } 
      
      //check for Timer4 commands
      else if(strncmp(T, UserIn, 1) == 0){
        T4_Commands(UserIn);
      }
      else if(strncmp(GC, UserIn, 2) == 0){
        OutCRLF();
        UART_OutString("COUNT = ");
        UART_OutUDec(OS_MsTime());
      }else if(strncmp(RC, UserIn, 2) == 0){
        OS_ClearMsTime();
        OutCRLF();
        UART_OutString("COUNT reset");
       }
      
      //check for ST7735 screen commands
      else if(strncmp(lcd, UserIn, 3) == 0){
        LCD_Commands(UserIn);
      }
      
      else if(strncmp(clr, UserIn, 3) == 0){
        CLR_Commands(UserIn);
      }
      
      else if(strncmp(tlcd, UserIn, 4) == 0){
        //ST7735_TestSplitScreen();
      }
      
      //check for ADC commands
			else if(strncmp(adc, UserIn, 3)== 0 ){
				//ADC_Commands(UserIn);
				OutCRLF();
			}
			
			//check for ADC many commands
			else if(strncmp(madc, UserIn, 4) == 0){
			  //MADC_Commands(UserIn);
				OutCRLF();
			}
      
      //Invalid command input
      else{
				OutCRLF();
        UART_OutString("Command not found");
      }

      OS_Signal(&userInFifo);      
      
    }
    
}
