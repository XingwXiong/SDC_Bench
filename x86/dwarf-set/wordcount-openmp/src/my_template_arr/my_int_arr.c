/*********************************************************************
 *       File Name: my_int_arr.c
 *     Description: to implement functions in my_int_arr.h
 *          Author: He Xiwen
 *           Email: hexiwen2000@163.com 
 *    Created Time: 2015-10-08 10:27:01
 *   Modified Time: 2015-10-08 10:50:11
 ********************************************************************/

#include <stdio.h>
#include <malloc.h>

#include "my_int_arr.h"


static int expandMyIntArr( MyIntArr *mia );


int conMyIntArr( MyIntArr *mia )
{
  mia->size = 0; // mia: empty;
  mia->capacity = 4;
  mia->arr = (int32_t*)malloc( mia->capacity*sizeof(int32_t) );
  if( mia->arr == NULL ) return -1;
  else return 0;
}


int desMyIntArr( MyIntArr *mia )
{
  if( mia->arr != NULL ) free( mia->arr );
  mia->arr = NULL;
  return 0;
}


int copyMyIntArr( MyIntArr *mia1, MyIntArr *mia2 )
{
  uint32_t i;
  if( mia1->capacity <= mia2->size+1 )
  { // reallocate the space.
    // res = recapacityMyIntArr( mia1, mia2->size+2 );
    // if( res < 0 ) return res;
    // mia1->size = mia2->size;
    mia1->size = mia2->size;
    mia1->capacity = mia2->size+2;
    if( mia1->arr != NULL ) free( mia1->arr );
    mia1->arr = NULL;
    mia1->arr = (int32_t*)malloc( mia1->capacity*sizeof(int32_t) );
    if( mia1->arr == NULL ) return -1; 
  }
  else
  {
    mia1->size = mia2->size;
  }
  for( i=0; i<mia1->size; ++i )
  {
    mia1->arr[i] = mia2->arr[i];
  }
  return 0;
}


int clearMyIntArr( MyIntArr *mia )
{
  mia->size = 0;
  // not reallocate the space;
  //mia->capacity = 4;
  //if( mia->arr != NULL ) free( mia->arr );
  //mia->arr = (int32_t*)malloc( sizeof(int32_t)*4 );
  //if( mia->arr == NULL ) return -1;
  return 0;
}


int32_t* getMyIntArr( MyIntArr *mia1, uint32_t loc )
{
  if( loc >= mia1->size ) return NULL;
  return &(mia1->arr[loc]);
}


int32_t pushMyIntArr( MyIntArr *mia1, int32_t s32 )
{
  expandMyIntArr( mia1 );
  return ( mia1->arr[mia1->size ++] = s32 );
}


int32_t insertFirstMyIntArr( MyIntArr *mia1, int32_t s32 )
{
  uint32_t i;
  expandMyIntArr( mia1 );
  if( mia1->size > 0 )
  {
    for( i=mia1->size; i>0; --i )
    {
      mia1->arr[i] = mia1->arr[i-1];
    }
    ++ mia1->size;
  }
  return ( mia1->arr[0]=s32 );
}


int32_t popMyIntArr( MyIntArr *mia1 )
{
  if( mia1->size <= 0 )return 0;
  return ( mia1->arr[-- mia1->size] );
}


int32_t deleteFirstMyIntArr( MyIntArr *mia1 )
{
  int32_t s32;
  uint32_t i;
  if( mia1->size <= 0 ) return 0;
  s32 = mia1->arr[0];
  for( i=1; i<mia1->size; ++i )
  {
    mia1->arr[i-1] = mia1->arr[i];
  }
  -- mia1->size;
  return s32;
}


static int expandMyIntArr( MyIntArr *mia1 )
{
  uint32_t i;
  int32_t* tmp;
  if( mia1->size <= mia1->capacity-2 ) return 0;
  // need to expand. capacity=capacity*2;
  tmp = (int32_t*)malloc( mia1->capacity*2*sizeof(int32_t) );
  if( tmp==NULL ) return -1;
  for( i=0; i<mia1->size; ++i )
  {
    tmp[i] = mia1->arr[i];
  }
  free( mia1->arr );
  mia1->arr = tmp;
  mia1->capacity *= 2;
  return 0;
}


int recapacityMyIntArr( MyIntArr *mia1, uint32_t cap )
{
  uint32_t i;
  int32_t* tmp;
  if( cap<mia1->size || cap<4 ) return -1;
  // recapacity
  tmp = (int32_t*)malloc( cap*sizeof(int32_t) ); 
  if( tmp==NULL ) return -1;
  for( i=0; i<mia1->size; ++i )
  {
    tmp[i] = mia1->arr[i];
  }
  free( mia1->arr );
  mia1->arr = tmp;
  mia1->capacity = cap;
  return 0;
}


uint32_t findMyIntArr( MyIntArr *mia1, int32_t s32 ) // first one found;
{
  uint32_t i;
  for( i=0; i<mia1->size; ++i )
  {
    if( mia1->arr[i] == s32 ) return i;
  }
  return mia1->size;
}


#ifdef DEBUG_TEST
int displayMyIntArr( MyIntArr *mia1 )
{
  uint32_t i;
  if( mia1==NULL ) return -1;
  printf( "MyIntArr->display(): \n" );
  printf( "size: %d, capacity: %d. \n", mia1->size, mia1->capacity );
  if( mia1->size>0 ) // mia1!=0
  {
    //for( i=mia1->size; i>0; --i ) printf( "%8x ", mia1->arr[i-1] );
    for( i=mia1->size; i>0; --i ) printf( "%d ", mia1->arr[i-1] );
  }
  //else printf( "0 " );
  printf( "\nDisplay end. \n" );
  return 0;
}
#endif

