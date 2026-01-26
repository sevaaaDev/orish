#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>

#define STB_DS_IMPLEMENTATION
#include "stb_ds.h"

void 
da_free(void** arr, size_t len) {
    for (size_t i = 0; i < len; ++i) {
        free(arr[i]);
    }
    arrfree(arr);
}

bool is_eol(char* arr, size_t len, i) {
    return (i >= len || arr[i] == '\0');
}

bool next_is(char c, char* arr, size_t len, i) {
    return (i != len && arr[i+1] == c);
}

Token** addToken(Token** token_arr, enum TokenType t, void* value) {
    Token* tok = malloc(sizeof Token);
    assert(tok);
    tok->type = t;
    tok->value = value;
    arrput(token_arr, tok);
    return token_arr;
}

enum TokenType {
    T_Command,
    T_Separator
};

typedef struct {
    enum Token_Type type; 
    void* value;
} Token;

#define and &&
#define or ||
Token** lexer(char* line, size_t len) {
    Token** token_arr = NULL;
    char** word_arr = NULL;
    char* ptr = line;
    int i = 0;
    //     echo    hello ;   cat main.c
    // echo hello; cat main.c
    while (!is_eol(line, len, i)) {
        char c = line[i];
        switch c {
        case ' ':
            break
        case ';':
            token_arr = addToken(token_arr, T_Separator, NULL);
            line[i] = '\0';
            break
        default: 
            do {
                ptr = line[i];
                // BUG: in "echo hello;" last o will be set to null
                // BUG: in "echo hello ;" the i is at ;, so cant be handled by main loop
                while((c != ' ') and !is_eol(line, len, i) and !next_is(';', line, len, i)) 
                    c = line[++i];
                arrput(word_arr, ptr);
                while(c == ' ') {
                    line[i] = '\0';
                    c = line[++i];
                }
            } while (!is_eol(line, len, i) and !next_is(';', line, len, i));
            token_arr = addToken(token_arr, T_Command, word_arr);
            word_arr = NULL;
        }
        i++;
    }
}

int
main(int argc, char** argv) {
    if (argc == 1) return 1; 
    char* commands = argv[1];

    Token** tokens = lexer(commands, strlen(command));
    if (!tokens) return 2;
/*    if (execvp(tokens[0], tokens) == -1) {
        printf("%s: %s: %s\n",argv[0], tokens[0], strerror(errno));
    }*/
/*    for (int i = 0; tokens[i] != NULL; ++i) {
        printf("%p = %s\n", tokens[i], tokens[i]);
    }*/
}
// split by space
// set token[0] to be cmd
// pass the rest token as param
/*char** parse_commands(char* commands) {
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

*/
