/******************************************************************
 *       File Name: do_one_file.c
 *     Description: 
 *          Author: He Xiwen
 *           Email: hexiwen2000@163.com 
 *    Created Time: 2015-11-23 17:27:24
 *   Modified Time: 
 *          License: 
 *****************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#ifdef _OPENMP
#include <omp.h>
#else
#include "my_single_omp.h"
#endif

#include "my_seq_ops.h"
#include "my_string.h"
#include "my_int_arr.h"
#include "my_data.h"
#include "wordcount.h"
#include "do_one_file.h"


clock_t clock_single_read;
clock_t clock_single_write;
clock_t clock_parallel;
double walltime_parallel;


static size_t single_read_block( FILE *ifp, size_t read_block_size, int linecnt_max, char *line, MyStringArr *msa, MyString *ms, MyString *ms_one_word, int *lineno );
static size_t single_read_msa_arr( FILE *ifp, char *line, MyStringArr *msa_arr, int arr_size, MyString *ms, MyString *ms_one_word, int *lineno );
static size_t merge_write_msa_arr( const char *ofpath, MyStringArr *msa_arr, int arr_size, MyString *ms );
static void for_parallel( MyStringArr *msa );
static void sort_one_msa( MyStringArr *msa );
static size_t read_one_line_to_msa( FILE *ifp, char *line, MyStringArr *msa, MyString *ms, MyString *ms_one_word, int *line_no );
static size_t read_one_line( FILE *ifp, char *line, MyString *ms, int *line_no );
static int lineToMsa( MyStringArr *msa, MyString *ms, MyString *ms_one_word );
static void one_word( const char *str, int *iter, int *word_begin, int *word_len );

// memory sort;
int do_one_file( const char *ifpath, const char *output_d, char *line, MyStringArr *msa_arr, int thread_cnt, MyString *ms, MyString *ms_one_word, int file_cnt )
{
  int i;
  FILE *ifp;
  int olen;
  char *ofpath;
  size_t one_readed_size;
  size_t readed_size;
  size_t writed_size;
  int lineno;
  int lineno_old;
  clock_t t_start, t_end;
  struct timespec ts_b, ts_e;

#ifdef DEBUG_TEST
  printf( "now to file: %s\n", ifpath );
#endif

  ifp = fopen(ifpath, "r");
  if (ifp == NULL) {
    perror("Cannot open input file");
    exit(EXIT_FAILURE);
  }
  olen = strlen( output_d );
  olen += MY_FILENAME_MAX + 2;
  ofpath = (char *) malloc( sizeof(char) * olen );
  if( ofpath == NULL ) {
    perror("malloc(): failure");
    fclose( ifp );
    return file_cnt;
  }
  readed_size = 0;
  lineno = 0;
  writed_size = 0;

  while( !feof( ifp ) ){
    lineno_old = lineno;
    t_start = clock(); 
    one_readed_size = single_read_msa_arr( ifp, line, msa_arr, thread_cnt, ms, ms_one_word, &lineno );
    t_end = clock();
    clock_single_read += t_end - t_start;
    readed_size += one_readed_size;
#ifdef DEBUG_TEST
    printf( "one msa_arr:  size:%ld  line:%d\n", one_readed_size, lineno-lineno_old );
#endif
    t_start = clock();
    clock_gettime( CLOCK_MONOTONIC, &ts_b );
#ifdef _OPENMP
#pragma omp parallel for
#endif
    for( i=0; i<thread_cnt; ++i ){
#ifdef DEBUG_TEST
      //if( 0==i ) printf( "thread_cnt: %d\n", omp_get_num_threads() );
#endif
       for_parallel( msa_arr + i );
    }
    t_end = clock();
    clock_gettime( CLOCK_MONOTONIC, &ts_e );
    clock_parallel += t_end - t_start;
    walltime_parallel += (ts_e.tv_sec - ts_b.tv_sec) + (ts_e.tv_nsec - ts_b.tv_nsec)/(1000000000.0);
    //printf( "parallel part time: %ld\n", t_end-t_start );
    sprintf( ofpath, "%s/tmp-%04d", output_d, file_cnt );
    t_start = clock(); 
    writed_size += merge_write_msa_arr( ofpath, msa_arr, thread_cnt, ms );
    t_end = clock();
    clock_single_write += t_end - t_start;
    ++ file_cnt;
  }

  free( ofpath );
  fclose(ifp);
  return file_cnt;
}


static size_t single_read_msa_arr( FILE *ifp, char *line, MyStringArr *msa_arr, int arr_size, MyString *ms, MyString *ms_one_word, int *lineno )
{
  size_t readed_size;
  int i;
  int one_block_size;
  int one_line_max;

  one_block_size = file_chunk_size/ arr_size;
  one_line_max = LINE_CNT_MAX;
  readed_size = 0;
  for( i=0; i<arr_size; ++i ) {
    readed_size += single_read_block( ifp, one_block_size, one_line_max, line, msa_arr+i, ms, ms_one_word, lineno );
  }
  return readed_size;
}


static size_t merge_write_msa_arr( const char *ofpath, MyStringArr *msa_arr, int arr_size, MyString *ms )
{
  int i;
  size_t writed_size;
  int one_writed_size;
  FILE *ofp;
  char *buf;
  int *w_cnt;
  int min_ms;
  int cnt;

  ofp = fopen(ofpath, "w");
  if (ofp == NULL) {
    perror("Cannot open output file");
    exit(EXIT_FAILURE);
  }
  w_cnt = (int *) malloc( sizeof(int) * arr_size );
  if( w_cnt == NULL ) {
    perror( "malloc: failure" );
    exit( EXIT_FAILURE );
  }
  for( i=0; i<arr_size; ++i ) w_cnt[i] = 0;

  writed_size = 0;
  // init: ms = msa_arr[0].arr[0];
  if( msa_arr[0].size < 1 ) {
    fprintf( stderr, "Wrong msa_arr in merge_write_msa_arr.\n" );
    return -1;
  }
  cpyMyString( ms, msa_arr[0].arr+0 );
  cnt = 1;
  ++ w_cnt[0];
  while( 1 ) {
    // init min_ms;
    for( i=0; i<arr_size; ++i ) {
      if( w_cnt[i] < msa_arr[i].size ) break;
    }
    if( i < arr_size ) min_ms = i;
    else break;
    ++ i;
    // min_ms
    for( ; i<arr_size; ++i ) {
      if( w_cnt[i] >= msa_arr[i].size ) continue;
      // cmp
      if( cmpMyString( &(msa_arr[i].arr[ w_cnt[i] ]), &(msa_arr[min_ms].arr[ w_cnt[min_ms] ]) ) < 0 ) min_ms = i;
    }
    // cmp
    if( cmpMyString( ms, msa_arr[min_ms].arr + w_cnt[min_ms] ) == 0 ) {
      ++ cnt;
      ++ w_cnt[min_ms];
      continue;
    }
    // new word; write old word;
    buf = ms->data;
    one_writed_size = fprintf( ofp, "%s %d\n", buf, cnt );
    if( one_writed_size >= 0 ) writed_size += one_writed_size;
    else fprintf( stderr, "Failure: fprintf in merge_write_msa_arr.\n" );
    // replace with new word;
    cpyMyString( ms, msa_arr[min_ms].arr + w_cnt[min_ms] );
    cnt = 1;
    ++ w_cnt[min_ms];
  }

  // last one;
  buf = ms->data;
  one_writed_size = fprintf( ofp, "%s %d\n", buf, cnt );
  if( one_writed_size >= 0 ) writed_size += one_writed_size;
  else fprintf( stderr, "Failure: fprintf in merge_write_msa_arr.\n" );

  fclose( ofp );
  free( w_cnt );
  return writed_size;
}


static size_t single_read_block( FILE *ifp, size_t read_block_size, int linecnt_max, char *line, MyStringArr *msa, MyString *ms, MyString *ms_one_word, int *lineno )
{
  size_t readed_size;
  int linecnt;
  int ret = 0;

  readed_size = 0;
  linecnt = 0;
  clearMyStringArr( msa );
  clearMyString( ms );

  while (1) {
    if( feof( ifp ) ) break;
    if( readed_size > read_block_size ) break;
    if( linecnt > linecnt_max ) break;
    ret = read_one_line_to_msa( ifp, line, msa, ms, ms_one_word, lineno );
    if( ret < 0 ) break; // eof;
    readed_size += ret;
    ++linecnt;
  }
  return readed_size;
}

/*
static size_t single_write_block( FILE *ofp, MyStringArr *msa )
{
  int i;
  char *buf;
  size_t writed_size = 0;
  for( i=0; i<msa->size; ++i ){
    buf = (msa->arr[i]).data;
    fprintf( ofp, "%s\n", buf );
    writed_size += strlen( buf ) + 1;
  }
  return writed_size;
}
*/

