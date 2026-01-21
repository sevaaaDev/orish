#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>

char** parse_commands(char* commands) {
    int size = 1;
    char** tokens = calloc(size+1, sizeof(char*));
    if (!tokens) return NULL;

    int i = 0;
    char* token = strtok(commands, " ");
    while (token) {
        if (i == size) {
            char** new_tokens = realloc(tokens, (size*2+1) * sizeof(char*));
            if (!new_tokens) {
                free(tokens);
                return NULL;
            }
            if (new_tokens != tokens) {
                free(tokens);
                tokens = new_tokens;
            }
            size *= 2;
        }
        tokens[i++] = token; 
        token = strtok(NULL, " ");
    }
    if (!tokens[0]) {
        return NULL;
    }
    tokens[i] = '\0';
    return tokens;
}

int
main(int argc, char** argv) {
    if (argc == 1) return 1; 
    char* commands = argv[1];

    char** tokens = parse_commands(commands);
    if (!tokens) return 2;
    if (execvp(tokens[0], tokens) == -1) {
        printf("%s: %s: %s\n",argv[0], tokens[0], strerror(errno));
    }
/*    for (int i = 0; tokens[i] != NULL; ++i) {
        printf("%p = %s\n", tokens[i], tokens[i]);
    }*/
}
// split by space
// set token[0] to be cmd
// pass the rest token as param
