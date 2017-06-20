#include "gendl.h"
#include <errno.h>
#include <string.h>
int errno;

int tolower();

int gd_lreset_getpar(line)
char *line;
{
  int i,lent;
  char sub[32], sub1[8];

  sscanf(line,"%s",sub);
  lent = strlen(sub);
  for(i=0;i<lent;i++)
	sub[i] = tolower(sub[i]);

  if( !strcmp(sub,"npvs") ) {
	sscanf(line,"%s %d",sub, &gdrecptr->lreset_npvs);
  }
  if( !strcmp(sub,"critical") ) {
	sscanf(line,"%s %s",sub, sub1);
	if( sub1[0] == 'Y' || sub1[0] == 'y' ) {
		gdrecptr->lreset_critical = 1;
	}
  }
  return 0;
}



int gd_writepvs_bo( npvs, pvs, values, connected )
int npvs;
GD_PV *pvs;
short *values;
int connected;
{
  int i, res, nconns;
  char sline[128];
  GD_PV *ppv;

#ifdef GD_DEB
  fprintf(stderr,"************ Enter gd_writepvs_bo: %d pvs\n", npvs);
#endif
  if( !connected ) {
	res =  gd_connectpvs(npvs,pvs);
  }

  if( res == -1 && gdrecptr->lreset_critical == 1 ) {
	printf("Some or All PVs are not connected; stop proceeding with local reset\n");
/*
	return -1;
*/
  }

  nconns = 0;
  ppv = pvs;

  for (i = 0; i < npvs; i++) {
	if( ca_state(ppv->chid) == cs_conn ) {
		++nconns;
		ppv->connected = 1;
/*
		if( i < 20 || i > npvs -20 ) {
			fprintf(stderr," ca_put: %s %hd\n", ppv->name,values[i]);
		}
*/
		res = ca_put(DBR_SHORT, ppv->chid, values+i);
	} else {
		fprintf(stderr,"NOT connected: %s\n",ppv->name);
		ppv->err = 1;
	}
	++ppv;
  }


  if( nconns == 0 ) {
	sprintf(sline, "No PVs connected");
	gd_log(2,1,sline);
	return -1;
  }

  res = ca_pend_io(5*gdrec.read_timeout);
  if (res == ECA_TIMEOUT) {
	sprintf(sline,"gd_writepvs_bo: %s",ca_message(res));
	gd_log(2,1,sline);
	return -1;
  }
  return 0;
}


int gd_lreset_init()
{
  int i;
  char str[320],sline[324];
  GD_PV *ppv;
  FILE *file;

  gdrecptr->ncells = 30;
  gdrecptr->lreset_critical = 0;
  sprintf(sline,"%s/%s",gdrecptr->datapath, gdrecptr->lreset_parfile);
  file = fopen(sline,"r");
  if (file==NULL) {
	sprintf(str,"gd_lreset_init: %s %s", sline, strerror(errno));
	gd_log(str);
	return -1;
  }

  while(1) {
	gd_getline(file,sline);
	if( sline[0] == 'E' && sline[1] == 'N' && sline[2] == 'D') break;
	gd_lreset_getpar(sline);
  }

  ppv = lreset_pvs = (GD_PV *)malloc(gdrecptr->lreset_npvs * sizeof(GD_PV));
  lreset_values = (short *)malloc(gdrecptr->lreset_npvs * sizeof(short));
  if( lreset_pvs == NULL || lreset_values == NULL ) {
	sprintf(sline, "Memory allocation in gd_lreset_init\n");
	gd_log(sline);
	fclose(file);
	return -1;
  }

#ifdef GD_DEB
  fprintf(stderr,"Number of local reset pvs: %d\n",gdrecptr->lreset_npvs);
#endif
  for(i=0;i<gdrecptr->lreset_npvs;i++) {
	gd_getline(file,sline);
	sscanf(sline,"%s %hd",ppv->name, lreset_values+i);
	++ppv;
  }
  fclose( file );

  return 0;
}

int gd_local_reset(connected)
int connected;
{
  gd_lreset_init();
  return gd_writepvs_bo( gdrecptr->lreset_npvs, lreset_pvs, lreset_values, connected);
}
