#ifndef builtin_h_INCLUDED
#define builtin_h_INCLUDED
#include <stdlib.h>
#include <limits.h>

// TODO: figure out the api for error handling
int orish_builtin_exit(int argc, char **argv) {
    int ret = 0;
    if (argc >= 2) {
       long v = strtol(argv[1], NULL, 10);
       if (v != LONG_MIN && v != LONG_MAX) 
           ret = (int)v;
    }
    printf("exit\n");
    exit(ret);
    return ret;
}

int orish_builtin_cd(int argc, char **argv) {
    char *path = argv[1];
    if (argc == 1) {
        path = getenv("HOME");
    }
    if (chdir(path) == -1)
        return 1;
    return 0;
}

#endif // builtin_h_INCLUDED
