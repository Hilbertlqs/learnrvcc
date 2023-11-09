#include "rvcc.h"

static char * current_input;

void error(char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    va_end(ap);

    exit(1);
}

static void verror_at(char *loc, char *fmt, va_list ap)
{
    int pos = loc - current_input;
    fprintf(stderr, "%s\n", current_input);
    fprintf(stderr, "%*s", pos, "");
    fprintf(stderr, "^ ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
}

void error_at(char *loc, char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    verror_at(loc, fmt, ap);
    va_end(ap);

    exit(1);
}

void error_tok(struct token *tok, char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    verror_at(tok->loc, fmt, ap);
    va_end(ap);

    exit(1);
}

bool equal(struct token *tok, char *op)
{
    return memcmp(tok->loc, op, tok->len) == 0 && op[tok->len] == '\0';
}

struct token *skip(struct token *tok, char *op)
{
    if (!equal(tok, op))
        error_tok(tok, "expect %s", op);
        
    return tok->next;
}

static struct token *new_token(enum token_kind kind, char *start, char *end)
{
    struct token *tok = calloc(1, sizeof(struct token));

    tok->kind = kind;
    tok->loc = start;
    tok->len = end - start;
    
    return tok;
}

static bool startswith(char *p, char *q)
{
    return strncmp(p, q, strlen(q)) == 0;
}

static int read_punct(char *p)
{
    if (startswith(p, "==") || startswith(p, "!=") || 
        startswith(p, "<=") || startswith(p, ">="))
        return 2;

    return ispunct(*p) ? 1 : 0;
}

struct token *tokenize(char *p)
{
    current_input = p;
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

        int punct_len = read_punct(p);
        if (punct_len) {
            cur->next = new_token(TK_PUNCT, p, p + punct_len);
            cur = cur->next;
            p += punct_len;
            continue;
        }

        error_at(p, "invalid token");
    }

    cur->next = new_token(TK_EOF, p, p);
    cur = cur->next;

    return head.next;
}
