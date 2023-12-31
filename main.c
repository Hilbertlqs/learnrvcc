#include "rvcc.h"

int main(int argc, char *argv[])
{
    if (argc != 2)
        error("%s: invalid number of arguments\n", argv[0]);

    struct token *tok = tokenize(argv[1]);

    struct function *prog = parse(tok);

    codegen(prog);

    return 0;
}
