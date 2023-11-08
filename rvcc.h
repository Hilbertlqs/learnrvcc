#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum token_kind {
    TK_PUNCT,
    TK_NUM,
    TK_EOF
};

struct token {
    enum token_kind kind;
    struct token *next;
    int val;
    char *loc;
    int len;
};

void error(char *fmt, ...);
void error_at(char *loc, char *fmt, ...);
void error_tok(struct token *tok, char *fmt, ...);
bool equal(struct token *tok, char *str);
struct token *skip(struct token *tok, char *str);
struct token *tokenize(char *input);

enum node_kind {
    ND_ADD,
    ND_SUB,
    ND_MUL,
    ND_DIV,
    ND_NEG,
    ND_EQ,
    ND_NE,
    ND_LT,
    ND_LE,
    ND_NUM
};

struct node {
    enum node_kind kind;
    struct node *lhs;    // left-hand side
    struct node *rhs;    // right-hand side
    int val;
};

struct node *parse(struct token *tok);

void codegen(struct node *nd);
