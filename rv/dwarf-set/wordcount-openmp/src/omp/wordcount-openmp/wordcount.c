/******************************************************************
 *       File Name: wordcount.c
 *     Description: 
 *          Author: He Xiwen
 *           Email: hexiwen2000@163.com 
 *    Created Time: 2015-11-25 10:08:14
 *   Modified Time: 
 *          License: 
 *****************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>

#include "wordcount.h"
#include "my_seq_ops.h"
#include "my_data.h"

// read: one line-->ms; ms-->words; one word-->push into msa;
// read: one k-v line-->ms and cnt;
// write: msa->arr[i] and mia->arr[i]-->one k-v line;

static inline void displayWCKVToFile( FILE *ofp, MyString *ms, int cnt );

static void merge_init( FILE **ifp_arr, int *if_has_next, int if_cnt, char *line, MyStringArr *msa, MyIntArr *mia, MyString *ms, MyString *ms_one_word, int *line_no );

static void merge_do_merge( FILE **ifp_arr, int *if_has_next, int if_cnt, char *line, MyStringArr *msa_merge_buf, MyIntArr *mia, MyString *ms, MyString *ms_one_word, MyString *min_ms, int *line_no, const char *output_f );

static void read_one_line_to_msa( FILE *ifp, int *if_has_next, char *line, MyStringArr *msa, MyIntArr *mia, MyString *ms, MyString *ms_one_word, int *line_no );

static int read_one_line( FILE *ifp, char *line, MyString *ms, int *line_no );

static void read_next_line( FILE* ifp, int *if_has_next, char *line, MyString *ms, MyString *ms_one_word, int *cnt, int *line_no );

// one file line -->ms; ms = word + cnt;
static void lineToWCKV( MyString *ms, MyString *ms_one_word, int *cnt );


// single thread multi-way merge sort; single-thread and multi-thread;
// file line --> msa[i] and mia[i] k-v; min_ms==min(msa[i]) ? min_cnt+=mia[i],read_next(i) : write(min_ms,min_cnt),min_ms=min(msa[i]),min_cnt=mia[i],read_next(i);
int merge_sort( const char *output_dir_tmp, const char *output_f, char *line, MyStringArr *msa_merge_buf, MyIntArr *mia, MyString *ms, MyString *ms_one_word, MyString *min_ms )
{
  int i;
  struct stat st;
  int tmp_file_cnt;
  int ilen;
  char *ifpath;
  DIR *dir;
  struct dirent *fi;
  FILE **ifp_arr;
  int *if_has_next;
  int line_no;

  // how many tmp_file in output_dir_tmp: tmp_file_cnt;
  if( stat( output_dir_tmp, &st ) < 0 )
  {
    // not exist;
    fprintf( stderr, "Error: Cannot access wc-tmp-dir\n" );
    return -1;
  }
  if( ! S_ISDIR( st.st_mode ) )
  {
    // not dir
    fprintf( stderr, "Error: Not dir: %s\n", output_dir_tmp );
    return -1;
  }

  ilen = strlen( output_dir_tmp );
  ilen += MY_FILENAME_MAX +1;
  ifpath = (char *) malloc( sizeof(char) * ilen );
  if( ifpath == NULL )
  {
    fprintf( stderr, "Error: malloc(): no memory\n" );
    return -1;
  }
  ifpath[0] = 0;

  if( (dir = opendir( output_dir_tmp )) == NULL )
  {
    fprintf( stderr, "Error: opendir( %s )\n", output_dir_tmp );
    free( ifpath );
    return -1;
  }
  tmp_file_cnt = 0;
  while( (fi = readdir( dir )) != NULL )
  {
    snprintf( ifpath, ilen, "%s/%s", output_dir_tmp, fi->d_name );
    if( fi->d_name[0] == '.' )
    {
      // ignore: hidden file, pwd, parent dir;
      continue;
    }
    if( stat( ifpath, &st ) < 0 )
    {
      // not exist;
      continue;
    }
    if( S_ISREG( st.st_mode ) )
    {
      ++ tmp_file_cnt;
    }
  }
  closedir( dir );
#ifdef DEBUG_TEST
  printf( "tmp_file_cnt: %d\n", tmp_file_cnt );
#endif

  ifp_arr = (FILE **)malloc( sizeof(FILE *) * tmp_file_cnt );
  if( ifp_arr == NULL )
  {
    free( ifpath );
    return -1;
  }
  if_has_next = (int *)malloc( sizeof(int) * tmp_file_cnt );
  if( if_has_next == NULL )
  {
    free( ifp_arr );
    free( ifpath );
    return -1;
  }

  i = 0;
  dir = opendir( output_dir_tmp );
  while( (fi = readdir( dir )) != NULL )
  {
    snprintf( ifpath, ilen, "%s/%s", output_dir_tmp, fi->d_name );
    if( fi->d_name[0] == '.' )
    {
      // ignore: hidden file, pwd, parent dir;
      continue;
    }
    if( stat( ifpath, &st ) < 0 )
    {
      // not exist;
      continue;
    }
    if( S_ISREG( st.st_mode ) )
    {
      ifp_arr[i] = fopen( ifpath, "r" );
      if( ifp_arr[i] != NULL ) ++i;
      else
      {
        fprintf( stderr, "Wordcount tmp file: fopen( %s ): failure.", ifpath );
      }
    }
  }
  closedir( dir );
  tmp_file_cnt = i;
#ifdef DEBUG_TEST
  printf( "ifp_arr_cnt: %d\n", tmp_file_cnt );
#endif

  line_no = 0;
  merge_init( ifp_arr, if_has_next, tmp_file_cnt, line, msa_merge_buf, mia, ms, ms_one_word, &line_no );
  merge_do_merge( ifp_arr, if_has_next, tmp_file_cnt, line, msa_merge_buf, mia, ms, ms_one_word, min_ms, &line_no, output_f );

  free( if_has_next );
  for( i=0; i<tmp_file_cnt; ++i ) fclose( ifp_arr[i] );
  free( ifp_arr );
  free( ifpath );
  return 0;
}


// file line --> msa[i] and mia[i] k-v;
static void merge_init( FILE **ifp_arr, int *if_has_next, int if_cnt, char *line, MyStringArr *msa, MyIntArr *mia, MyString *ms, MyString *ms_one_word, int *line_no )
{
  int i;

  clearMyStringArr( msa );
  for( i=0; i<if_cnt; ++i ) if_has_next[i] = 1;
  for( i=0; i<if_cnt; ++i )
  {
    // first line of one file;
    read_one_line_to_msa( ifp_arr[i], if_has_next+i, line, msa, mia, ms, ms_one_word, line_no );
    // ifp_arr[i], if_has_next[i], msa->arr[i] for the same file;
  }
}


static void read_one_line_to_msa( FILE *ifp, int *if_has_next, char *line, MyStringArr *msa, MyIntArr *mia, MyString *ms, MyString *ms_one_word, int *line_no )
{
  int ret;
  int cnt;

  ret = read_one_line( ifp, line, ms, line_no );
  if( ms->data[0] != 0 || ret != 0 ) *if_has_next = 1;
  else *if_has_next = 0;
  // push one ms for one file, whether there is data in ms or not;
  clearMyString( ms_one_word );
  cnt = 0;
  if( ms->data[0] != 0 ) lineToWCKV( ms, ms_one_word, &cnt );
  pushMyStringArr( msa, ms_one_word );
  pushMyIntArr( mia, cnt );
  clearMyString( ms );
}


static int read_one_line( FILE *ifp, char *line, MyString *ms, int *line_no ) // 0:eof; 1:not eof;
{
  char *s_res;
  const int line_size = MY_LINE_MAX + 1;
  int lline;
  int ret;

  clearMyString( ms );
  while( 1 )
  {
    s_res = fgets( line, line_size, ifp );
    if( s_res == NULL )
    {
      if( ms->data[0] != 0 )
      {
        // line in ms; // no eol for last line in file;
        ++ (*line_no);
      }
      ret = 0;
      break; // eof;
    }
    lline = strlen( line );
    if( line[lline-1] == '\n' )
    {
      // eol;
      ++ (*line_no);
      //if( (*line_no) % 10000 == 0 ) printf( "reading: line_no:%d\n", *line_no );
      appendnMyString( ms, line, lline-1 );
      ret = 1;
      break;
    }
    else
    {
      appendnMyString( ms, line, lline );
    }
  }
  return ret;
}


// file line --> msa[i] and mia[i] k-v; 
// min_ms==min(msa[i]) ? min_cnt+=mia[i],read_next(i) : write(min_ms,min_cnt),min_ms=min(msa[i]),min_cnt=mia[i],read_next(i);
//  or
//    for( i=0; i<if_cnt; ++i ) 
//      if( msa[i]==min_ms ){
//        min_cnt += mia[i];
//        read_next( i );
//      } (now)
// no need for msa_output_buf;
static void merge_do_merge( FILE **ifp_arr, int *if_has_next, int if_cnt, char *line, MyStringArr *msa_merge_buf, MyIntArr *mia, MyString *ms, MyString *ms_one_word, MyString *min_ms, int *line_no, const char *output_f )
{
  int i;
  int min_ms_i;
  int min_cnt;
  FILE *ofp;
  int end_flag;

  ofp = fopen( output_f, "w" );
  if( ofp == NULL )
  {
    fprintf( stderr, "Error: fopen( %s ): failure.\n", output_f );
    return;
  }

  // init: min_ms = min(msa_merge_buf[i]); min_cnt = mia[i];
  // init min_ms_i;
  for( i=0; i<if_cnt; ++i )
  {
    if( if_has_next[i] != 0 ) break;
  }
  if( i < if_cnt ) min_ms_i = i;
  else
  {
    // no input word;
    fclose( ofp );
    return;
  }
  ++ i;
  for( ; i<if_cnt; ++i )
  {
    if( if_has_next[i] == 0 ) continue;
    // cmp 
    if( cmpMyString( &(msa_merge_buf->arr[i]), &(msa_merge_buf->arr[min_ms_i]) ) < 0 )
      min_ms_i = i;
  }
  cpyMyString( min_ms, msa_merge_buf->arr+min_ms_i );
  min_cnt = mia->arr[min_ms_i];
  read_next_line( ifp_arr[min_ms_i], if_has_next+min_ms_i, line, ms, msa_merge_buf->arr+min_ms_i, mia->arr+min_ms_i, line_no );

  while( 1 )
  {
    end_flag = 1;
    for( i=0; i<if_cnt; ++i )
    {
      if( if_has_next[i] == 0 ) continue;
      end_flag = 0;
      if( cmpMyString( msa_merge_buf->arr+i, min_ms ) == 0 )
      {
        min_cnt += mia->arr[i];
        read_next_line( ifp_arr[i], if_has_next+i, line, ms, msa_merge_buf->arr+i, mia->arr+i, line_no );
      }
    }
    if( end_flag ) break;

    // write min_ms, min_cnt;
    displayWCKVToFile( ofp, min_ms, min_cnt );
    clearMyString( min_ms );

    // init: min_ms = min(msa_merge_buf[i]); min_cnt = mia[i];
    // init min_ms_i;
    for( i=0; i<if_cnt; ++i )
    {
      if( if_has_next[i] != 0 ) break;
    }
    if( i < if_cnt ) min_ms_i = i;
    else break; // end_flag=1;
    ++ i;
    for( ; i<if_cnt; ++i )
    {
      if( if_has_next[i] == 0 ) continue;
      // cmp 
      if( cmpMyString( &(msa_merge_buf->arr[i]), &(msa_merge_buf->arr[min_ms_i]) ) < 0 )
        min_ms_i = i;
    }
    cpyMyString( min_ms, msa_merge_buf->arr+min_ms_i );
    min_cnt = mia->arr[min_ms_i];
    read_next_line( ifp_arr[min_ms_i], if_has_next+min_ms_i, line, ms, msa_merge_buf->arr+min_ms_i, mia->arr+min_ms_i, line_no );
  }
  // last one;
  if( min_ms->data[0] != 0 ) displayWCKVToFile( ofp, min_ms, min_cnt );

  fclose( ofp );
}


static void read_next_line( FILE* ifp, int *if_has_next, char *line, MyString *ms, MyString *ms_one_word, int *cnt, int *line_no )
{
  int ret;

  clearMyString( ms );
  ret = read_one_line( ifp, line, ms, line_no );
  if( ms->data[0] != 0 || ret != 0 ) *if_has_next = 1;
  else *if_has_next = 0;

  // push one ms for one file, whether there is data in ms or not;
  clearMyString( ms_one_word );
  if( ms->data[0] != 0 ) lineToWCKV( ms, ms_one_word, cnt );
  clearMyString( ms );
}


// one file line -->ms; ms = word + cnt;
static void lineToWCKV( MyString *ms, MyString *ms_one_word, int *cnt )
{
  cpyMyString( ms_one_word, ms );
  sscanf( ms->data, "%s %d", ms_one_word->data, cnt );
}


static inline void displayWCKVToFile( FILE *ofp, MyString *ms, int cnt )
{
  fprintf( ofp, "%s %d\n", ms->data, cnt );
}

