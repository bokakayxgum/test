// os.c
// Runs on LM4F120/TM4C123
// A very simple real time operating system with minimal features.
// Daniel Valvano
// January 29, 2015

/* This example accompanies the book
   "Embedded Systems: Real Time Interfacing to ARM Cortex M Microcontrollers",
   ISBN: 978-1463590154, Jonathan Valvano, copyright (c) 2015

   Programs 4.4 through 4.12, section 4.2

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
#include "os.h"
#include "PLL.h"
#include "Timer4A.h"
#include "Timer0A.h"
#include "UART.h"
#include "ST7735.h"
#include "tm4c123gh6pm.h"

#define NVIC_ST_CTRL_R          (*((volatile uint32_t *)0xE000E010))
#define NVIC_ST_CTRL_CLK_SRC    0x00000004  // Clock Source
#define NVIC_ST_CTRL_INTEN      0x00000002  // Interrupt enable
#define NVIC_ST_CTRL_ENABLE     0x00000001  // Counter mode
#define NVIC_ST_RELOAD_R        (*((volatile uint32_t *)0xE000E014))
#define NVIC_ST_CURRENT_R       (*((volatile uint32_t *)0xE000E018))
#define NVIC_INT_CTRL_R         (*((volatile uint32_t *)0xE000ED04))
#define NVIC_INT_CTRL_PENDSTSET 0x04000000  // Set pending SysTick interrupt
#define NVIC_SYS_PRI3_R         (*((volatile uint32_t *)0xE000ED20))  // Sys. Handlers 12 to 15 Priority

// function definitions in osasm.s
void OS_DisableInterrupts(void); // Disable interrupts
void OS_EnableInterrupts(void);  // Enable interrupts
int32_t StartCritical(void);
void EndCritical(int32_t primask);
void StartOS(void);

#define STACKSIZE   100      // number of 32-bit words in stack
#define MAXTHREADS  20       // maximum number of threads

struct tcb{
  int32_t *sp;          // pointer to stack (valid for threads not running
  struct tcb *next;     // linked-list pointer
	int32_t id;           //
  int32_t sleepRate;    //
  int32_t priority;     //Unused in round-robin
  int32_t blockedState; //If waiting for an event
  int32_t index;        //stores the tcb's position in the tcbs
	unsigned long startTime;    //start time for sleep
//  int32_t flagged;      //might not use
};
typedef struct tcb tcbType;
tcbType tcbs[MAXTHREADS];
tcbType *RunPt;
tcbType *LastPt;
tcbType *FlaggedPt;
int32_t Stacks[MAXTHREADS][STACKSIZE];

int8_t  flaggedThread;     //flag a thread for the system to watch for
int8_t  numOfThreads = 0;
int8_t  hasThreads = 0;
int8_t  deadThreads = 0;
int32_t DeadPt = 0;                     //Pointer to dead
int8_t  threadCemetary[MAXTHREADS];     //array of indexes for dead threads

unsigned long  MAILBOX;
// ******** Schedule ***********
void Scheduler (void){
  RunPt = RunPt->next;
	while ((RunPt -> sleepRate) || (RunPt ->blockedState)){
		if (RunPt -> sleepRate > 0){
			//
			//Awake, if (current time - start time ) > sleep time
			if((Timer0A_GetSystemTime() - RunPt -> startTime) > RunPt -> sleepRate) {RunPt -> sleepRate = 0;}
			//sleepingTime = sleepingTime - (current - start)
			else {RunPt -> sleepRate = RunPt -> sleepRate - (Timer0A_GetSystemTime() - RunPt -> startTime);} 
		}
	  RunPt = RunPt -> next; //find one not sleeping and not blocked
	}
	
}

// ******** OS_Init ************
// initialize operating system, disable interrupts until OS_Launch
// initialize OS controlled I/O: systick, 50 MHz PLL
// input:  none
// output: none
void OS_Init(void){
  OS_DisableInterrupts();
  PLL_Init();                 // set processor clock to 80 MHz
	UART_Init();
	ST7735_InitScreens();
	Timer0A_Init(80000);
  NVIC_ST_CTRL_R = 0;         // disable SysTick during setup
  NVIC_ST_CURRENT_R = 0;      // any write to current clears it
  NVIC_SYS_PRI3_R =(NVIC_SYS_PRI3_R&0x00FFFFFF)|0xE0000000; // priority 7
	
	OS_Fifo_Init(128);
	OS_MailBox_Init();
}

void SetInitialStack(int i){
  tcbs[i].sp = &Stacks[i][STACKSIZE-16]; // thread stack pointer
  Stacks[i][STACKSIZE-1] = 0x01000000;   // thumb bit
  Stacks[i][STACKSIZE-3] = 0x14141414;   // R14
  Stacks[i][STACKSIZE-4] = 0x12121212;   // R12
  Stacks[i][STACKSIZE-5] = 0x03030303;   // R3
  Stacks[i][STACKSIZE-6] = 0x02020202;   // R2
  Stacks[i][STACKSIZE-7] = 0x01010101;   // R1
  Stacks[i][STACKSIZE-8] = 0x00000000;   // R0
  Stacks[i][STACKSIZE-9] = 0x11111111;   // R11
  Stacks[i][STACKSIZE-10] = 0x10101010;  // R10
  Stacks[i][STACKSIZE-11] = 0x09090909;  // R9
  Stacks[i][STACKSIZE-12] = 0x08080808;  // R8
  Stacks[i][STACKSIZE-13] = 0x07070707;  // R7
  Stacks[i][STACKSIZE-14] = 0x06060606;  // R6
  Stacks[i][STACKSIZE-15] = 0x05050505;  // R5
  Stacks[i][STACKSIZE-16] = 0x04040404;  // R4
}

//******** OS_AddThread *************** 
// add a foregound thread to the scheduler
// Inputs: pointer to a void/void foreground task
//         number of bytes allocated for its stack
//         priority, 0 is highest, 5 is the lowest
// Outputs: 1 if successful, 0 if this thread can not be added
// stack size must be divisable by 8 (aligned to double word boundary)
// In Lab 2, you can ignore both the stackSize and priority fields
// In Lab 3, you can ignore the stackSize fields
int OS_AddThread(void(*task)(void), unsigned long stackSize, unsigned long priority){
  int32_t status;

  //Check that the max number of threads hasn't already been added
  if(numOfThreads >= MAXTHREADS){ return 0; }


  //Adds a new thread to tcbs or replaces a dead thread in the cemetary
  status = StartCritical();
  
  //if there are dead threads, add new thread to dead thread's spot
  if(deadThreads){  
    OS_AddThreadToDead(task, stackSize, priority);
  } 
  
  //There are no current dead thread spots available
  else {
    
      //If at least one tcb, link previous tcb to incoming tcb
      if(hasThreads){
        tcbs[numOfThreads-1].next = &tcbs[numOfThreads]; // set previous thread to point to new thread

      //****************************** MIGHT NEED TO CHANGE **********************************************//
        tcbs[numOfThreads].next = &tcbs[0];             //each latest thread points to the first one
        LastPt = &tcbs[numOfThreads];                   //LastPt gets pointer to last active thread in linked list
                
      } 
      
      //No existing threads in link
      else {
        tcbs[numOfThreads].next = &tcbs[0];
        RunPt = &tcbs[0];       // thread 0 will run first
        LastPt = &tcbs[0];      // The first thread is the last thread in a 1 thread linked-list
      }
          
      tcbs[numOfThreads].priority = priority;
      
      //Set index
      tcbs[numOfThreads].index = numOfThreads;
      
      //set ID to active
      tcbs[numOfThreads].id = 1;
      
      SetInitialStack(numOfThreads); 
      Stacks[numOfThreads][STACKSIZE-2] = (int32_t)(task); // PC
      
      //See if system needs to watch for last added thread
      if(flaggedThread){
        FlaggedPt = &tcbs[numOfThreads];
      }
      
      
      numOfThreads++;
      hasThreads = 1;
            
      
    }

    EndCritical(status);
    return 1;               // successful
    
}

