
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


int g_cnt = 0;

void printReply(redisReply *reply)
{
    switch(reply->type){
        case REDIS_REPLY_STRING:
            printf("type:REDIS_REPLY_STRING, val=%s\n", reply->str);
            break;

        case REDIS_REPLY_ARRAY:
            break;

        case REDIS_REPLY_INTEGER:
            printf("type:REDIS_REPLY_INTEGER, val=%lld\n", reply->integer);
            break;

        case REDIS_REPLY_NIL:

            break;

        case REDIS_REPLY_STATUS:

            break;

        case REDIS_REPLY_ERROR:

            break;

        default:
            printf("reply type error\n");
            break;
    }
}

int g_yuv_size ;

uint8_t *g_yuv ;
    int start_time ;
    int now_time;


int pipe_fd[2];

void subCallback(redisAsyncContext *c, void *r, void *priv) {
    redisReply *reply = r;

    int mem_id = -1;
    uint8_t *mem_ptr = NULL;
    int ret;
    float rate=0;

    if (reply == NULL) return;
    if ( reply->type == REDIS_REPLY_ARRAY && reply->elements == 3 ) {

        //printf( "Received[%s] channel %s: %s, cnt=%d\n",(char*)priv,reply->element[1]->str, reply->element[2]->str , g_cnt++);
        
        if(reply->element[2]->type == REDIS_REPLY_INTEGER){
            //printf("type:REDIS_REPLY_INTEGER, val=%lld\n", reply->element[2]->integer);
            mem_id = reply->element[2]->integer;
        }else if(reply->element[2]->type == REDIS_REPLY_STRING){
            //printf("type:REDIS_REPLY_STRING, val=%s\n", reply->element[2]->str);
            mem_id = atoi(reply->element[2]->str);
        }

        // get the memory ptr
        mem_ptr = shmat(mem_id, 0, 0);
        if(mem_ptr == (void *)-1){
            printf("get share memory(%d) ptr error.\n", mem_id);
            return ;
        }

        // write data to share memory 
        //memcpy(g_yuv, mem_ptr, g_yuv_size);

        // release share memory
        if(mem_ptr){
            ret = shmdt(mem_ptr);
            if(ret < 0){
                printf("release share memory ptr error:%d\n", ret);
                return;
            }
        }

        // write to pip
        write(pipe_fd[1], &mem_id, sizeof(mem_id));


        g_cnt++;

        now_time = time((time_t*)NULL);
        if((now_time-start_time) > 0)
            rate = (float)g_cnt/(now_time-start_time);

        printf("g_cnt=%d, time=%d, rate=%f\n", g_cnt, now_time-start_time, rate);

    }

    int i;
    // format should be :  message  channel  id
    //for(i = 0; i < reply->elements; i++){
    //    printReply(reply->element[i]);
    //}

    /* Disconnect after receiving the reply to GET */
    //redisAsyncDisconnect(c);
}

void connectCallback(const redisAsyncContext *c, int status) {
    if (status != REDIS_OK) {
        printf("Error: %s\n", c->errstr);
        return;
    }
    printf("Connected...\n");
}

void disconnectCallback(const redisAsyncContext *c, int status) {
    if (status != REDIS_OK) {
        printf("Error: %s\n", c->errstr);
        return;
    }
    printf("Disconnected...\n");
}



void thread_function(void *arg) 
{
    printf("This is thread_function.\n");

    redisContext *c;
    redisReply *reply;
    const char *hostname = "127.0.0.1";
    int port = 6379;
    const char *pubname = "test";

    struct timeval timeout = { 1, 500000 }; // 1.5 seconds
    c = redisConnectWithTimeout(hostname, port, timeout);
    //c = redisConnectUnixWithTimeout(hostname, timeout);
    if (c == NULL || c->err) {
        if (c) {
            printf("Connection %s error: %s\n", hostname, c->errstr);
            redisFree(c);
        } else {
            printf("Connection %s error: can't allocate redis context\n", hostname);
        }
        exit(1);
    }

    printf("Connect redis:%s ok.\n", hostname);


    int mem_id;



    while(1){

        read(pipe_fd[0],&mem_id,sizeof(mem_id));
        //printf("mem_id=%d\n", mem_id);
        // free share memory id
        reply = redisCommand(c,"SMEMFREE %d", mem_id);
        if(!reply){
            printf("SMEFREE reply error\n");
            return ;
        }
        freeReplyObject(reply);
 
    }

}

int main (int argc, char **argv) {
    signal(SIGPIPE, SIG_IGN);
    struct event_base *base = event_base_new();

    const char *hostname = (argc > 1) ? argv[1] : "127.0.0.1";
    int port = (argc > 2) ? atoi(argv[2]) : 6379;
    const char *subname = (argc > 3) ? argv[3] : "test";

    redisAsyncContext *c = redisAsyncConnect(hostname, port);
    if (c->err) {
        /* Let *c leak for now... */
        printf("Error: %s\n", c->errstr);
        return 1;
    }

    g_yuv_size = 1920*1080*3/2;
    g_yuv = malloc(g_yuv_size);
    start_time = time((time_t*)NULL);
    now_time = time((time_t*)NULL);


    // init pipe
    if(pipe(pipe_fd)){
        printf("pipe error\n");
        return -1;
    }

    // create thread
    pthread_attr_t attr;
    struct sched_param param;
    pthread_t tsk_id;

    pthread_attr_init(&attr);
    pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
    pthread_attr_getschedparam(&attr, &param);
     
    pthread_create(&tsk_id, &attr, (void *)thread_function, NULL);



    // event
    redisLibeventAttach(c,base);
    redisAsyncSetConnectCallback(c,connectCallback);
    redisAsyncSetDisconnectCallback(c,disconnectCallback);
    redisAsyncCommand(c, subCallback, (char*) "sub", "SMEMSUBSCRIBE %s", subname);

    event_base_dispatch(base);
    return 0;
}
