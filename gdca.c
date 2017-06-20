#include "gendl.h"

#undef GD_DEB

static struct event_handler_args cb_args;
static int npvs_completion;

void gd_mycallback(args)
evargs args;
{
  int ipv = (int)args.usr;

  if( args.status == ECA_NORMAL ) {
	++npvs_completion;
	memcpy( h5_data[ipv], args.dbr, (gd_pvs + ipv)->nelems * sizeof(SAVE_DBRTYPE));
  }
}
	

int gd_createchan(pv)
GD_PV *pv;
{
  int res;
  char sline[132];

  if( pv->created ) return 0;

/*
#ifdef GD_DEB
  fprintf(stderr,"***********  Enter gd_createchan for %s\n", pv->name);
#endif
*/

  pv->err = 0;
  res = ca_create_channel(pv->name,NULL,0,gdrec.priority,&(pv->chid));
  if(res != ECA_NORMAL) {
	pv->created = 0;
	pv->err = 1;
	sprintf(sline,"%s: %s",pv->name,ca_message(res));
#ifdef GD_DEB
  fprintf(stderr,"***********  "%s: %s\n",pv->name,ca_message(res));
#endif
	gd_log(sline,1);
	res = -1;
	exit(1);
  } else {
	pv->created = 1;
	pv->err = 0;
	res = 0;
  }
/*
#ifdef GD_DEB
  fprintf(stderr,"***********  Exit gd_createchan with %d\n", res);
#endif
*/

  return res; /* 0 or -1 */
}


int gd_connectpv(pv)
GD_PV *pv;
{
  int res;
  char sline[132];

  if( pv->connected ) return 0;

/*
#ifdef GD_DEB
  fprintf(stderr,"***********  Enter gd_connectpv with pv %s\n",pv->name);
#endif
*/

  if( !pv->created ) {
	gd_createchan(pv);
  }

  pv->err = 0;
  res = ca_pend_io(gdrec.connect_timeout);
  if( res != ECA_NORMAL ) {
	pv->connected = 0;
	pv->err = 1;
	sprintf(sline,"Connection timeout: %s",ca_message(res));
	gd_log(sline,1);
	res = -1;
  } else {
	pv->connected = 1;
	res = pv->err = 0;
  }
  if( pv->err == 0 ) {
	pv->nelems = ca_element_count(pv->chid);
	pv->dbrtype = ca_field_type(pv->chid);
  }

/*
#ifdef GD_DEB
  fprintf(stderr,"     PV name %s\n", pv->name);
  fprintf(stderr,"     nlelems %d, dbrtype %d\n", pv->nelems, (int)(pv->dbrtype));
#endif

#ifdef GD_DEB
  fprintf(stderr,"***********  Exit gd_connectpv with code %d\n",res);
#endif
*/

  return res; /* 0 or -1 */
}


int gd_connectpvs(npvs,pvs)
int npvs;
GD_PV *pvs;
{
  int i,res=0;
  char sline[132];
  GD_PV *pv;

/*
#ifdef GD_DEB
  fprintf(stderr,"Enter gd_connectpvs with %d pvs: %s\n",npvs,pvs->name);
#endif
*/
  if( npvs == 1 ) {
	return gd_connectpv(pvs);
  }
  pv = pvs;
  for(i=0;i<npvs;i++) {
	if( pv->created == 0 )
		gd_createchan(pv);
	++pv;
  }

  res = ca_pend_io(gdrec.connect_timeout);

  if( res != ECA_NORMAL ) {
	pv = pvs;
	for(i=0;i<npvs;i++) {
		pv->connected = 0;
		pv->err = 1;
		++pv;
	}
	sprintf(sline,"Connection timeout: %s",ca_message(res));
	gd_log(sline,1);
	res = -1;
  } else {
	pv = pvs;
	for(i=0;i<npvs;i++) {
		pv->connected = 1;
		pv->err = 0;
		++pv;
	}
	res = 0;
  }
  pv = pvs;
  for(i=0;i<npvs;i++) {
	if( pv->err == 0 ) {
		pv->nelems = ca_element_count(pv->chid);
		pv->dbrtype = ca_field_type(pv->chid);
	}
	++pv;
  }
/*
#ifdef GD_DEB
  fprintf(stderr,"Exit gd_connectpvs with %d\n",res);
#endif
*/
  return res; /* 0 or -1 */
}


int gd_readpvs( npvs, pvs, connected, allocated )
int npvs;
GD_PV *pvs;
int connected;
int allocated;
{
  int i;
  int res;
  int nconns;
  char sline[132];
  GD_PV *ppv;

/*
#ifdef GD_DEB
  fprintf(stderr,"************ Enter gd_readpvs: %s\n", pvs->name);
#endif
*/
  if( !connected ) {
	res =  gd_connectpvs(npvs,pvs);
  }

/*
  if( res == -1 && gdrecptr->lreset_critical == 1 ) {
	sprintf(sline,"Some or ALL pvs are not connected");
	gd_log(sline,1);
	printf("Some or ALL pvs are not connected\n");
	return -1;
  }
*/

  nconns = 0;
  ppv = pvs;
  for (i = 0; i < npvs; i++) {
	if( ca_state(ppv->chid) == cs_conn ) {
		ppv->value_size = dbr_size_n(ppv->dbrtype, ppv->nelems);
/*
#ifdef GD_DEB
		fprintf(stderr,"    Number of elem, DBR type, size: %d %d %d\n",ppv->nelems,
			ppv->dbrtype, ppv->value_size);
#endif
*/
		if( !allocated ) {
			ppv->dbr_value = realloc(ppv->dbr_value, ppv->value_size);
			if( ppv->dbr_value == NULL ) {
				ppv->err = 1;
				sprintf(sline,"gd_readpvs: Memory allocation error");
				gd_log(sline,1);
				return -1;
			}
			ppv->allocated = 1;
		}
		++nconns;
		ppv->connected = 1;
		res = ca_array_get(ppv->dbrtype, ppv->nelems, ppv->chid, ppv->dbr_value);
	} else {
		ppv->err = 1;
	}
	++ppv;
  }
  if( nconns == 0 ) {
	sprintf(sline, "No PVs connected");
	gd_log(sline,1);
	return -1;
  }
/*
  if( nconns < npvs && gdrecptr->lreset_critical == 1 ) {
	sprintf(sline,"Some PVs are not connected");
	gd_log(sline,1);
	printf("Some PVs are not connected\n");
	return -1;
  }
*/
  res = ca_pend_io(5*gdrec.read_timeout);
  if (res != ECA_NORMAL) {
	sprintf(sline,"gd_readpvs: %s",ca_message(res));
	gd_log(sline,1);
	return -1;
  }

  return 0;
}


int gd_readpvs_dbrtype( npvs, pvs, dbrtype, connected, allocated )
int npvs;
GD_PV *pvs;
int dbrtype;
int connected;
int allocated;
{
  int i;
  int res;
  int nconns;
  char sline[132];
  GD_PV *ppv;

  if( !connected ) {
	res =  gd_connectpvs(npvs,pvs);
  }

  nconns = 0;
  ppv = pvs;
  for (i = 0; i < npvs; i++) {
	if( ca_state(ppv->chid) == cs_conn ) {
		ppv->value_size = dbr_size_n(dbrtype, ppv->nelems);
		if( !allocated ) {
			ppv->dbr_value = realloc(ppv->dbr_value, ppv->value_size);
			if( ppv->dbr_value == NULL ) {
				ppv->err = 1;
				sprintf(sline,"gd_readpv: Memory allocation error");
				gd_log(sline,1);
				return -1;
			}
			ppv->allocated = 1;
		}
		++nconns;
		ppv->connected = 1;
		res = ca_array_get(dbrtype, ppv->nelems, ppv->chid, ppv->dbr_value);
	} else {
		ppv->err = 1;
	}
	++ppv;
  }
  if( nconns == 0 ) {
	sprintf(sline, "No PVs connected");
	gd_log(sline,1);
	return -1;
  }

  res = ca_pend_io(5*gdrec.read_timeout);
  if (res != ECA_NORMAL) {
	sprintf(sline,"gd_readpvs_dbrtype: %s",ca_message(res));
	gd_log(sline,1);
	return -1;
  }

  return 0;
}

/* BBB */
int gd_asynreadpvs_intohdf5data( connected )
int connected;
{
  int ipv;
  int res,et1;
  int nconns;
  char sline[80];
  GD_PV *ppv;
  FILE *tfile;

/*
#ifdef GD_DEB
  fprintf(stderr,"Enter gd_asynreadpvs_intohdf5data\n");
#endif
*/

  tfile = fopen("Timing.dat","a");
  if( tfile != NULL ) {
	fprintf(tfile,"\n\n**** Async reading at     : %s\n",utimestring_local());
	fprintf(tfile,"     Total number of PVs  : %d\n",gdrecptr->npvs);
	fprintf(tfile,"     Total number of bytes: %d\n",gdrecptr->nbytes_per_set);
/* ADD */
	fclose(tfile);
  }

  if( !connected ) {
	res =  gd_connectpvs(gdrecptr->npvs,gd_pvs);
  }
  nconns = 0;
  ppv = gd_pvs;
  npvs_completion = 0;

/* Async */
  elapsetime(0);
  for (ipv = 0; ipv < gdrecptr->npvs; ipv++) {
	if( ca_state(ppv->chid) == cs_conn ) {
		++nconns;
		ppv->connected = 1;
		res = ca_array_get_callback(SAVE_DBRTYPE, ppv->nelems, ppv->chid,
			gd_mycallback, (void *)ipv);
	} else {
		ppv->err = 1;
	}
	++ppv;
  }

  if( nconns == 0 ) {
	sprintf(sline, "No PVs connected");
	gd_log(sline,1);
	return -1;
  }
  res = ca_pend_io(gdrec.read_timeout);
  while( npvs_completion < nconns ) {
	ca_pend_event(1.0);
  }
  if( npvs_completion == gdrecptr->npvs ) {
	et1 = elapsetime(1);
	tfile = fopen("Timing.dat","a");
	if( tfile != NULL )
		fprintf(tfile,"     Elapse time for read: %.5f secs\n", et1/1000000.0);
	elapsetime(0);
	gd_savewf_toh5file();
	et1 = elapsetime(1);
	if( tfile != NULL ) {
		fprintf(tfile,"     Elapse time for save: %.5f secs\n", et1/1000000.0);
		fclose(tfile);
	}
  }

  return 0;
}

int gd_readpvs_intohdf5data( npvs, pvs, ipv, connected )
int npvs;
GD_PV *pvs;
int ipv;
int connected;
{
  int i, kpv;
  int res;
  int nconns;
  char sline[132];
  GD_PV *ppv;

/*
#ifdef GD_DEB
  fprintf(stderr,"\n!!!!!HDF5 readpvs:  Enter gd_readpvs_intohdf5data: %d pvs\n", npvs);
  fprintf(stderr,"    SAVE dbrtype: %d\n",SAVE_DBRTYPE);
#endif
*/
  if( !connected ) {
	res =  gd_connectpvs(npvs,pvs);
  }

  nconns = 0;
  ppv = pvs;
  kpv = ipv;
  for (i = 0; i < npvs; i++) {
	if( ca_state(ppv->chid) == cs_conn ) {
		ppv->value_size = dbr_size_n(SAVE_DBRTYPE, ppv->nelems);
		++nconns;
		ppv->connected = 1;
		res = ca_array_get(SAVE_DBRTYPE, ppv->nelems, ppv->chid,
			h5_data[kpv]);
	} else {
		ppv->err = 1;
	}
	++ppv;
	++kpv;
  }
  if( nconns == 0 ) {
	sprintf(sline, "No PVs connected");
	gd_log(sline,1);
	return -1;
  }

  res = ca_pend_io(5*gdrec.read_timeout);
  if (res != ECA_NORMAL) {
	sprintf(sline,"gd_readpvs_intohdf5data: %s",ca_message(res));
	gd_log(sline,1);
	return -1;
  }

/*
#ifdef GD_DEB
  fprintf(stderr,"\n!!!!!HDF5 readpvs:  Exit gd_readpvs_intohdf5data: %s\n", pvs->name);
#endif
*/
  return 0;
}


int gd_readtimestamp( npvs, pvs, ipv, connected )
int npvs;
GD_PV *pvs;
int ipv;
int connected;
{
  int i, n, kpv;
  int res;
  int nconns;
  char sline[132];
  GD_PV *ppv;

/*
#ifdef GD_DEB
  fprintf(stderr,"\nEnter gd_readtimestamp: %d %d pvs\n", ipv, npvs);
#endif
*/
  if( !connected ) {
	res =  gd_connectpvs(npvs,pvs);
  }

  nconns = 0;
  ppv = pvs;
  kpv = ipv;
  for (i = 0; i < npvs; i++) {
	if( ca_state(ppv->chid) == cs_conn ) {
		++nconns;
		n = 1;
		res = ca_array_get(DBR_TIME_FLOAT, n, ppv->chid, &(timefloat[kpv]));
	} else {
		ppv->err = 1;
	}
	++ppv;
	++kpv;
  }
  if( nconns == 0 ) {
	sprintf(sline, "No PVs connected");
	gd_log(sline,1);
	return -1;
  }

  res = ca_pend_io(5*gdrec.read_timeout);
  if (res != ECA_NORMAL) {
	sprintf(sline,"gd_readtimestamp: %s",ca_message(res));
	gd_log(sline,1);
	return -1;
  }

/*
#ifdef GD_DEB
  fprintf(stderr,"Exit gd_readtimestamp\n");
#endif
*/
  return 0;
}


int gd_readaddr( npvs, pvs, ipv, connected )
int npvs;
GD_PV *pvs;
int ipv;
int connected;
{
  int i, n, kpv;
  int res;
  int nconns;
  char sline[132];
  GD_PV *ppv;

/*
#ifdef GD_DEB
  fprintf(stderr,"\nEnter gd_readaddr: %d %d pvs\n", ipv, npvs);
  for(i=0;i<5;i++) {
	fprintf(stderr,"%s\n",(pvs+i)->name);
  }
#endif
*/
  if( !connected ) {
	res =  gd_connectpvs(npvs,pvs);
  }

  nconns = 0;
  ppv = pvs;
  kpv = ipv;
  for (i = 0; i < npvs; i++) {
	if( ca_state(ppv->chid) == cs_conn ) {
		++nconns;
		n = 1;
		res = ca_get(DBR_LONG, ppv->chid, &(addr[kpv]));
/*
		res = ca_array_get(DBR_LONG, n, ppv->chid, &(addr[kpv]));
*/
	} else {
		ppv->err = 1;
	}
	++ppv;
	++kpv;
  }
  if( nconns == 0 ) {
	sprintf(sline, "No PVs connected");
	gd_log(sline,1);
	return -1;
  }

  res = ca_pend_io(5*gdrec.read_timeout);
  if (res != ECA_NORMAL) {
	sprintf(sline,"gd_readaddr: %s",ca_message(res));
	gd_log(sline,1);
	return -1;
  }

/*
#ifdef GD_DEB
  for(i=0;i<5;i++) {
	fprintf(stderr,"%ld\n",addr[i]);
  }
  fprintf(stderr,"Exit gd_readaddr\n");
#endif
*/
  return 0;
}


int gd_readpvs_intovalue( npvs, pvs, dbrtype, value, connected )
int npvs;
GD_PV *pvs;
int dbrtype;
char *value;
int connected;
{
  int i;
  int res;
  int nconns;
  char *pval, sline[132];
  GD_PV *ppv;

/*
#ifdef GD_DEB
  fprintf(stderr,"\n!!!!!gd_readpvs_intovalue:   %d pvs\n", npvs);
#endif
*/
  if( !connected ) {
	res =  gd_connectpvs(npvs,pvs);
  }

  nconns = 0;
  pval = value;
  ppv = pvs;
  for (i = 0; i < npvs; i++) {
	if( ca_state(ppv->chid) == cs_conn ) {
		ppv->value_size = dbr_size_n(dbrtype, ppv->nelems);
		++nconns;
		ppv->connected = 1;
		res = ca_array_get(dbrtype, ppv->nelems, ppv->chid, pval);
		pval += ppv->value_size;
	} else {
		ppv->err = 1;
	}
	++ppv;
  }
  if( nconns == 0 ) {
	sprintf(sline, "No PVs connected");
	gd_log(sline,1);
	return -1;
  }

  res = ca_pend_io(5*gdrec.read_timeout);
  if (res != ECA_NORMAL) {
	sprintf(sline,"gd_readpvs_intovalue: %s",ca_message(res));
	gd_log(sline,1);
	return -1;
  }

/*
#ifdef GD_DEB
  fprintf(stderr,"\n!!!!!Exit gd_readpvs_intovalye\n");
#endif
*/
  return 0;
}
