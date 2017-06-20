#include "gendl.h"
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <hdf5.h>

int gd_saveeg_toh5file(file,ig)
hid_t file;
int ig;
{
  int i,j,ret,nsubg,*pi;
  PV_NAME *pvnames;
  GD_PV *ppv;
  GD_EGROUP *grp;
  char *subgname, gname[16];
  float *pf;

  herr_t status;
  hid_t npvs_id, sid;

  hsize_t npvs, n, nels=1;
  hid_t strtype_id, group_id, dset_id;
  hsize_t fdim[2];

/* All extra_group has two subgroups */

  grp = gd_egroups + ig;
  npvs = (hsize_t)(grp->npvs);

  nsubg = grp->nsubgroups;
  group_id = H5Gcreate(file, grp->name, 0);

  pvnames = (PV_NAME *)malloc(npvs * PV_NAMELEN);
  ppv = grp->pvs;
  for(i=0;i<npvs;i++) {
	strcpy((char *)(pvnames+i),(char *)ppv->name);
	++ppv;
  }
  if( grp->nsubgroups > 0 ) {
	subgname = grp->subg_name[i];
  } else {
	sprintf(gname,"/%s",grp->name);
  }

  strtype_id = H5Tcopy(H5T_C_S1);
  H5Tset_size(strtype_id, PV_NAMELEN);

  n = npvs;
  npvs_id = H5Screate_simple(1, &n, NULL);
  fdim[0] = n;
  if( grp->wf_flag ) fdim[1] = grp->nelems;

/* Subgroup 0: PV names */
  if( grp->nsubgroups > 0 ) {
	subgname = grp->subg_name[0];
	dset_id = H5Dcreate(group_id,subgname,strtype_id,npvs_id,H5P_DEFAULT);
  } else {
	sprintf(gname,"/%s",grp->name);
	dset_id = H5Dcreate(file,gname,strtype_id,npvs_id,H5P_DEFAULT);
  }
  status = H5Dwrite(dset_id, strtype_id, H5S_ALL , H5S_ALL, H5P_DEFAULT, pvnames);
  H5Dclose(dset_id);
  H5Tclose(strtype_id);
  free(pvnames);

/* Subgroup 1: PV_Values */
/* AAA */
  subgname = grp->subg_name[1];
  switch (grp->dbrtype) {
	case DBR_INT:
		dset_id = H5Dcreate(group_id,subgname,H5T_NATIVE_SHORT, npvs_id, H5P_DEFAULT);
		status = H5Dwrite(dset_id, H5T_NATIVE_SHORT, H5S_ALL , H5S_ALL, H5P_DEFAULT,
			&grp->values[0]);
		break;
	case DBR_LONG:
		pi = (int *)grp->values;
		dset_id = H5Dcreate(group_id,subgname,H5T_NATIVE_INT, npvs_id, H5P_DEFAULT);
		status = H5Dwrite(dset_id, H5T_NATIVE_INT, H5S_ALL , H5S_ALL, H5P_DEFAULT, pi);
		break;
	case DBR_FLOAT:
		pf = (float *)grp->values;
		if( grp->wf_flag == 1 ) {
			sid = H5Screate_simple(2, fdim, NULL);
			dset_id = H5Dcreate(group_id,subgname,H5T_NATIVE_FLOAT,sid,H5P_DEFAULT);
		} else {
			dset_id = H5Dcreate(group_id,subgname,H5T_NATIVE_FLOAT,npvs_id,H5P_DEFAULT);
		}
		status = H5Dwrite(dset_id, H5T_NATIVE_FLOAT, H5S_ALL , H5S_ALL, H5P_DEFAULT,pf);
		if( grp->wf_flag == 1 ) H5Sclose(sid);
		break;
	case DBR_DOUBLE:
		dset_id = H5Dcreate(group_id,subgname,H5T_NATIVE_DOUBLE, npvs_id, H5P_DEFAULT);
		status = H5Dwrite(dset_id, H5T_NATIVE_DOUBLE, H5S_ALL , H5S_ALL, H5P_DEFAULT,
			&grp->values[0]);
		break;
	case DBR_STRING:
		strtype_id = H5Tcopy(H5T_C_S1);
		H5Tset_size(strtype_id, TS_LENGTH);
		dset_id = H5Dcreate(group_id,subgname,strtype_id, npvs_id, H5P_DEFAULT);
		status = H5Dwrite(dset_id, strtype_id, H5S_ALL , H5S_ALL, H5P_DEFAULT,
			&grp->values[0]);
		H5Tclose(strtype_id);
		break;
  }
  H5Sclose( npvs_id );
  H5Gclose( group_id );
  H5Dclose( dset_id );
}


int gd_saveegroups(file)
hid_t *file;
{
  int ig, ier;
  GD_EGROUP *egrps;

#ifdef GD_DEB
  fprintf(stderr,"**** Enter gd_saveegroup: %d\n",gdrecptr->ntopegroups);
#endif

  egrps = gd_egroups;
  for(ig=0;ig<gdrecptr->ntopegroups;ig++) {
	if( egrps->err == 0 )
		gd_saveeg_toh5file(file,ig);

	egrps++;
  }

#ifdef GD_DEB
  fprintf(stderr,"**** Exit from gd_saveegroup\n");
#endif
  return 0;
}

/* NNN */

int gd_readegroupvals(connected)
int connected;
{
  int i, ig, ier;
  GD_EGROUP *egrps;
  int *lval;
  GD_PV *ppv;

#ifdef GD_DEB
  fprintf(stderr,"\n************ Enter gd_readegroupvals\n");
#endif

  egrps = gd_egroups;

  for(ig=0;ig<gdrecptr->ntopegroups;ig++) {
	ier = gd_readpvs_intovalue(egrps->npvs, egrps->pvs, egrps->dbrtype,
		egrps->values, connected);
	lval = (int *)egrps->values;
/*
	ppv = egrps->pvs;
	for( i = 0; i<egrps->npvs; i++) {
		fprintf(stderr, "    ----- %s: %d\n", ppv->name, *lval);
		++ppv;
		lval++;
	}
*/
	egrps->err = ier;
	egrps->nelems = (egrps->pvs)->nelems;
	egrps++;
  }

#ifdef GD_DEB
  fprintf(stderr,"************ Exit from gd_readegroupvals\n");
#endif
  return 0;
}
