#include <stdio.h>


// Fixme: 以后需要增加返回码


struct commandObject{
    int status;  /* status return of command */
    int result;  /* the return value of the command */
    char * outFile;  /* the output file of the cammand */
    char * command;  /* the command */
    FILE * fp;  /* the popen return FILE* */
};


struct commandObject * createCommand(char * command, char * outFile);

int execCommand(struct commandObject * obj);

int releaseCommand(struct commandObject * obj);