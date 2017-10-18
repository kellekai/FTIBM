#include "FTIBM_decl.h"

void init_config_file(const char *config_file)
{
    FILE *ini ;

    if ((ini=fopen("fti.ini", "w"))==NULL) {
        fprintf(stderr, "iniparser: cannot create example.ini\n");
        return ;
    }

    fprintf(ini,
            "[ Basic ]\n"
            "head                        = 0\n"
            "node_size                   = %i\n"
            "ckpt_dir                    = %s\n"
            "glbl_dir                    = %s\n"
            "meta_dir                    = %s\n"
            "ckpt_L1                     = 0\n"
            "ckpt_L2                     = 0\n"
            "ckpt_L3                     = 0\n"
            "ckpt_L4                     = 0\n"
            "inline_L2                   = 1\n"
            "inline_L3                   = 1\n"
            "inline_L4                   = 1\n"
            "keep_last_ckpt              = 0\n"
            "group_size                  = %i\n"
            "max_sync_intv               = %i\n"
            "ckpt_io                     = 1\n"
            "verbosity                   = 3\n"
            "[ Restart ]\n"
            "failure                     = 0\n"
            "exec_id                     = NULL\n"
            "[ Advanced ]\n"
            "block_size                  = 1024\n"
            "transfer_size               = 16\n"
            "mpi_tag                     = 2612\n"
            "lustre_striping_unit        = %li\n"
            "lustre_striping_factor      = %i\n"
            "lustre_striping_offset      = -1\n"
            "local_test                  = %i\n",
        FTI_CPU_PER_NODE,
        FTI_LOCAL_DIR,
        FTI_GLOBAL_DIR,
        FTI_META_DIR,
        FTI_GROUP_SIZE,
        FTI_MAX_SYNC_ITER,
        L_SU,
        L_SF,
        FTI_LOCAL_TEST);    
    fclose(ini);
}
