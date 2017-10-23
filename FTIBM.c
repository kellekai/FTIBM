#include "include/FTIBM_decl.h"

int main(int argc, char *argv[]) {
    
    int i;

    MPI_Init(NULL,NULL);
    MPI_Comm_rank(MPI_COMM_WORLD,&rank);
    MPI_Comm_size(MPI_COMM_WORLD,&size);    
    arr = (char*) malloc(SIZE);
    
    if (rank == 0) {
        init_config_file(config_file);
    }
    
    MPI_Barrier(MPI_COMM_WORLD);

    parse_arguments(argc, argv);

    if (rank == 0) {
        printf("\n"
               "#######################\n"
               "##   CONFIGURATION   ##\n"
               "#######################\n"
               "\n"
               "Number of processes: %i\n"
               "Allocated memory per process: \n%llu B | %lf MB | %lf GB\n"
               "Allocated memory in total: \n%llu B | %lf MB | %lf GB\n"
               "\n"
               "#######################\n"
               "##     START TEST    ##\n"
               "#######################\n"
               "\n",
               size,
               SIZE,
               (SIZE*1.0)/(1024*1024),
               (SIZE*1.0)/(1024*1024*1024),
               SIZE*size,
               (SIZE*1.0*size)/(1024*1024),
               (SIZE*1.0*size)/(1024*1024*1024));
    } 

    MPI_Barrier(MPI_COMM_WORLD);

// +-----------------------------------------------------------------------------+

// +================+    
// | PURE MPI BEGIN |
// +================+

    MPI_Info_create(&info);
    MPI_Info_set(info, "romio_cb_write", "enable");
    MPI_Info_set(info, "striping_unit", R_SU);
    
    if (R_SF_SET) {
        MPI_Info_set(info, "striping_factor", R_SF); // set striping over # of I/O units manually
    }
    for(i=0; i<NUM_ITER; i++) {
        
        if ( rank == 0 ){
            printf("\n## PURE MPIIO - ITERATION %i ##\n\n",i+1);
        }

        // reset temp file pattern
        strcpy(tmpdir,"tmpXXXXXX");

        if(rank == 0) {
            printf("MPIIO: start writing in file\n");
            // create temp directory
            if (mkdtemp(tmpdir) == NULL) {
                perr = errno;
                printf("'mkdtemp() call failed' [POSIX ERROR]: %s\n", strerror(perr));
                perr = 0;
                MPI_Abort(MPI_COMM_WORLD, -1);
            }
        }

        MPI_Bcast(tmpdir,10,MPI_CHAR,0,MPI_COMM_WORLD);
        sprintf(tempfile,"%s/mpiio.f",tmpdir);

#ifdef FTI_LUSTRE
        if(rank == 0) {
            /* 
             * default L_SU = 4194304 -> 4MB ## striping unit
             * L_SF = -1 -> stipe over all OST's ## striping factor
             */
            llapi_file_create(tempfile, L_SU, -1, L_SF, 0);
            //llapi_file_create("tmp/test.file", 0, -1, 0, 0);  // set to Lustre default.
            lum_file = alloc_lum();
            llapi_file_get_stripe(tempfile, lum_file); 
            printf("stripe-size: %d, stripe-count: %d\n", 
                    lum_file->lmm_stripe_size, lum_file->lmm_stripe_count);
        }
#endif

        /* BEGIN WRITE */

        start = MPI_Wtime();
        ierr = MPI_File_open(MPI_COMM_WORLD, tempfile, MPI_MODE_WRONLY|MPI_MODE_CREATE, info, &pfh);
        //MPI_File_open(MPI_COMM_WORLD, tempfile, MPI_MODE_WRONLY, MPI_INFO_NULL, &pfh); // do not pass hints

        if (ierr!=0) {
            perr = errno;
            printf("'MPI_File_open call failed' [POSIX ERROR]: %s\n",strerror(perr));
            MPI_Error_string(ierr, ierr_str, &ierr_len);
            printf("'MPI_File_open call failed' [MPI ERROR]: %s\n", ierr_str);
            perr = 0;
            MPI_Abort(MPI_COMM_WORLD, -1);
        }

        ierr = MPI_File_write_at(pfh, SIZE*rank, arr, SIZE, MPI_CHAR, &status);

        if (ierr!=0) {
            perr = errno;
            printf("'MPI_File_write_at call failed' [POSIX ERROR]: %s\n",strerror(perr));
            MPI_Error_string(ierr, ierr_str, &ierr_len);
            printf("'MPI_File_write_at call failed' [MPI ERROR]: %s\n", ierr_str);
            perr = 0;
            MPI_File_close(&pfh);
            MPI_Abort(MPI_COMM_WORLD, -1);
        }

        MPI_File_close(&pfh);
        end = MPI_Wtime();

        dTMpiWrite[i] = end-start;

        /* END  WRITE */

        MPI_Barrier(MPI_COMM_WORLD);

        if(rank == 0) {
            printf("MPIIO: finished write in %lf seconds!\n", dTMpiWrite[i]);
        }

        if(rank == 0) {
            printf("MPIIO: Start reading from file\n");
        }

        MPI_Barrier(MPI_COMM_WORLD);

        /* BEGIN READ */

        start = MPI_Wtime();
        ierr = MPI_File_open(MPI_COMM_WORLD, tempfile, MPI_MODE_RDONLY, info, &pfh);
        //MPI_File_open(MPI_COMM_WORLD, tempfile, MPI_MODE_WRONLY, MPI_INFO_NULL, &pfh); // do not pass hints

        if (ierr!=0) {
            perr = errno;
            printf("'MPI_File_open call failed' [POSIX ERROR]: %s\n",strerror(perr));
            MPI_Error_string(ierr, ierr_str, &ierr_len);
            printf("'MPI_File_open call failed' [MPI ERROR]: %s\n", ierr_str);
            perr = 0;
            MPI_Abort(MPI_COMM_WORLD, -1);
        }

        ierr = MPI_File_read_at(pfh, SIZE*rank, arr, SIZE, MPI_CHAR, &status);

        if (ierr!=0) {
            perr = errno;
            printf("'MPI_File_read_at call failed' [POSIX ERROR]: %s\n",strerror(perr));
            MPI_Error_string(ierr, ierr_str, &ierr_len);
            printf("'MPI_File_read_at call failed' [MPI ERROR]: %s\n", ierr_str);
            perr = 0;
            MPI_File_close(&pfh);
            MPI_Abort(MPI_COMM_WORLD, -1);
        }

        MPI_File_close(&pfh);
        end = MPI_Wtime();

        dTMpiRead[i] = end-start;

        /* END  WRITE */

        if(rank == 0) {
            printf("MPIIO: finished read in %lf seconds!\n", dTMpiRead[i]);
            remove(tempfile);
            rmdir(tmpdir);
        }
        MPI_Barrier(MPI_COMM_WORLD);
    }    

// +==============+
// | PURE MPI END |
// +==============+

// +-----------------------------------------------------------------------------+

// +==================+
// | PURE POSIX BEGIN |
// +==================+

    // reset temp file pattern
    strcpy(tmpdir,"tmpXXXXXX");

    if(rank == 0) {
        if (mkdtemp(tmpdir) == NULL) {
            perr = errno;
            printf("'mkdtemp call failed' [POSIX ERROR]: %s\n", strerror(perr));
            perr = 0;
            MPI_Abort(MPI_COMM_WORLD, -1);
        }
    }
    
    MPI_Barrier(MPI_COMM_WORLD);

    MPI_Bcast(tmpdir,10,MPI_CHAR,0,MPI_COMM_WORLD);
    
    sprintf(tempfile, "%s/rank-%i.f", tmpdir, rank);

/* BEGIN WRITE */

    for(i=0; i<NUM_ITER; i++) {
        
        if( rank == 0 ) {
            printf("\n## PURE POSIX - ITERATION %i ##\n\n",i+1 );
            printf("POSIX: start writing in file\n");
        }        
        
        start = MPI_Wtime();

        fd = fopen(tempfile, "wb");  

        if (fd == NULL) {
            perr = errno;
            printf("'fopen call failed' [POSIX ERROR - %i]: %s\n", 
                    rank, strerror(perr));
            perr = 0;
            rmdir(tmpdir);
#ifdef FTI_LUSTRE
            free(lum_file);
#endif
            MPI_Abort(MPI_COMM_WORLD, -1);
        }

        long written = 0;
        char *ptrWrite = (char*) arr;
        while(written < SIZE) {
            written += fwrite(ptrWrite, sizeof(char), SIZE - written, fd);

            if (written == 0) {
                perr = errno;
                printf("'fwrite call failed' [POSIX ERROR - %i]: %s\n", 
                        rank, strerror(perr));
                perr = 0;
                fclose(fd);
                remove(tempfile);
                rmdir(tmpdir);
#ifdef FTI_LUSTRE
                free(lum_file);
#endif
                MPI_Abort(MPI_COMM_WORLD, -1);
            } else if (written < SIZE) { 
                printf("%lu Bytes of %lu Bytes written...", written, SIZE);
            }

            ptrWrite += written; 
        }

        fclose(fd); 

        MPI_Barrier(MPI_COMM_WORLD);    
        end = MPI_Wtime();

        dTPosixWrite[i] = end-start;

        /* END WRITE */

        if(rank == 0) {
            printf("POSIX: finished write in %lf seconds!\n", dTPosixWrite[i]);
            printf("POSIX: start reading from file\n");
        }

        /* BEGIN READ */

        MPI_Barrier(MPI_COMM_WORLD);    
        start = MPI_Wtime();

        fd = fopen(tempfile, "rb");  

        if (fd == NULL) {
            perr = errno;
            printf("'fopen call failed' [POSIX ERROR - %i]: %s\n", 
                    rank, strerror(perr));
            perr = 0;
            rmdir(tmpdir);
#ifdef FTI_LUSTRE
            free(lum_file);
#endif
            MPI_Abort(MPI_COMM_WORLD, -1);
        }

        long read = 0;
        char *ptrRead = (char*) arr;
        while(read < SIZE) {
            read += fread(ptrRead, sizeof(char), SIZE - read, fd);

            if (read == 0) {
                perr = errno;
                printf("'fread call failed' [POSIX ERROR]: %s\n",strerror(perr));
                perr = 0;
                fclose(fd);
                remove(tempfile);
                rmdir(tmpdir);
#ifdef FTI_LUSTRE
                free(lum_file);
#endif
                MPI_Abort(MPI_COMM_WORLD, -1);
            } else if (written < SIZE) { 
                printf("%lu Bytes of %lu Bytes written...", written, SIZE);
            }

            ptrRead += read; 
        }

        fclose(fd); 

        MPI_Barrier(MPI_COMM_WORLD);    
        end = MPI_Wtime();

        dTPosixRead[i] = end-start;

        /* END READ */

        if(rank == 0) {
            printf("POSIX: finished read in %lf seconds!\n", dTPosixRead[i]);
        }

#ifdef FTI_LUSTRE
        if(rank == 0) {
            llapi_file_get_stripe(tempfile, lum_file); 
            printf("stripe-size: %d, stripe-count: %d\n", 
                    lum_file->lmm_stripe_size, lum_file->lmm_stripe_count);
        }
#endif
        remove(tempfile);
        MPI_Barrier(MPI_COMM_WORLD);    

    }

    if(rank == 0) {
        rmdir(tmpdir);
#ifdef FTI_LUSTRE
        free(lum_file);
#endif
    }

// +================+
// | PURE POSIX END |
// +================+

// +-----------------------------------------------------------------------------+

// +===========+
// | FTI BEGIN |
// +===========+

/* DO CHECKPOINT FOR ALL LEVELS AND DO RESTART FROM LEVEL 4 */

    dictionary *ini;
    int POSIXFlag, MPIIOFlag, keepFlag;
    
    for(i=0; i<(8*NUM_ITER); i++) {
        
        if(rank == 0 && (i%8 == 0) ) {
            printf("\n## FTI POSIX - ITERATION %i ##\n\n",i/8+1);
        }

        if(rank == 0 && (i%8 == 4) ) {
            printf("\n## FTI MPIIO - ITERATION %i ##\n\n",i/8+1);
        }

        // if level 4, keep last checkpoint
        keepFlag = ((i%4+1) == 4);
        POSIXFlag = (i%8 == 0);
        MPIIOFlag = (i%8 == 4);
        if (rank == 0 && ( keepFlag || POSIXFlag || MPIIOFlag )) {
            ini = iniparser_load(config_file);
            if (ini == NULL) {
                printf("[ERROR] can't open configuration file.\n");
                MPI_Abort(MPI_COMM_WORLD, -1);
            }
            fd = fopen(config_file, "w");
            if (keepFlag) {
                iniparser_set(ini, "Basic:keep_last_ckpt", "1");
            }
            if (POSIXFlag) {
            iniparser_set(ini, "Basic:ckpt_io", "1");
            }
            if (MPIIOFlag) {
            iniparser_set(ini, "Basic:ckpt_io", "2");
            }
            iniparser_dump_ini(ini, fd);
            fclose(fd);
            iniparser_freedict(ini);
        }
        MPI_Barrier(MPI_COMM_WORLD);
        if (FTI_Init(config_file, MPI_COMM_WORLD) != FTI_SCES) {
            printf("FTI: failed to initialize.\n");
            MPI_Abort(MPI_COMM_WORLD, -1);
        }
        FTI_Protect(0, arr, SIZE, FTI_CHAR);
        if (rank == 0) {
            printf("FTI: Perform level %i Checkpoint.\n", i%4+1);
        }
        start = MPI_Wtime();
        FTI_Checkpoint(i%4+1,i%4+1);
        end = MPI_Wtime();
        dTFtiWrite[i] = end-start;
        if (rank == 0) {
            printf("FTI: Level %i Checkpoint took: %lf seconds!.\n", 
                    i%4+1, dTFtiWrite[i]);
        }
        // if level 4, restart from PFS
        if ( i%4+1 == 4 ) {
            FTI_Finalize();
            if (rank == 0) {
                ini = iniparser_load(config_file);
                if (ini == NULL) {
                    printf("[ERROR] can't open configuration file.\n");
                    MPI_Abort(MPI_COMM_WORLD, -1);
                }
                fd = fopen(config_file, "w");
                // set keep_last_ckpt to 0 to have a cleanup at FTI_Finalize.
                iniparser_set(ini, "Basic:keep_last_ckpt", "0");
                iniparser_dump_ini(ini, fd);
                fclose(fd);
                iniparser_freedict(ini);
                printf("FTI: Recover from level %i.\n", i%4+1);
            }
            MPI_Barrier(MPI_COMM_WORLD);
            start = MPI_Wtime();
            if (FTI_Init(config_file, MPI_COMM_WORLD) != FTI_SCES) {
                printf("FTI: failed to initialize.\n");
                MPI_Abort(MPI_COMM_WORLD, -1);
            }
            FTI_Protect(0, arr, SIZE, FTI_CHAR);
            FTI_Recover();
            end = MPI_Wtime();
            dTFtiRead[i] = end-start;
            if (rank == 0) {
                printf("FTI: Recovery from level %i Checkpoint took: %lf seconds!.\n", 
                        i%4+1, dTFtiRead[i]);
            }
        } else {
            if (rank == 0) {
                printf("FTI: Perform recovery of level %i Checkpoint.\n", i%4+1);
            }
            MPI_Barrier(MPI_COMM_WORLD);
            start = MPI_Wtime();
            if (FTI_Init(config_file, MPI_COMM_WORLD) != FTI_SCES) {
                printf("FTI: failed to initialize.\n");
                MPI_Abort(MPI_COMM_WORLD, -1);
            }
            FTI_Protect(0, arr, SIZE, FTI_CHAR);
            FTI_Recover();
            MPI_Barrier(MPI_COMM_WORLD);
            end = MPI_Wtime();
            dTFtiRead[i] = end-start;
            if (rank == 0) {
                printf("FTI: recovery of level %i Checkpoint took %lf s\n", 
                        i%4+1, dTFtiRead[i]);
            }
        }
        FTI_Finalize();
    }
    
    remove(config_file);

// +=========+
// | FTI END |
// +=========+

// +-----------------------------------------------------------------------------+
    
    if (rank == 0) {
        // compute mean and sigma of measured times
        memset(dTFtiReadMean,0,8*sizeof(double));
        memset(dTFtiWriteMean,0,8*sizeof(double));
        memset(dTFtiReadSigma,0,8*sizeof(double));
        memset(dTFtiWriteSigma,0,8*sizeof(double));
        dTPosixReadMean=0;
        dTPosixWriteMean=0;
        dTPosixReadSigma=0;
        dTPosixWriteSigma=0;
        dTMpiReadMean=0;
        dTMpiWriteMean=0;
        dTMpiReadSigma=0;
        dTMpiWriteSigma=0;
        
        // POSIX AND MPIIO
        // mean
        for(i=0; i<NUM_ITER; i++) {
            dTPosixReadMean += dTPosixRead[i];
            dTPosixWriteMean += dTPosixWrite[i];
            dTMpiReadMean += dTMpiRead[i];
            dTMpiWriteMean += dTMpiWrite[i];
        }
        
        dTPosixReadMean /= NUM_ITER;
        dTPosixWriteMean /= NUM_ITER;
        dTMpiReadMean /= NUM_ITER;
        dTMpiWriteMean /= NUM_ITER;
        // sigma
        for(i=0; i<NUM_ITER; i++) {
           dTPosixReadSigma += pow(dTPosixRead[i]-dTPosixReadMean,2); 
           dTPosixWriteSigma += pow(dTPosixWrite[i]-dTPosixWriteMean,2); 
           dTMpiReadSigma += pow(dTMpiRead[i]-dTMpiReadMean,2); 
           dTMpiWriteSigma += pow(dTMpiWrite[i]-dTMpiWriteMean,2); 
        }
        dTPosixWriteSigma = sqrt(dTPosixWriteSigma/NUM_ITER);
        dTPosixReadSigma = sqrt(dTPosixReadSigma/NUM_ITER);
        dTMpiWriteSigma = sqrt(dTMpiWriteSigma/NUM_ITER);
        dTMpiReadSigma = sqrt(dTMpiReadSigma/NUM_ITER);
        
        // FTI
        // mean
        for(i=0; i<8*NUM_ITER; i++) {
            dTFtiReadMean[i%8] += dTFtiRead[i];
            dTFtiWriteMean[i%8] += dTFtiWrite[i];
        }
        for(i=0; i<8; i++) {
            dTFtiReadMean[i] /= NUM_ITER;
            dTFtiWriteMean[i] /= NUM_ITER;
        }
        // sigma
        for(i=0; i<8*NUM_ITER; i++) {
            dTFtiReadSigma[i%8] += pow(dTFtiRead[i]-dTFtiReadMean[i%8],2);        
            dTFtiWriteSigma[i%8] += pow(dTFtiWrite[i]-dTFtiWriteMean[i%8],2);        
        }
        for(i=0; i<8; i++) {
            dTFtiReadSigma[i] = sqrt(dTFtiReadSigma[i]/NUM_ITER);
            dTFtiWriteSigma[i] = sqrt(dTFtiWriteSigma[i]/NUM_ITER);
        }

        printf("\n"
               "#######################\n"
               "##   RESULTS         ##\n"
               "#######################\n"
               "\n"
               "Number of processes: %i\n"
               "Allocated memory per process: \n%llu B | %lf MB | %lf GB\n"
               "Allocated memory in total: \n%llu B | %lf MB | %lf GB\n"
               "\n"
               "=========================================\n"
               "FTI - POSIX\n"
               "-----------------------------------------\n"
               " L1 Write = %lf s (dT = %lf s) \n"
               " L1 Read  = %lf s (dT = %lf s) \n"
               " L2 Write = %lf s (dT = %lf s) \n"
               " L2 Read  = %lf s (dT = %lf s) \n"
               " L3 Write = %lf s (dT = %lf s) \n"
               " L3 Read  = %lf s (dT = %lf s) \n"
               " L4 Write = %lf s (dT = %lf s) \n"
               " L4 Read  = %lf s (dT = %lf s) \n"
               "=========================================\n"
               "\n"
               "=========================================\n"
               "FTI - MPI-I/O\n"
               "-----------------------------------------\n"
               " L1 Write = %lf s (dT = %lf s) \n"
               " L1 Read  = %lf s (dT = %lf s) \n"
               " L2 Write = %lf s (dT = %lf s) \n"
               " L2 Read  = %lf s (dT = %lf s) \n"
               " L3 Write = %lf s (dT = %lf s) \n"
               " L3 Read  = %lf s (dT = %lf s) \n"
               " L4 Write = %lf s (dT = %lf s) \n"
               " L4 Read  = %lf s (dT = %lf s) \n"
               "=========================================\n"
               "\n"
               "=========================================\n"
               "Pure MPI-I/O\n"
               "-----------------------------------------\n"
               " Write    = %lf s (dT = %lf s)\n"
               " Read     = %lf s (dT = %lf s)\n"
               "=========================================\n"
               "\n"
               "=========================================\n"
               "Pure POSIX\n"
               "-----------------------------------------\n"
               " Write    = %lf s (dT = %lf s)\n"
               " Read     = %lf s (dT = %lf s)\n"
               "=========================================\n"
               "\n",
               size,
               SIZE,
               (SIZE*1.0)/(1024*1024),
               (SIZE*1.0)/(1024*1024*1024),
               SIZE*size,
               (SIZE*1.0*size)/(1024*1024),
               (SIZE*1.0*size)/(1024*1024*1024),
               dTFtiWriteMean[0],dTFtiWriteSigma[0],
               dTFtiReadMean[0],dTFtiReadSigma[0],
               dTFtiWriteMean[1],dTFtiWriteSigma[1],
               dTFtiReadMean[1],dTFtiReadSigma[1],
               dTFtiWriteMean[2],dTFtiWriteSigma[2],
               dTFtiReadMean[2],dTFtiReadSigma[2],
               dTFtiWriteMean[3],dTFtiWriteSigma[3],
               dTFtiReadMean[3],dTFtiReadSigma[3],
               dTFtiWriteMean[4],dTFtiWriteSigma[4],
               dTFtiReadMean[0],dTFtiReadSigma[4],
               dTFtiWriteMean[5],dTFtiWriteSigma[5],
               dTFtiReadMean[1],dTFtiReadSigma[5],
               dTFtiWriteMean[6],dTFtiWriteSigma[6],
               dTFtiReadMean[2],dTFtiReadSigma[6],
               dTFtiWriteMean[7],dTFtiWriteSigma[7],
               dTFtiReadMean[7],dTFtiReadSigma[7],
               dTMpiWriteMean,dTMpiWriteSigma, 
               dTMpiReadMean,dTMpiReadSigma,
               dTPosixWriteMean,dTPosixWriteSigma, 
               dTPosixReadMean,dTPosixReadSigma);
    } 


    MPI_Finalize();
    
    return 0;
}

