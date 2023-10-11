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

static void error(char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    va_end(ap);

    exit(1);
}

static bool equal(struct token *tok, char *str)
{
    return memcmp(tok->loc, str, tok->len) == 0 && str[tok->len] == '\0';
}

static struct token *skip(struct token *tok, char *str)
{
    if (!equal(tok, str))
        error("expect %s", str);
        
    return tok->next;
}

static int get_number(struct token *tok)
{
    if (tok->kind != TK_NUM)
        error("expect a number");

    return tok->val;
}

static struct token *new_token(enum token_kind kind, char *start, char *end)
{
    struct token *tok = calloc(1, sizeof(struct token));

    tok->kind = kind;
    tok->loc = start;
    tok->len = end - start;
    
    return tok;
}

static struct token *tokenize(char *p)
{
    struct token head = {};
    struct token *cur = &head;

    while (*p) {
        if (isspace(*p)) {
            ++p;
            continue;
        }

        if (isdigit(*p)) {
            cur->next = new_token(TK_NUM, p, p);
            cur = cur->next;
            const char *q = p;
            cur->val = strtoul(p, &p, 10);
            cur->len = p - q;
            continue;
        }

        if (*p == '+' || *p == '-') {
            cur->next = new_token(TK_PUNCT, p, p + 1);
            cur = cur->next;
            ++p;
            continue;
        }

        error("invalid token: %c", *p);
    }

    cur->next = new_token(TK_EOF, p, p);

    return head.next;
}

int main(int argc, char *argv[])
{
    if (argc != 2)
        error("%s: invalid number of arguments\n", argv[0]);

    struct token *tok = tokenize(argv[1]);

    printf("    .globl main\n");
    printf("main:\n");
    printf("    li a0, %d\n", get_number(tok));
    tok = tok->next;

    while (tok->kind != TK_EOF) {
        if (equal(tok, "+")) {
            tok = tok->next;
            printf("    addi a0, a0, %d\n", get_number(tok));
            tok = tok->next;
            continue;
        }

        tok = skip(tok, "-");
        printf("    addi a0, a0, -%d\n", get_number(tok));
        tok = tok->next;
    }
    
    printf("    ret\n");

    return 0;
}