// ******** OS_InitSemaphore ************
// initialize semaphore 
// input:  pointer to a semaphore
// output: none
void OS_InitSemaphore(Sema4Type *semaPt, long value){
  
  semaPt->Value = value;
  
}

// ******** OS_Wait ************
// decrement semaphore 
// Lab2 spinlock
// Lab3 block if less than zero
// input:  pointer to a counting semaphore
// output: none
void OS_Wait(Sema4Type *semaPt){
  
  OS_DisableInterrupts(); 
 
  //Interrupts are allowed while waiting on the value to go up
  while(semaPt->Value == 0){
    OS_EnableInterrupts(); 
    OS_DisableInterrupts(); 
  } 
 
  semaPt->Value = semaPt->Value - 1;
  
  OS_EnableInterrupts();

}  

// ******** OS_Signal ************
// increment semaphore 
// Lab2 spinlock
// Lab3 wakeup blocked thread if appropriate 
// input:  pointer to a counting semaphore
// output: none
void OS_Signal(Sema4Type *semaPt){

  OS_DisableInterrupts(); 

  semaPt->Value = semaPt->Value + 1;
  
  OS_EnableInterrupts();   

}  

// ******** OS_bWait ************
// Lab2 spinlock, set to 0
// Lab3 block if less than zero
// input:  pointer to a binary semaphore
// output: none
void OS_bWait(Sema4Type *semaPt){
   
  OS_DisableInterrupts(); 
 
  while(semaPt->Value == 0){
    OS_EnableInterrupts(); // <- interrupts can occur here 
    OS_DisableInterrupts(); 
  } 
 
  semaPt->Value = 0;
  
  OS_EnableInterrupts();


}



