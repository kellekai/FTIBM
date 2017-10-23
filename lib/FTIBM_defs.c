#include "FTIBM_decl.h"

int perr = 0;
unsigned long long SIZE = 1073741824;
char tmpdir[10]="tmpXXXXXX";
char config_file[256] = "fti.ini";
int NUM_ITER = 1;
int FTI_CPU_PER_NODE = 2;
char FTI_LOCAL_DIR[256] = "Local";
char FTI_GLOBAL_DIR[256] = "Global";
char FTI_META_DIR[256] = "Meta";
int FTI_GROUP_SIZE = 4;
int FTI_MAX_SYNC_ITER = 0;
int FTI_LOCAL_TEST = 0;
int R_FS_SET = 0;
int L_SF = -1;
long L_SU = 4194304;
char R_SU[256] = "4194304";
#ifdef FTI_LUSTRE
struct lov_user_md *lum_file = NULL;
#endif
