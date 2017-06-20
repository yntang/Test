#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/stat.h>
#include <setjmp.h>
#include <unistd.h>
#include <dirent.h>
#include <db_access.h>

jmp_buf begin;
static int sigcount;
static int loopcount;

int gd_savewf();
/*
int elapsetime_t();
*/
void gd_saveidhost();
int gd_readstatus();
int gd_local_reset();
int gd_readmultistatus();
pid_t wait();

int first_read_status = 1;
int first_read_wf=1;

static int nreadpvs_errors = 0;

#define GDMAIN
#include "gendl.h"

void gd_restart()
{
  char sline[300], sub[300];

  sprintf(sline,"nohup /WFdata/bin/gendl_v5.4 -root %s ",gdrecptr->root);
  sprintf(sub,"-datadir %s -parfile %s -stand_alone Y &",gdrecptr->datadir,gdrecptr->parfile);
  strcat(sline,sub);
  sprintf(sub,"Start: %s\n",sline);
  gd_log(sub);
  system(sline);
}

/*
void gd_distriggers()
{
  int i;
  char sline[80];

  for( i = 0; i<gdrecptr->ntriggers; i++) {
	sprintf(sline,"%s: %d",gdrecptr->trigger_pvs[i].name, (int)(gdrecptr->trigger_rtvalues[i]));
	gd_log(sline,0);
  }
}
*/

int gd_checkts()
{
  int i;
  int changed = 0;
  char sline[256];

#ifdef GD_DEB
  fprintf(stderr,"Enter gd_checkts, ntriggers: %d\n",gdrecptr->ntriggers);
#endif

  for(i=0;i<gdrecptr->ntriggers;i++) {
	if( gdrecptr->trigger_detected[i] == 1 ) {
/*
#ifdef GD_DEB
		fprintf(stderr,"trigger %02d detected: %s %s\n",i,
			gdrecptr->time_stamp[i], gdrecptr->old_time_stamp[i]);
#endif
		sprintf(sline,"%s: value %d at %s",
			gdrecptr->trigger_pvs[i].name, (float)(gdrecptr->trigger_rtvalues[i]),
			gdrecptr->time_stamp[i]);
		gd_log(sline,0);
*/

		if( strcmp(gdrecptr->time_stamp[i], gdrecptr->old_time_stamp[i]) ) {
			changed = 1;
			gdrecptr->ts_change_index = i;
			break;
		}
	}
  }
  if( changed ) {
  	for(i=0;i<gdrecptr->ntriggers;i++) {
		strcpy(gdrecptr->old_time_stamp[i], gdrecptr->time_stamp[i]);
  	}
  }

#ifdef GD_DEB
  fprintf(stderr,"Enter gd_checkts, changed: %d\n",changed);
#endif
  return changed;
}
  

void gd_dlwf()
{
  int res;
  if( gdrecptr->in_dl == 1 ) return;
  gdrecptr->in_dl = 1;
  if( first_read_wf == 1 ) {
	res = gd_savewf(0);
	if(res==0) first_read_wf = 0;
  } else {
	res = gd_savewf(1);
  }
  gdrecptr->in_dl = 0;
}

void gd_saveinfo()
{
}

void gd_onsig(sig)
int sig;
{
  char sline[128];

  signal(sig,gd_onsig);
  if( sigcount % 20 == 0 ) {
	sprintf(sline,"!!!  Signal %d generated and jump back\n", sig );
	gd_log(sline,0);
  }
  ++sigcount;
  if( sigcount >= 20 ) {
	sprintf(sline,"!!!  Too many signals, process exits\n");
	gd_log(sline,0);
	sleep(1);
	exit(1);
  }
  sleep(1);
  if( gdrecptr->secs > 0 ) sleep( gdrecptr->secs);
  longjmp(begin,0);
}

/*
void gd_onsig(sig)
int sig;
{
  char sline[128];

  signal(sig,gd_onsig);
  if( sigcount % 20 == 0 ) {
	sprintf(sline,"At time %s 20 signals %d generated and jump back\n", utimestring_local(),sig );
	gd_log(sline,0);
  }
  ++sigcount;
  sleep(1);
  if( gdrecptr->secs > 0 ) sleep( gdrecptr->secs);
  longjmp(begin,0);
}
*/