static void sort_one_msa( MyStringArr *msa )
{
  sortMyStringArr( msa );
}


static void for_parallel( MyStringArr *msa )
{
  sort_one_msa( msa );
}


static size_t read_one_line_to_msa( FILE *ifp, char *line, MyStringArr *msa, MyString *ms, MyString *ms_one_word, int *line_no )
{
  size_t ret = 0;
  int line_no_old = *line_no;

  if( feof( ifp ) ) return -1; // eof;
  ret = read_one_line( ifp, line, ms, line_no );
  if( line_no_old == *line_no ) return -1; // eof;
  lineToMsa( msa, ms, ms_one_word ); //pushMyStringArr( msa, ms );
  clearMyString( ms );
  return ret;
}


static size_t read_one_line( FILE *ifp, char *line, MyString *ms, int *line_no ) // ret: line len; ret!=0 <=> ms->data[0]!=0;
{
  char *s_res;
  const int line_size = MY_LINE_MAX + 1;
  int lline;
  size_t ret;

  clearMyString( ms );
  ret = 0;
  while( 1 )
  {
    s_res = fgets( line, line_size, ifp );
    if( s_res == NULL )
    {
      if( ms->data[0] != 0 )
      {
        // line in ms; // no eol for last line in file;
        printf( "no EOL for the last line in file!" );
        ++ (*line_no);
      }
      break; // eof;
    }
    lline = strlen( line );
    ret += lline;
    if( line[lline-1] == '\n' )
    {
      // eol;
      ++ (*line_no);
      //if( (*line_no) % 10000 == 0 ) printf( "reading: line_no:%d\n", *line_no );
      appendnMyString( ms, line, lline-1 );
      break;
    }
    else
    {
      appendnMyString( ms, line, lline );
    }
  }
  return ret;
}


