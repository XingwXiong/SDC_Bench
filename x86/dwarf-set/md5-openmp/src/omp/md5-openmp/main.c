/******************************************************************
 *       File Name: main.c
 *     Description: 
 *          Author: He Xiwen
 *           Email: hexiwen2000@163.com 
 *    Created Time: 2015-11-17 15:19:05
 *   Modified Time: 
 *          License: 
 *****************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <malloc.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>

#ifdef _OPENMP
#include <omp.h>
#else
#include "my_single_omp.h"
#endif

#include "my_string.h"
#include "my_data.h"
#include "md5.h"


static size_t single_read_block( FILE *ifp, size_t read_block_size, int linecnt_max, char *line, MyStringArr *msa, MyString *ms, int *lineno );
static size_t single_write_block( FILE *ofp, MyStringArr *msa );
static void md5_one_line( MyString *ms, MD5Context *ctx, uint8_t *buf );
static void for_parallel( MyString *ms );
static size_t read_one_line_to_msa( FILE *ifp, char *line, MyStringArr *msa, MyString *ms, int *line_no );
static size_t read_one_line( FILE *ifp, char *line, MyString *ms, int *line_no );
void do_one_file( const char *ifpath, const char *ofpath, char *line, MyStringArr *msa, MyString *ms );

int main(int argc, const char *argv[])
{
  int i;
  DIR *dir;
  struct dirent *fi;
  struct stat st;
  const char *input_f_d;
  const char *output_f_d;
  int ilen, olen;
  char *ifpath;
  char *ofpath;
  char *line;
  int thread_cnt;
  MyString ms;
  MyStringArr msa;
  time_t time_start, time_end;
  time_start = time( NULL );
#ifdef _OPENMP
  // usage: exe input_f_d output_f_d thread_cnt
  if (argc != 4)
#else
  // usage: exe input_f_d output_f_d
  if (argc != 3)
#endif
  {
    if(argc != 1) printf("Wrong arguments!\n");
#ifdef _OPENMP
    printf("Usage: %s input_f_d output_f_d thread_cnt\n", argv[0]);
#else
    printf("Usage: %s input_f_d output_f_d\n", argv[0]);
#endif
    if( argc==1 ) exit(EXIT_SUCCESS);
    else exit( EXIT_FAILURE );
  }
#ifdef DEBUG_TEST
  printf("argc: %d\n", argc);
  printf("argv: ");
  for (i = 0; i < argc; ++i) printf("%s ", argv[i]);
  printf("\n");
#endif

#ifdef _OPENMP
  thread_cnt = atoi( argv[3] );
  omp_set_num_threads( thread_cnt );
#pragma omp parallel for
  for( i=0; i<10; ++i ){
    if( 0==i ) printf( "thread_cnt: %d\n", omp_get_num_threads() );
  }
#else
  thread_cnt = omp_get_num_threads();
  printf( "thread_cnt: %d\n", thread_cnt );
#endif

  conMyString( &ms );
  conMyStringArr( &msa );

  input_f_d = argv[1];
  output_f_d = argv[2];

  thread_cnt = omp_get_num_threads();

  line = (char *) malloc(sizeof(char) * (MY_LINE_MAX + 1));
  line[MY_LINE_MAX] = 0;

  ilen = strlen(input_f_d);
  ilen += MY_FILENAME_MAX + 1;
  ifpath = (char *) malloc(sizeof(char) * ilen);
  ifpath[0] = 0;

  olen = strlen(output_f_d);
  olen += MY_FILENAME_MAX + 1;
  ofpath = (char *) malloc(sizeof(char) * olen);
  ofpath[0] = 0;

  if (stat(input_f_d, &st) < 0) {
    // not exist;
    perror("Cannot access input_file_or_dir");
    exit(EXIT_FAILURE);
  }
  if (S_ISDIR(st.st_mode)) {
    // input_f_d: input_d
    printf("process dir.\n");

    if ((dir = opendir(input_f_d)) == NULL) {
      perror("Cannot open input_f_d");
      exit(EXIT_FAILURE);
    }

    while ((fi = readdir(dir)) != NULL) {
      snprintf(ifpath, ilen, "%s/%s", input_f_d,
         fi->d_name);
      if (fi->d_name[0] == '.') {
        // ignore: hidden file, or pwd, or parent dir of pwd;
        continue;
      }
      if (stat(ifpath, &st) < 0) {
        // not exist
        continue;
      }
      if (S_ISDIR(st.st_mode)) {
        // ignore: child dir
        continue;
      }
#ifdef DEBUG_TEST
      //printf( "ifpath: %s\n", ifpath );
#endif
      if (S_ISREG(st.st_mode)) {
        snprintf( ofpath, olen, "%s/%s", output_f_d, fi->d_name );
        do_one_file( ifpath, ofpath, line, &msa, &ms );
      }

    }
    closedir(dir);

  } else if (S_ISREG(st.st_mode)) {
    // input_f_d: input_f
    printf("process file.\n");

    snprintf( ifpath, olen, "%s", input_f_d );
    snprintf( ofpath, olen, "%s", output_f_d );
    do_one_file( ifpath, ofpath, line, &msa, &ms );
  }

  free(ifpath);
  free(ofpath);
  free(line);
  desMyStringArr( &msa );
  desMyString( &ms );
  time_end = time(NULL);
  printf( "time: %ld\n", time_end-time_start );
  
  return 0;
}


void do_one_file( const char *ifpath, const char *ofpath, char *line, MyStringArr *msa, MyString *ms )
{
  int i;
  FILE *ifp;
  FILE *ofp;
  size_t one_readed_size;
  size_t readed_size;
  size_t writed_size;
  int lineno;
  int lineno_old;

#ifdef DEBUG_TEST
  printf( "now to file: %s\n", ifpath );
#endif

  ifp = fopen(ifpath, "r");
  if (ifp == NULL) {
    perror("Cannot open input file");
    exit(EXIT_FAILURE);
  }
  ofp = fopen(ofpath, "w");
  if (ofp == NULL) {
    perror("Cannot open output file");
    exit(EXIT_FAILURE);
  }
  readed_size = 0;
  lineno = 0;
  writed_size = 0;

  while( !feof( ifp ) ){
    lineno_old = lineno;
    one_readed_size = single_read_block( ifp, FILE_CHUNK_SIZE, LINE_CNT_MAX, line, msa, ms, &lineno );
    readed_size += one_readed_size;
#ifdef DEBUG_TEST
    printf( "one block:  size:%ld  line:%d\n", one_readed_size, lineno-lineno_old );
#endif
    // make sure no malloc in omp; see "bash: man malloc"
    for( i=0; i<msa->size; ++i ){
      if( (msa->arr[i]).capacity < 2*DATA_MD5_SIZE+2 ) recapacityMyString( &(msa->arr[i]), 2*DATA_MD5_SIZE+2 );
    }
#ifdef _OPENMP
#pragma omp parallel for
#endif
    for( i=0; i<msa->size; ++i ){
#ifdef DEBUG_TEST
      //if( 0==i ) printf( "thread_cnt: %d\n", omp_get_num_threads() );
#endif
       for_parallel( &(msa->arr[i]) );
    }
#ifdef _OPENMP
#pragma omp parallel
#endif
    writed_size += single_write_block( ofp, msa );
  }

  fclose(ifp);
  fclose(ofp);
}


static size_t single_read_block( FILE *ifp, size_t read_block_size, int linecnt_max, char *line, MyStringArr *msa, MyString *ms, int *lineno ){
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
    ret = read_one_line_to_msa( ifp, line, msa, ms, lineno );
    if( ret < 0 ) break; // eof;
    readed_size += ret;
    ++linecnt;
  }
  return readed_size;
}


static size_t single_write_block( FILE *ofp, MyStringArr *msa ){
  int i;
  char *buf;
  size_t writed_size = 0;
  for( i=0; i<msa->size; ++i ){
    buf = (msa->arr[i]).data;
    if(0==omp_get_thread_num()) fprintf( ofp, "%s\n", buf );
    writed_size += strlen( buf ) + 1;
  }
  return writed_size;
}


static void md5_one_line( MyString *ms, MD5Context *ctx, uint8_t *buf ){
  int i;
  char *str;
  size_t size;
  MD5_Init(ctx);
  size = strlen( ms->data );
  MD5_Update(ctx, (uint8_t *) (ms->data), size);
  MD5_Final(ctx, buf);
  clearMyString( ms );
  str = (char *) ms->data;
  for (i = 0; i < DATA_MD5_SIZE; i++, str+=2)
    sprintf(str, "%02x", buf[i]);
  *str = '\0';
}


static void for_parallel( MyString *ms){
  //int ithread = omp_get_thread_num();
  //md5_one_line( ms, &(ctx_arr[ithread]), &(buf_arr[ithread*DATA_MD5_SIZE]) );

  MD5Context ctx;
  uint8_t buf[DATA_MD5_SIZE];
  int i;
  for(i=0;i<LOOP_NUM;i++)
  	md5_one_line( ms, &ctx, buf );
}


static size_t read_one_line_to_msa( FILE *ifp, char *line, MyStringArr *msa, MyString *ms, int *line_no )
{
  size_t ret = 0;
  int line_no_old = *line_no;

  if( feof( ifp ) ) return -1; // eof;
  ret = read_one_line( ifp, line, ms, line_no );
  if( line_no_old == *line_no ) return -1; // eof;
  pushMyStringArr( msa, ms );
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

