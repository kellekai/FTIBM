#include "include/FTIBM_decl.h"

int main(int argc, char *argv[]) {
    
    striping_factor = (argc > 1) ? atoi(argv[1]) : -1;

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
    
/* PURE MPI WRITE BEGIN */

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

    if(rank == 0) {
        printf("MPIIO: finished in %lf seconds!\n", end-start);
        remove(tempfile);
        rmdir(tmpdir);
    }
    
    MPI_Barrier(gcomm);

    strcpy(tmpdir,"tmpXXXXXX");

/* PURE MPI WRITE END */

/* PURE POSIX WRITE BEGIN */
    
    if(rank == 0) {
        printf("POSIX: start writing in file\n");
        if (mkdtemp(tmpdir) == NULL) {
            perr = errno;
            printf("[POSIX ERROR] %s\n", strerror(perr));
            perr = 0;
            MPI_Abort(gcomm, -1);
        }
    }

    MPI_Bcast(tmpdir,10,MPI_CHAR,0,gcomm);
    sprintf(tempfile, "%s/rank-%i.f", tmpdir, rank);
    
    start = MPI_Wtime();

    fd = fopen(tempfile, "wb");  
    
    if (fd == NULL) {
        perr = errno;
        printf("[POSIX ERROR] %s\n",strerror(perr));
        perr = 0;
        rmdir(tmpdir);
#ifdef FTI_LUSTRE
        free(lum_file);
#endif
        MPI_Abort(gcomm, -1);
    }

    long written = 0;
    char *ptr = (char*) arr;
    while(written < SIZE) {
        written += fwrite(ptr, sizeof(char), SIZE - written, fd);
    
        if (written == 0) {
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

        ptr += written; 
    }

    fclose(fd); 

    MPI_Barrier(gcomm);    
    end = MPI_Wtime();
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
        printf("POSIX: finished in %lf seconds!\n", end-start);
        rmdir(tmpdir);
#ifdef FTI_LUSTRE
        free(lum_file);
#endif
    }

/* PURE POSIX WRITE END */
 
/* INITIALIZE FTI */
    
    if (rank == 0) {
    printf("FTI: Initializing...\n");
    }
    if (FTI_Init(config_file, gcomm) != FTI_SCES) {
        printf("FTI: failed to initialize.\n");
        MPI_Abort(MPI_COMM_WORLD, -1);
    }
    if (rank == 0) {
    printf("FTI: Initialization successful.\n");
    }
    gcomm = FTI_COMM_WORLD;
    
    FTI_Protect(0, arr, SIZE, FTI_CHAR);

    int i;
    for(i=1; i<5; i++) {
        if (rank == 0) {
            printf("FTI: Level %i Checkpoint.\n", i);
        }
        start = MPI_Wtime();
        FTI_Checkpoint(i,i);
        end = MPI_Wtime();
        if (rank == 0) {
            printf("FTI: Level %i Checkpoint took: %lf seconds!.\n", i, end-start);
        }
    }

    FTI_Finalize();
    MPI_Finalize();

    remove(config_file);
    return 0;
}

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

    while ((oc = getopt(argc, argv, ":ln:u:f:D:d:m:M:")) != -1) {
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
            case 'l':
                /* local test */
                if (rank == 0) {
                    iniparser_set(ini, "Advanced:local_test", "1");
                    printf("[CONFIG] FTI: local_test is set to 1\n");
                }
                break;
            case 'n':
                /* cpus per node */
                FTI_CPU_PER_NODE = atoi(optarg);
                if (rank == 0) {
                    iniparser_set(ini, "Basic:node_size", optarg);
                    printf("[CONFIG] FTI: CPUs per node is set to %i\n", atoi(optarg));
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
