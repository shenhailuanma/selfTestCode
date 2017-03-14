#include <stdio.h>

#include <sys/wait.h>  // for WEXITSTATUS()

int main(void)
{

    //char * cmd = "../ffmpeg -i ../video/dabaojian.mp4 -acodec copy  -vcodec h264 -s 640x360 -b 600k -f flv -y /tmp/dbj.mp4";
    char * cmd = "./ffmpeg";
    FILE * fp = popen(cmd, "r");
    char buffer[1024];

    int cnt = 0;
    while(fgets(buffer, sizeof(buffer), fp)){
        printf("Line[%d] : %s", ++cnt, buffer);

    }

    // et是pclose的返回值，通过宏WEXITSTATUS转换为命令的返回值
    int ret = pclose(fp);
    printf("return:%d\n", WEXITSTATUS(ret));
    

    return 0;
}