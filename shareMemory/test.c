#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/shm.h>


// test1
int main1(int argc, char **argv)
{
    printf("This is producer.\n");

    const char *test_dir = (argc > 1) ? argv[1] : "/tmp";



    printf("Test dir is:%s ok.\n", test_dir);

    int cnt = 0;
    float rate=0;
    int start_time = time((time_t*)NULL);
    int now_time = time((time_t*)NULL);

    int yuv_size = 1920*1080*3/2;
    //int yuv_size = 1920*1080*3;
    char *yuv = malloc(yuv_size);

    int ret = 0;
    int mem_id = -1;
    char * mem_ptr = NULL;

    while(cnt < 10000){

        // get share memory 
        mem_id = shmget(IPC_PRIVATE, yuv_size, 600);
        if(mem_id < 0){
            printf("get share memory error:%d\n", mem_id);
            return -1;
        }

        mem_ptr = shmat(mem_id, 0, 0);
        if(mem_ptr == (void *)-1){
            printf("get share memory ptr error.\n");
            return -1;
        }

        // copy data
        memcpy(mem_ptr, yuv, yuv_size);

        cnt++;

        now_time = time((time_t*)NULL);
        if((now_time-start_time) > 0)
            rate = (float)cnt/(now_time-start_time);

        printf("cnt=%d, time=%d, rate=%f\n", cnt, now_time-start_time, rate);

        // release share memory
        ret = shmdt(mem_ptr);
        if(ret < 0){
            printf("release share memory ptr error:%d\n", ret);
            return -1;
        }

        ret = shmctl(mem_id, IPC_RMID, 0);
        if(ret < 0){
            printf("release share memory id error:%d\n", ret);
            return -1;
        }
    }

    return 0;
}



// test2
int main2(int argc, char **argv)
{
    printf("This is producer.\n");

    const char *test_dir = (argc > 1) ? argv[1] : "/tmp";



    printf("Test dir is:%s ok.\n", test_dir);

    int cnt = 0;
    float rate=0;
    int start_time = time((time_t*)NULL);
    int now_time = time((time_t*)NULL);

    int yuv_size = 1920*1080*3/2;
    //int yuv_size = 1920*1080*3;
    char *yuv = malloc(yuv_size);

    int ret = 0;
    int mem_id = -1;
    char * mem_ptr = NULL;

    // get share memory 
    mem_id = shmget(IPC_PRIVATE, yuv_size, 600);
    if(mem_id < 0){
        printf("get share memory error:%d\n", mem_id);
        return -1;
    }



    while(cnt < 10000){
        
        mem_ptr = shmat(mem_id, 0, 0);
        if(mem_ptr == (void *)-1){
            printf("get share memory ptr error.\n");
            return -1;
        }

        // copy data
        //memcpy(mem_ptr, yuv, yuv_size);

        ret = shmdt(mem_ptr);
        if(ret < 0){
            printf("release share memory ptr error:%d\n", ret);
            return -1;
        }
        
        cnt++;
        now_time = time((time_t*)NULL);
        if((now_time-start_time) > 0)
            rate = (float)cnt/(now_time-start_time);
        //printf("cnt=%d, time=%d, rate=%f\n", cnt, now_time-start_time, rate);


        // read 
        mem_ptr = shmat(mem_id, 0, 0);
        if(mem_ptr == (void *)-1){
            printf("get share memory ptr error.\n");
            return -1;
        }

        // copy data
        memcpy(yuv, mem_ptr, yuv_size);

        ret = shmdt(mem_ptr);
        if(ret < 0){
            printf("release share memory ptr error:%d\n", ret);
            return -1;
        }

    }
    printf("cnt=%d, time=%d, rate=%f\n", cnt, now_time-start_time, rate);
    // release share memory

    ret = shmctl(mem_id, IPC_RMID, 0);
    if(ret < 0){
        printf("release share memory id error:%d\n", ret);
        return -1;
    }

    return 0;
}

// test3
int main3(int argc, char **argv)
{
    printf("This is producer.\n");

    const char *test_dir = (argc > 1) ? argv[1] : "/tmp";



    printf("Test dir is:%s ok.\n", test_dir);

    int cnt = 0;
    float rate=0;
    int start_time = time((time_t*)NULL);
    int now_time = time((time_t*)NULL);

    int yuv_size = 1920*1080*3/2;
    //int yuv_size = 1920*1080*3;
    char *yuv = malloc(yuv_size);
    char *yuv2 = malloc(yuv_size);

    int ret = 0;
    int mem_id = -1;
    char * mem_ptr = NULL;

    while(cnt < 100000){


        // copy data
        memcpy(yuv2, yuv, yuv_size);

        cnt++;

        now_time = time((time_t*)NULL);
        if((now_time-start_time) > 0)
            rate = (float)cnt/(now_time-start_time);

        //printf("cnt=%d, time=%d, rate=%f\n", cnt, now_time-start_time, rate);

    }
    printf("cnt=%d, time=%d, rate=%f\n", cnt, now_time-start_time, rate);
    return 0;
}


