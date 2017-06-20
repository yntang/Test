#include "gendl.h"
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <hdf5.h>

int elapsetime();

int gd_readpvs_intohdf5data();

char *gd_getsavepath()
{
  int i,ind;
  char yearname[8], monname[4], dayname[4];
  static char savepath[132];

  ind = gdrecptr->ts_change_index;
  yearname[0] = 'Y';
  for(i=0;i<4;i++)
	yearname[i+1] = gdrecptr->time_stamp[ind][i];
  yearname[i+1] = 0;

  monname[0] = 'M';
  for(i=0;i<2;i++)
	monname[i+1] = gdrecptr->time_stamp[ind][i+4];
  monname[i+1] = 0;

  dayname[0] = 'D';
  for(i=0;i<2;i++)
	dayname[i+1] = gdrecptr->time_stamp[ind][i+6];
  dayname[i+1] = 0;

  sprintf(savepath,"%s/%s/%s/%s",gdrecptr->datapath, yearname,monname,dayname);
  return savepath;
}


static group_id_flag;

void gd_close_groupid(id)
hid_t id;
{
  if(group_id_flag == 1) {
	H5Gclose(id);
	group_id_flag = 0;
  }
}

#define NEW
int gd_savewf_toh5file()
{
  int i,j,ig,niter;
  char path[160];
  char *dirname;
  PV_NAME *pvnames;
  TS_STR *tmstamps;
  int trig_values[GD_MAXTRIGGERS];
  GD_PV *ppv;
  GD_GROUP *grp;
  char *subgn, gname[16];

  hid_t file;
  herr_t status;

  hsize_t npvs, nelems_per_pv, n, nels=1;
  hid_t strtype_id, group_id, dset_id, t_id;
  hid_t s_id;
  hid_t pid;
  hsize_t fdim[2],dim[1];

  int *holder;
  float *fholder;
  B40B40B4B4 *strigs;

  npvs = (hsize_t)(gdrecptr->npvs);
  nelems_per_pv = (hsize_t)(gdrecptr->h5_dim[1]);

  dirname = gd_getsavepath();
  sprintf(path, "%s/%s-%s.h5",dirname, gdrecptr->save_prefix,
	gdrecptr->time_stamp[gdrecptr->ts_change_index]);
#ifdef GD_DEB
  fprintf(stderr,"Output path: \033[1;32m%s::%s\033[0m\n",gdrecptr->host,path);
#endif

  if( gdrecptr->large_attr_flag ) {
	pid = H5Pcreate( H5P_FILE_ACCESS );
	status = H5Pset_libver_bounds(pid, H5F_LIBVER_LATEST, H5F_LIBVER_LATEST);
  }

  if( gdrecptr->large_attr_flag ) {
	file = H5Fcreate(path, H5F_ACC_TRUNC, H5P_DEFAULT, pid);
  } else {
	file = H5Fcreate(path, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
  }

  for(ig=0;ig<gdrecptr->ntopgroups;ig++) {
	grp = gd_groups + ig;
	if( grp->type == GD_PLAIN ) {
		if( grp->nsubgroups > 0 ) {
			niter = grp->nsubgroups;
			group_id = H5Gcreate(file, grp->name, 0);
			group_id_flag = 1;
		} else {
			niter = 1;
		}
	} else {
		niter = 1;
	}

	if(!strcmp(grp->name,"Meta")) {
	    if( grp->type == GD_STRUCT ) {
		holder = (int *)meta.item1_4;
		*holder = gdrecptr->npvs;
		holder = (int *)meta.item2_4;
		*holder = gdrecptr->nelems_per_pv;

		t_id = H5Tcreate(H5T_COMPOUND, sizeof(B4B4));
		H5Tinsert(t_id, "npvs", HOFFSET(B4B4, item1_4),H5T_NATIVE_INT);
		H5Tinsert(t_id, "nelems", HOFFSET(B4B4, item2_4),H5T_NATIVE_INT);
/*
		dim[0] = 1;
		s_id = H5Screate_simple(1,dim,NULL);
*/
		s_id = H5Screate(H5S_SCALAR);

		sprintf(gname,"/%s",grp->name);

		dset_id = H5Dcreate(file, gname, t_id, s_id, H5P_DEFAULT);
		status = H5Dwrite(dset_id, t_id, H5S_ALL , H5S_ALL, H5P_DEFAULT, &meta);
		H5Sclose(s_id);
		H5Tclose(t_id);
		H5Dclose(dset_id);
	    } else {
		for(i=0;i<grp->nsubgroups;i++) {
			subgn = (char *)(grp->subg_name[i]);
			if( !strcmp(subgn,"Num_PVs") ) {
				s_id = H5Screate(H5S_SCALAR);
				dset_id = H5Dcreate(group_id, subgn,
					H5T_NATIVE_INT, s_id, H5P_DEFAULT);
				status = H5Dwrite(dset_id, H5T_NATIVE_INT,
					H5S_ALL , H5S_ALL, H5P_DEFAULT, &npvs); 
				H5Sclose(s_id);
				H5Dclose(dset_id);
			}
			if( !strcmp(subgn,"Nelems_perPV") ) {
				s_id = H5Screate(H5S_SCALAR);
  				dset_id = H5Dcreate(group_id, subgn,
					H5T_NATIVE_INT, s_id, H5P_DEFAULT);
				status = H5Dwrite(dset_id, H5T_NATIVE_INT, H5S_ALL,
					H5S_ALL, H5P_DEFAULT, &gdrecptr->nelems_per_pv); 
				H5Sclose(s_id);
				H5Dclose(dset_id);
			}
		}
	    }
	}

	if(!strcmp(grp->name,"PV_Names")) {
		pvnames = (PV_NAME *)malloc(npvs * PV_NAMELEN);
		ppv = gd_pvs;
		for(i=0;i<npvs;i++) {
			strcpy((char *)(pvnames+i),(char *)ppv->name);
			++ppv;
		}
		strtype_id = H5Tcopy(H5T_C_S1);
		H5Tset_size(strtype_id, PV_NAMELEN);

		for(i=0;i<niter;i++) {
			if( grp->nsubgroups > 0 ) {
				subgn = grp->subg_name[i];
				n = grp->subg_end[i] - grp->subg_start[i] + 1;
			} else {
				sprintf(gname,"/%s",grp->name);
				n = npvs;
			}
			s_id = H5Screate_simple(1, &n, NULL);
			if( grp->nsubgroups > 0 )
				dset_id = H5Dcreate(group_id, subgn, strtype_id,s_id,H5P_DEFAULT);
			else
				dset_id = H5Dcreate(file, gname, strtype_id,s_id,H5P_DEFAULT);

			if( grp->nsubgroups > 0 ) {
				status = H5Dwrite(dset_id, strtype_id, H5S_ALL ,
					H5S_ALL, H5P_DEFAULT, pvnames+grp->subg_start[i]);
			} else {
				status = H5Dwrite(dset_id, strtype_id, H5S_ALL ,
					H5S_ALL, H5P_DEFAULT, pvnames);
			}
			H5Dclose(dset_id);
			H5Sclose(s_id);
		}
		H5Tclose(strtype_id);
		free(pvnames);
	}
	if(!strcmp(grp->name,"Trigger")) {
	    if(grp->type == GD_STRUCT) {
		strigs = trigs = (B40B40B4B4 *)malloc(gdrecptr->ntriggers * sizeof(B40B40B4B4));
		for(i=0;i<gdrecptr->ntriggers;i++) {
			strcpy( strigs->item1_40, (char *)gdrecptr->trigger_pvs[i].name);
			strcpy( strigs->item2_40, (char *)gdrecptr->time_stamp[i]);
			if( gdrecptr->trigger_type == GD_TRIGGER_LESS ||
			    gdrecptr->trigger_type == GD_TRIGGER_MORE ) {
				fholder = (float *)strigs->item3_4;
				*fholder = gdrecptr->trigger_frtvalues[i];
				fholder = (float *)strigs->item4_4;
				*fholder = gdrecptr->trigger_values[i];;
			} else {
				holder = (int *)strigs->item3_4;
				*holder = gdrecptr->trigger_rtvalues[i];
				holder = (int *)strigs->item4_4;
				*holder = 1 - gdrecptr->trigger_values[i];;
			}
			++strigs;
		}
			
		t_id = H5Tcreate(H5T_COMPOUND, sizeof(B40B40B4B4));

		strtype_id = H5Tcopy(H5T_C_S1);
		H5Tset_size(strtype_id, PV_NAMELEN);
		H5Tinsert( t_id, "PV_names", HOFFSET(B40B40B4B4, item1_40),strtype_id);
		H5Tclose(strtype_id);

		strtype_id = H5Tcopy(H5T_C_S1);
		H5Tset_size(strtype_id, TS_LENGTH);
		H5Tinsert( t_id, "Time_stamps", HOFFSET(B40B40B4B4, item2_40),strtype_id);
		H5Tclose(strtype_id);

		if( gdrecptr->trigger_type == GD_TRIGGER_LESS ||
			gdrecptr->trigger_type == GD_TRIGGER_MORE ) {
			H5Tinsert(t_id, "Curr Values",HOFFSET(B40B40B4B4,item3_4),H5T_NATIVE_FLOAT);
			H5Tinsert(t_id, "Norm Values",HOFFSET(B40B40B4B4,item4_4),H5T_NATIVE_FLOAT);
		} else {
			H5Tinsert(t_id, "Curr Values", HOFFSET(B40B40B4B4, item3_4),H5T_NATIVE_INT);
			H5Tinsert(t_id, "Norm Values", HOFFSET(B40B40B4B4, item4_4),H5T_NATIVE_INT);
		}

		dim[0] = gdrecptr->ntriggers;
		s_id = H5Screate_simple(1,dim,NULL);

		sprintf(gname,"/%s",grp->name);

		dset_id = H5Dcreate(file, gname, t_id, s_id, H5P_DEFAULT);
		status = H5Dwrite(dset_id, t_id, H5S_ALL , H5S_ALL, H5P_DEFAULT, trigs);
		H5Tclose(t_id);
		H5Sclose(s_id);
		H5Dclose(dset_id);
	    } else {
		for(i=0;i<grp->nsubgroups;i++) {
			subgn = (char *)(grp->subg_name[i]);
			if( !strcmp(subgn,"PV_names") ) {
				strtype_id = H5Tcopy(H5T_C_S1);
				H5Tset_size(strtype_id, PV_NAMELEN);
				if( gdrecptr->version <= 2.0 ) {
					nels = 1;
				} else if( gdrecptr->version < 5.9 ) {
					nels = gdrecptr->ntriggers;
				}
				s_id = H5Screate_simple(1, &nels, NULL);
				dset_id = H5Dcreate(group_id, subgn, strtype_id, s_id, H5P_DEFAULT);
				if( gdrecptr->version <= 2.0 ) {
					status = H5Dwrite(dset_id, strtype_id, H5S_ALL ,
						H5S_ALL, H5P_DEFAULT,
						(char *)gdrecptr->trigger_pvs[0].name);
				} else if( gdrecptr->version <= 5.90) {
					pvnames = (PV_NAME *)malloc(gdrecptr->ntriggers*PV_NAMELEN);
					for(j=0;j<gdrecptr->ntriggers;j++) {
						strcpy((char *)(pvnames+j),
							gdrecptr->trigger_pvs[j].name);
					}
					status = H5Dwrite(dset_id, strtype_id, H5S_ALL ,
						H5S_ALL, H5P_DEFAULT, (char *)pvnames);
					free(pvnames);
				}
				H5Tclose(strtype_id);
				H5Sclose(s_id);
				H5Dclose(dset_id);
			}
			if( !strcmp(subgn,"Time_stamps") ) {
				strtype_id = H5Tcopy(H5T_C_S1);
				H5Tset_size(strtype_id, TS_LENGTH);
				if( gdrecptr->version <= 2.0 ) {
					nels = 1;
				} else if( gdrecptr->version <= 5.90) {
					nels = gdrecptr->ntriggers;
				}
				s_id = H5Screate_simple(1, &nels, NULL);
				dset_id = H5Dcreate(group_id, subgn, strtype_id, s_id, H5P_DEFAULT);
				if( gdrecptr->version <= 2.0 ) {
					status = H5Dwrite(dset_id, strtype_id, H5S_ALL ,
						H5S_ALL, H5P_DEFAULT,
						(char *)gdrecptr->time_stamp[0]); /* early version*/
				} else if( gdrecptr->version <= 5.90 ) {
					tmstamps = (TS_STR *)malloc(gdrecptr->ntriggers*TS_LENGTH);
					for(j=0;j<gdrecptr->ntriggers;j++) {
						strcpy((char *)(tmstamps+j),
							gdrecptr->time_stamp[j]);
					}
					status = H5Dwrite(dset_id, strtype_id, H5S_ALL ,
						H5S_ALL, H5P_DEFAULT, (char *)tmstamps);
					free(tmstamps);
				}
				H5Tclose(strtype_id);
				H5Sclose(s_id);
				H5Dclose(dset_id);
			}
			if( !strcmp(subgn,"Trigger_values" )) {
				if( gdrecptr->version <= 2.0 ) {
					s_id = H5Screate(H5S_SCALAR);
				} else if( gdrecptr->version <= 5.90 ) {
					nels = gdrecptr->ntriggers;
					s_id = H5Screate_simple(1, &nels, NULL);
				}
				dset_id = H5Dcreate(group_id, subgn,
					H5T_NATIVE_INT, s_id, H5P_DEFAULT);
				if( gdrecptr->version <= 2.0 ) {
					status = H5Dwrite(dset_id, H5T_NATIVE_INT,
						H5S_ALL , H5S_ALL, H5P_DEFAULT,
						&gdrecptr->trigger_rtvalues[0]); 
				} else if( gdrecptr->version <= 5.90 ) {
					for(j=0;j<gdrecptr->ntriggers;j++) {
						trig_values[j] = gdrecptr->trigger_rtvalues[j];
					}
					status = H5Dwrite(dset_id, H5T_NATIVE_INT,
						H5S_ALL , H5S_ALL, H5P_DEFAULT,&trig_values[0]);
				}
				H5Sclose(s_id);
				H5Dclose(dset_id);
			}
		} /* Loop for subgroups */
	    }
	}

	if(!strcmp(grp->name,"WFdata")) {
		for(i=0;i<niter;i++) {
			if( grp->nsubgroups > 0 ) {
				fdim[0] = grp->subg_end[i] - grp->subg_start[i] + 1;
			} else {
				fdim[0] = npvs;
			}
			fdim[1] = nelems_per_pv;
			s_id = H5Screate_simple(2, fdim, NULL);
			if( grp->nsubgroups > 0 ) {
				subgn = grp->subg_name[i];
				dset_id = H5Dcreate(group_id, subgn, H5T_NATIVE_FLOAT, s_id,
					H5P_DEFAULT);
				status = H5Dwrite(dset_id, H5T_NATIVE_FLOAT,
					H5S_ALL , H5S_ALL, H5P_DEFAULT,
					&h5_data[grp->subg_start[i]][0]);
			} else {
				sprintf(gname,"/%s",grp->name);
				dset_id = H5Dcreate(file, gname, H5T_NATIVE_FLOAT, s_id,
					H5P_DEFAULT);
				status = H5Dwrite(dset_id, H5T_NATIVE_FLOAT,
					H5S_ALL , H5S_ALL, H5P_DEFAULT, &h5_data[0][0]);
			}
			H5Dclose(dset_id);
			H5Sclose(s_id);
		}
	}

	if(!strcmp(grp->name,"PV_TimeStamp")) {
		for(i=0;i<niter;i++) {
			strtype_id = H5Tcopy(H5T_C_S1);
			H5Tset_size(strtype_id, TS_LENGTH);
			if( grp->nsubgroups > 0 ) {
				n = grp->subg_end[i] - grp->subg_start[i] + 1;
			} else {
				n = npvs;
			}
			s_id = H5Screate_simple(1, &n, NULL);
			if( grp->nsubgroups > 0 ) {
				subgn = grp->subg_name[i];
				dset_id = H5Dcreate(group_id, subgn, strtype_id, s_id, H5P_DEFAULT);
				status = H5Dwrite(dset_id, strtype_id, H5S_ALL , H5S_ALL,
					H5P_DEFAULT, pvts + grp->subg_start[i]);
			} else {
				sprintf(gname,"/%s",grp->name);
				dset_id = H5Dcreate(file, gname, strtype_id, s_id, H5P_DEFAULT);
				status = H5Dwrite(dset_id, strtype_id, H5S_ALL , H5S_ALL,
					H5P_DEFAULT, pvts);
			}
			H5Tclose(strtype_id);
			H5Dclose(dset_id);
			H5Sclose(s_id);
		}
	}
	if(!strcmp(grp->name,"PV_StopAddr")) {
		for(i=0;i<niter;i++) {
			if( grp->nsubgroups > 0 ) {
				n = grp->subg_end[i] - grp->subg_start[i] + 1;
			} else {
				n = gdrecptr->naddrs;
			}
			s_id = H5Screate_simple(1, &n, NULL);
			if( grp->nsubgroups > 0 ) {
				subgn = grp->subg_name[i];
				dset_id = H5Dcreate(group_id,subgn,H5T_NATIVE_LONG,s_id,H5P_DEFAULT);
				status = H5Dwrite(dset_id, H5T_NATIVE_INT, H5S_ALL , H5S_ALL,
					H5P_DEFAULT, addr + grp->subg_start[i]);
			} else {
				sprintf(gname,"/%s",grp->name);
				dset_id = H5Dcreate(file, gname, H5T_NATIVE_LONG, s_id, H5P_DEFAULT);
				status = H5Dwrite(dset_id, H5T_NATIVE_INT, H5S_ALL , H5S_ALL,
					H5P_DEFAULT, addr);
			}
			H5Sclose(s_id);
			H5Dclose(dset_id);
		}
	}
	gd_close_groupid( group_id );
  }

  if( gdrecptr->ntopegroups > 0 ) gd_saveegroups(file);

  H5Fflush(file, H5F_SCOPE_GLOBAL);
  H5Fclose(file);

  return 0;
}


int gd_savewf(connected)
int connected;
{
  int i, ipv, ier, rier, npvs;
  int et1;
  int ntimes, nrest;
  char tname[16], sline[132];
  GD_PV *pvs;
  FILE *tfile;
  TS_STR *ts;
  struct dbr_time_float *tf;
  epicsTimeStamp stamp;
  struct dbr_time_float *tint;
  char *timeFormatStr = "%Y%m%d-%H:%M:%S.%06f";

#ifdef GD_DEB
  fprintf(stderr,"**** Enter gd_savewf: %d\n",gdrecptr->npvs_per_read);
#endif

  ipv = 0;
  npvs = gdrecptr->npvs;
  pvs = gd_pvs;

  elapsetime(0);

  if( gdrecptr->async_flag == 0 ) {
	if( gdrecptr->npvs_per_read == 1) {
		for(ipv = 0; ipv<npvs; ipv++) {
			rier = gd_readpvs_intohdf5data(1, pvs, ipv, connected);
			ier = gd_readtimestamp(1, pvs, ipv, connected);
			++pvs;
		}
	} else {
		ntimes = npvs/gdrecptr->npvs_per_read;
		if( (npvs%(gdrecptr->npvs_per_read)) > 0 ) {
			++ntimes;
			nrest = npvs%gdrecptr->npvs_per_read;
  		} else {
			nrest = gdrecptr->npvs_per_read;
		}

		for(i=0;i<ntimes;i++) {
			if(i == ntimes - 1) {
				rier = gd_readpvs_intohdf5data(nrest, pvs, ipv, connected);
				ier = gd_readtimestamp(nrest, pvs, ipv, connected);
				pvs += nrest;
				ipv += nrest;
			} else {
				rier = gd_readpvs_intohdf5data(gdrecptr->npvs_per_read,
					pvs, ipv, connected);
				ier = gd_readtimestamp(gdrecptr->npvs_per_read,
					pvs, ipv, connected);
				pvs += gdrecptr->npvs_per_read;
				ipv += gdrecptr->npvs_per_read;
			}
			if( rier == -1 ) {
				sprintf(sline,"*****  Error in gd_readpvs_intohdf5data"); 
				gd_log(sline);
				return -1;
			}
		}
	}
	if( gdrecptr->naddrs > 0 )
		ier = gd_readaddr(gdrecptr->naddrs, gd_addrpvs, 0, connected);
	if( gdrecptr->ntopegroups > 0 )
		ier = gd_readegroupvals(connected);

	ts = pvts;
	tf = timefloat;
	for(i=0;i<gdrecptr->npvs;i++) {
		stamp = (epicsTimeStamp)tf->stamp;
		epicsTimeToStrftime(ts, TS_LENGTH, timeFormatStr, &stamp);
		++tf;
		++ts;
	}
	et1 = elapsetime(1);
	fprintf(stderr,"     Elapse time for read: %.5f secs\n", et1/1000000.0);
	sprintf(tname,"%s_Timing.dat",gdrecptr->save_prefix);
	tfile = fopen(gdrecptr->timepath,"a");

	if( tfile != NULL ) {
		fprintf(tfile,"Elapse time for reading WFs: %.5f secs\n", et1/1000000.0);
		fclose(tfile);
	}

	gd_savewf_toh5file();
  } else {
	ier = gd_asynreadpvs_intohdf5data(connected);
  }

#ifdef GD_DEB
  fprintf(stderr,"Exit from savewf\n");
#endif
  return 0;
}
