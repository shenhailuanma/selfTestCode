#ifndef SMEM_CLIENT_H
#define SMEM_CLIENT_H

#ifdef __cplusplus
extern "C" {
#endif


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include "hiredis.h"
#include "async.h"
#include "adapters/libevent.h"

#include <sys/shm.h>
#include <unistd.h>
#include <inttypes.h>
#include <stdint.h>
#include <sys/types.h>

#include <pthread.h>



#define SMEM_CLIENT_TYPE_PRODUCER   0
#define SMEM_CLIENT_TYPE_CONSUMER   1

#define SMEM_ERROR_REDIS_CONNECT    -1
#define SMEM_ERROR_INPUT_POINTER    -2
#define SMEM_ERROR_INPUT_VALUE      -3
#define SMEM_ERROR_TYPE             -4

#define SMEM_CONNECT_TYPE_IP        0
#define SMEM_CONNECT_TYPE_UNIX      1


#define SMEM_MAX_QUEUE_LENGTH       128

struct smemContext{
    int type;   /* define in SMEM_CLIENT_TYPE_XXX */
    int err; /* Error flags, 0 when there is no error */
    char errstr[128]; /* String representation of error when applicable */
    redisContext *rctx;  /* redis context */
    redisContext *rctx2;  /* redis context, used for consumer */
    redisAsyncContext *rasync;  /* redis async context */
    pthread_t async_tid;     /* thread for async redis */

    int connect_type;  /* ip or unix socket */
    char url[128];     /* the url of the redis */
    int port;          /* the port for redis ip connect */
    char channel[128]; /* the pub/sub channel name */

    /* simple queue */
    int queue[SMEM_MAX_QUEUE_LENGTH];
    int queue_head;
    int queue_end;
};



struct smemContext * smemCreateProducer(const char *ip, int port, const char * name);
struct smemContext * smemCreateConsumer(const char *ip, int port, const char * name);

// free
void smemFree(struct smemContext * c);

// get share memory
int smemGetShareMemory(struct smemContext * c, int size);

// free share memory
void smemFreeShareMemory(struct smemContext * c, int id);

// publish the share memory
void smemPublish(struct smemContext * c, int id);

int smem_queue_size(struct smemContext * c);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif