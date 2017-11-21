
#include <stdio.h>


struct zExecObject{
    int status;  /* status return of command */
    int result;  /* the return value of the command */
    char * outputFile;  /* the output file of the cammand */
    char * command;
};


struct zExecObject * createZExecObject(char * command);
