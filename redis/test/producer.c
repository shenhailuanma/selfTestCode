#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <hiredis.h>

#include <sys/shm.h>
#include <unistd.h>
#include <inttypes.h>
#include <stdint.h>
#include <sys/types.h>

int main(int argc, char **argv)
{
    printf("This is producer.\n");

    redisContext *c;
    redisReply *reply;
    const char *hostname = (argc > 1) ? argv[1] : "127.0.0.1";
    int port = (argc > 2) ? atoi(argv[2]) : 6379;
    const char *pubname = (argc > 3) ? argv[3] : "test";

    //const char *hostname = "127.0.0.1";
    //int port =  6379;
    //const char *pubname = (argc > 1) ? argv[1] : "test";

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

    int cnt = 0;
    float rate=0;
    int start_time = time((time_t*)NULL);
    int now_time = time((time_t*)NULL);

    int yuv_size = 1920*1080*3/2;
    //int yuv_size = 1920;
    uint8_t *yuv = malloc(yuv_size);

    int mem_id = -1;
    uint8_t *mem_ptr = NULL;
    int ret;
    while(1){

        mem_id = -1;
        mem_ptr = NULL;

        // get share memory id
        reply = redisCommand(c,"SMEMGET %d",  yuv_size);
        if(reply){
            printf("reply type:%d, integer:%lld, len:%d, str:%s\n", reply->type, reply->integer, reply->len, reply->str);
            if(reply->type == REDIS_REPLY_INTEGER){
                mem_id = reply->integer;

                // get the memory ptr
                mem_ptr = shmat(mem_id, 0, 0);
                if(mem_ptr == (void *)-1){
                    printf("get share memory(%d) ptr error.\n", mem_id);
                    return -1;
                }
            }
        }else{
            printf("get share memory id reply error\n");
            break;
        }
        freeReplyObject(reply);


        //printf("To write data to %d, ptr=%lld\n", mem_id, mem_ptr);
        // write data to share memory 
        memcpy(mem_ptr, yuv, yuv_size);

        // publish share memory id
        printf("To SMEMPUBLISH %d to %s\n", mem_id, pubname);

        //reply = redisCommand(c,"PUBLISH test %d", cnt);
        reply = redisCommand(c,"SMEMPUBLISH %s %d", pubname, mem_id);
        if(!reply){
            printf("SMEMPUBLISH reply error\n");
            break;
        }
        freeReplyObject(reply);

        // release share memory
        if(mem_ptr){
            ret = shmdt(mem_ptr);
            if(ret < 0){
                printf("release share memory ptr error:%d\n", ret);
                return -1;
            }
        }

        
        // free share memory id
        reply = redisCommand(c,"SMEMFREE %d", mem_id);
        if(!reply){
            printf("SMEFREE reply error\n");
            break;
        }
        freeReplyObject(reply);


        cnt++;

        now_time = time((time_t*)NULL);
        if((now_time-start_time) > 0)
            rate = (float)cnt/(now_time-start_time);

        printf("cnt=%d, time=%d, rate=%f\n", cnt, now_time-start_time, rate);
        //sleep(1);
    }

    return 0;
}