// ******** OS_bSignal ************
// Lab2 spinlock, set to 1
// Lab3 wakeup blocked thread if appropriate 
// input:  pointer to a binary semaphore
// output: none
void OS_bSignal(Sema4Type *semaPt){
  
  OS_DisableInterrupts(); 

  semaPt->Value = 1;
  
  OS_EnableInterrupts(); 

  
}


// ******** OS_Kill ************
// kill the currently running thread, release its TCB and stack
// input:  none
// output: none
void OS_Kill(void){
  
  //grab RunPt safely and disable current thread
  OS_DisableInterrupts();
  tcbType *SavePt = RunPt;  
  RunPt->id = 0;
  OS_EnableInterrupts();
  // RunPt now safe to change and current thread is inactive
  
  
  //Save the dying thread's tcbs index in the thread cemetary
  threadCemetary[deadThreads] = SavePt->index;
  deadThreads++;
  
  //Tell the living threads to let the dead thread go
  for(int i = 0; i < numOfThreads; i++){
    if(tcbs[i].next == SavePt){
      tcbs[i].next = SavePt->next;      //the thread that pointed to this thread now points to the thread this thread pointed to
    }
  }
 
  //update LastPt to a new thread if this dead thread is the LastPt
  if(SavePt == LastPt){
    for(int i = numOfThreads-1; 0 < i; i--){
      //Look for latest active thread to become the LastPt
      //Shouldn't have to search long for an active thread
      if(tcbs[i].id){
        LastPt = &tcbs[i];
      }
    }      
  }
  
  //Should the stack pt to a dummy function that initiates a context switch?
  
  //Should OS_Kill initiate a context switch?
  OS_Suspend();
  
  //This should never run, but if it does, will prevent a hard faulr
  while(1){}
  
  
}

//******** OS_AddThreadToDead *************** 
// add a foregound thread to a dead TCB then ressurect it
// Inputs: pointer to a void/void foreground task
//         number of bytes allocated for its stack
//         priority, 0 is highest, 5 is the lowest
// Outputs: 1 if successful, 0 if this thread can not be added
// stack size must be divisable by 8 (aligned to double word boundary)
void OS_AddThreadToDead(void(*task)(void), unsigned long stackSize, unsigned long priority){
  
  //Dead thread's PC points to new task
  Stacks[threadCemetary[deadThreads-1]][STACKSIZE-2] = (int32_t)(task); // New task re-writes old PC
  
  //The LastPt thread's next thread becomes this thread's next thread
  tcbs[threadCemetary[deadThreads-1]].next = LastPt->next;
  
  //Latest active thread now points to this thread
  LastPt->next = &tcbs[threadCemetary[deadThreads-1]];
  
  deadThreads--;
  
  //See if system needs to watch for last added thread
  if(flaggedThread){
     FlaggedPt = &tcbs[threadCemetary[deadThreads-1]];
  }
  
}

//******** OS_Id *************** 
// returns the thread ID for the currently running thread
// Inputs: none
// Outputs: Thread ID, number greater than zero 
unsigned long OS_Id(void){
  return RunPt->id;
}

//******** OS_AddPeriodicThread *************** 
// add a background periodic task
// typically this function receives the highest priority
// Inputs: pointer to a void/void background function
//         period given in system time units (12.5ns)
//         priority 0 is the highest, 5 is the lowest
// Outputs: 1 if successful, 0 if this thread can not be added
// You are free to select the time resolution for this function
// It is assumed that the user task will run to completion and return
// This task can not spin, block, loop, sleep, or kill
// This task can call OS_Signal  OS_bSignal	 OS_AddThread
// This task does not have a Thread ID
// In lab 2, this command will be called 0 or 1 times
// In lab 2, the priority field can be ignored
// In lab 3, this command will be called 0 1 or 2 times
// In lab 3, there will be up to four background threads, and this priority field 
//           determines the relative priority of these four threads
//int OS_AddPeriodicThread(void(*task)(void), 
//   unsigned long period, unsigned long priority);
int OS_AddPeriodicThread (void(*task)(void), unsigned long period, unsigned long priority){
  Timer4A_Init(task, period, priority);  
  return 1;
}

