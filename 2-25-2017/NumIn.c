//NumIn.C
//Description: Adds, subtracts, multiplies, or divides natural numbers
//Input: Currently does not handle negative input
//Output: Currrently does not handle negative or fractional output
//Alana Wilcoxson
//Febuary 4, 2017


#include  <stdlib.h>


// -----------|| GLOBALS ||-------------
int VALUE1 = 0;
int VALUE2 = 0;

char sum[] = "+";
char sub[] = "-";
char mult[] = "*";
char divide[] = "/";



// -----------|| convToInt ||-------------
//Converts char to int for numerical operation
//input: "operation VALUE1 VALUE2"
//       i.e. "+ 3 4" --> VALUE1 is 3, VALUE2 is 4, operation is ignored
//output: None
void convToInt (char* userIn){
  VALUE1 = atoi(&userIn[2]);
  VALUE2 = atoi(&userIn[4]);
}


// -----------|| NumIn_Add ||-------------
//Adds VALUE2 to VALUE1
//input: VALUE1, VALUE2
//output: VALUE1 + VALUE2
int NumIn_Add(void){
  return VALUE1 + VALUE2;
}


// -----------|| NumIn_Sub ||-------------
//Subtracts VALUE2 from value 1
//input: VALUE1, VALUE2
//output: VALUE1 - VALUE2
int NumIn_Sub(void){
  return VALUE1 - VALUE2;
}


// -----------|| NumIn_Div ||-------------
//Divides VALUE1 by value 2
//input: VALUE1, VALUE2
//output: VALUE1 / VALUE2 (no remainder)
int NumIn_Div(void){
  return VALUE1 / VALUE2;
}


// -----------|| NumIn_Mult ||-------------
//Multiplies VALUE1 by value 2
//input: VALUE1, VALUE2
//output: VALUE1 * VALUE2
int NumIn_Mult(void){
  return VALUE1 * VALUE2;
}


// -----------|| NumIn ||-------------
//Handles the processing of string input for numerical operations
//input: User input string from UART
//output: result of numerical operation
int NumIn(char* userIn){
  convToInt(userIn);
  int result = 0;
  
      if(sum[0] == userIn[0]){
          result = NumIn_Add();
      }else if (sub[0] == userIn[0]){
          result = NumIn_Sub();
      }else if (mult[0] == userIn[0]){
          result = NumIn_Mult();
      }else if (divide[0] == userIn[0]){
          result = NumIn_Div();
      }
      
      return result;
  
}
