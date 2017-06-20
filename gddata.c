#include "gendl.h"
#include <db_access.h>
int gd_readpvs();

/*
#define GD_SIMULATION 1
*/

int gd_readstatus(dbrtype, status, ts, connected, allocated)
int dbrtype;
int *status;
TS_STR ts;
int connected, allocated;
{
  int res;
  epicsTimeStamp stamp;
  struct dbr_time_short *tint;
  char *timeFormatStr = "%Y%m%d-%H:%M:%S.%06f";

  res = gd_readpvs_dbrtype(1, &gdrecptr->trigger_pvs[0], dbrtype, connected, allocated);
  if( res == 0 ) {
        tint = (struct dbr_time_short *)(gdrecptr->trigger_pvs[0].dbr_value);
        *status = tint->value;
/*
        fprintf(stderr,"Status PV: %s value: %d\n",gdrecptr->trigger_pvs[0].name, *status);
*/
        stamp = (epicsTimeStamp)tint->stamp;
        epicsTimeToStrftime(ts, TS_LENGTH, timeFormatStr,&stamp);
        strcpy(gdrecptr->time_stamp[0], ts);
        return 0;
  } else return -1;
}

int gd_readprecondpv(value, connected, allocated) 
int *value;
int connected, allocated;
{ 
  int res;

  res = gd_readpvs_dbrtype(1, &gdrecptr->pre_cond_pv, DBR_LONG, connected, allocated);
  *value = *(long *)gdrecptr->pre_cond_pv.dbr_value;
  return res;
}

/*
int aw_readtimesecoff() 
{ 
  char sline[64];
  unsigned long sec, off;
  FILE *file;

  sprintf(sline,"caget %s %s",time_pv_sec.name,time_pv_off.name);
  file = popen(sline, "r");
  fscanf( file, "%s %d %s %d", sline, &sec ,sline, &off); 
  pclose(file);
  awrecptr->sec = sec;
  awrecptr->off = off;
  return 0;
}

int aw_readtimepv(ts, connected, allocated) 
TS_STR ts;
int connected, allocated;
{ 
  int res;

  res = aw_readpvs(1, &time_pv, connected, allocated);
  strcpy( (char *)ts, (char *)time_pv.dbr_value );
  return res;
}
*/


static char *old_trigger_values[GD_MAXTRIGGERS];
static int trigger_nelems[GD_MAXTRIGGERS];

void *gd_gettriggervalues(itrig)
int itrig;
{
  return (void *)old_trigger_values[itrig];
}


