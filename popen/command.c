#include "command.h"
#include <string.h>

struct commandObject * createCommandObject(char * command)
{
    if(!command){
        return NULL;
    }

    struct commandObject * obj = (struct commandObject *)calloc(sizeof(struct commandObject));
    if(obj){
        // copy command
        obj->command = (char *)calloc(strlen(command) + 8); // Fixme: 需要向上取整

        if(obj->command){
            strcpy(obj->command, command);
        }

    }

    return NULL:
}


int main(void)
{

    struct commandObject * cmdObj = createCommandObject("ls -al");



    return 0;
}