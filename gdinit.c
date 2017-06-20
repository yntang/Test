#include "gendl.h"
#include <errno.h>
#include <string.h>
int errno;
/*
#define PV_MAXGROUPS 4
#define PV_MAXLAYERS 4
#define PV_PARTLEN 40
*/

int tolower();

int gd_readgroupdefs(file)
FILE *file;
{
  int i, ig = 0;
  char sub[8],sline[132],sub4[16];

  while(ig < gdrecptr->ntopgroups) {
	gd_getline(file,sline);
	sscanf(sline,"%s",sub);
	if( !strcmp(sub,"group") ) {
		if( gdrecptr->version < 4.95 ) {
			sscanf(sline,"%s %s %d",sub,gd_groups[ig].name,&(gd_groups[ig].nsubgroups));
			gd_groups[ig].type = GD_PLAIN;
		} else {
			sscanf(sline,"%s %s %d %s %s",
				sub,gd_groups[ig].name,&(gd_groups[ig].nsubgroups),sub4, 
					gd_groups[ig].subg_pat);
			if( sub4[0] == 'S' || sub4[0] == 's' )
				gd_groups[ig].type = GD_STRUCT;
			else
				gd_groups[ig].type = GD_PLAIN;
		}
		if( gd_groups[ig].nsubgroups > 0 ) {
			for(i=0;i<gd_groups[ig].nsubgroups;i++) {
				gd_getline(file,sline);
				sscanf(sline,"%s %d %d",gd_groups[ig].subg_name[i],
					&(gd_groups[ig].subg_start[i]),
					&(gd_groups[ig].subg_end[i]));
			}
		}
		++ig;
	}
  }
  return 0;
}
	
int gd_readegroupdefs(file)
FILE *file;
{
  int i, ig = 0;
  char sub[8],sub1[8],sline[132];
  GD_PV *ppv;

#ifdef GD_DEB
  fprintf(stderr,"\n****  Reading %d Egroups\n",gdrecptr->ntopegroups);
#endif
  while(ig < gdrecptr->ntopegroups) {
	gd_getline(file,sline);
	sscanf(sline,"%s",sub);
	if( !strcmp(sub,"egroup") ) {
		sscanf(sline,"%s %s %d",sub, gd_egroups[ig].name,&(gd_egroups[ig].nsubgroups));
		if( gd_egroups[ig].nsubgroups < 2 ) {
			printf("Must have at least two subgroup of each egroup\n");
			exit(1);
		}
#ifdef GD_DEB
		fprintf(stderr,"    EGroup Name: %s Number of subgroups: %d\n",
			gd_egroups[ig].name,gd_egroups[ig].nsubgroups);
#endif 
		gd_getline(file,sline);
		sscanf(sline,"%s %d %s",gd_egroups[ig].subg_name[0],
			&(gd_egroups[ig].npvs), sub1);
#ifdef GD_DEB
		fprintf(stderr,"        Subg name: %s, number of PVs: %d\n",
			gd_egroups[ig].subg_name[0],gd_egroups[ig].npvs);
#endif
		if( gd_egroups[ig].npvs <= 0 ) {
			printf("No PVs in the egroup\n");
			exit(1);
		}
		switch (sub1[0]) {
			case 'i':
			case 'I':
				gd_egroups[ig].dbrtype = DBR_SHORT;
				gd_egroups[ig].wf_flag = 0;
				gd_egroups[ig].values =
					(char *)malloc(sizeof(short)*gd_egroups[ig].npvs);
#ifdef GD_DEB
				fprintf(stderr,"        Value type: short\n");
#endif
				break;
			case 'f':
			case 'F':
				gd_egroups[ig].dbrtype = DBR_FLOAT;
				gd_egroups[ig].wf_flag = 0;
				gd_egroups[ig].values =
					(char *)malloc(sizeof(float)*gd_egroups[ig].npvs);
#ifdef GD_DEB
				fprintf(stderr,"        Value type: float\n");
#endif
				break;
			case 'l':
			case 'L':
				gd_egroups[ig].dbrtype = DBR_LONG;
				gd_egroups[ig].wf_flag = 0;
				gd_egroups[ig].values =
					(char *)malloc(sizeof(int)*gd_egroups[ig].npvs);
#ifdef GD_DEB
				fprintf(stderr,"        Value type: int\n");
#endif
				break;
			case 's':
			case 'S':
				gd_egroups[ig].dbrtype = DBR_STRING;
				gd_egroups[ig].wf_flag = 0;
				gd_egroups[ig].values =
					(char *)malloc(TS_LENGTH*gd_egroups[ig].npvs);
#ifdef GD_DEB
				fprintf(stderr,"        Value type: Time string\n");
#endif
				break;
			case 'w':
			case 'W':
				gd_egroups[ig].dbrtype = DBR_FLOAT;
				gd_egroups[ig].wf_flag = 1;
				if( sub1[3] == 'f' || sub1[3] == 'F' ) {
					gd_egroups[ig].dbrtype = DBR_FLOAT;
					gd_egroups[ig].values =
						(char *)malloc(sizeof(float)*gd_egroups[ig].npvs);
				} else if( sub1[3] == 'd' || sub1[3] == 'D' ) {
					gd_egroups[ig].dbrtype = DBR_DOUBLE;
					gd_egroups[ig].values =
						(char *)malloc(sizeof(double)*gd_egroups[ig].npvs);
				}
				gd_egroups[ig].nelems = gd_egroups[ig].npvs;
				gd_egroups[ig].npvs = 1;
#ifdef GD_DEB
				fprintf(stderr,"        Value type: Float waveform\n");
				fprintf(stderr,"        Num Elems : %d\n",gd_egroups[ig].nelems);
#endif
				break;
		}
		ppv = gd_egroups[ig].pvs = (GD_PV *)malloc(gd_egroups[ig].npvs * sizeof(GD_PV));
		for(i=0;i<gd_egroups[ig].npvs;i++) {
			gd_getline(file,sline);
			sscanf(sline,"%s",ppv->name);
#ifdef GD_DEB
				fprintf(stderr,"            PV: %s\n", ppv->name);
#endif
			++ppv;
		}
		gd_getline(file,sline);
		sscanf(sline,"%s",gd_egroups[ig].subg_name[1]);
/*** AAA ***/
#ifdef GD_DEB
		fprintf(stderr,"        Subg name: %s\n", gd_egroups[ig].subg_name[1]);
#endif

	}
	++ig;
  }
  return 0;
}
	

