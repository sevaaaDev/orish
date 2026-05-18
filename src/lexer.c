#include <stdlib.h>
#include <string.h>
#include <setjmp.h>

#include "./lexer.h"

const char *tokentostr[] = {
    [TOKEN_separator] = "semicolon",
    [TOKEN_linebreak] = "linebreak",
    [TOKEN_word] = "string",
    [TOKEN_reserved] = "keyword",
};

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
lexer_scan(Arena *arena, struct Lexer *l, jmp_buf *jmp)
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
        case '\'':
        squote:
            while (*(l->cur) != '\0' 
                && *(l->cur) != '\'') l->cur++;
            if (*(l->cur) != '\'') {
                printf("unmatched quotes\n");
                longjmp(*jmp, 1);
            }
            l->cur++;

            goto normal;
        case '"':
        dquote:
            while (*(l->cur) != '\0' && *(l->cur) != '"') {
                if (*(l->cur) == '\\') l->cur++;
                l->cur++;
            }
            if (*(l->cur) != '"') {
                printf("unmatched quotes\n");
                longjmp(*jmp, 1);
            }
            l->cur++;
            goto normal;
        default: 
        normal:
            while (*(l->cur) != ';'  && *(l->cur) != '\0' 
                && *(l->cur) != '\n' && *(l->cur) != ' ') {
                if (*(l->cur) == '\\') l->cur++;
                l->cur++;
                if (*(l->cur) == '\'') {
                    l->cur++;
                    goto squote;
                }
                if (*(l->cur) == '"') {
                    l->cur++;
                    goto dquote;
                }
            }
            int len = l->cur - start;  
            char *cmd = arena_strndup(arena, start, len);
            return make_token(arena, TOKEN_word, cmd);
        }
    }
    // eof
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

char * 
lexer_util_quote_remover(Arena *arena, char *original)
{
    // use memmove to shift char
    // when found \ ' ", we shift
    int len = strlen(original);
    char *duped = arena_strdup(arena, original);  
    enum {no, squote, dquote} inside = no;
    for (int i = 0; i < len; i++) {
        if (duped[i] == '\0') {
            break;
        }
        switch (duped[i]) {
        case '\'':
            if (inside == dquote) continue;
            if (inside == squote)
                inside = no;
            else
                inside = squote;
            memmove(&duped[i], &duped[i+1], len - i);
            break;
        case '"':
            if (inside == squote) continue;
            if (inside == dquote)
                inside = no;
            else
                inside = dquote;
            memmove(&duped[i], &duped[i+1], len - i);
            break;
        case '\\': 
            if (inside == squote) continue;
            if (inside == dquote) {
                if (duped[i+1] != '"') continue;
            }
            // the next i need to be moved -1
            memmove(&duped[i], &duped[i+1], len - i);
            break;
        }
    }
    return duped;
}
