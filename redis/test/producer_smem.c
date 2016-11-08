#include "smem_client.h"

int main(int argc, char **argv)
{
    printf("This is producer.\n");

    redisContext *c;
    redisReply *reply;
    const char *hostname = (argc > 1) ? argv[1] : "127.0.0.1";
    int port = (argc > 2) ? atoi(argv[2]) : 6379;
    const char *pubname = (argc > 3) ? argv[3] : "test";


    // create the smem procuder
    struct smemContext * ctx = smemCreateProducer(hostname, port, pubname);  // step1: create the ctx

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
        mem_id = smemGetShareMemory(ctx, yuv_size);  // step2: get share memory
        if(mem_id != -1){
            //printf("get memory id: %d\n", mem_id);

            // get the memory ptr
            mem_ptr = shmat(mem_id, 0, 0);
            if(mem_ptr == (void *)-1){
                printf("get share memory(%d) ptr error.\n", mem_id);
                return -1;
            }
        }else{
            printf("get memory failed...\n");
            break;
        }


        //printf("To write data to %d, ptr=%lld\n", mem_id, mem_ptr);
        // write data to share memory 
        memcpy(mem_ptr, yuv, yuv_size);

        // publish share memory id
        smemPublish(ctx, mem_id);   // step3: publish the memory


        // release share memory
        if(mem_ptr){
            ret = shmdt(mem_ptr);
            if(ret < 0){
                printf("release share memory ptr error:%d\n", ret);
                return -1;
            }
        }

        
        // free share memory id
        smemFreeShareMemory(ctx, mem_id); // step4: free the memory


        cnt++;

        now_time = time((time_t*)NULL);
        if((now_time-start_time) > 0)
            rate = (float)cnt/(now_time-start_time);

        printf("cnt=%d, time=%d, rate=%f\n", cnt, now_time-start_time, rate);

        if(cnt%50 == 0)
            sleep(1);
    }

    return 0;
}
