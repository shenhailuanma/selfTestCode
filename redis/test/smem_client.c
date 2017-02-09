
#include "smem_client.h"


static int queue_full(struct smemContext * c)
{
    if((c->queue_end + 1) % SMEM_MAX_QUEUE_LENGTH == c->queue_head)
        return 1;
    else
        return 0;
}



static int queue_empty(struct smemContext * c)
{
    if(c->queue_end == c->queue_head)
        return 1;
    else
        return 0;
}


static int smem_queue_push(struct smemContext * c, int val)
{
    if(queue_full(c)){
        // the queue is full
        return 0;
    }

    // add the new one
    c->queue[c->queue_end] = val;
    c->queue_end = (c->queue_end + 1) % SMEM_MAX_QUEUE_LENGTH;
    
    return 1;
}

static int smem_queue_pop(struct smemContext * c, int * val)
{
    if(queue_empty(c)){
        // the queue is empty, no data can be pop
        return 0;
    }

    *val = c->queue[c->queue_head];
    c->queue_head = (c->queue_head + 1) % SMEM_MAX_QUEUE_LENGTH;

    return 1;
}

int smem_queue_size(struct smemContext * c)
{

    return (c->queue_end - c->queue_head + SMEM_MAX_QUEUE_LENGTH) % SMEM_MAX_QUEUE_LENGTH;
}

void smemFree(struct smemContext * c)
{
    if(!c)
        return;

    if(c->rctx){
        redisFree(c->rctx);
    }

    if(c->rctx2){
        redisFree(c->rctx2);
    }

    if(c->rasync)
        redisFree(c->rasync);

    free(c);
}

static int smemProducerGetShareMemory(struct smemContext * c, int size)
{
    int mem_id = -1;
    redisReply *reply;

    if(size <= 0)
        return -1;

    // get share memory id
    reply = redisCommand(c->rctx,"SMEMGET %d",  size);
    if(reply){
        //printf("[smem_client] reply type:%d, integer:%lld, len:%d, str:%s\n", reply->type, reply->integer, reply->len, reply->str);
        if(reply->type == REDIS_REPLY_INTEGER){
            mem_id = reply->integer;
        }
    }else{
        printf("[smem_client] get share memory id reply error\n");
        return -1;
    }
    freeReplyObject(reply);

    return mem_id;
}


static int smemConsumerGetShareMemory(struct smemContext * c)
{
    int mem_id = -1;

    smem_queue_pop(c, &mem_id);

    return mem_id;
}


int smemGetShareMemory(struct smemContext * c, int size)
{

    if(!c)
        return SMEM_ERROR_INPUT_POINTER;

    if(!c->rctx)
        return SMEM_ERROR_INPUT_POINTER;


    int mem_id = -1;

    switch(c->type){

        case SMEM_CLIENT_TYPE_PRODUCER:
            mem_id = smemProducerGetShareMemory(c,size);
            break;

        case SMEM_CLIENT_TYPE_CONSUMER:
            mem_id = smemConsumerGetShareMemory(c);
            break;

        default:
            mem_id = -1;
            break;
    }

    return mem_id;
}

void smemFreeShareMemory(struct smemContext * c, int id)
{
    if(!c)
        return;

    if(id < 0)
        return;

    if(!c->rctx)
        return;

    redisReply *reply;
    // free share memory id
    reply = redisCommand(c->rctx,"SMEMFREE %d", id);
    if(!reply){
        printf("[smem_client] smemFreeShareMemory SMEFREE reply error\n");
        return;
    }
    freeReplyObject(reply);
}


static void smemFreeShareMemory2(struct smemContext * c, int id)
{

    if(!c)
        return ;

    if(id < 0)
        return ;

    if(!c->rctx)
        return ;

    redisReply *reply;
    // free share memory id
    reply = redisCommand(c->rctx2,"SMEMFREE %d", id);
    if(!reply){
        printf("[smem_client] smemFreeShareMemory2 SMEFREE reply error\n");
        return;
    }
    freeReplyObject(reply);

}


static redisContext * smemCreateConnect(const char *ip, int port)
{

    redisContext * rctx = NULL;

    struct timeval timeout = { 1, 500000 }; // 1.5 seconds
    rctx = redisConnectWithTimeout(ip, port, timeout);
    if (rctx == NULL || rctx->err) {
        if (rctx) {
            redisFree(rctx);        
        }
        rctx = NULL;
    }

    return rctx;
}

static redisContext * smemCreateConnectUnix(const char *path)
{

    redisContext * rctx = NULL;

    struct timeval timeout = { 1, 500000 }; // 1.5 seconds
    rctx = redisConnectUnixWithTimeout(path, timeout);
    if (rctx == NULL || rctx->err) {
        if (rctx) {
            redisFree(rctx);        
        }
        rctx = NULL;
    }


    return rctx;
}