// single-thread multi-way merge sort;
void do_merge_sort( const char *input_d, const char *output_f, char *line, MyString *ms, MyString *ms_one_word )
{
  MyStringArr msa_merge_buf;
  MyIntArr mia;
  MyString min_ms;

  conMyStringArr( &msa_merge_buf );
  conMyIntArr( &mia );
  conMyString( &min_ms );

  merge_sort( input_d, output_f, line, &msa_merge_buf, &mia, ms, ms_one_word, &min_ms );

  desMyString( &min_ms );
  desMyIntArr( &mia );
  desMyStringArr( &msa_merge_buf );
}


// one file line --> ms; ms = words; word --> ms_one_word; ms_one_word --> push to msa;
static int lineToMsa( MyStringArr *msa, MyString *ms, MyString *ms_one_word )
{
  int cnt;
  int i;
  int word_begin, word_len;

  cnt = 0;
  i = 0;
  while( 1 )
  {
    one_word( ms->data, &i, &word_begin, &word_len );
    if( word_len == 0 ) break; // no more words;
    // one word;
    cpynStrMyString( ms_one_word, ms->data+word_begin, word_len );
#ifdef DEBUG_TEST
    //printf( "word: \n  " );
    //displayMyString( ms_one_word );
#endif
    pushMyStringArr( msa, ms_one_word );
    ++ cnt;
  }
  return cnt;
}


// find one word;
static void one_word( const char *str, int *iter, int *word_begin, int *word_len )
{
  int i = *iter;
  //int ascii_flag;

  for( ; str[i]!=0; ++i )
  {
    if( ! is_blank( str[i] ) ) break;
  }
  *word_begin = i;
  //ascii_flag = -1;
  for( ; str[i]!=0; ++i )
  {
    if( is_letter( str[i] ) ) continue;
    //if( is_letter_ascii( str[i] ) )
    //{
    //  if( ascii_flag == 0 ) break;
    //  ascii_flag = 1;
    //  continue;
    //}
    //else if( is_letter_not_ascii( str[i] ) )
    //{
    //  if( ascii_flag == 1 ) break;
    //  ascii_flag = 0;
    //  continue;
    //}
    else if( is_blank( str[i] ) ) break;
    else
    {
      if( i== (*word_begin) ) ++i;
      break;
    }
  }
  *word_len = i-(*word_begin);
  *iter = i;
}


