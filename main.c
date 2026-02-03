#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>

#define STB_DS_IMPLEMENTATION
#include "stb_ds.h"

void 
da_free(void **arr, size_t len) {
    for (size_t i = 0; i < len; ++i) {
        free(arr[i]);
    }
    arrfree(arr);
}

enum Token_Type {
    TOKEN_word,
    TOKEN_separator
};

struct Token {
    enum Token_Type type; 
    const char *value;
}; 

void
tok_free_array(struct Token **tokens) {
    for (int i = 0; i < arrlen(tokens); ++i) {
        struct Token *tok = tokens[i];
        if (!tok) return;
        free((void*)tok->value);
        free(tok);
    }
    arrfree(tokens);
}

void 
lexer_test(struct Token **tokens) {
    for (int i = 0; tokens[i]; ++i) {
        struct Token *t = tokens[i];
        if (t->value)
            printf("type: %d, value: %s, len: %ld\n", t->type, t->value, strlen(t->value));
        else
            printf("type: %d\n", t->type);
    } 
}

struct Token **
add_token(struct Token **token_arr, enum Token_Type t, char *value) {
    struct Token *tok = malloc(sizeof(struct Token));
    if (!tok) {
        printf("malloc error");
        exit(1);
    }
    tok->type = t;
    tok->value = value;
    arrput(token_arr, tok);
    return token_arr;
}

struct Token *
make_token(enum Token_Type t, char *value) {
    struct Token *tok = malloc(sizeof(struct Token));
    if (!tok) {
        printf("malloc error");
        exit(1);
    }
    tok->type = t;
    tok->value = value;
    return tok;
}

struct Token **
lexer(char *line) {
    struct Token **token_arr = NULL;
    char *cur = line;
    while (*cur != '\0') {
        char *start = cur++;
        switch (*start) {
        case ' ':
            break;
        case ';':
            token_arr = add_token(token_arr, TOKEN_separator, strndup(start, cur - start));
            break;
        default: 
            while (*(cur) != ';' && *(cur) != '\0' && *(cur) != ' ') cur++;
            int len = cur - start;  
            char *cmd = strndup(start, len); 

            token_arr = add_token(token_arr, TOKEN_word, cmd);
        }
    }
    // TODO: should you do arrput(token_arr, NULL)?
    return token_arr;
}

struct Lexer {
    size_t current_col;
    size_t buf_len;
    const char *buf;
};

bool
lex_scan(struct Lexer *l, struct Token **out) {
    char *cur = l->buf[l->current_col];
    while (*cur != '\0') {
        char *start = cur++;
        switch (*start) {
        case ' ':
            break;
        case ';':
            *out = make_token(TOKEN_separator, strndup(start, cur - start));
            return true;
            break;
        default: 
            while (*(cur) != ';' && *(cur) != '\0' && *(cur) != ' ') cur++;
            int len = cur - start;  
            char *cmd = strndup(start, len); 
            *out = make_token(TOKEN_word, cmd);
            return true;
        }
    }
    return false;
}

bool
lex_expect(enum Token_Type type, struct Lexer *lexer, struct Token **out) {
    if (!lex_scan(lexer, out)) {
        return false;
    }
    if (tok->type != type) {
        *out = NULL;
        return false;
    }
    return true
}


enum Ast_Type {
    AST_TYPE_list,
    AST_TYPE_cmd,
    AST_TYPE_cmd_word,
};

struct Ast_Node {
    enum Ast_Type type;
    const struct Token *token;
    struct Ast_Node **children;
};

void
ast_free(struct Ast_Node *root) {
    if (root->children) {
        for (int i = 0; i < arrlen(root->children); ++i) {
            ast_free(root->children[i]);
        }
        arrfree(root->children);
    }
    free(root);
}

struct Ast_Node *
make_node(enum Ast_Type t, struct Token *tok) {
    struct Ast_Node *node = malloc(sizeof(struct Ast_Node));
    node->type = t; 
    node->token = tok; 
    node->children = NULL;
    return node;
}

struct Token*
tok_consume(TokenIter *iter) {
    return iter->buf[iter->i++];
}

struct Token*
tok_peek(TokenIter *iter) {
    return iter->buf[iter->i];
}

enum Parser_Stat_Kind {
    PARSER_STAT_KIND_success,
    PARSER_STAT_KIND_unexpected_token,
};

static const char *PARSER_ERROR_TABLE[] = {
    [PARSER_STAT_KIND_success] = "Success",
    [PARSER_STAT_KIND_unexpected_token] = "Unexpected token",
};

struct Parser_Status {
    enum Parser_Stat_Kind kind;
    union {
        const struct Token *err_token;
    } data;
};


struct Parser_Status
parse_simple_command(struct Lexer *lexer, struct Ast_Node **out) {
    struct Token *tok;
    if (!lex_expect(TOKEN_word, lexer, &tok)) { 
        return (struct Parser_Status){ 
            .kind = PARSER_STAT_KIND_unexpected_token, 
            .data.err_token = tok_consume(iter)
        };
    }
    struct Ast_Node *node = make_node(AST_TYPE_cmd, NULL);
    do {
        arrput(node->children, make_node(AST_TYPE_cmd_word, tok));
    } while(lex_expect(TOKEN_word, lexer, &tok));
    *out = node;
    return (struct Parser_Status) {
        .kind = PARSER_STAT_KIND_success,
    };
}

struct Parser_Status
parser(struct Token **tokens, struct Ast_Node **out) {
    TokenIter iter = make_token_iter(tokens);
    return parse_simple_command(&iter, out);
} 

int
main(int argc, char **argv) {
    int ret = 0;
    if (argc == 1) {
        ret = 1; 
        goto quit;
    }
    char *commands = argv[1];

    struct Token **tokens = lexer(commands);
    if (!tokens) {
        ret = 2;
        goto quit;
    };
    struct Ast_Node *root;
    struct Parser_Status stat = parser(tokens, &root);
    if (stat.kind) { // error is non zero
        printf("%s: \"%s\"\n", PARSER_ERROR_TABLE[stat.kind], stat.data.err_token->value);
        ret = 3;
        goto cleanup_token_quit;
    }

    if (stat.kind == PARSER_STAT_KIND_success) ast_free(root);
cleanup_token_quit:
    tok_free_array(tokens);
quit:
    return ret;
}