// test4
int main4(int argc, char **argv)
{
    printf("This is producer.\n");

    const char *test_dir = (argc > 1) ? argv[1] : "/tmp";



    printf("Test dir is:%s ok.\n", test_dir);

    int cnt = 0;
    float rate=0;
    int start_time = time((time_t*)NULL);
    int now_time = time((time_t*)NULL);

    int yuv_size = 1920*1080*3/2;
    //int yuv_size = 1920*1080*3;
    char *yuv = malloc(yuv_size);

    int ret = 0;
    int mem_id = -1;
    char * mem_ptr = NULL;

    // get share memory 
    mem_id = shmget(IPC_PRIVATE, yuv_size, 600);
    if(mem_id < 0){
        printf("get share memory error:%d\n", mem_id);
        return -1;
    }

    mem_ptr = shmat(mem_id, 0, 0);
    if(mem_ptr == (void *)-1){
        printf("get share memory ptr error.\n");
        return -1;
    }


    while(cnt < 100000){
        


        // copy data
        memcpy(mem_ptr, yuv, yuv_size);

        cnt++;
        now_time = time((time_t*)NULL);
        if((now_time-start_time) > 0)
            rate = (float)cnt/(now_time-start_time);
        //printf("cnt=%d, time=%d, rate=%f\n", cnt, now_time-start_time, rate);


    }
    printf("cnt=%d, time=%d, rate=%f\n", cnt, now_time-start_time, rate);
    // release share memory

    ret = shmdt(mem_ptr);
    if(ret < 0){
        printf("release share memory ptr error:%d\n", ret);
        return -1;
    }

    ret = shmctl(mem_id, IPC_RMID, 0);
    if(ret < 0){
        printf("release share memory id error:%d\n", ret);
        return -1;
    }



    return 0;
}


// test4
int main(int argc, char **argv)
{
    printf("This is producer.\n");

    const char *test_dir = (argc > 1) ? argv[1] : "/tmp";



    printf("Test dir is:%s ok.\n", test_dir);

    int cnt = 0;
    float rate=0;
    int start_time = time((time_t*)NULL);
    int now_time = time((time_t*)NULL);

    int yuv_size = 1920*1080*3/2;
    //int yuv_size = 1920*1080*3;
    char *yuv = malloc(yuv_size);

    int ret = 0;
    int mem_id = -1;
    char * mem_ptr = NULL;
    int mem_id2 = -1;
    char * mem_ptr2 = NULL;

    // get share memory 
    mem_id = shmget(IPC_PRIVATE, yuv_size, 600);
    if(mem_id < 0){
        printf("get share memory error:%d\n", mem_id);
        return -1;
    }

    mem_ptr = shmat(mem_id, 0, 0);
    if(mem_ptr == (void *)-1){
        printf("get share memory ptr error.\n");
        return -1;
    }

    mem_id2 = shmget(IPC_PRIVATE, yuv_size, 600);
    if(mem_id2 < 0){
        printf("get share memory error:%d\n", mem_id2);
        return -1;
    }

    mem_ptr2 = shmat(mem_id2, 0, 0);
    if(mem_ptr2 == (void *)-1){
        printf("get share memory ptr2 error.\n");
        return -1;
    }

    while(cnt < 100000){
        


        // copy data
        memcpy(mem_ptr, mem_ptr2, yuv_size);

        cnt++;
        now_time = time((time_t*)NULL);
        if((now_time-start_time) > 0)
            rate = (float)cnt/(now_time-start_time);
        //printf("cnt=%d, time=%d, rate=%f\n", cnt, now_time-start_time, rate);


    }
    printf("cnt=%d, time=%d, rate=%f\n", cnt, now_time-start_time, rate);
    // release share memory

    ret = shmdt(mem_ptr);
    if(ret < 0){
        printf("release share memory ptr error:%d\n", ret);
        return -1;
    }

    ret = shmctl(mem_id, IPC_RMID, 0);
    if(ret < 0){
        printf("release share memory id error:%d\n", ret);
        return -1;
    }

    ret = shmdt(mem_ptr2);
    if(ret < 0){
        printf("release share memory ptr error:%d\n", ret);
        return -1;
    }

    ret = shmctl(mem_id2, IPC_RMID, 0);
    if(ret < 0){
        printf("release share memory id error:%d\n", ret);
        return -1;
    }

    return 0;
}
