#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>
#include <sys/wait.h>
#define ARENA_IMPLEMENTATION
#include "arena.h"

/* TODO: should arena be global? */
static Arena main_arena = {0};
static Arena *context_arena = &main_arena;
void *main_alloc(size_t size) {
    assert(context_arena);
    return arena_alloc(context_arena, size);
}

struct Lexer {
    const char *buf_start;
    const char *cur;
    const char *last_newline;
    size_t cur_line;
};

struct Lexer 
lex_new(const char *input) {
    struct Lexer l;
    l.buf_start = input;
    l.cur = input;
    l.last_newline = input;
    l.cur_line = 1;
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
    struct Token *tok = arena_alloc(context_arena, sizeof(struct Token));
    if (!tok) {
        printf("arena_alloc error");
        exit(1);
    }
    tok->type = t;
    tok->value = value;
    return tok;
}

char *arena_strndup(const char *src, size_t len) {
    char *str = arena_alloc(context_arena, len+1);
    strncpy(str, src, len);
    str[len] = '\0';
    return str;
}

struct Token *
lex_scan(struct Lexer *l) {
    while (*(l->cur) != '\0') {
        const char *start = l->cur++;
        switch (*start) {
        case ' ':
            break;
        case ';':
            return make_token(TOKEN_separator, arena_strndup(start, l->cur - start));
            break;
        default: 
            while (*(l->cur) != ';' && *(l->cur) != '\0' && *(l->cur) != ' ') l->cur++;
            int len = l->cur - start;  
            char *cmd = arena_strndup(start, len);
            return make_token(TOKEN_word, cmd);
        }
    }
    return NULL;
}

/* this function will be used when we implement keyword */
/* TODO: find good name for this
 *    - lexer_token_classified_as
 *    - lexer_token_allow_reclassify
 * */
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

struct Array_Ast_Node {
    size_t capacity;
    size_t count;
    struct Ast_Node **items;
};

struct Ast_Node {
    enum Ast_Type type;
    const struct Token *token;
    struct Array_Ast_Node children;
};

struct Ast_Node *
make_node(enum Ast_Type t, struct Token *tok) {
    struct Ast_Node *node = arena_alloc(context_arena, sizeof(struct Ast_Node));
    node->type = t; 
    node->token = tok; 
    node->children = (struct Array_Ast_Node){0};
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

/* TODO: add second lookahead */
struct Parser {
    struct Lexer *l;
    struct Token *lookahead;
};

struct Parser 
parser_new(struct Lexer *l) {
    struct Parser p;
    p.l = l;
    p.lookahead = NULL;
    return p;
}

/* TODO: add second lookahead peek */
const struct Token *
peek_token(struct Parser *p) {
    if (p->lookahead == NULL)
        p->lookahead = lex_scan(p->l);
    return p->lookahead;
}

/* TODO: consume first lookahead, then move the second lookahead to the first */
struct Token *
consume_token(struct Parser *p) {
    if (p->lookahead == NULL)
        p->lookahead = lex_scan(p->l);
    struct Token *tok = p->lookahead;
    p->lookahead = NULL;
    return tok;
}

/* TODO: overhaul error handling */
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
        arena_da_append(context_arena, &node->children, make_node(AST_TYPE_cmd_word, consume_token(p)));
    } while(peek_token(p) && lex_classify_word(TOKEN_word, peek_token(p)));
    *out = node;
    return (struct Parser_Status) {
        .kind = PARSER_STAT_KIND_success,
    };
}

/* TODO: use second lookahead to match grammar */
struct Parser_Status
parse_list(struct Parser *p, struct Ast_Node **out) {
    *out = NULL;
    struct Ast_Node *cmd; 
    struct Parser_Status err = parse_simple_command(p, &cmd);
    if (err.kind) return err;
    struct Ast_Node *node = make_node(AST_TYPE_list, NULL);
    arena_da_append(context_arena, &node->children, cmd);
    *out = node;
    while(peek_token(p) && peek_token(p)->type == TOKEN_separator) {  
        consume_token(p);
        struct Parser_Status err = parse_simple_command(p, &cmd);
        if (err.kind) return err;
        arena_da_append(context_arena, &node->children, cmd);
    }
    return (struct Parser_Status) {
        .kind = PARSER_STAT_KIND_success,
    };
}


void
exec_cmd(struct Ast_Node *root) {
    if (root->type != AST_TYPE_cmd) return;
    /* WARN: is empty init equal to zero init? the last elem must be 0 */
    /* WARN: variable size array means we cant do argument expansion,
     *       because the argc will be greater than children count */
    char *argv[root->children.count+1] = {};
    for (size_t i = 0; i < root->children.count; ++i) {
        struct Ast_Node *child = root->children.items[i];
        argv[i] = child->token->value;
    }
    pid_t pid = fork();
    if (pid == -1) return; 
    if (pid == 0) {
        if (execvp(argv[0], argv) == -1) {
            perror(argv[0]);
            exit(127);
        } 
    } else {
        wait(NULL);
    }
}

void
exec_prog(struct Ast_Node *root) {
    if (root->type != AST_TYPE_list) return;
    for (size_t i = 0; i < root->children.count; ++i) {
        struct Ast_Node *child = root->children.items[i];
        switch (child->type) {
            case AST_TYPE_cmd:
                exec_cmd(child);
                break;
            default:
                assert(0 == "TODO: implement exec non simple cmd");
        }
    }
}

int
main(int argc, char **argv) {
    int ret = 0;
    if (argc == 1) {
        ret = 1; 
        goto quit;
    }
    /* TODO: read input from file */
    /* TODO: read input from stdin */
    char *commands = argv[1];
    struct Lexer lexer = lex_new(commands);
    struct Parser parser = parser_new(&lexer);
    struct Ast_Node *root;
    struct Parser_Status stat = parse_list(&parser, &root);
    if (stat.kind) {
        ret = 3;
        goto quit;
    }
    exec_prog(root);

quit:
    arena_free(context_arena);
    return ret;
}
