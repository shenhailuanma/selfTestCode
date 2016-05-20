#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/shm.h>
#include <unistd.h>
#include <inttypes.h>
#include <stdint.h>

#ifndef SHM_STAT
#define SHM_STAT   13
#define SHM_INFO   14
struct shm_info {
    int used_ids;
    ulong shm_tot;      /* total allocated shm */
    ulong shm_rss;      /* total resident shm */
    ulong shm_swp;      /* total swapped shm */
    ulong swap_attempts;
    ulong swap_successes;
};
#endif

/*
 * X/OPEN (Jan 1987) does not define fields key, seq in struct ipc_perm;
 *  glibc-1.09 has no support for sysv ipc.
 *  glibc 2 uses __key, __seq
 */
#if defined (__GLIBC__) && __GLIBC__ >= 2
# define KEY __key
#else
# define KEY key
#endif


struct ipc_stat {
    int     id;
    key_t       key;
    uid_t       uid;    /* current uid */
    gid_t       gid;    /* current gid */
    uid_t       cuid;    /* creator uid */
    gid_t       cgid;    /* creator gid */
    unsigned int    mode;
};



/* See 'struct shmid_kernel' in kernel sources
 */
struct shm_data {
    struct ipc_stat shm_perm;

    uint64_t    shm_nattch;
    uint64_t    shm_segsz;
    int64_t     shm_atim;   /* __kernel_time_t is signed long */
    int64_t     shm_dtim;
    int64_t     shm_ctim;
    pid_t       shm_cprid;
    pid_t       shm_lprid;
    uint64_t    shm_rss;
    uint64_t    shm_swp;

    struct shm_data  *next;
};


int ipc_shm_get_info(int id, struct shm_data **shmds)
{
    FILE *f;
    int i = 0, maxid;
    struct shm_data *p;
    struct shm_info dummy;

    p = *shmds = calloc(1, sizeof(struct shm_data));
    if(!p){
        printf("error: calloc failed.\n");
        return -1;
    }
    p->next = NULL;

    f = fopen("/proc/sysvipc/shm", "r");
    if (!f)
        goto fallback;

    while (fgetc(f) != '\n');       /* skip header */

    while (feof(f) == 0) {
        if (fscanf(f,
              "%d %d  %o %"SCNu64 " %u %u  "
              "%"SCNu64 " %u %u %u %u %"SCNi64 " %"SCNi64 " %"SCNi64
              " %"SCNu64 " %"SCNu64 "\n",
               &p->shm_perm.key,
               &p->shm_perm.id,
               &p->shm_perm.mode,
               &p->shm_segsz,
               &p->shm_cprid,
               &p->shm_lprid,
               &p->shm_nattch,
               &p->shm_perm.uid,
               &p->shm_perm.gid,
               &p->shm_perm.cuid,
               &p->shm_perm.cgid,
               &p->shm_atim,
               &p->shm_dtim,
               &p->shm_ctim,
               &p->shm_rss,
               &p->shm_swp) != 16)
            continue;

        if (id > -1) {
            /* ID specified */
            if (id == p->shm_perm.id) {
                i = 1;
                break;
            } else
                continue;
        }

        p->next = calloc(1, sizeof(struct shm_data));
        p = p->next;
        p->next = NULL;
        i++;
    }

    if (i == 0)
        free(*shmds);
    fclose(f);
    return i;

    /* Fallback; /proc or /sys file(s) missing. */
fallback:
    i = id < 0 ? 0 : id;

    maxid = shmctl(0, SHM_INFO, (struct shmid_ds *) &dummy);
    if (maxid < 0)
        return 0;

    while (i <= maxid) {
        int shmid;
        struct shmid_ds shmseg;
        struct ipc_perm *ipcp = &shmseg.shm_perm;

        shmid = shmctl(i, SHM_STAT, &shmseg);
        if (shmid < 0) {
            if (-1 < id) {
                free(*shmds);
                return 0;
            }
            i++;
            continue;
        }

        p->shm_perm.key = ipcp->KEY;
        p->shm_perm.id = shmid;
        p->shm_perm.mode = ipcp->mode;
        p->shm_segsz = shmseg.shm_segsz;
        p->shm_cprid = shmseg.shm_cpid;
        p->shm_lprid = shmseg.shm_lpid;
        p->shm_nattch = shmseg.shm_nattch;
        p->shm_perm.uid = ipcp->uid;
        p->shm_perm.gid = ipcp->gid;
        p->shm_perm.cuid = ipcp->cuid;
        p->shm_perm.cgid = ipcp->cuid;
        p->shm_atim = shmseg.shm_atime;
        p->shm_dtim = shmseg.shm_dtime;
        p->shm_ctim = shmseg.shm_ctime;
        p->shm_rss = 0xdead;
        p->shm_swp = 0xdead;

        if (id < 0) {
            p->next = calloc(1, sizeof(struct shm_data));
            if(!p){
                printf("error: calloc failed.\n");
                return -1;
            }
            p = p->next;
            p->next = NULL;
            i++;
        } else
            return 1;
    }

    return i;
}

void ipc_shm_free_info(struct shm_data *shmds)
{
    while (shmds) {
        struct shm_data *next = shmds->next;
        free(shmds);
        shmds = next;
    }
}




int main(void)
{
    int ret;
    int maxid;
    struct shmid_ds shmbuf;
    struct shm_info *shm_info;

    maxid = shmctl (0, SHM_INFO, &shmbuf);
    shm_info =  (struct shm_info *) &shmbuf;
    if (maxid < 0) {
        printf ("kernel not configured for shared memory\n");
        return -1;
    }

    printf ("------ Shared Memory Status --------\n");
    printf ("segments allocated %d\n"
              "pages allocated %ld\n"
              "pages resident  %ld\n"
              "pages swapped   %ld\n"
              "Swap performance: %ld attempts\t %ld successes\n",
            shm_info->used_ids,
            shm_info->shm_tot,
            shm_info->shm_rss,
            shm_info->shm_swp,
            shm_info->swap_attempts, shm_info->swap_successes);


    printf("ipc_shm_get_info\n");
        printf ("------ Shared Memory Segments --------\n");
        printf ("%-10s %-10s %-10s %-10s %-10s %-10s %-12s\n",
            "key","shmid","owner","perms","bytes","nattch","status");

    struct shm_data *shmds, *shmdsp;

    ret = ipc_shm_get_info(-1, &shmds);
    if(ret < 0){
        printf("ipc_shm_get_info return:%d\n", ret);
        return -1;
    }
    shmdsp = shmds;

    for (shmdsp = shmds; shmdsp->next != NULL; shmdsp = shmdsp->next) {


        printf("0x%08x ", shmdsp->shm_perm.key);

        printf ("%-10d %-10u", shmdsp->shm_perm.id, shmdsp->shm_perm.uid);
        printf (" %-10o ", shmdsp->shm_perm.mode & 0777);


        printf (" %-10lld ", shmdsp->shm_segsz);

        printf (" %-10ju %-6s %-6s\n",
            shmdsp->shm_nattch,
            shmdsp->shm_perm.mode & SHM_DEST ? "dest" : " ",
            shmdsp->shm_perm.mode & SHM_LOCKED ? "locked" : " ");
       
        
    }

    ipc_shm_free_info(shmds);

    printf("ipc_shm_free_info\n");
    printf("over.\n");

    return 0;
}