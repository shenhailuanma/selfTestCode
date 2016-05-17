#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <hiredis.h>


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

    while(1){

        //reply = redisCommand(c,"PUBLISH test %d", cnt);
        reply = redisCommand(c,"PUBLISH %s %b", pubname, yuv, yuv_size);
        //reply = redisCommand(c,"SET number%d %b", cnt, yuv, yuv_size);
        if(reply){
            printf("reply type:%d, integer:%lld, len:%d, str:%s\n", reply->type, reply->integer, reply->len, reply->str);
        }else{
            printf("reply error\n");
        }
        freeReplyObject(reply);
        
        reply = redisCommand(c,"EXPIRE number%d 1", cnt);
        if(reply){
            printf("reply type:%d, integer:%lld, len:%d, str:%s\n", reply->type, reply->integer, reply->len, reply->str);
        }else{
            printf("reply error\n");
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