// +======================+
// | Function definitions |
// +======================+

void parse_arguments(int argc, char *argv[]) {
    
    int oc;
    dictionary *ini;
    FILE *fd;

    if (rank == 0) {
        ini = iniparser_load(config_file);
        if (ini == NULL) {
            printf("[ERROR] can't open configuration file.\n");
            MPI_Abort(gcomm, -1);
        }
    }

    while ((oc = getopt(argc, argv, ":lvVi:n:g:u:f:d:m:M:")) != -1) {
        switch (oc) {
            case 'i':
                /* # of iterations */
                NUM_ITER = atoi(optarg);
                if (rank == 0) {
                    printf("[CONFIG] Number of iterations: %i\n", NUM_ITER);
                }
                break;
            case 'm':
                /* mem-size in bytes */
                SIZE = strtoull(optarg, NULL, 10);
                if (rank == 0) {
                    printf("[CONFIG] memsize is: %llu bytes\n", SIZE);
                }
                break;
            case 'M':
                /* mem-size in megabytes */
                SIZE = strtoull(optarg, NULL, 10)*1024*1024;
                if (rank == 0) {
                    printf("[CONFIG] memsize is: %llu MB\n", SIZE/(1024*1024));
                }
                break;
            case 'd':
                /* local directory */
                if (rank == 0) {
                    iniparser_set(ini, "Basic:ckpt_dir", optarg);
                    printf("[CONFIG] FTI: ckpt_dir is set to '%s'\n", optarg);
                }
                break;
            case 'v':
                /* FTI verbosity */
                if (rank == 0) {
                    iniparser_set(ini, "basic:verbosity", "3");
                    printf("[CONFIG] FTI: verbosity is set to 3 (quiet)\n");
                }
                break;
            case 'V':
                /* FTI verbosity */
                if (rank == 0) {
                    iniparser_set(ini, "basic:verbosity", "1");
                    printf("[CONFIG] FTI: verbosity is set to 1 (debug)\n");
                }
                break;
            case 'l':
                /* local test */
                if (rank == 0) {
                    iniparser_set(ini, "Advanced:local_test", "1");
                    printf("[CONFIG] FTI: local_test is set to 1\n");
                }
                break;
            case 'g':
                /* group size */
                FTI_GROUP_SIZE = atoi(optarg);
                if (rank == 0) {
                    iniparser_set(ini, "Basic:group_size", optarg);
                    printf("[CONFIG] FTI: group_size is set to %i\n", atoi(optarg));
                }
                break;
            case 'n':
                /* cpus per node */
                FTI_CPU_PER_NODE = atoi(optarg);
                if (rank == 0) {
                    iniparser_set(ini, "Basic:node_size", optarg);
                    printf("[CONFIG] FTI: node_size is set to %i\n", atoi(optarg));
                }
                break;
            case 'u':
                /* striping unit */
                L_SU = atoi(optarg);
                strcpy(R_SU,optarg);
                break;
            case 'f':
                /* striping factor */
                L_SF = atoi(optarg);
                strcpy(R_SF,optarg);
                break;
            case ':':
                /* missing option argument */
                fprintf(stderr, "%s: option '-%c' requires an argument\n",
                        argv[0], optopt);
                break;
            case '?':
            default:
                /* invalid option */
                fprintf(stderr, "%s: option '-%c' is invalid: ignored\n",
                        argv[0], optopt);
                break;
        }
    }
    if (rank == 0) {
        fd = fopen(config_file, "w");
        iniparser_dump_ini(ini, fd);
        fclose(fd);
        iniparser_freedict(ini);
    }
}
#ifdef FTI_LUSTRE
static inline int maxint(int a, int b)
{
return a > b ? a : b;
}

static void *alloc_lum()
{
    int v1, v3, join;
    v1 = sizeof(struct lov_user_md_v1) +
    LOV_MAX_STRIPE_COUNT * sizeof(struct lov_user_ost_data_v1);
    v3 = sizeof(struct lov_user_md_v3) +
    LOV_MAX_STRIPE_COUNT * sizeof(struct lov_user_ost_data_v1);
    return malloc(maxint(v1, v3));
}
#endif
