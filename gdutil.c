#include "gendl.h"
#include <sys/types.h>
#include <sys/time.h>
#include <time.h>

static int microsec;
static int microsec_t;

void gd_saveidhost()
{
  char path[132];
  FILE *file;
  char name[80];

  sprintf(path,"%s/%s-id",gdrecptr->root, gdrecptr->save_prefix);
  file = fopen(path,"w");
  if( file != NULL ) {
	gethostname(name,80);
	fprintf(file,"%d %s\n",getpid(),name);
	fclose(file);
  } /* else part: ??? */
}
  

/*
int elapsetime_t( val )
int val;
{
  struct timeval tp;
  struct timezone tzp;

  gettimeofday(&tp,&tzp);
  if( !val )
        return (microsec_t = 1000000*tp.tv_sec + tp.tv_usec );
  else
        return (1000000*tp.tv_sec + tp.tv_usec - microsec_t);
}

int elapsetime( val )
int val;
{
  struct timeval tp;
  struct timezone tzp;

  gettimeofday(&tp,&tzp);
  if( !val )
        return (microsec = 1000000*tp.tv_sec + tp.tv_usec );
  else
        return (1000000*tp.tv_sec + tp.tv_usec - microsec);
}
*/


void dropnewline(line)
char *line;
{
  char *pnt;

  pnt = line ;
  while( *pnt )
  {
        if( *pnt == '\n' )
        {
                *pnt = 0;
                break;
        }
        ++pnt;
  }
}

char *utimestring_local()
{
  time_t i;
  static char tstr[32];

  i = time((time_t *)0);
  strcpy( tstr, ctime( (const time_t *)&i ) );
  dropnewline( tstr );
  return tstr;
}

void gd_getline( file, sline )
FILE *file;
char *sline;
{
  fgets( sline, 320, file );
  while( sline[0]==';' || sline[0] =='/' || sline[0]=='*' || sline[0]=='#' )
        fgets( sline, 320, file );
}

void gd_log(str, timeflag)
char *str;
int timeflag;
{
  FILE *logfile;

  if( (logfile = fopen( gdrecptr->abslogfile, "a") ) != NULL ) {
	if( timeflag ) {
        	fprintf(logfile,"%s: %s\n", utimestring_local(), str);
/*
        	fprintf(stderr,"LOG: %s: %s\n", utimestring_local(), str);
*/
	} else {
        	fprintf(logfile,"%s\n", str);
/*
        	fprintf(stderr,"LOG: %s\n", str);
*/
	}
        fclose(logfile);
  }
}


int gd_realloc( ptr, size )
void **ptr;
size_t size;
{
  char sline[128];
  *ptr = realloc(*ptr,size);
  if( *ptr == NULL ) {
	sprintf(sline, "Memory allocation (%d bytes) error",(int)size);
	gd_log(sline,1);
	return -1;
  } else return 0;
}


int gd_malloc( ptr, size )
void **ptr;
size_t size;
{
  char sline[128];
  *ptr = malloc(size);
  if( *ptr == NULL ) {
	sprintf(sline, "Memory allocation (%d bytes) error",(int)size);
	gd_log(sline,1);
	return -1;
  } else return 0;
}

