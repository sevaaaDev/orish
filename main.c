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

enum TokenType {
    T_Word,
    T_Separator
};

typedef struct {
    enum TokenType type; 
    void* value;
} Token;

void 
lexer_test(Token** tokens) {
    for (int i = 0; tokens[i]; ++i) {
        Token* t = tokens[i];
        if (t->value)
            printf("type: %d, value: %s, len: %ld\n", t->type, (char*)t->value, strlen((char*)t->value));
        else
            printf("type: %d\n", t->type);
    } 
}

Token**
addToken(Token** token_arr, enum TokenType t, void* value) {
    Token* tok = malloc(sizeof(Token));
    assert(tok);
    tok->type = t;
    tok->value = value;
    arrput(token_arr, tok);
    return token_arr;
}

Token**
lexer(char* line) {
    Token** token_arr = NULL;
    char* cur = line;
    while (*cur != '\0') {
        char* start = cur++;
        switch (*start) {
        case ' ':
            break;
        case ';':
            token_arr = addToken(token_arr, T_Separator, NULL);
            break;
        default: 
            while (*(cur) != ';' && *(cur) != '\0' && *(cur) != ' ') cur++;
            int len = cur - start;  
            char* cmd = strndup(start, len); 

            token_arr = addToken(token_arr, T_Word, cmd);
        }
    }
    arrput(token_arr, NULL);
    return token_arr;
}

int
main(int argc, char** argv) {
    if (argc == 1) return 1; 
    char* commands = argv[1];

    Token** tokens = lexer(commands);
    if (!tokens) return 2;
    lexer_test(tokens);
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
