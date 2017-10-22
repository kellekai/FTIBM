#include "include/FTIBM_decl.h"

int main(int argc, char *argv[]) {
    
    int i;

    MPI_Init(NULL,NULL);
    MPI_Comm_rank(MPI_COMM_WORLD,&rank);
    MPI_Comm_size(MPI_COMM_WORLD,&size);    
    arr = (char*) malloc(SIZE);
    gcomm = MPI_COMM_WORLD;
    
    if (rank == 0) {
        init_config_file(config_file);
    }
    
    MPI_Barrier(gcomm);

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

    MPI_Barrier(gcomm);

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

    if(rank == 0) {
        printf("MPIIO: start writing in file\n");
        if (mkdtemp(tmpdir) == NULL) {
            perr = errno;
            printf("[POSIX ERROR] %s\n", strerror(perr));
            perr = 0;
            MPI_Abort(gcomm, -1);
        }
    }
    
    MPI_Bcast(tmpdir,10,MPI_CHAR,0,gcomm);
    sprintf(tempfile,"%s/mpiio.f",tmpdir);
    
#ifdef FTI_LUSTRE
    if(rank == 0) {
        llapi_file_create(tempfile, L_SU, -1, L_SF, 0); // default L_SU = 4194304, L_SF = -1 -> stipe over all OST's
        //llapi_file_create("tmp/test.file", 0, -1, 0, 0);  // set to Lustre default.
        lum_file = alloc_lum();
        llapi_file_get_stripe(tempfile, lum_file); 
        printf("stripe-size: %d, stripe-count: %d\n", 
        lum_file->lmm_stripe_size, lum_file->lmm_stripe_count);
    }
#endif

/* BEGIN WRITE */
    
    start = MPI_Wtime();
    ierr = MPI_File_open(gcomm, tempfile, MPI_MODE_WRONLY|MPI_MODE_CREATE, info, &pfh);
    //MPI_File_open(gcomm, "tmp/test.file", MPI_MODE_WRONLY, MPI_INFO_NULL, &pfh); // do not pass hints
    
    if (ierr!=0) {
        perr = errno;
        printf("[POSIX ERROR] %s\n",strerror(perr));
        MPI_Error_string(ierr, ierr_str, &ierr_len);
        printf("[MPI ERROR] %s\n", ierr_str);
        perr = 0;
        MPI_Abort(gcomm, -1);
    }

    ierr = MPI_File_write_at(pfh, SIZE*rank, arr, SIZE, MPI_CHAR, &status);
    
    if (ierr!=0) {
        perr = errno;
        printf("[POSIX ERROR] %s\n",strerror(perr));
        MPI_Error_string(ierr, ierr_str, &ierr_len);
        printf("[ERROR] %s\n", ierr_str);
        perr = 0;
        MPI_File_close(&pfh);
        MPI_Abort(gcomm, -1);
    }

    MPI_File_close(&pfh);
    end = MPI_Wtime();
    
    dTMpiWrite = end-start;

/* END  WRITE */
    
    MPI_Barrier(gcomm);
    
    if(rank == 0) {
        printf("MPIIO: finished write in %lf seconds!\n", dTMpiWrite);
    }
    
    if(rank == 0) {
        printf("MPIIO: Start reading from file\n");
    }

    MPI_Barrier(gcomm);
    
/* BEGIN READ */
    
    start = MPI_Wtime();
    ierr = MPI_File_open(gcomm, tempfile, MPI_MODE_RDONLY, info, &pfh);
    //MPI_File_open(gcomm, "tmp/test.file", MPI_MODE_WRONLY, MPI_INFO_NULL, &pfh); // do not pass hints
    
    if (ierr!=0) {
        perr = errno;
        printf("[POSIX ERROR] %s\n",strerror(perr));
        MPI_Error_string(ierr, ierr_str, &ierr_len);
        printf("[MPI ERROR] %s\n", ierr_str);
        perr = 0;
        MPI_Abort(gcomm, -1);
    }

    ierr = MPI_File_read_at(pfh, SIZE*rank, arr, SIZE, MPI_CHAR, &status);
    
    if (ierr!=0) {
        perr = errno;
        printf("[POSIX ERROR] %s\n",strerror(perr));
        MPI_Error_string(ierr, ierr_str, &ierr_len);
        printf("[ERROR] %s\n", ierr_str);
        perr = 0;
        MPI_File_close(&pfh);
        MPI_Abort(gcomm, -1);
    }

    MPI_File_close(&pfh);
    end = MPI_Wtime();
    
    dTMpiRead = end-start;

/* END  WRITE */
    
    if(rank == 0) {
        printf("MPIIO: finished read in %lf seconds!\n", dTMpiRead);
        remove(tempfile);
        rmdir(tmpdir);
    }
    
    MPI_Barrier(gcomm);

// +==============+
// | PURE MPI END |
// +==============+

// +-----------------------------------------------------------------------------+

// +==================+
// | PURE POSIX BEGIN |
// +==================+

    strcpy(tmpdir,"tmpXXXXXX");

    if(rank == 0) {
        printf("POSIX: start writing in file\n");
        if (mkdtemp(tmpdir) == NULL) {
            perr = errno;
            printf("[POSIX ERROR] %s\n", strerror(perr));
            perr = 0;
            MPI_Abort(gcomm, -1);
        }
    }
    
    MPI_Barrier(gcomm);

    MPI_Bcast(tmpdir,10,MPI_CHAR,0,gcomm);
    
    sprintf(tempfile, "%s/rank-%i.f", tmpdir, rank);

/* BEGIN WRITE */

    start = MPI_Wtime();

    fd = fopen(tempfile, "wb");  
    
    if (fd == NULL) {
        perr = errno;
        printf("[POSIX ERROR - %i] %s\n", rank, strerror(perr));
        perr = 0;
        rmdir(tmpdir);
#ifdef FTI_LUSTRE
        free(lum_file);
#endif
        MPI_Abort(gcomm, -1);
    }

    long written = 0;
    char *ptrWrite = (char*) arr;
    while(written < SIZE) {
        written += fwrite(ptrWrite, sizeof(char), SIZE - written, fd);
    
        if (written == 0) {
            perr = errno;
            printf("[POSIX ERROR - %i] %s\n", rank, strerror(perr));
            perr = 0;
            fclose(fd);
            remove(tempfile);
            rmdir(tmpdir);
#ifdef FTI_LUSTRE
            free(lum_file);
#endif
            MPI_Abort(gcomm, -1);
        } else if (written < SIZE) { 
            printf("%lu Bytes of %lu Bytes written...", written, SIZE);
        }

        ptrWrite += written; 
    }

    fclose(fd); 

    MPI_Barrier(gcomm);    
    end = MPI_Wtime();

    dTPosixWrite = end-start;

/* END WRITE */

    if(rank == 0) {
        printf("POSIX: finished write in %lf seconds!\n", dTPosixWrite);
        printf("POSIX: start reading from file\n");
    }
    
/* BEGIN READ */

    start = MPI_Wtime();

    fd = fopen(tempfile, "rb");  
    
    if (fd == NULL) {
        perr = errno;
        printf("[POSIX ERROR - %i] %s\n", rank, strerror(perr));
        perr = 0;
        rmdir(tmpdir);
#ifdef FTI_LUSTRE
        free(lum_file);
#endif
        MPI_Abort(gcomm, -1);
    }

    long read = 0;
    char *ptrRead = (char*) arr;
    while(read < SIZE) {
        read += fread(ptrRead, sizeof(char), SIZE - read, fd);
    
        if (read == 0) {
            perr = errno;
            printf("[POSIX ERROR] %s\n",strerror(perr));
            perr = 0;
            fclose(fd);
            remove(tempfile);
            rmdir(tmpdir);
#ifdef FTI_LUSTRE
            free(lum_file);
#endif
            MPI_Abort(gcomm, -1);
        } else if (written < SIZE) { 
            printf("%lu Bytes of %lu Bytes written...", written, SIZE);
        }

        ptrRead += read; 
    }

    fclose(fd); 

    MPI_Barrier(gcomm);    
    end = MPI_Wtime();

    dTPosixRead = end-start;

/* END READ */

#ifdef FTI_LUSTRE
    if(rank == 0) {
        llapi_file_get_stripe(tempfile, lum_file); 
        printf("stripe-size: %d, stripe-count: %d\n", 
        lum_file->lmm_stripe_size, lum_file->lmm_stripe_count);
    }
#endif
    remove(tempfile);
    MPI_Barrier(gcomm);    

    if(rank == 0) {
        printf("POSIX: finished read in %lf seconds!\n", dTPosixRead);
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

/* DO CHECKPOINT AND RESTART FOR ALL LEVELS */

    dictionary *ini;
    FTIT_type CHAR_TYPE;

    if (rank == 0) {
        ini = iniparser_load(config_file);
        if (ini == NULL) {
            printf("[ERROR] can't open configuration file.\n");
            MPI_Abort(gcomm, -1);
        }
    }
    MPI_Barrier(gcomm);
    for(i=1; i<5; i++) {
        //if (rank == 0) {
        //    ini = iniparser_load(config_file);
        //    if (ini == NULL) {
        //        printf("[ERROR] can't open configuration file.\n");
        //        MPI_Abort(gcomm, -1);
        //    }
        //    fd = fopen(config_file, "w");
        //    iniparser_set(ini, "Basic:keep_last_ckpt", "1");
        //    iniparser_dump_ini(ini, fd);
        //    fclose(fd);
        //    iniparser_freedict(ini);
        //}
        gcomm = MPI_COMM_WORLD;
        MPI_Barrier(gcomm);
        if (FTI_Init(config_file, gcomm) != FTI_SCES) {
            printf("FTI: failed to initialize.\n");
            MPI_Abort(MPI_COMM_WORLD, -1);
        }
        gcomm = FTI_COMM_WORLD;
        FTI_Protect(0, arr, SIZE, FTI_CHAR);
        if (rank == 0) {
            printf("FTI: Perform level %i Checkpoint.\n", i);
        }
        start = MPI_Wtime();
        FTI_Checkpoint(i,i);
        end = MPI_Wtime();
        dTFtiWrite[i-1] = end-start;
        if (rank == 0) {
            printf("FTI: Level %i Checkpoint took: %lf seconds!.\n", i, dTFtiWrite[i-1]);
        }
        //FTI_Finalize();
        //if (rank == 0) {
        //    ini = iniparser_load(config_file);
        //    if (ini == NULL) {
        //        printf("[ERROR] can't open configuration file.\n");
        //        MPI_Abort(gcomm, -1);
        //    }
        //    fd = fopen(config_file, "w");
        //    iniparser_set(ini, "Basic:keep_last_ckpt", "0");
        //    iniparser_dump_ini(ini, fd);
        //    fclose(fd);
        //    iniparser_freedict(ini);
        //    printf("FTI: Recover from level %i.\n", i);
        //}
        MPI_Barrier(gcomm);
        gcomm = MPI_COMM_WORLD;
        MPI_Barrier(gcomm);
        start = MPI_Wtime();
        if (FTI_Init(config_file, gcomm) != FTI_SCES) {
            printf("FTI: failed to initialize.\n");
            MPI_Abort(MPI_COMM_WORLD, -1);
        }
        gcomm = FTI_COMM_WORLD;
        FTI_Protect(0, arr, SIZE, FTI_CHAR);
        FTI_Recover();
        end = MPI_Wtime();
        dTFtiRead[i-1] = end-start;
        if (rank == 0) {
            printf("FTI: Recovery from level %i Checkpoint took: %lf seconds!.\n", i, dTFtiRead[i-1]);
        }
        FTI_Finalize();
        sleep(1);
    }

    remove(config_file);

// +=========+
// | FTI END |
// +=========+

// +-----------------------------------------------------------------------------+
    
    if (rank == 0) {
        printf("\n"
               "#######################\n"
               "##   RESULTS         ##\n"
               "#######################\n"
               "\n"
               "FTI\n"
               "-----------------------\n"
               " L1 Write = %lf s\n"
               " L1 Read  = %lf s\n"
               " L2 Write = %lf s\n"
               " L2 Read  = %lf s\n"
               " L3 Write = %lf s\n"
               " L3 Read  = %lf s\n"
               " L4 Write = %lf s\n"
               " L4 Read  = %lf s\n"
               "-----------------------\n"
               "\n"
               "Pure MPI-I/O\n"
               "-----------------------\n"
               " Write    = %lf s\n"
               " Read     = %lf s\n"
               "-----------------------\n"
               "\n"
               "Pure POSIX\n"
               "-----------------------\n"
               " Write    = %lf s\n"
               " Read     = %lf s\n"
               "-----------------------\n"
               "\n",
               dTFtiWrite[0],dTFtiRead[0],
               dTFtiWrite[1],dTFtiRead[1],
               dTFtiWrite[2],dTFtiRead[2],
               dTFtiWrite[3],dTFtiRead[3],
               dTMpiWrite, dTMpiRead,
               dTPosixWrite, dTPosixRead);
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

    while ((oc = getopt(argc, argv, ":ln:g:u:f:d:m:M:")) != -1) {
        switch (oc) {
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
