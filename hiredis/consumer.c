
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include "hiredis.h"
#include "async.h"
#include "adapters/libevent.h"



int main0(int argc, char **argv)
{
    printf("This is consumer.\n");

    redisContext *c;
    redisReply *reply;
    const char *hostname = (argc > 1) ? argv[1] : "127.0.0.1";
    int port = (argc > 2) ? atoi(argv[2]) : 6379;

    struct timeval timeout = { 1, 500000 }; // 1.5 seconds
    c = redisConnectWithTimeout(hostname, port, timeout);
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

    while(1){
        reply = redisCommand(c,"SUBSCRIBE test");
        if(reply){
            printf("reply type:%d, integer:%lld, len:%d\n", reply->type, reply->integer, reply->len);
        }else{
            printf("reply error\n");
        }
        freeReplyObject(reply);
        cnt++;
        now_time = time((time_t*)NULL);
        if((now_time-start_time) > 0)
            rate = (float)cnt/(now_time-start_time);

        printf("cnt=%d, time=%d, rate=%f\n", cnt, now_time-start_time, rate);

    }
 
    return 0;
}


int g_cnt = 0;

void subCallback(redisAsyncContext *c, void *r, void *priv) {
    redisReply *reply = r;
    if (reply == NULL) return;
    if ( reply->type == REDIS_REPLY_ARRAY && reply->elements == 3 ) {
        if ( strcmp( reply->element[0]->str, "subscribe" ) != 0 ) {
            printf( "Received[%s] channel %s: %s, cnt=%d\n",
                    (char*)priv,
                    reply->element[1]->str,
                    reply->element[2]->str , g_cnt++);
        }
    }
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

    redisLibeventAttach(c,base);
    redisAsyncSetConnectCallback(c,connectCallback);
    redisAsyncSetDisconnectCallback(c,disconnectCallback);
    redisAsyncCommand(c, subCallback, (char*) "sub", "SUBSCRIBE %s", subname);

    event_base_dispatch(base);
    return 0;
}
