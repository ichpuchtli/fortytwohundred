#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define USAGE "USAGE: %s [-d] server port filename\n"
#define BAD_ARGS 1
#define BAD_PORT 2
#define BAD_FILE 3

int main(int argc, char **argv){
    int debug = 0;
    unsigned int port = 0;
    FILE *file = NULL;
    if (argc == 5) {
        /* -d not supplied */
        if (strncmp("-d", argv[1], 2) != 0) {
            fprintf(stderr, USAGE, argv[0]);
            return BAD_ARGS;
        }
        debug = 1;
    } else if (argc == 4) {
        /* -d supplied */
        if (strncmp("-d", argv[1], 2) == 0) {
            fprintf(stderr, USAGE, argv[0]);
            return BAD_ARGS;
        }
    } else {
        fprintf(stderr, USAGE, argv[0]);
        return BAD_ARGS;
    }
    /* check the port */
    char crap;
    if ((sscanf(argv[2 + debug], "%u%c", &port, &crap)) != 1 || port < 1 
            || port > 65535) {
        fprintf(stderr, "The port '%s' is invalid, please provide a "
                "port in the range 1 to 65535\n", argv[2 + debug]);
        return BAD_PORT;
    }
    /* check the file */
    if ((file = fopen(argv[3 + debug], "r")) == NULL) {
        perror("Error opening given file");
        return BAD_FILE;
    }
    
    /* connect and request to write (reads not required in the assignment) */
    
    fclose(file);
    return 0;
}
