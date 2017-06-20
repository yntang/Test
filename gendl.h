#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <cadef.h>
#include <epicsStdlib.h>
#include <epicsString.h>
#include <epicsGetopt.h>
#include <epicsTime.h>
/*
#include <mclib.h>
#include <his_def.h>
*/

/*
#define GD_DEB 1
#define GD_SIM 1
*/

#define TYPE_FLOAT 1
#define REG_FILE 1

#define PC_MSGSIZE 132
#define TIMETEXTLEN 40
#define MAX_READPVS_ERRORS 10

#ifndef PV_NAMELEN
#define PV_NAMELEN 40
typedef char PV_NAME[PV_NAMELEN];
#endif

extern int errno;
extern char *strerror();
extern char *strcpy();
char *utimestring_local();

/* --------------------------------------------------------------------- */
#ifdef REFERENCE
typedef struct
{
  PV_NAME name;
  int nelems, value_size; /* tsize in bytes of value*/
  int created;
  int connected;
  int error;
  chid chid;
  int dbfType;
  int dbrType;
  int status;
  void *value;
  epicsTimeStamp ts;
  char mess[132];
} EPICS_PV; /* EPICS System PV */

typedef struct epicsTimeStamp {
    epicsUInt32    secPastEpoch;   /* seconds since 0000 Jan 1, 1990 */
    epicsUInt32    nsec;           /* nanoseconds within second */
} 
#endif
/* --------------------------------------------------------------------- */

#define GD_ROOT "/WFdata"
#define GD_LOGDIR "Log"

typedef struct {
	char item1_4[4];
	char item2_4[4];
} B4B4;

typedef struct {
	char item1_40[40];
	char item2_40[40];
	char item3_4[4];
	char item4_4[4];
} B40B40B4B4;;

typedef struct {
	char item1_40[40];
	char item3_4[4];
	char item4_4[4];
} B40B4B4;;

#ifdef GDMAIN
#define GD_EXTERN
#else
#define GD_EXTERN extern
#endif

char *utimestring();
int gd_init();
void gd_getline();
void gd_log();
int gd_realloc();
int gd_connectpv();
int gd_readpvs();
int gd_readpvs_dbrtype();
int gd_saveegroups();
int gd_readtimestamp();
int gd_readaddr();
int gd_readegroupvals();

#define MAX_CELLS 30
#define MAX_PART_LEN PV_NAMELEN/2
#define MAX_CELL_LEN 4
#define MAX_TYPE_LEN 8

#define GRP_STRUCTURED 0
#define GRP_PLAIN 1

#define GD_DETECTED 0
#define GD_NORMAL 1

#define TS_LENGTH 40

typedef struct {
	PV_NAME name;
	int nelems, value_size; 
	int err;
	int created, connected;
	chid chid;
	int allocated;
	int dbrtype;
	char *dbr_value;
} GD_PV;

typedef char TS_STR[TS_LENGTH];
typedef char GRP_NAME[16];
#define GD_MAX_GROUPS 20
#define GD_MAX_SUBGS 8
#define GD_PLAIN 0
#define GD_STRUCT 1
typedef struct {
	GRP_NAME name;
	int type;
	char subg_pat[16];
	int nsubgroups;
	int subg_start[GD_MAX_SUBGS];
	int subg_end[GD_MAX_SUBGS];
	GRP_NAME subg_name[GD_MAX_SUBGS];
} GD_GROUP;

typedef struct {
	GRP_NAME name;
	int nsubgroups;
	int subg_start;
	int subg_end;
	int err;
	GRP_NAME subg_name[2]; /* a egroup has 2 and only 2 subgroups: PV names and values */
	int npvs, wf_flag;
	int nelems;
	GD_PV *pvs;
	int dbrtype;
	char *values;
} GD_EGROUP;

GD_EXTERN GD_GROUP *gd_groups;
GD_EXTERN GD_EGROUP *gd_egroups;
/*
GD_EXTERN GD_GROUP gd_groups[GD_MAX_GROUPS];
GD_EXTERN GD_EGROUP gd_egroups[GD_MAX_GROUPS];
*/

#define NUM_H5_RANK 3

