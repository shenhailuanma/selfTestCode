#include <stdio.h>


struct commandObject{
    int status;  /* status return of command */
    int result;  /* the return value of the command */
    char * outputFile;  /* the output file of the cammand */
    char * command;  /* the command */
    FILE * fp;  /* the popen return FILE* */
};


struct commandObject * createCommandObject(char * command);