
#include "smem_client.h"


int main(void)
{
    struct smemContext * ctx = smemCreateConsumer("127.0.0.1", 6379, "test");

    if(!ctx || ctx->err < 0){
        if (ctx) {
            printf("Connection error: %s\n", ctx->errstr);
            smemFree(ctx);
        } else {
            printf("Connection error: can't allocate redis context\n");
        }
        exit(1);
    }

    int mem_id = -1;
    while(1){
        // get memory 
        mem_id = smemGetShareMemory(ctx, 0);
        if(mem_id > 0){
            printf("get memory id: %d\n", mem_id);

            // free the memory id
            smemFreeShareMemory(ctx, mem_id);
        }else{
            sleep(1);
        }
        
    }


    return 0;
}