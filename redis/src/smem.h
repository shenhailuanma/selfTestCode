#ifndef __SMEM_H
#define __SMEM_H



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/shm.h>
#include <unistd.h>
#include <inttypes.h>
#include <stdint.h>
#include <sys/types.h>

/* define the max share memory buffer size is the 4K picture size:
   4096*2160*3 = 26542080   */
#define MAX_SMEM_SIZE   26542080

struct smem_info {
    int     id;   /* the id of the share memory */  
    int     size; /* the share memory buffer size */
    unsigned int    uid;
    unsigned int    mode; 
    struct smem_info  *next;
};


int  smem_get_info(int id, struct smem_info ** infos);
void smem_free_info(struct smem_info * infos);
int  smem_get_buffer(int size);
int  smem_free_buffer(int id);
void smem_free_all_buffers(void);




#endif