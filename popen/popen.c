#include <stdio.h>



int main(void)
{

    //char * cmd = "../ffmpeg -i ../video/dabaojian.mp4 -acodec copy  -vcodec h264 -s 640x360 -b 600k -f flv -y /tmp/dbj.mp4";
    char * cmd = "ls -l";
    FILE * fp = popen(cmd, "r");
    char buffer[1024];

    int cnt = 0;
    while(fgets(buffer, sizeof(buffer), fp)){
        printf("Line[%d] : %s", ++cnt, buffer);

    }


    pclose(fp);

    return 0;
}