/******************************************************************
 *       File Name: wordcount.h
 *     Description: 
 *          Author: He Xiwen
 *           Email: hexiwen2000@163.com 
 *    Created Time: 2015-11-25 10:08:11
 *   Modified Time: 
 *          License: 
 *****************************************************************/

#ifndef _WORDCOUNT_H_
#define _WORDCOUNT_H_ 1

#include "my_string.h"
#include "my_int_arr.h"

#define MEMORY_SORT_SIZE 0X10000000 // 256M;

// in my_string.h and my_string.c;
// int sortMyStringArr( MyStringArr *msa );

// multi-way merge sort for single-thread and multi-thread;
// single thread multi-way merge sort; so it is not so efficient;
extern int merge_sort( const char *output_dir_tmp, const char *output_f, char *line, MyStringArr *msa_merge_buf, MyIntArr *mia, MyString *ms, MyString *ms_one_word, MyString *min_ms );

#endif /* _WORDCOUNT_H_ */
