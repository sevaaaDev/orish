#include <stdlib.h>
#include <string.h>
#include "./lexer.h"

struct Lexer 
lexer_new(const char *input)
{
    struct Lexer l;
    l.buf_start = input;
    l.cur = input;
    l.last_newline = input;
    l.cur_line = 1;
    return l;
}

struct Token *
make_token(Arena *arena, enum Token_Type t, char *value)
{
    struct Token *tok = arena_alloc(arena, sizeof(struct Token));
    if (!tok) {
        printf("arena_alloc error");
        exit(1);
    }
    tok->type = t;
    tok->value = value;
    return tok;
}

char *
arena_strndup(Arena *arena, const char *src, size_t len)
{
    char *str = arena_alloc(arena, len+1);
    strncpy(str, src, len);
    str[len] = '\0';
    return str;
}

/* handle reading from file (handle newline) */
struct Token *
lexer_scan(Arena *arena, struct Lexer *l)
{
    while (*(l->cur) != '\0') {
        const char *start = l->cur++;
        switch (*start) {
        case ' ':
            break;
        case ';':
            return make_token(arena, TOKEN_separator, arena_strndup(arena, start, l->cur - start));
            break;
        case '\n':
            l->cur_line++;
            l->last_newline = l->cur;
            while (*(l->cur) == '\n' || *(l->cur) == ' ') {
                if (*(l->cur) == '\n') {
                    l->cur_line++;
                    l->last_newline = l->cur;
                }
                l->cur++;
            }
            return make_token(arena, TOKEN_linebreak, NULL);
        case '"':
            while (*(l->cur) != ';' 
                && *(l->cur) != '\0' 
                && *(l->cur) != '\n' 
                && *(l->cur) != '"') l->cur++;
            if (*(l->cur) != '"') {
                l->err = LEX_ERROR_unmatched_dquotes;
                return NULL;
            }
            // uncomment if you want to include quotes
            // l->cur++;
            // and set start + 1 to be start
            // and set l->cur - start - 1 to be l->cur - start
            return make_token(arena, TOKEN_word, arena_strndup(arena, start + 1, l->cur - start - 1));
        default: 
            while (*(l->cur) != ';' 
                && *(l->cur) != '\0' 
                && *(l->cur) != '\n' 
                && *(l->cur) != ' ') l->cur++;
            int len = l->cur - start;  
            char *cmd = arena_strndup(arena, start, len);
            return make_token(arena, TOKEN_word, cmd);
        }
    }
    l->err = LEX_ERROR_eof;
    return NULL;
}

/* this function will be used when we implement keyword */
/* TODO: find good name for this
 *    - lexer_token_classified_as
 *    - lexer_token_allow_reclassify
 * */
bool
lexer_classify_word(enum Token_Type type, const struct Token *tok)
{
    if (tok->type != TOKEN_word) return false;
    switch (type) {
    case TOKEN_word:
        return true;
    default:
        return false;
    }
}