struct smemContext * smemCreateProducer(const char *ip, int port, const char *channel)
{
    struct smemContext *c;

    c = calloc(1,sizeof(struct smemContext));
    if (c == NULL)
        return NULL;

    c->type = SMEM_CLIENT_TYPE_PRODUCER;
    c->connect_type = SMEM_CONNECT_TYPE_IP;
    strcpy(c->url, ip);
    strcpy(c->channel, channel);
    c->port = port;

    // create connect 
    c->rctx = smemCreateConnect(ip, port);
    if (c->rctx == NULL) {
        c->err = SMEM_ERROR_REDIS_CONNECT;
        return c;
    }

    return c;
}

struct smemContext * smemCreateProducerUnix(const char *path, const char *channel)
{
    struct smemContext *c;

    c = calloc(1,sizeof(struct smemContext));
    if (c == NULL)
        return NULL;

    c->type = SMEM_CLIENT_TYPE_PRODUCER;
    c->connect_type = SMEM_CONNECT_TYPE_UNIX;
    strcpy(c->url, path);
    strcpy(c->channel, channel);

    // create connect 
    c->rctx = smemCreateConnectUnix(path);
    if (c->rctx == NULL) {
        c->err = SMEM_ERROR_REDIS_CONNECT;
        return c;
    }

    return c;
}


static void subCallback(redisAsyncContext *c, void *r, void *priv) {
    redisReply *reply = r;

    struct smemContext *ctx = priv;

    int mem_id = -1;
    uint8_t *mem_ptr = NULL;
    int val;
    float rate=0;

    if (reply == NULL) return;
    if ( reply->type == REDIS_REPLY_ARRAY && reply->elements == 3 ) {

        
        if(reply->element[2]->type == REDIS_REPLY_INTEGER){
            //printf("[smem_client] type:REDIS_REPLY_INTEGER, val=%lld\n", reply->element[2]->integer);
            mem_id = reply->element[2]->integer;
        }else if(reply->element[2]->type == REDIS_REPLY_STRING){
            //printf("[smem_client] type:REDIS_REPLY_STRING, val=%s\n", reply->element[2]->str);
            mem_id = atoi(reply->element[2]->str);
        }

        if(mem_id <= 0)
            return;

        //printf("[smem_client] mem_id=%d\n", mem_id);

        // write to queue
        if(!smem_queue_push(ctx, mem_id)){
            // push the val to queue failed, because the queue is full

            smemFreeShareMemory2(ctx, mem_id); // fixme: for test
            printf("[smem_client] the queue is full, now to delete the new one\n");
            return;

            /*
            printf("[smem_client] the queue is full, now to delete the oldest one and add the new one\n");
            val = -1;
            smem_queue_pop(ctx, &val);
            if(val != -1){
                // free the old one mem_id
                smemFreeShareMemory2(ctx, val);

                // push the mem_id again
                smem_queue_push(ctx, mem_id);
            }
            */
        }
        
    }

    /* Disconnect after receiving the reply to GET */
    //redisAsyncDisconnect(c);
}

static void setnameCallback(redisAsyncContext *c, void *r, void *priv) {
    redisReply *reply = r;

    struct smemContext *ctx = priv;

    int mem_id = -1;
    uint8_t *mem_ptr = NULL;
    int val;
    float rate=0;

    printf("[smem_client] setnameCallback\n");

    if (reply == NULL){
        printf("[smem_client] setnameCallback, reply == NULL\n");
        return;
    } 

    printf("[smem_client] setnameCallback, reply->type = %d, elements=%d\n", reply->type, reply->elements);

    if(reply->type == REDIS_REPLY_ARRAY){
        printf("[smem_client] setnameCallback, reply->type == REDIS_REPLY_ARRAY\n");
    }else if(reply->type == REDIS_REPLY_STRING){
        printf("[smem_client] setnameCallback, reply->type == REDIS_REPLY_STRING, val=%s\n", reply->str);
    }else if(reply->type == REDIS_REPLY_INTEGER){
        printf("[smem_client] setnameCallback, reply->type == REDIS_REPLY_INTEGER, val=%lld\n", reply->integer);
    }

    /* Disconnect after receiving the reply to GET */
    //redisAsyncDisconnect(c);
}


static void connectCallback(const redisAsyncContext *c, int status) {
    if (status != REDIS_OK) {
        printf("[smem_client] Error: %s\n", c->errstr);
        return;
    }
    printf("[smem_client] Connected...\n");
}

static void disconnectCallback(const redisAsyncContext *c, int status) {
    if (status != REDIS_OK) {
        printf("[smem_client] Error: %s\n", c->errstr);
        return;
    }
    printf("[smem_client] Disconnected...\n");
}