// ******** OS_Sleep ************
// place this thread into a dormant state
// input:  number of msec to sleep
// output: none
// You are free to select the time resolution for this function
// OS_Sleep(0) implements cooperative multitasking
void OS_Sleep(unsigned long sleepTime){
  RunPt->sleepRate = sleepTime;
	RunPt->startTime = Timer0A_GetSystemTime();
	OS_Suspend();
}

// ******** OS_Suspend ************
// suspend execution of currently running thread
// scheduler will choose another thread to execute
// Can be used to implement cooperative multitasking 
// Same function as OS_Sleep(0)
// input:  none
// output: none
void OS_Suspend(void){

  NVIC_ST_CURRENT_R = 0; // reset counter
  NVIC_INT_CTRL_R = 0x04000000; // trigger SysTick
}

// ******** OS_Fifo_Init ************
// Initialize the Fifo to be empty
// Inputs: size
// Outputs: none 
// In Lab 2, you can ignore the size field
// In Lab 3, you should implement the user-defined fifo size
// In Lab 3, you can put whatever restrictions you want on size
//    e.g., 4 to 64 elements
//    e.g., must be a power of 2,4,8,16,32,64,128

#define FIFOSIZE 32 //can be any size
uint32_t volatile PutI; //index of where to put next
uint32_t volatile GetI; //index of where to get next
uint32_t static Fifo[FIFOSIZE];
Sema4Type CurrentSize; //0 means FIFO empty, FIFOSIZE means full
uint32_t LostData;
void OS_Fifo_Init(unsigned long size){
  PutI = GetI = 0; // Empty
  OS_InitSemaphore(&CurrentSize,0);
	LostData = 0;
}

// ******** OS_Fifo_Put ************
// Enter one data sample into the Fifo
// Called from the background, so no waiting 
// Inputs:  data
// Outputs: true if data is properly saved,
//          false if data not saved, because it was full
// Since this is called by interrupt handlers 
//  this function can not disable or enable interrupts
int OS_Fifo_Put(unsigned long data){
  if (CurrentSize.Value == FIFOSIZE){
	  LostData++;
		return 0; //full
	}
	else{
	  Fifo[PutI] = data; //Put
		PutI = (PutI+1)%FIFOSIZE;
		OS_Signal(&CurrentSize);
		return 1; //success
 	}
}

// ******** OS_Fifo_Get ************
// Remove one data sample from the Fifo
// Called in foreground, will spin/block if empty
// Inputs:  none
// Outputs: data 
unsigned long OS_Fifo_Get(void){uint32_t data;
  OS_Wait(&CurrentSize);    //block if empty
	data = Fifo[GetI]; 		    //get
	GetI = (GetI+1)%FIFOSIZE; //place to get next
	return data;
}

// ******** OS_Fifo_Size ************
// Check the status of the Fifo
// Inputs: none
// Outputs: returns the number of elements in the Fifo
//          greater than zero if a call to OS_Fifo_Get will return right away
//          zero or less than zero if the Fifo is empty 
//          zero or less than zero if a call to OS_Fifo_Get will spin or block
long OS_Fifo_Size(void){
  return CurrentSize.Value;
}


// ******** OS_MailBox_Init ************
// Initialize communication channel
// Inputs:  none
// Outputs: none
Sema4Type DataValid;
Sema4Type BoxFree;
void OS_MailBox_Init(void){
  
  OS_InitSemaphore(&DataValid,0);
  OS_InitSemaphore(&BoxFree, 1);
  
  
}

// ******** OS_MailBox_Send ************
// enter mail into the MailBox
// Inputs:  data to be sent
// Outputs: none
// This function will be called from a foreground thread
// It will spin/block if the MailBox contains data not yet received 
void OS_MailBox_Send(unsigned long data){

  OS_bWait(&BoxFree);
	MAILBOX = data;
	OS_bSignal(&DataValid);

}

// ******** OS_MailBox_Recv ************
// remove mail from the MailBox
// Inputs:  none
// Outputs: data received
// This function will be called from a foreground thread
// It will spin/block if the MailBox is empty 
unsigned long OS_MailBox_Recv(void){
  
  OS_bWait(&DataValid);
	
  //Retrieve the data from the mailbox
	unsigned long carrier = MAILBOX;
  OS_bSignal(&BoxFree);
  return carrier;
}

