#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>

#define STB_DS_IMPLEMENTATION
#include "stb_ds.h"

struct Lexer {
    const char *buf_start;
    const char *cur;
    const char *last_newline;
    size_t cur_line;
};

struct Lexer *
lex_new(const char *input) {
    struct Lexer *l = malloc(sizeof(struct Lexer));
    if (!l) {
        printf("malloc error\nabborting");
        exit(1);
    }
    l->buf_start = input;
    l->cur = input;
    l->last_newline = input;
    l->cur_line = 1;
    return l;
}

enum Token_Type {
    TOKEN_word,
    TOKEN_separator
};

struct Token {
    enum Token_Type type; 
    char *value;
}; 

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

void
lex_destroy_token(struct Token *tok) {
   free(tok->value);
   free(tok);
} 

struct Token *
lex_scan(struct Lexer *l) {
    while (*(l->cur) != '\0') {
        const char *start = l->cur++;
        switch (*start) {
        case ' ':
            break;
        case ';':
            return make_token(TOKEN_separator, strndup(start, l->cur - start));
            break;
        default: 
            while (*(l->cur) != ';' && *(l->cur) != '\0' && *(l->cur) != ' ') l->cur++;
            int len = l->cur - start;  
            char *cmd = strndup(start, len); 
            return make_token(TOKEN_word, cmd);
        }
    }
    return NULL;
}

/* this function will be used when we implement keyword */
bool
lex_classify_word(enum Token_Type type, const struct Token *tok) {
    if (tok->type != TOKEN_word) return false;
    switch (type) {
    case TOKEN_word:
        return true;
    default:
        return false;
    }
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

enum Parser_Stat_Kind {
    PARSER_STAT_KIND_success,
    PARSER_STAT_KIND_unexpected_token,
    PARSER_STAT_KIND_missing_token,
};

static const char *PARSER_ERROR_TABLE[] = {
    [PARSER_STAT_KIND_success] = "Success",
    [PARSER_STAT_KIND_unexpected_token] = "Unexpected token: %s",
    [PARSER_STAT_KIND_missing_token] = "Missing token: %s",
};

struct Parser_Status {
    enum Parser_Stat_Kind kind;
    struct Token *tok;
};

struct Parser {
    struct Lexer *l;
    struct Token *lookahead;
};

struct Parser *
parser_new(struct Lexer *l) {
    struct Parser *p = malloc(sizeof(struct Parser));
    if (!p) {
        printf("malloc error");
        exit(1);
    }
    p->l = l;
    p->lookahead = NULL;
    return p;
}

const struct Token *
peek_token(struct Parser *p) {
    if (p->lookahead == NULL)
        p->lookahead = lex_scan(p->l);
    return p->lookahead;
}

struct Token *
consume_token(struct Parser *p) {
    if (p->lookahead == NULL)
        p->lookahead = lex_scan(p->l);
    struct Token *tok = p->lookahead;
    p->lookahead = NULL;
    return tok;
}

struct Parser_Status
parse_simple_command(struct Parser *p, struct Ast_Node **out) {
    if (!peek_token(p)) {
       return (struct Parser_Status){
            .kind = PARSER_STAT_KIND_missing_token,
            .tok = consume_token(p),
        };
    }
    if (!lex_classify_word(TOKEN_word, peek_token(p))) {
       return (struct Parser_Status){
            .kind = PARSER_STAT_KIND_unexpected_token,
            .tok = consume_token(p),
        };
    }
    struct Ast_Node *node = make_node(AST_TYPE_cmd, NULL);
    do {
        arrput(node->children, make_node(AST_TYPE_cmd_word, consume_token(p)));
    } while(peek_token(p) && lex_classify_word(TOKEN_word, peek_token(p)));
    *out = node;
    return (struct Parser_Status) {
        .kind = PARSER_STAT_KIND_success,
    };
}

void
exec_ast(struct Ast_Node *root) {
    for (int i = 0; i < arrlen(root->children); ++i) {
        struct Ast_Node *node = root->children[i];
        if (node->type == AST_TYPE_cmd_word)
            printf("(%s)", node->token->value);
    }
    printf("\n");
}

int
main(int argc, char **argv) {
    int ret = 0;
    if (argc == 1) {
        ret = 1; 
        goto quit;
    }
    char *commands = argv[1];
    struct Lexer *lexer = lex_new(commands);
    struct Parser *parser = parser_new(lexer);
    struct Ast_Node *root;
    struct Parser_Status stat = parse_simple_command(parser, &root);

    exec_ast(root);
quit:
    free(lexer);
    free(parser);

    return ret;
}