int gd_readmultistatus(dlflag, connected, allocated)
int *dlflag;
int connected, allocated;
{
  int i,iv,res;
  int vtype;
  epicsTimeStamp stamp;
  struct dbr_time_short *tint;
  struct dbr_time_float *tfloat;
  char *timeFormatStr = "%Y%m%d-%H:%M:%S.%06f";
  char sline[132];
  float rt, reg;
  float *fvalues, fval[GD_MAXTRIGGERS], *fptr, diff;
  short *ivalues,*iptr;
  GD_PV *pvs;

/*
#ifdef GD_DEB
  fprintf(stderr,"\n\n-----  Enter gd_readmultistatus\n");
#endif
*/
  if(gdrecptr->value_type == GD_INT) {
	vtype = DBR_TIME_SHORT;
  } else {
	vtype = DBR_TIME_FLOAT;
  }

  res = gd_readpvs_dbrtype(gdrecptr->ntriggers, gdrecptr->trigger_pvs, vtype, connected, allocated);

  if(res==0 && connected == 0 && gdrecptr->trigger_type == GD_TRIGGER_CHANGE) {
	pvs = gdrecptr->trigger_pvs;
	for(i=0;i<gdrecptr->ntriggers;i++) {
		trigger_nelems[i] = ca_element_count(pvs->chid);
		old_trigger_values[i] = (char *)malloc(sizeof(float) * trigger_nelems[i]);
		++pvs;
	}
  }
  if( res == 0 ) {
	gdrecptr->ndetected = 0;
	for(i=0;i<gdrecptr->ntriggers;i++) {
		gdrecptr->trigger_detected[i] = 0;
	}
	if( gdrecptr->trigger_type == GD_TRIGGER_VALUE ||
	    gdrecptr->trigger_type == GD_TRIGGER_LESS ||
	    gdrecptr->trigger_type == GD_TRIGGER_MORE) {
		for(i=0;i<gdrecptr->ntriggers;i++) {
			if( gdrecptr->value_type == GD_INT ) {
        			tint=(struct dbr_time_short *)(gdrecptr->trigger_pvs[i].dbr_value);
				
        			fval[i] = gdrecptr->trigger_rtvalues[i] = (int)(tint->value);
        			stamp = (epicsTimeStamp)tint->stamp;
#ifdef GD_DEB
        			fprintf(stderr,"%2d", gdrecptr->trigger_rtvalues[i]);
#endif
			} else {
        			tfloat = (struct dbr_time_float *)
					(gdrecptr->trigger_pvs[0].dbr_value);
        			fval[i] = gdrecptr->trigger_frtvalues[i] = (float)(tfloat->value);
					gdrecptr->trigger_rtvalues[i] = (int)(tfloat->value);
#ifdef GD_SIMULATION
				if( gdrecptr->ntotal_readings % 3 == 0 ) {
					gdrecptr->trigger_frtvalues[i] = fval[i] -= 100;
				}
#endif
        			stamp = (epicsTimeStamp)tfloat->stamp;

#ifdef GD_DEB
        			fprintf(stderr,"Trigger PV: %s value: %f\n\n",
					gdrecptr->trigger_pvs[i].name, fval[i]);
#endif
			}
        		epicsTimeToStrftime(gdrecptr->time_stamp[i],TS_LENGTH,timeFormatStr,&stamp);
			if(gdrecptr->trigger_type == GD_TRIGGER_VALUE) {
			    if((gdrecptr->value_type == GD_INT &&
			    gdrecptr->trigger_rtvalues[i] == gdrecptr->trigger_values[i]) ||
			    (gdrecptr->value_type == GD_FLOAT &&
			    fval[i] == gdrecptr->trigger_values[i])) {
				gdrecptr->trigger_detected[i] = 1;
				++gdrecptr->ndetected;
			    } else {
				gdrecptr->in_fault = 0;
			    }
			} else {
/*
			    if( gdrecptr->in_fault == 1 ) continue;
*/
			    rt = fval[i];
			    reg = gdrecptr->trigger_values[i];
			    if( gdrecptr->trigger_type == GD_TRIGGER_LESS ) {
				if( rt < reg ) {
					gdrecptr->trigger_detected[i] = 1;
                                	++gdrecptr->ndetected;
					sprintf(sline,"!!!Trigger reading: %.3f at %s\n",rt,
						gdrecptr->time_stamp[i]);
					gd_log(sline,1);
				} else {
					if(gdrecptr->ntotal_readings % gdrecptr->triglog_nreadings == 0) {
						sprintf(sline,"Trigger reading: %.3f at %s\n",rt,
							gdrecptr->time_stamp[i]);
						gd_log(sline,1);
					}
					
					gdrecptr->in_fault = 0;
				}
			    }
			    if( gdrecptr->trigger_type == GD_TRIGGER_MORE ) {
				if( rt > reg ) {
					gdrecptr->trigger_detected[i] = 1;
                                	++gdrecptr->ndetected;
				} else {
					gdrecptr->in_fault = 0;
				}
			    }
			}
		}
	}
	if( gdrecptr->trigger_type == GD_TRIGGER_CHANGE) {
		for(i=0;i<gdrecptr->ntriggers;i++) {
			if( gdrecptr->value_type == GD_INT ) {
        			tint=(struct dbr_time_short *)(gdrecptr->trigger_pvs[0].dbr_value);
        			ivalues=(short *)&(tint->value);
        			stamp = (epicsTimeStamp)tint->stamp;
				iptr = (short *)old_trigger_values[i];
				if( connected != 0 ) {
					for(iv=0;iv<trigger_nelems[i];iv++) {
						diff = *iptr - *ivalues;
						if( diff < 0 ) diff = -diff;
						if( diff > gdrecptr->trigger_thresh[i] ) {
							gdrecptr->trigger_detected[i] = 1;
							++gdrecptr->ndetected;
							break;
						}
						*iptr = *ivalues;
						++iptr;
						++ivalues;
					}
				}
        			ivalues=(short *)&(tint->value);
				iptr = (short *)old_trigger_values[i];
				for(iv=0;iv<trigger_nelems[i];iv++) {
					*iptr = *ivalues;
					++iptr;
					++ivalues;
				}
			} else {
        			tfloat=(struct dbr_time_float *)
					(gdrecptr->trigger_pvs[0].dbr_value);
        			fvalues=(float *)&(tfloat->value);
        			stamp = (epicsTimeStamp)tfloat->stamp;
				fptr = (float *)old_trigger_values[i];
				for(iv=0;iv<trigger_nelems[i];iv++) {
					diff = *fptr - *fvalues;
					if( diff < 0 ) diff = -diff;
/*
#ifdef GD_DEB
					if( iv < 20 ) {
						fprintf(stderr,"%8.3f %8.3f %8.3f\n",
							*fptr, *fvalues, diff);
					}
#endif
*/
					if( diff > gdrecptr->trigger_thresh[i] ) {
						gdrecptr->trigger_detected[i] = 1;
						++gdrecptr->ndetected;
						break;
					}
					*fptr = *fvalues;
					++fptr;
					++fvalues;
				}
        			fvalues=(float *)&(tfloat->value);
				fptr = (float *)old_trigger_values[i];
				for(iv=0;iv<trigger_nelems[i];iv++) {
					*fptr = *fvalues;
					++fptr;
					++fvalues;
				}
			}
        		epicsTimeToStrftime(gdrecptr->time_stamp[i],
				TS_LENGTH,timeFormatStr,&stamp);
		}
	}
	if( gdrecptr->trigger_or_flag ) {
		if( gdrecptr->ndetected > 0 ) {
			*dlflag = 1;
		} else {
			*dlflag = 0;
		}
	} else {
		if( gdrecptr->ndetected == gdrecptr->ntriggers ) {
			*dlflag = 1;
		} else {
			*dlflag = 0;
		}
	}
	if( *dlflag == 0 && gdrecptr->test_flag == 1 ) {
		*dlflag = 1;
		gdrecptr->ndetected = 1;
		gdrecptr->trigger_detected[gdrecptr->ntriggers-1] = 1;
	}
	return 0;
  } else {
	gdrecptr->err_flag = 1;
	return -1;
  }
  return 0;
}