// ******** OS_Time ************
// return the system time 
// Inputs:  none
// Outputs: time in 12.5ns units, 0 to 4294967295
// The time resolution should be less than or equal to 1us, and the precision 32 bits
// It is ok to change the resolution and precision of this function as long as 
//   this function and OS_TimeDifference have the same resolution and precision 
unsigned long OS_Time(void){
  
  return (Timer0A_GetSystemTime()*12.5);
}

// ******** OS_TimeDifference ************
// Calculates difference between two times
// Inputs:  two times measured with OS_Time
// Outputs: time difference in 12.5ns units 
// The time resolution should be less than or equal to 1us, and the precision at least 12 bits
// It is ok to change the resolution and precision of this function as long as 
//   this function and OS_Time have the same resolution and precision 
unsigned long OS_TimeDifference(unsigned long start, unsigned long stop){
 return stop-start; 
}

// ******** OS_ClearMsTime ************
// sets the system time to zero (from Lab 1)
// Inputs:  none
// Outputs: none
// You are free to change how this works
void OS_ClearMsTime(void){
  
  Timer4A_ResetCount();
  
}


//******** OS_AddSW1Task *************** 
// add a background task to run whenever the SW1 (PF4) button is pushed
// Inputs: pointer to a void/void background function
//         priority 0 is the highest, 5 is the lowest
// Outputs: 1 if successful, 0 if this thread can not be added
// It is assumed that the user task will run to completion and return
// This task can not spin, block, loop, sleep, or kill
// This task can call OS_Signal  OS_bSignal	 OS_AddThread
// This task does not have a Thread ID
// In labs 2 and 3, this command will be called 0 or 1 times
// In lab 2, the priority field can be ignored
// In lab 3, there will be up to four background threads, and this priority field 
//           determines the relative priority of these four threads
volatile uint32_t FallingEdges = 0;
void (*sw1task)(void);
int OS_AddSW1Task(void(*task)(void), unsigned long priority){
  SYSCTL_RCGCGPIO_R |= 0x00000020; // (a) activate clock for port F
  FallingEdges = 0;             // (b) Give clock time to finish
  //Initialize PF4 (onboard switch)
  GPIO_PORTF_DIR_R &= ~0x10;          // (c) make PF4 in (built-in button)
  GPIO_PORTF_AFSEL_R &= ~0x10;        //     disable alt funct on PF4
  GPIO_PORTF_DEN_R |= 0x10;           //     enable digital I/O on PF4   
  GPIO_PORTF_PCTL_R &= ~0x000F0000;   //     configure PF4 as GPIO
  GPIO_PORTF_AMSEL_R = 0;             //     disable analog functionality on PF
  GPIO_PORTF_PUR_R |= 0x10;           //     enable weak pull-up on PF4
  GPIO_PORTF_IS_R &= ~0x10;           // (d) PF4 is edge-sensitive
  GPIO_PORTF_IBE_R &= ~0x10;          //     PF4 is not both edges
  GPIO_PORTF_IEV_R &= ~0x10;          //     PF4 falling edge event
  GPIO_PORTF_ICR_R = 0x10;            // (e) clear flag4
  GPIO_PORTF_IM_R |= 0x10;            // (f) arm interrupt on PF4 *** No IME bit as mentioned in Book ***
  
  NVIC_PRI7_R = (NVIC_PRI7_R&0xFF00FFFF)|((priority<<21)); // (g) priority 0
  NVIC_EN0_R = 0x40000000;      // (h) enable interrupt 30 in NVIC
	
	sw1task = task;
	
  return 1;
}

void GPIOPortF_Handler(void){
  GPIO_PORTF_ICR_R = 0x10;      // acknowledge flag
  (*sw1task)();             //Run task added by OS_SW1
}

// ******** OS_MsTime ************
// reads the current time in msec (from Lab 1)
// Inputs:  none
// Outputs: time in ms units
// You are free to select the time resolution for this function
// It is ok to make the resolution to match the first call to OS_AddPeriodicThread
unsigned long OS_MsTime(void){
  unsigned long COUNT = Timer4A_ReturnCount();
  return COUNT;
}

///******** OS_Launch ***************
// start the scheduler, enable interrupts
// Inputs: number of 20ns clock cycles for each time slice
//         (maximum of 24 bits)
// Outputs: none (does not return)
void OS_Launch(unsigned long theTimeSlice){
  NVIC_ST_RELOAD_R = theTimeSlice - 1; // reload value
  NVIC_ST_CTRL_R = 0x00000007; // enable, core clock and interrupt arm
  StartOS();                   // start on the first task
}
