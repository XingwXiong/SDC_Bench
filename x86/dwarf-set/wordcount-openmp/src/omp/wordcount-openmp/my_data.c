/******************************************************************
 *       File Name: my_data.c
 *     Description: 
 *          Author: He Xiwen
 *           Email: hexiwen2000@163.com 
 *    Created Time: 2015-11-23 21:00:37
 *   Modified Time: 
 *          License: 
 *****************************************************************/

#include <stdio.h>

#include "my_data.h"

//#define FILE_CHUNK_SIZE 0x8000000 // 128M; //0x10000000 // 256M; //0x20000000 // 512M; //0x100000 // 1M; //0x4000000 // 64M;
size_t file_chunk_size = 0x2000000; // 32M;

int my_data_init()
{
  // code from: http://blog.chinaunix.net/uid-20672257-id-3139061.html
  // the code also can be got in package /usr/bin/top
  // set file_chunk_size = sys_memtotal / 2^5;
  FILE *ifp;
  char buff[256];
  char name[20];
  unsigned long total;
  char kb[10];

  ifp = fopen ("/proc/meminfo", "r");

  fgets (buff, sizeof(buff), ifp);
  sscanf (buff, "%s %lu %s", name, &total, kb);
  // kB
  total <<= 10;
  file_chunk_size = total / (1<<5);
#ifdef DEBUG_TEST
  printf( "sys mem total: 0x%lx  file chunk size: 0x%lx\n", total, file_chunk_size );
#endif

  fclose(ifp);
  return 0;
}


int my_data_end()
{
  return 0;
}


