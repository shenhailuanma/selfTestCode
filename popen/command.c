#include "command.h"
#include <string.h>

struct commandObject * createCommand(char * command, char * outFile)
{
    if(!command){
        return NULL;
    }
    
    struct commandObject * obj = (struct commandObject *)calloc(sizeof(struct commandObject), 1);
    if(obj){
        // copy command
        obj->command = (char *)calloc(strlen(command) + 8, 1); // Fixme: 需要向上取整

        if(obj->command){
            printf("copy command:%s\n", command);
            strcpy(obj->command, command);
        }

        if(outFile){
            obj->outFile = (char *)calloc(strlen(outFile) + 8, 1); // Fixme: 需要向上取整

            if(obj->outFile){
                printf("copy outFile:%s\n", outFile);
                strcpy(obj->outFile, outFile);
            }
        }

        return obj;
    }

    return NULL;
}

int execCommand(struct commandObject * obj)
{
    if(!obj)
        return -1;
    
    if(!obj->command)
        return -1;

    obj->fp = popen(obj->command, "r");


    char buffer[1024];

    while(fgets(buffer, sizeof(buffer), obj->fp)){

        if(obj->outFile){
            // if set output file, write the data to the file

        }else{
            printf("%s", buffer);
        }
        
    }
    return 0;
}


int releaseCommand(struct commandObject * obj)
{
    if(!obj)
        return -1;

    if(obj->fp){
        pclose(obj->fp);
    }



    return 0;
}


int main(void)
{

    //struct commandObject * cmdObj = createCommand("ls -al", "/tmp/out.txt");
    struct commandObject * cmdObj = createCommand("ls -al", NULL);

    if(execCommand(cmdObj) < 0){
        printf("execCommand error\n");
    }

    if(releaseCommand(cmdObj) < 0){
        printf("releaseCommand error\n");
    }

    return 0;
}