#define GD_MAXTRIGGERS 32
#define GD_TRIGGER_OR 1
#define GD_TRIGGER_AND 2
#define GD_TRIGGER_VALUE 1
#define GD_TRIGGER_CHANGE 2
#define GD_TRIGGER_LESS 3
#define GD_TRIGGER_MORE 4
#define GD_INT 1
#define GD_FLOAT 2
#define GD_STRING 3
#define GD_TS 4

typedef struct {
	double connect_timeout, read_timeout, write_timeout;
	float version;
	int err_flag;
	int stand_alone,async_flag,test_flag,in_dl,menu_flag;
	int large_attr_flag;
	int in_fault;
	int secs,msecs;
	useconds_t usecs;
	int pause; /* in milliseconds */
	int triglog_interval, triglog_nreadings;
	int ncells, ntotal_readings;
	int npvs, npvs_per_cell, npvs_per_read;
	int nlayers, ntopgroups, ntopegroups;
	int max_readpvs_errors;
	int naddrs;
	int nelems_per_pv;
	int nbytes_per_pv;
	int nbytes_per_pvt;
	int nbytes_per_set;
	char str_dbrtype[16];
	char save_prefix[16];
	TS_STR time_stamp[GD_MAXTRIGGERS]; /* example: 20150323-16:02:29.857491 */
	TS_STR old_time_stamp[GD_MAXTRIGGERS]; /* example: 20150323-16:02:29.857491 */
	int ts_change_index;
	int priority;
	int dbrtype;
	int h5_rank, h5_dim[NUM_H5_RANK];
	int ntriggers;
	int trigger_or_flag, value_type, trigger_ts_ind;
	int trigger_type, ndetected;
	float trigger_thresh[GD_MAXTRIGGERS];
	GD_PV trigger_pvs[GD_MAXTRIGGERS];
	int trigger_types[GD_MAXTRIGGERS];
        int trigger_values[GD_MAXTRIGGERS];
        int trigger_rtvalues[GD_MAXTRIGGERS];
        float trigger_frtvalues[GD_MAXTRIGGERS];
        int trigger_detected[GD_MAXTRIGGERS];

	int pre_cond_flag;
	GD_PV pre_cond_pv;
	int pre_cond_value;

	int need_reset;
	char reset_func[132];
	GD_PV reset_pv;
	int reset_value;
	int need_init;
	char init_func[132];
	GD_PV init_pv;
	int init_value;
	int lreset_flag, lreset_critical;
	char lreset_func[132];
	char lreset_parfile[32];
	int lreset_npvs;
	long max_mem;
	int tmval_size;

	char host[64];
	char root[80]; /* Any directory */
	char datadir[64];
	char datapath[132]; /* directory including root */
	char parfile[64]; /* parfile is in the datapath */
	char logfile[64]; /* specified in parameter file */
	char logdir[64]; /* root + Log */
	char abslogfile[132];  /* root + logdir + logfile  */
	char timepath[132];
	char spare[1024];
} GDREC;

GD_EXTERN GDREC gdrec, *gdrecptr;

#ifdef TYPE_DOUBLE
#define SAVE_DTYPE double
#define SAVE_DBRTYPE DBR_DOUBLE
#endif

#ifdef TYPE_FLOAT
#define SAVE_DTYPE float
#define SAVE_DBRTYPE DBR_FLOAT
#endif

/* TYPE_TIME_DOUBLE and TYPE_TIME_FLOAT have not beem implemented.
   It will be implemented if needed.
*/
/* Questions:
   1. Do we need to save the time stamps for every BPM?
   2. Why does the status PV store the current time as time stamp, not the time stamp when
      detected?
*/
#ifdef TYPE_TIME_DOUBLE
#define SAVE_DTYPE double
#define SAVE_DBRTYPE DBR_TIME_DOUBLE
#endif

#ifdef TYPE_TIME_FLOAT
#define SAVE_DTYPE float
#define SAVE_DBRTYPE DBR_TIME_FLOAT
#endif

GD_EXTERN GD_PV *gd_pvs;
GD_EXTERN GD_PV *gd_addrpvs;
GD_EXTERN GD_PV *lreset_pvs;
GD_EXTERN short *lreset_values;

GD_EXTERN SAVE_DTYPE **h5_data, ***h5_data3;
GD_EXTERN struct dbr_time_float *timefloat;
GD_EXTERN TS_STR *pvts;
GD_EXTERN int *addr;

GD_EXTERN B4B4 meta;
GD_EXTERN B40B40B4B4 *trigs;
