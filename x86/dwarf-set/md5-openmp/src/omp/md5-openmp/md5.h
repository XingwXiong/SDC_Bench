#ifndef MD5_H
#define MD5_H

/*
 * organized from rfc1321.txt by some one on internet;
 *
 * include:
 * test.h type.h
 * string.h
 */

#include <stdint.h>

#define DATA_MD5_SIZE 16

typedef struct S_MD5Context
{
  uint32_t state[4];
  uint32_t count[2];
  uint8_t  buffer[64];
} MD5Context;

extern void MD5_Init( MD5Context* context );
extern void MD5_Update( MD5Context* context, uint8_t* buf, int len );
extern void MD5_Final( MD5Context* context, uint8_t digest[16] );
extern void MD5_File( char* filename );
extern void MD5_Print( uint8_t digest[], int len );

#endif /* MD5_H */

