#include <stdio.h>
#include <stdlib.h>
#include <string.h>


int write_file(char * filepath, char * data, int data_size)
{
    int ret;

    FILE * fp = fopen(filepath, "w+");
    if(fp == NULL){
        return -1;
    }

    // write data
    ret = fwrite(data, 1, data_size, fp);
    if(ret<0){
        printf("fwrite file error:%d\n", ret);
        return ret;
    }

    if(ret != data_size){
        printf("write size(%d) != data_size(%d)\n", ret, data_size);
        return -1;
    }

    fclose(fp);

    return 0;
}

int delete_file(char * filepath)
{
    return remove(filepath);
}

int main(int argc, char **argv)
{
    printf("This is producer.\n");

    const char *test_dir = (argc > 1) ? argv[1] : "/tmp";



    printf("Test dir is:%s ok.\n", test_dir);

    int cnt = 0;
    float rate=0;
    int start_time = time((time_t*)NULL);
    int now_time = time((time_t*)NULL);

    //int yuv_size = 1920*1080*3/2;
    int yuv_size = 1920*1080*3;
    char *yuv = malloc(yuv_size);

    int ret = 0;
    char filepath[128];

    while(1){
        // generate filepath
        sprintf(filepath, "%s/%d.yuv", test_dir, cnt);
        printf("filepath:%s\n", filepath);

        // write file
        ret = write_file(filepath, yuv, yuv_size);
        if(ret){
            printf("write_file error:%d\n", ret);
            break;
        }

        cnt++;

        now_time = time((time_t*)NULL);
        if((now_time-start_time) > 0)
            rate = (float)cnt/(now_time-start_time);

        printf("cnt=%d, time=%d, rate=%f\n", cnt, now_time-start_time, rate);

        // delete file
        ret = delete_file(filepath);
        if(ret){
            printf("delete_file error:%d\n", ret);
            break;
        }
    }

    return 0;
}