void gd_onchld(sig)
int sig;
{
  ;
/*
  int etime;
  etime = elapsetime_t(1);
  tfile = fopen(gdrecptr->timepath,"a");
  fprintf(stderr,"Total read and save time (in child): %.5f secs\n", etime/1000000.0);
  if( tfile != NULL ) {
	fprintf(tfile,"Total read and save time (in child): %.5f secs\n", etime/1000000.0);
	fclose(tfile);
  }
*/
}

char *ns2dlwf_root()
{
  char *ret_val;

  ret_val = getenv("NS2_DLWF");
  if( ret_val != NULL ) return ret_val;
  return (char *)NULL;
}


int main(argc,argv)
int argc;
char *argv[];
{
  int i,iarg,res,msecs;
  int idir, ipar;
  int status, download_flag;
  pid_t done;
  char *ch, sline[240];
  TS_STR ts;
/*
  int etime;
  FILE *tfile;
*/

  fprintf(stderr,"\n\n\n --------------------- * ------------------------\n\n");
  
  idir = ipar = 0;
  for(i=0;i<argc;i++) {
	if( !strcmp(argv[i],"-datadir") ) idir = 1;
	if( !strcmp(argv[i],"-parfile") ) ipar = 1;
  }
  if( idir == 0 || ipar == 0 ) {
	printf("\nMust specify both data directory and parameter file on the command line!\n");
	printf("Usage: %s -datadir xxxx -parfile xxxx\n\n", argv[0]);
	exit(1);
  }
  gdrecptr = &gdrec;
  gdrecptr->stand_alone = gdrecptr->test_flag = 0;
  strcpy(gdrecptr->root, GD_ROOT);
  strcpy( gdrecptr->logdir, GD_LOGDIR);
  strcpy( gdrecptr->time_stamp[0], "20150421-10:32:33.480582" ); /* Initial value */
  gdrecptr->npvs_per_read = 1;
  gdrecptr->large_attr_flag = 0;
  ch = ns2dlwf_root();
  if( ch == NULL )
	strcpy( gdrecptr->root, GD_ROOT);
  else
	strcpy( gdrecptr->root, ch );

  gdrecptr->max_readpvs_errors = MAX_READPVS_ERRORS;

  iarg = 1;

  if( argc > 3 ) {
	while (iarg < argc - 1) {
		if( !strcmp(argv[iarg], "-root") ) {
			strcpy(gdrecptr->root, argv[iarg+1]);
		}
		if( !strcmp(argv[iarg], "-datadir") ) {
			strcpy(gdrecptr->datadir, argv[iarg+1]);
		}
		if( !strcmp(argv[iarg], "-parfile") ) {
			strcpy(gdrecptr->parfile, argv[iarg+1]);
		}
		if( !strcmp(argv[iarg], "-sleeptime") ) {
			msecs = atoi(argv[iarg+1]);
			gdrecptr->secs = msecs / 1000;
			gdrecptr->usecs = (msecs % 1000) * 1000;
		}
		if( !strcmp(argv[iarg], "-npvs_per_read") ) {
			gdrecptr->npvs_per_read = atoi(argv[iarg+1]);
		}
		if( !strcmp(argv[iarg], "-stand_alone") ) {
			if( argv[iarg+1][0] == 'y' || argv[iarg+1][0] == 'Y' )
				gdrecptr->stand_alone = 1;
			else
				gdrecptr->stand_alone = 0;
		}
		if( !strcmp(argv[iarg], "-menu_flag") ) {
			if( argv[iarg+1][0] == 'y' || argv[iarg+1][0] == 'Y' )
				gdrecptr->menu_flag = 1;
			else
				gdrecptr->menu_flag = 0;
		}
		if( !strcmp(argv[iarg], "-test") ) {
			if( argv[iarg+1][0] == 'y' || argv[iarg+1][0] == 'Y' )
				gdrecptr->test_flag = 1;
			else
				gdrecptr->test_flag = 0;
		}
		if( !strcmp(argv[iarg], "-timestamp") || !strcmp(argv[iarg], "-ts") ) {
			strcpy(gdrecptr->time_stamp[0], argv[iarg+1]);
		}
		iarg += 2;
	}
  }

  sprintf( gdrecptr->datapath, "%s/%s", gdrecptr->root, gdrecptr->datadir);
  fprintf( stderr,"---------------------- Par  file: %s\n",gdrecptr->parfile);
  sprintf( gdrecptr->abslogfile, "%s/%s/%s.log", gdrecptr->root, gdrecptr->logdir, gdrecptr->parfile);

  fprintf( stderr,"---------------------- Log  file: %s\n",gdrecptr->abslogfile);


  if( gdrecptr->menu_flag ) {
	printf("\033[1;31mDo you like to proceed to download waveforms? (Y/N):\033[0m ");
	i = getchar();
	if( i == 'N' || i == 'n' ) exit(0);
	getchar();
  }

  gdrecptr->triglog_interval = 180;
  gdrecptr->connect_timeout = gdrecptr->read_timeout = gdrecptr->write_timeout = 2.0;
  gd_init();

  sprintf(sline,"At %s: %s started\n", utimestring_local(), "gendl_6.0");
  gd_log( sline, 0 );

  sprintf(sline,"     root: %s",gdrecptr->root);
  gd_log( sline, 0 );
  sprintf(sline,"Par  file: %s",gdrecptr->parfile);
  gd_log( sline, 0 );
  sprintf(sline,"Data path: %s",gdrecptr->datapath);
  gd_log( sline, 0 );
  sprintf(sline,"Log  path: %s",gdrecptr->abslogfile);
  gd_log( sline, 0 );
  sprintf(sline,"Log  Freq: %d",gdrecptr->triglog_nreadings);
  gd_log( sline, 0 );
  sprintf(sline,"Tot  PVs : %d",gdrecptr->npvs);
  gd_log( sline, 0 );
  sprintf(sline,"Tot bytes: %d",gdrecptr->nbytes_per_set);
  gd_log( sline, 0 );

#ifdef GD_DEB
  fprintf(stderr,"     root: %s\n",gdrecptr->root);
  fprintf(stderr,"Par  file: %s\n",gdrecptr->parfile);
  fprintf(stderr,"Data path: %s\n",gdrecptr->datapath);
  fprintf(stderr,"Log  path: %s\n",gdrecptr->abslogfile);
#endif

#ifdef GD_SHM
  gd_saveidhost();
  signal( SIGUSR1,gd_dlwf);
  signal( SIGUSR2,gd_saveinfo);
#endif

/*
  signal( SIGINT,gd_onsig);
*/
  signal( SIGILL,gd_onsig);
  signal( SIGFPE,gd_onsig);
  signal( SIGBUS,gd_onsig);
  signal( SIGSEGV,gd_onsig);

/*
  signal( SIGCHLD,gd_onchld);
  sprintf(gdrecptr->timepath,"%s/%s_Timing.dat",gdrecptr->datapath, gdrecptr->save_prefix);
*/
  sprintf(gdrecptr->timepath,"%s_Timing.dat",gdrecptr->save_prefix);

/*
  tfile = fopen(gdrecptr->timepath,"a");
  if( tfile != NULL ) {
	gethostname(gdrecptr->host,64);
	fprintf(tfile,"\n\n**** Sync reading WFs     : %s on %s\n",utimestring_local(),gdrecptr->host);
	fprintf(tfile,"The root directory        : %s\n",gdrecptr->root);
	fprintf(tfile,"The data directory        : %s\n",gdrecptr->datapath);
	fprintf(tfile,"The input file            : %s\n",gdrecptr->parfile);
	fprintf(tfile,"Total number of PVs       : %d\n",gdrecptr->npvs);
	fprintf(tfile,"Total number of bytes     : %d\n",gdrecptr->nbytes_per_set);
	fprintf(tfile,"Number of PVs per read    : %d\n",gdrecptr->npvs_per_read);
	fclose(tfile);
  }
*/

  if( gdrecptr->need_init == 1 ) {
	if( strlen(gdrecptr->init_func) > 1 ) {
		system(gdrecptr->init_func);
	} else {
		sprintf(sline,"caput %s %d",
			gdrecptr->init_pv.name, gdrecptr->init_value);
		system(sline);
	}
  }

  if( gdrecptr->pre_cond_flag == 1 ) {
	res = gd_readprecondpv(&status, 0, 0);
	if( res == -1 ) {
		fprintf(stderr,"Error in reading %s\n",gdrecptr->pre_cond_pv.name);
	}
  }
  if( gdrecptr->stand_alone) {
	while(1) {
		if( gdrecptr->ntotal_readings % gdrecptr->triglog_nreadings == 0 ) {
			sprintf(sline,"\nAt %s total readings: %d",utimestring_local(), gdrecptr->ntotal_readings);
			gd_log(sline,0);
		}
		if( gdrecptr->pre_cond_flag == 1 ) {
			res = gd_readprecondpv(&status, 1, 1);
			if( res == -1 || status == gdrecptr->pre_cond_value ||
				gdrecptr->test_flag == 1 ) {
				sleep(3);
				continue;
			}
		}
		gdrecptr->err_flag = download_flag = 0;
		if( gdrecptr->version <= 2.00 ) {
			if( first_read_status) {
				res = gd_readstatus(DBR_TIME_INT, &status, ts, 0, 0);
				if( res == 0 ) {
					first_read_status = 0;
					strcpy(gdrecptr->time_stamp[0],ts); /* early version */
				}
			} else {
				res = gd_readstatus(DBR_TIME_INT, &status, ts, 1, 1);
				strcpy(gdrecptr->time_stamp[0],ts); /* early version */
			}
			if (res != 0 ) {
				gdrecptr->err_flag = 1;
			}
			if( status == gdrecptr->trigger_values[0] )
				download_flag = 1;
		} else if( gdrecptr->version <= 5.90) {
			if( first_read_status) {
				res = gd_readmultistatus(&download_flag, 0, 0);
				if( res == 0 ) {
					first_read_status = 0;
				}
			} else {
				res = gd_readmultistatus(&download_flag, 1, 1);
			}
			if (res != 0 ) {
				++nreadpvs_errors;
				if( nreadpvs_errors > gdrecptr->max_readpvs_errors ) {
					gd_restart();
					sleep(5);
					exit(1);
				}
				gdrecptr->err_flag = 1;
			}
		}
		if( gdrecptr->err_flag == 0 ) {
			++gdrecptr->ntotal_readings;
		}
		if( download_flag ) {
			sprintf(sline,"At %s: download_flag %d", utimestring_local(), download_flag);
			gd_log(sline,0);
		}
/*
		else {
			if( gdrecptr->ntotal_readings % gdrecptr->triglog_nreadings == 0 ) {
				sprintf(sline,"At %s",utimestring_local());
				gd_log(sline,0);
			}
		}
*/

		if( gdrecptr->err_flag == 0 ) {
		    if(gdrecptr->test_flag == 1 || (download_flag && gd_checkts() )) {
/*
			elapsetime_t(0);
*/
			gdrecptr->in_dl = 1;
			gd_log("Download waveforms",0);
			if( first_read_wf == 1 ) {
				res = gd_savewf(0);
				if(res==0) first_read_wf = 0;
			} else {
				res = gd_savewf(1);
			}
			if( res != 0 ) {
				++nreadpvs_errors;
				if( nreadpvs_errors > gdrecptr->max_readpvs_errors ) {
					gd_restart();
					sleep(5);
					exit(1);
				}
			}
			gdrecptr->in_dl = 0;
			if( gdrecptr->need_reset == 1 ) {
				if( strlen(gdrecptr->reset_func) > 1 ) {
					system(gdrecptr->reset_func);
				} else {
					sprintf(sline,"caput %s %d",
						gdrecptr->reset_pv.name, gdrecptr->reset_value);
					system(sline);
				}
			}
			if( gdrecptr->lreset_flag ) gd_local_reset(0);
		    }
		    if( gdrecptr->test_flag == 1 ) {
			while(1) {
				done = wait(&res);
				if( done == -1 ) {
					if( errno == ECHILD ) break;
				}
			}
			printf("Bye\n");
			system("date");
/*
			etime = elapsetime_t(1);
			tfile = fopen(gdrecptr->timepath,"a");
			if( tfile != NULL ) {
				fprintf(stderr,"Total read and save time (in parent): %.5f secs\n",
					etime/1000000.0);
				fprintf(tfile,"Total read and save time (in parent): %.5f secs\n",
					etime/1000000.0);
				fclose(tfile);
			}
*/
			if( gdrecptr->menu_flag ) {
				printf("\033[1;31mType <enter> to exit:\033[0m");
				i = getchar();
			}
			exit(0);
		    }
		    ++loopcount;
		}
		if( gdrecptr->secs > 0 ) 
			sleep(gdrecptr->secs);
		if( gdrecptr->usecs > 0 )
			usleep(gdrecptr->usecs);
	}
  } else {
	res = gd_savewf(0);
	if( gdrecptr->lreset_flag ) gd_local_reset(0);
	exit(0);
  }
}