int gd_getpar(file, line)
FILE *file;
char *line;
{
  int i,lent;
  int msecs;
  char sub[32], sub1[132], sub2[4], sub3[4], sline[320];

/*
#ifdef GD_DEB
  fprintf(stderr,"Get parameter from line %s\n",line);
#endif
*/
  sscanf(line,"%s",sub);
  lent = strlen(sub);
  for(i=0;i<lent;i++)
	sub[i] = tolower(sub[i]);

  if( !strcmp(sub,"version") ) {
	sscanf(line,"%s %f",sub, &(gdrecptr->version));
  }
  if( !strcmp(sub,"root") ) {
	sscanf(line,"%s %s",sub, gdrecptr->root);
  }
  if( !strcmp(sub,"datadir") ) {
	sscanf(line,"%s %s",sub, gdrecptr->datadir);
  }
  if( !strcmp(sub,"parfile") ) {
	sscanf(line,"%s %s",sub, gdrecptr->parfile);
  }
  if( !strcmp(sub,"save_prefix") ) {
	sscanf(line,"%s %s",sub, gdrecptr->save_prefix);
  }

  if( !strcmp(sub,"ncells") ) {
	sscanf(line,"%s %d",sub, &gdrecptr->ncells);
  }

  if( !strcmp(sub,"max_errors") ) {
	sscanf(line,"%s %d",sub, &gdrecptr->max_readpvs_errors);
  }

  if( !strcmp(sub,"ngroups") ) {
	sscanf(line,"%s %d",sub, &gdrecptr->ntopgroups);
	gd_groups = (GD_GROUP *)malloc(gdrecptr->ntopgroups*sizeof(GD_GROUP));
	gd_readgroupdefs(file);
  }
  if( !strcmp(sub,"negroups") ) {
	sscanf(line,"%s %d",sub, &gdrecptr->ntopegroups);
	gd_egroups = (GD_EGROUP *)malloc(gdrecptr->ntopegroups*sizeof(GD_GROUP));
	gd_readegroupdefs(file);
  }
  if( !strcmp(sub,"npvs") ) {
	sscanf(line,"%s %d",sub, &gdrecptr->npvs);
  }
  if( !strcmp(sub,"nrpvs") ) {
	sscanf(line,"%s %d",sub, &gdrecptr->npvs_per_read);
  }
  if( !strcmp(sub,"naddrs") ) {
	sscanf(line,"%s %d",sub, &gdrecptr->naddrs);
  }
  if( !strcmp(sub,"setmem") ) {
/*
	sscanf(line,"%s %ld",sub, &gdrecptr->max_mem);
	sprintf(sub1,"EPICS_CA_MAX_ARRAY_BYTES=%ld",gdrecptr->max_mem);
	putenv(sub1);
*/
	epicsEnvSet("EPICS_CA_MAX_ARRAY_BYTES","33554434");
  }
  if( !strcmp(sub,"pause") ) {
	sscanf(line,"%s %d",sub, &gdrecptr->pause);
	gdrecptr->secs = gdrecptr->pause/1000;
	gdrecptr->msecs = gdrecptr->pause - gdrecptr->secs * 1000;
	if( gdrecptr->msecs < 0 ) gdrecptr->msecs = 0;
	if( gdrecptr->msecs >= 1000 ) {
        	++gdrecptr->secs;
        	gdrecptr->msecs -= 1000;
	}
	gdrecptr->usecs = 1000 * gdrecptr->msecs;
  }
  if( !strcmp(sub,"triglog") ) {
	sscanf(line,"%s %d",sub, &gdrecptr->triglog_interval);
  }
  if( !strcmp(sub,"logfile") ) {
	sscanf(line,"%s %s",sub, gdrecptr->logfile);
  	sprintf(gdrecptr->abslogfile, "%s/%s/%s",gdrecptr->root, gdrecptr->logdir, gdrecptr->logfile);
  }
  if( !strcmp(sub,"priority") ) {
	sscanf(line,"%s %d",sub, &gdrecptr->priority);
  }
  if( !strcmp(sub,"nlayers") ) {
	sscanf(line,"%s %d",sub, &gdrecptr->nlayers);
  }
  if( !strcmp(sub,"timeouts") ) {
	sscanf(line,"%s %lf %lf %lf",sub, &gdrecptr->connect_timeout,
        	&gdrecptr->read_timeout, &gdrecptr->write_timeout);
  }
/* TTT */
  if( !strcmp(sub,"trigger") ) {
	gdrecptr->value_type = GD_INT;
	if( gdrecptr->version <= 2.00 ) {
        	sscanf(line,"%s %s %d",sub, gdrecptr->trigger_pvs[0].name,
				&gdrecptr->trigger_values[0]);
	} else if( gdrecptr->version <= 5.90 ) {
        	sscanf(line,"%s %s %d",sub,sub3,&gdrecptr->ntriggers);
		if( !strcmp(sub3,"OR") || !strcmp(sub3,"or")) {
			gdrecptr->trigger_or_flag = 1;
		} else {
			gdrecptr->trigger_or_flag = 0;
		}
		for(i=0;i<gdrecptr->ntriggers;i++) {
			gd_getline(file,sub1);
			sscanf(sub1,"%s %s %s %d %f",gdrecptr->trigger_pvs[i].name,
			    sub2,sub3,&gdrecptr->trigger_values[i]);
			if( sub2[0] == 'V' || sub2[0] == 'v' )
			    gdrecptr->trigger_types[i] = GD_TRIGGER_VALUE;
			else if( sub2[0] == 'C' || sub2[0] == 'c' ) {
			    gdrecptr->trigger_types[i] = GD_TRIGGER_CHANGE;
			    gdrecptr->trigger_thresh[i] = gdrecptr->trigger_values[i];
			}
			else if( sub2[0] == 'L' || sub2[0] == 'l' )
			    gdrecptr->trigger_types[i] = GD_TRIGGER_LESS;
			else if( sub2[0] == 'M' || sub2[0] == 'm' )
			    gdrecptr->trigger_types[i] = GD_TRIGGER_MORE;
			else {
				printf("Wrong format in the paramter file\n");
				exit(1);
			}
			if( sub3[0] == 'i' || sub3[0] == 'I') gdrecptr->value_type = GD_INT;
			else if( sub3[0] == 'f' || sub3[0] == 'F') gdrecptr->value_type = GD_FLOAT;
			else if( sub3[0] == 't' || sub3[0] == 'T') {
				if( gdrecptr->ntriggers > 1 ) {
					fprintf(stderr,"There must be only one trigger PV,\n");
					fprintf(stderr,"if the timestamp is used as a trigger!\n");
					exit(1);
				}
				gdrecptr->value_type = GD_TS;
			} else {
			  printf("Only int, float, and timestamp are supported in this version\n");
			  exit(1);
			}
		}
		gdrecptr->trigger_type = gdrecptr->trigger_types[0];
		if( gdrecptr->ntriggers > 1 ) {
			for(i=0;i<gdrecptr->ntriggers;i++) {
				if( gdrecptr->trigger_types[i] != gdrecptr->trigger_type ) {
					gd_log("All triggers must have the save type",0);
					sprintf(sub1,"Please correct the parameter file: %s",
						gdrecptr->parfile);
					gd_log(sub1,0);
					return -1;
				}
			}
		}
	} else {
		printf(
		  "We need a higher version (at least v6.0) of gendl to run this parameter file\n");
		exit(1);
	}
  }
  if( !strcmp(sub,"pre_cond") ) {
        sscanf(line,"%s %s",sub, sub1);
	if( !strcmp(sub1,"yes") ) {
		gdrecptr->pre_cond_flag = 1;
		gd_getline(file,sline);
		sscanf(sline,"%s %d",gdrecptr->pre_cond_pv.name, &gdrecptr->pre_cond_value);
	}
  }
  if( !strcmp(sub,"reset") ) {
        sscanf(line,"%s %s %d",sub, gdrecptr->reset_pv.name, &gdrecptr->reset_value);
  }
  if( !strcmp(sub,"async") ) {
        sscanf(line,"%s %s",sub, sub1);
	if( !strcmp(sub1,"yes") ) {
		gdrecptr->async_flag = 1;
	}
  }
  if( !strcmp(sub,"large_attr") ) {
        sscanf(line,"%s %s",sub, sub1);
	if( !strcmp(sub1,"yes") ) {
		gdrecptr->large_attr_flag = 1;
	}
  }
  if( !strcmp(sub,"need_reset") ) {
        sscanf(line,"%s %s",sub, sub1);
	if( !strcmp(sub1,"yes") ) {
		gdrecptr->need_reset = 1;
	}
  }
  if( !strcmp(sub,"reset_func") ) {
        sscanf(line,"%s %s",sub, sub1);
	if( !strcmp(sub1,"null") ) {
		gdrecptr->reset_func[0] = 0;
	} else {
		strcpy(gdrecptr->reset_func, sub1);
	}
  }
  if( !strcmp(sub,"reset_pv") ) {
        sscanf(line,"%s %s %d",sub, gdrecptr->reset_pv.name, &gdrecptr->reset_value);
  }
  if( !strcmp(sub,"need_init") ) {
        sscanf(line,"%s %s",sub, sub1);
	if( !strcmp(sub1,"yes") ) {
		gdrecptr->need_init = 1;
	}
  }
  if( !strcmp(sub,"init_func") ) {
        sscanf(line,"%s %s",sub, sub1);
	if( !strcmp(sub1,"null") ) {
		gdrecptr->init_func[0] = 0;
	} else {
		strcpy(gdrecptr->init_func, sub1);
	}
  }
  if( !strcmp(sub,"init_pv") ) {
        sscanf(line,"%s %s %d",sub, gdrecptr->reset_pv.name, &gdrecptr->reset_value);
  }
  if( !strcmp(sub,"lreset_flag") ) {
        sscanf(line,"%s %d",sub, &gdrecptr->lreset_flag);
  }
  if( !strcmp(sub,"lreset_func") ) {
        sscanf(line,"%s %s",sub, sub1);
	if( !strcmp(sub1,"null") ) {
		gdrecptr->lreset_func[0] = 0;
	} else {
		strcpy(gdrecptr->lreset_func, sub1);
	}
  }
  if( !strcmp(sub,"lreset_parfile") ) {
        sscanf(line,"%s %s",sub, sub1);
	if( !strcmp(sub1,"null") ) {
		gdrecptr->lreset_parfile[0] = 0;
	} else {
		strcpy(gdrecptr->lreset_parfile, sub1);
	}
  }

  return 0;
}

