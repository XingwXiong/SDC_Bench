/******************************************************************
 *       File Name: my_data.h
 *     Description: 
 *          Author: He Xiwen
 *           Email: hexiwen2000@163.com 
 *    Created Time: 2015-11-17 15:12:57
 *   Modified Time: 
 *          License: 
 *****************************************************************/

#ifndef _MY_DATA_H_
#define _MY_DATA_H_

#define MY_FILENAME_MAX 64
#define MY_LINE_MAX 1024
//#define FILE_CHUNK_SIZE 0x10000000 //  M; // 10000000 // 256M; //0x100000 // 1M; //0x4000000 // 64M; //0x20000000 // 512M;
#define FILE_CHUNK_SIZE 0x8000000 // 128 M
//#define FILE_CHUNK_SIZE 0x4000000 //  64 M
//#define FILE_CHUNK_SIZE 0x2000000 //  32 M
#define LOOP_NUM 1

#define LINE_CNT_MAX 0x1000000 // 16M;

#endif /* _MY_DATA_H_ */