static void * smemAsyncThread(void * h)
{
    struct smemContext *c = h;


    if(c->connect_type == SMEM_CONNECT_TYPE_UNIX){
        c->rasync = redisAsyncConnectUnix(c->url);
    }else{
        c->rasync = redisAsyncConnect(c->url, c->port);
    }

    if (!c->rasync || c->rasync->err) {
        /* Let *c leak for now... */
        if(c->rasync){
            printf("[smem_client] redisAsyncConnect Error: %s\n", c->rasync->errstr);
        }

        return NULL;
    }



    struct event_base *base = event_base_new();

    redisLibeventAttach(c->rasync,base);
    redisAsyncSetConnectCallback(c->rasync,connectCallback);
    redisAsyncSetDisconnectCallback(c->rasync,disconnectCallback);

    if(strlen(c->name) > 0){
        redisAsyncCommand(c->rasync, setnameCallback, (void *)c, "CLIENT SETNAME %s", c->name);
    }
    
    redisAsyncCommand(c->rasync, subCallback, (void *)c, "SMEMSUBSCRIBE %s", c->channel);

    event_base_dispatch(base);

    return NULL;
}

struct smemContext * smemCreateConsumerWithName(const char *ip, int port, const char *channel, const char * name)
{
    struct smemContext *c;


    c = calloc(1,sizeof(struct smemContext));
    if (c == NULL)
        return NULL;

    c->type = SMEM_CLIENT_TYPE_CONSUMER;
    c->connect_type = SMEM_CONNECT_TYPE_IP;
    strcpy(c->url, ip);
    strcpy(c->channel, channel);
    if(name && strlen(name) > 0 && strlen(name) < sizeof(c->name)){
        strcpy(c->name, name);
    }
    
    c->port = port;


    // create connect 
    c->rctx = smemCreateConnect(ip, port);
    if (c->rctx == NULL) {
        c->err = SMEM_ERROR_REDIS_CONNECT;
        return c;
    }

    c->rctx2 = smemCreateConnect(ip, port);
    if(c->rctx2 == NULL){
        c->err = SMEM_ERROR_REDIS_CONNECT;
        return NULL;
    }

    c->queue_head = 0;
    c->queue_end  = 0;

    // create thread
    pthread_attr_t attr;
    struct sched_param param;

    pthread_attr_init(&attr);
    pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
    //pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    pthread_attr_getschedparam(&attr, &param);
     
    pthread_create(&c->async_tid, &attr, (void *)smemAsyncThread, c);

    return c;
}

struct smemContext * smemCreateConsumer(const char *ip, int port, const char *channel)
{
    return smemCreateConsumerWithName(ip, port, channel, NULL);
}

struct smemContext * smemCreateConsumerUnixWithName(const char *path, const char *channel, const char * name)
{
    struct smemContext *c;


    c = calloc(1,sizeof(struct smemContext));
    if (c == NULL)
        return NULL;

    c->type = SMEM_CLIENT_TYPE_CONSUMER;
    c->connect_type = SMEM_CONNECT_TYPE_UNIX;
    strcpy(c->url, path);
    strcpy(c->channel, channel);
    if(name)
        strcpy(c->name, name);

    // create connect 
    c->rctx = smemCreateConnectUnix(path); 
    if (c->rctx == NULL) {
        c->err = SMEM_ERROR_REDIS_CONNECT;
        return c;
    }

    c->rctx2 = smemCreateConnectUnix(path);
    if(c->rctx2 == NULL){
        c->err = SMEM_ERROR_REDIS_CONNECT;
        return NULL;
    }

    c->queue_head = 0;
    c->queue_end  = 0;

    // create thread
    pthread_attr_t attr;
    struct sched_param param;

    pthread_attr_init(&attr);
    pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
    //pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    pthread_attr_getschedparam(&attr, &param);
     
    pthread_create(&c->async_tid, &attr, (void *)smemAsyncThread, c);

    return c;
}

struct smemContext * smemCreateConsumerUnix(const char *path, const char *channel)
{
    return smemCreateConsumerUnixWithName(path, channel, NULL);
}


void smemPublish(struct smemContext * c, int id)
{
    if(!c)
        return ;

    if(id < 0)
        return ;

    if(!c->rctx)
        return ;

    redisReply *reply;
    // free share memory id
    reply = redisCommand(c->rctx,"SMEMPUBLISH %s %d", c->channel, id);
    if(!reply){
        printf("[smem_client] SMEMPUBLISH reply error\n");
        return;
    }
    freeReplyObject(reply);
}




int smemSetName(struct smemContext * c, char * name)
{

    if(!c)
        return SMEM_ERROR_INPUT_POINTER;

    if(!c->rasync)
        return SMEM_ERROR_INPUT_POINTER;



    if(c->rasync){
        printf("[smem_client] smemSetName before setname %s\n", name);
        redisAsyncCommand(c->rasync, setnameCallback, (void *)c, "SETNAME %s", name);
        printf("[smem_client] smemSetName after setname %s\n", name);

    }

    

    return 0;
}