int gd_init()
{
  int i,j,fpvs;
  int res;
  char str[320],sline[324];
  char *pos, cstr[4];

  GD_PV *ppv, *sppv, *apvs;
  FILE *file;
#ifdef GD_DEB
  FILE *pvfile;
#endif

  fprintf(stderr,"\n\n ----------------------- *** -----------------------\n\n");
  fprintf(stderr,"In gd_init\n\n");
  gdrecptr->h5_rank = 2;
  gdrecptr->ncells = 30;
  gdrecptr->need_reset = 0;
  gdrecptr->lreset_flag = 0;
  gdrecptr->async_flag = 0;
  gdrecptr->ts_change_index = 0;
  gdrecptr->in_fault = 0;
  sprintf(sline,"%s/%s",gdrecptr->datapath, gdrecptr->parfile);
  fprintf(stderr,"The input file: %s\n",sline);
#ifdef GD_DEB
  pvfile = fopen("gendl.pvs","w");
  fprintf(stderr,"The input file: %s\n",sline);
#endif
  file = fopen(sline,"r");
  if (file==NULL) {
	sprintf(str,"gd_init: %s %s", sline, strerror(errno));
	gd_log(str,0);
#ifdef GD_DEB
  fprintf(stderr,"%s\n",str);
#endif
	exit(1);
  }

  while(!feof(file)) {
	gd_getline(file,sline);
	if( sline[0] == 'E' && sline[1] == 'N' && sline[2] == 'D') break;
	res = gd_getpar(file,sline);
	if( res < 0 ) {
		exit(1);
	}
  }
  gdrecptr->triglog_nreadings=(1000.0*gdrecptr->triglog_interval)/gdrecptr->pause;

  fprintf(stderr,"Log freq: %d\n",gdrecptr->triglog_nreadings);

  ppv = gd_pvs = (GD_PV *)malloc(gdrecptr->npvs * sizeof(GD_PV));
  if( gdrecptr->naddrs > 0 ) {
  	apvs = gd_addrpvs = (GD_PV *)malloc(gdrecptr->naddrs * sizeof(GD_PV));
  	addr = (int *)malloc(gdrecptr->naddrs * sizeof(int));
  }
  pvts = (TS_STR *)malloc(gdrecptr->npvs * sizeof(TS_STR));

  if( gd_pvs == NULL ) {
	sprintf(sline, "Memory allocation (%d bytes) error",(int)(gdrecptr->npvs*sizeof(GD_PV)));
	gd_log(sline,0);
	fclose(file);
	exit(1);
  }

/* BBB */
  sprintf(sline,"\n\n*********************** At %s: gendl_v6.0 started\n",utimestring_local());
  gd_log(sline,0);
  sprintf(sline,"Root       : %s",gdrecptr->root);
  gd_log(sline,0);
  sprintf(sline,"Par  file  : %s",gdrecptr->parfile);
  gd_log(sline,0);
  sprintf(sline,"Data path  : %s",gdrecptr->datapath);
  gd_log(sline,0);
  sprintf(sline,"Log  path  : %s",gdrecptr->abslogfile);
  gd_log(sline,0);
  sprintf(sline,"Log  Freq  : every %d readings",gdrecptr->triglog_nreadings);
  gd_log(sline,0);

  sprintf(sline,"Num Cells  : %d",gdrecptr->ncells);
  gd_log(sline,0);
  sprintf(sline,"Num PVs    : %d",gdrecptr->npvs);
  gd_log(sline,0);
  sprintf(sline,"Priority   : %d",gdrecptr->priority);
  gd_log(sline,0);
  sprintf(sline,"Pause time : %d",gdrecptr->pause);
  gd_log(sline,0);
  sprintf(sline,"Time out   : %lf %lf",gdrecptr->connect_timeout, gdrecptr->read_timeout);
  gd_log(sline,0);
  sprintf(sline,"h5 rank    : %d",gdrecptr->h5_rank);
  gd_log(sline,0);
  sprintf(sline,"Stand-alone: %d",gdrecptr->stand_alone);
  gd_log(sline,0);
  sprintf(sline,"npvs_per_rd: %d",gdrecptr->npvs_per_read);
  gd_log(sline,0);
  sprintf(sline,"Number of triggers: %d",gdrecptr->ntriggers);
  gd_log(sline,0);

fprintf(stderr,"\n\n  *** Number of triggers: %d\n",gdrecptr->ntriggers);

  if(gdrecptr->trigger_or_flag) {
	sprintf(sline,"Trigger logical type: OR");
	gd_log(sline,0);
  } else {
	sprintf(sline,"Trigger typ: AND");
	gd_log(sline,0);
  }
  sprintf(sline,"Trigger type: Value");
  gd_log(sline,0);
  for(i=0;i<gdrecptr->ntriggers;i++) {
	fprintf(stderr,"        %s: %d\n",gdrecptr->trigger_pvs[i].name,
		gdrecptr->trigger_values[i]);
	sprintf(sline,"%s: %d",gdrecptr->trigger_pvs[i].name,
		gdrecptr->trigger_values[i]);
	gd_log(sline,0);
  }

/*
  if(gdrecptr->trigger_type == GD_TRIGGER_VALUE ) {
	sprintf(sline,"Trigger type: Value");
	gd_log(sline,0);
	for(i=0;i<gdrecptr->ntriggers;i++) {
		sprintf(sline,"%s: %d",gdrecptr->trigger_pvs[i].name,
			gdrecptr->trigger_values[i]);
		gd_log(sline,0);
	}
  } else {
	sprintf(sline,"Trigger type: Change");
	gd_log(sline,0);
	for(i=0;i<gdrecptr->ntriggers;i++) {
		sprintf(sline,"%s: %.4f",gdrecptr->trigger_pvs[i].name,
			gdrecptr->trigger_thresh[i]);
		gd_log(sline,0);
	}
  }
*/

#ifdef GD_DEB
  fprintf(stderr,"\n----------------------------------------------------------------\n");
  fprintf(stderr,"Num Cells  : %d\n",gdrecptr->ncells);
  fprintf(stderr,"Num PVs    : %d\n",gdrecptr->npvs);
  fprintf(stderr,"Priority   : %d\n",gdrecptr->priority);
  fprintf(stderr,"Time out   : %lf %lf\n",gdrecptr->connect_timeout, gdrecptr->read_timeout);
  fprintf(stderr,"h5 rank    : %d\n",gdrecptr->h5_rank);
  fprintf(stderr,"Stand-alone: %d\n",gdrecptr->stand_alone);
  fprintf(stderr,"npvs_per_rd: %d\n",gdrecptr->npvs_per_read);
  fprintf(stderr,"Number of triggers: %d\n",gdrecptr->ntriggers);
  if(gdrecptr->trigger_or_flag) fprintf(stderr,"Trigger logical type: OR\n");
  else fprintf(stderr,"Trigger typ: AND\n");
  if(gdrecptr->trigger_type == GD_TRIGGER_VALUE ) {
	fprintf(stderr,"Trigger type: Value\n");
	for(i=0;i<gdrecptr->ntriggers;i++) {
		fprintf(stderr,"%s: %d\n",gdrecptr->trigger_pvs[i].name,
			gdrecptr->trigger_values[i]);
	}
  } else {
	fprintf(stderr,"Trigger type: Change\n");
	for(i=0;i<gdrecptr->ntriggers;i++) {
		fprintf(stderr,"%s: %.4f\n",gdrecptr->trigger_pvs[i].name,
			gdrecptr->trigger_thresh[i]);
	}
  }
#endif

  if( gdrecptr->nlayers == 0 ) {
	for(i=0;i<gdrecptr->npvs;i++) {
		gd_getline(file,sline);
		sscanf(sline,"%s",ppv->name);
		++ppv;
	}
  } else {
	fpvs = gdrecptr->npvs / gdrecptr->ncells;
	for(i=0;i<fpvs;i++) {
		gd_getline(file,sline);
		sscanf(sline,"%s",ppv->name);
		++ppv;
	}
	for(j=2; j <= gdrecptr->ncells; j++) {
		sppv = ppv - fpvs;
		sprintf(cstr,"C%02d",j);
		for(i=0;i<fpvs;i++) {
			strcpy((char *)ppv->name, sppv->name);
			pos = strstr(ppv->name, "C01");
			*(pos+1) = cstr[1];
			*(pos+2) = cstr[2];
			sppv++;
			ppv++;
		}
	}
  }
  if( gdrecptr->naddrs > 0 ) {
	for(i=0;i<gdrecptr->naddrs;i++) {
		gd_getline(file,sline);
		sscanf(sline,"%s",apvs->name);
		++apvs;
	}
  }
  fclose( file );

  ppv = gd_pvs;
  for(i=0;i<gdrecptr->npvs;i++) {
	ppv->dbr_value = (char *)NULL;
	++ppv;
  }

#ifdef GD_DEB
  ppv = gd_pvs;
  fprintf(pvfile,"total number of PVs: %d\n",gdrecptr->npvs);
  for(i=0;i<gdrecptr->npvs;i++) {
	fprintf(pvfile,"%s\n",ppv->name);
	++ppv;
  }
  fclose(pvfile);
#endif

  res = gd_connectpv(gd_pvs);
  if( res == 0 ) {
	gdrecptr->tmval_size = dbr_size_n(DBR_TIME_FLOAT, 1);
	if( gdrecptr->h5_rank == 2 ) {
		gdrecptr->h5_dim[1] = gd_pvs->nelems;
	} else if( gdrecptr->h5_rank == 3 ) {
		gdrecptr->h5_dim[2] = gd_pvs->nelems;
	}
	ca_clear_channel(gd_pvs->chid);
  }
  timefloat = (struct dbr_time_float *)malloc(gdrecptr->tmval_size * gdrecptr->npvs);
  gdrecptr->nbytes_per_pvt = dbr_size_n(DBR_TIME_FLOAT, gd_pvs->nelems);
  gd_pvs->created = 0;
  gd_pvs->connected = 0;
  gd_pvs->allocated = 0;
  gdrecptr->dbrtype = SAVE_DBRTYPE;

  if(gdrecptr->dbrtype == DBR_DOUBLE)
	strcpy(gdrecptr->str_dbrtype,"double");
  else
	strcpy(gdrecptr->str_dbrtype,"float");

  if( gdrecptr->h5_rank == 2 ) {
	gdrecptr->h5_dim[0] = gdrecptr->npvs;
	gdrecptr->nelems_per_pv = gdrecptr->h5_dim[1];
  } else if( gdrecptr->h5_rank == 3 ) {
	gdrecptr->h5_dim[0] = gdrecptr->ncells;
	gdrecptr->h5_dim[1] = gdrecptr->npvs_per_cell;
	gdrecptr->nelems_per_pv = gdrecptr->h5_dim[2];
  }
  gdrecptr->nbytes_per_pv = gdrecptr->nelems_per_pv * sizeof(SAVE_DTYPE);
  gdrecptr->nbytes_per_set = gdrecptr->npvs * gdrecptr->nbytes_per_pv;
#ifdef TYPE_DOUBLE
  strcpy(gdrecptr->str_dbrtype,"double");
#else
  strcpy(gdrecptr->str_dbrtype,"float");
#endif

  if( gdrecptr->h5_rank == 2 ) {
	h5_data = (float **)malloc( gdrecptr->h5_dim[0] * sizeof( float *) );
	h5_data[0] = (float *)malloc( gdrecptr->nbytes_per_set );
	if( h5_data[0] == NULL ) {
		sprintf(sline, "Memory allocation (%d bytes) error",gdrecptr->nbytes_per_set);
		gd_log(sline,0);
		exit(1);
	}
	for(i=0;i<gdrecptr->h5_dim[0];i++) {
		h5_data[i] = h5_data[0] + i * gdrecptr->h5_dim[1];
	}
  } else if( gdrecptr->h5_rank == 3 ) {
	h5_data3 = (float ***)malloc( gdrecptr->h5_dim[0] * sizeof( float **) );
	h5_data3[0] = (float **)malloc( gdrecptr->h5_dim[1] * sizeof( float *) );
	h5_data3[0][0] = (float *)malloc( gdrecptr->nbytes_per_set );
	if( h5_data3[0][0] == NULL ) {
		sprintf(sline, "Memory allocation (%d bytes) error",gdrecptr->nbytes_per_set);
		gd_log(sline,0);
		exit(1);
	}
	for(i=0;i<gdrecptr->h5_dim[0];i++) {
		for(j=0;j<gdrecptr->h5_dim[1]; j++) {
			h5_data[i][j] = h5_data[0][0] +
				(i * gdrecptr->h5_dim[1] + j)*gdrecptr->h5_dim[2];
		}
	}
  }
#ifdef GD_DEB
  fprintf(stderr,"H5 struct, rank %d:\n",gdrecptr->h5_rank);
  fprintf(stderr,"Number of rows: %d\n",gdrecptr->h5_dim[0]);
  fprintf(stderr,"Number of cols: %d\n",gdrecptr->h5_dim[1]);
#endif

  fprintf(stderr,"\n\n ----------------------- *** -----------------------\n\n");
  fprintf(stderr,"Exit gd_init\n\n");
  return 0;
}

