/*********************************************************************
 *       File Name: my_int_arr.h
 *     Description: my_int_array;
 *          Author: He Xiwen
 *           Email: hexiwen2000@163.com 
 *    Created Time: 2015-10-08 10:25:57
 *   Modified Time: 2015-10-08 10:30:44
 ********************************************************************/

#ifndef MY_INT_ARR_H
#define MY_INT_ARR_H

#include <stdint.h>
//#include "type.h"

// my_int_arr: int32_t_t;
typedef struct S_MyIntArr // array
{
  int32_t *arr; // myArray;
  uint32_t capacity; // total capacity of arr // capacity<0x80000000
  uint32_t size; // size
} MyIntArr;


extern int conMyIntArr( MyIntArr *mia );
extern int desMyIntArr( MyIntArr *mia );
extern int copyMyIntArr( MyIntArr *mia1, MyIntArr *mia2 );
extern int clearMyIntArr( MyIntArr *mia );
extern int32_t* getMyIntArr( MyIntArr *mia, uint32_t loc );
extern int32_t pushMyIntArr( MyIntArr *mia, int32_t s32 );
extern int32_t popMyIntArr( MyIntArr *mia );
extern int32_t insertFirstMyIntArr( MyIntArr *mia, int32_t s32 );
extern int32_t deleteFirstMyIntArr( MyIntArr *mia );
extern int recapacityMyIntArr( MyIntArr *mia, uint32_t cap );
extern uint32_t findMyIntArr( MyIntArr *mia, int32_t s32 ); // first one found;
#ifdef DEBUG_TEST
extern int displayMyIntArr( MyIntArr *mia );
#endif


#endif /* MY_INT_ARR_H */
