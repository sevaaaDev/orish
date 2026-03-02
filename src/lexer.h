#ifndef LEXER_H
#define LEXER_H

#include "../arena.h"

enum Lex_Error {
    LEX_ERROR_eof,
    LEX_ERROR_unmatched_dquotes,
    LEX_ERROR_unmatched_quotes,
    LEX_ERROR_unmatched_parenthesis
};

struct Lexer {
    size_t cur_line;
    const char *buf_start;
    const char *cur;
    const char *last_newline;
    enum Lex_Error err;
};


/* these are generic token, you must use TODO: lexer_classify_reserved() to determine the specific token.
 * all reserved word is also a normal word, this mean you can use TOKEN_reserved
 * in the place of normal word.
 * 
 * TOKEN_reserved can only be converted to specific reserved word if and only if it was preceeded by
 * TOKEN_separator or linebreak, else it is treated as normal word.
 * */
enum Token_Type {
    TOKEN_separator,
    TOKEN_linebreak,
    TOKEN_word,
    TOKEN_reserved,
};

// TODO: embed linenum and column to token
struct Token {
    enum Token_Type type; 
    char *value;
}; 

struct Lexer   lexer_new(const char *input);
struct Token * make_token(Arena *arena, enum Token_Type t, char *value); 
struct Token * lexer_scan(Arena *arena, struct Lexer *l);

/* this function will be used when we implement keyword */
/* TODO: find good name for this
 *    - lexer_token_classified_as
 *    - lexer_token_allow_reclassify
 * */
bool lexer_classify_word(enum Token_Type type, const struct Token *tok);
#endif
