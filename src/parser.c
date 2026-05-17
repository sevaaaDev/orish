#include <setjmp.h>

#include "parser.h"

static const struct Token *
peek_token(Arena *arena, struct Parser *p) {
    if (p->lookahead == NULL)
        p->lookahead = lexer_scan(arena, p->l, p->exception_jump);
    return p->lookahead;
}

static const struct Token *
peek_token_2(Arena *arena, struct Parser *p) {
    if (p->lookahead_2 == NULL)
        p->lookahead_2 = lexer_scan(arena, p->l, p->exception_jump);
    return p->lookahead_2;
}

static struct Token *
consume_token(Arena *arena, struct Parser *p) {
    if (!p->lookahead) { 
        assert(!p->lookahead_2);
        p->lookahead = lexer_scan(arena, p->l, p->exception_jump);
    }
    struct Token *tok = p->lookahead;
    p->lookahead = p->lookahead_2;
    p->lookahead_2 = NULL;
    return tok;
}

static struct Node * simple_command(struct Parser *p);
static struct Node * complete_cmd(struct Parser *p);
static struct Node * list(struct Parser *p);
static struct Node * all_commands(struct Parser *p);

struct Node * 
parse(struct Parser *p) 
{
    return simple_command(p);
}

struct Parser 
parser_new(struct Lexer *l, jmp_buf *err_jmp, Arena *a, Arena *m)
{
    struct Parser p;
    p.l = l;
    p.lookahead = NULL;
    p.lookahead_2 = NULL;
    p.exception_jump = err_jmp; 
    p.array_arena = a;
    p.main_arena = m;
    return p;
}

struct Node *
make_node(Arena *arena, enum Node_Kind t, struct Token *tok) {
    struct Node *node = arena_alloc(arena, sizeof(struct Node));
    node->type = t; 
    switch (t) {
    case NODE_cmd:
        node->simple.cmd = NULL;
        node->simple.argv = NULL;
        break;
    }
    return node;
}

static struct Node *
simple_command(struct Parser *p) 
{
    if (!peek_token(p->main_arena, p)) {
       printf("expect a string\n");
       longjmp(*p->exception_jump, 1);
    }
    if (!lexer_classify_word(TOKEN_word, peek_token(p->main_arena, p))) {
       struct Token *got = consume_token(p->main_arena, p);
       printf("expect a command, got '%s'\n", got->value);
       longjmp(*p->exception_jump, 1);
    }
    struct Node *node = make_node(p->main_arena, NODE_cmd, NULL);
    struct Token_da arr = {0};
    do {
        struct Token *tok = consume_token(p->main_arena, p);
        arena_da_append(p->array_arena, &arr, tok); // this is one shot usage, use malloc?
    } while(peek_token(p->main_arena, p) && lexer_classify_word(TOKEN_word, peek_token(p->main_arena, p)));

    node->simple.argv = arena_alloc(p->main_arena, sizeof(char *) * (arr.count + 1));
    for (int i = 0; i < arr.count; i++) {
        node->simple.argv[i] = arr.items[i]->value;
    }
    node->simple.argv[arr.count] = NULL; 
    node->simple.cmd = node->simple.argv[0];
   
    return node;
}


// Parser_Error *
// parse_list(Arena *arena, struct Parser *p, struct Ast_Node **out) {
//     *out = NULL;
//     struct Ast_Node *node = make_node(arena, AST_TYPE_list, NULL);
//     struct Ast_Node *cmd; 
//     do {
//         Parser_Error *err = parse_simple_command(arena, p, &cmd);
//         if (err) return err;
//         arena_da_append(arena, &node->children, cmd);
//         if (peek_token(arena, p) && peek_token(arena, p)->type == TOKEN_separator)
//             consume_token(arena, p);
//         *out = node;
//     } while(peek_token(arena, p) && peek_token(arena, p)->type == TOKEN_word);
//     return NULL;
// }

// Parser_Error *
// parse_complete_cmd(Arena *arena, struct Parser *p, struct Ast_Node **out) {
//     *out = NULL;
//     struct Ast_Node *list; 
//     Parser_Error *err = parse_list(arena, p, &list);
//     if (err) return err;
//     if (peek_token(arena, p) && peek_token(arena, p)->type == TOKEN_separator) {
//         consume_token(arena, p);
//     }
//     *out = list;
//     return NULL;
// }

// Parser_Error *
// parse_all_commands(Arena *arena, struct Parser *p, struct Ast_Node **out) {
//     *out = NULL;
//     struct Ast_Node *all_cmds = make_node(arena, AST_TYPE_program, NULL);;
//     if (peek_token(arena, p) && peek_token(arena, p)->type == TOKEN_linebreak)
//         consume_token(arena, p);
//     while (peek_token(arena, p)) {
//         struct Ast_Node *list;
//         Parser_Error *err = parse_complete_cmd(arena, p, &list);
//         if (err) return err;
//         arena_da_append(arena, &all_cmds->children, list);
//         if (peek_token(arena, p) && peek_token(arena, p)->type == TOKEN_linebreak)
//             consume_token(arena, p);
//     }
//     *out = all_cmds;
//     return NULL;
// }
