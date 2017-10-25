#ifndef _FTIBM_H
#define _FTIBM_H

#include <mpi.h>
#include <fti.h>
#include <errno.h>
#include "include/iniparser/iniparser.h"
#include "include/iniparser/dictionary.h"
#ifdef FTI_LUSTRE
#  include "lustreapi.h"
#endif
#include <sys/stat.h>
#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <math.h>

unsigned long long SIZE;

void init_config_file(const char *config_file);
static inline int maxint(int a, int b);
static void *alloc_lum();
void parse_arguments(int argc, char *argv[], int rank);

extern int errno;

int rank, size, perr, ierr, ierr_len, striping_factor, R_SF_SET, L_SF;
char FTI_LOCAL_DIR[256], FTI_GLOBAL_DIR[256], FTI_META_DIR[256];
int FTI_CPU_PER_NODE, FTI_GROUP_SIZE, FTI_MAX_SYNC_ITER, FTI_LOCAL_TEST;
long L_SU;
int NUM_ITER; // max 100 iterations
double start, end;
double dTPosixReadMean, dTPosixReadSigma;
double dTPosixWriteMean, dTPosixWriteSigma;
double dTMpiReadMean, dTMpiReadSigma;
double dTMpiWriteMean, dTMpiWriteSigma;
double dTPosixRead[100], dTPosixWrite[100], dTMpiRead[100], dTMpiWrite[100]; 
double dTFtiRead[800], dTFtiWrite[800]; // max 100 iterations
double dTFtiReadMean[8], dTFtiWriteMean[8];
double dTFtiReadSigma[8], dTFtiWriteSigma[8];
char *arr;
FILE *fd;
char tmpdir[10];
char config_file[256];
char tempfile[256];
char ierr_str[256];
char R_SU[256];
char R_SF[256];

#ifdef FTI_LUSTRE
struct lov_user_md *lum_file;
#endif
MPI_File pfh;
MPI_Info info;
MPI_Status status;
MPI_Comm gcomm;

#endif
