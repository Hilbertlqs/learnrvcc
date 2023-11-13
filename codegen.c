#include "rvcc.h"

static int depth;

static void push(void)
{
    printf("    addi sp, sp, -8\n");
    printf("    sd a0, 0(sp)\n");
    depth++;
}

static void pop(char *reg)
{
    printf("    ld %s, 0(sp)\n", reg);
    printf("    addi sp, sp, 8\n");
    depth--;
}

static int align_to(int n, int align)
{
    return (n + align - 1) / align * align;
}

static void gen_addr(struct node *nd)
{
    if (nd->kind == ND_VAR) {
        printf("    addi a0, fp, %d\n", nd->var->offset);
        return;
    }

    error("not a lvalue");
}

static void gen_expr(struct node *nd)
{
    switch (nd->kind) {
    case ND_NUM:
        // li rd, immediate: RV32I: lui and/or addi; RV64I: lui, addi, slli, addi, slli, addi, slli, addi
        printf("    li a0, %d\n", nd->val);
        return;
    case ND_NEG:
        // neg rd, rs: sub rd, x0, rs
        gen_expr(nd->lhs);
        printf("    neg a0, a0\n");
        return;
    case ND_VAR:
        gen_addr(nd);
        printf("    ld a0, 0(a0)\n");
        return;
    case ND_ASSIGN:
        gen_addr(nd->lhs);
        push();
        gen_expr(nd->rhs);
        pop("a1");
        printf("    sd a0, 0(a1)\n");
        return;
    default:
        break;
    }

    gen_expr(nd->rhs);
    push();
    gen_expr(nd->lhs);
    pop("a1");

    switch (nd->kind) {
    case ND_ADD:
        printf("    add a0, a0, a1\n");
        return;
    case ND_SUB:
        printf("    sub a0, a0, a1\n");
        return;
    case ND_MUL:
        printf("    mul a0, a0, a1\n");
        return;
    case ND_DIV:
        printf("    div a0, a0, a1\n");
        return;
    case ND_EQ:
    case ND_NE:
        printf("    xor a0, a0, a1\n");
        if (nd->kind == ND_EQ)
            // seqz rd, rs: sltiu rd, rs, 1
            printf("    seqz a0, a0\n");
        else
            // snez rd, rs: sltu rd, x0, rs
            printf("    snez a0, a0\n");
        return;
    case ND_LT:
        printf("    slt a0, a0, a1\n");
        return;
    case ND_LE:
        printf("    slt a0, a1, a0\n");
        printf("    xori a0, a0, 1\n");
        return;
    default:
        break;
    }

    error("invalid expression");
}

static void gen_stmt(struct node *nd)
{
    if (nd->kind == ND_EXPR_STMT) {
        gen_expr(nd->lhs);
        return;
    }

    error("invalid statement");
}

static void assign_lvar_offsets(struct function *prog)
{
    int offset = 0;
    for (struct obj *var = prog->locals; var; var = var->next) {
        offset += 8;
        var->offset = -offset;
    }
    prog->stack_size = align_to(offset, 16);
}

void codegen(struct function *prog)
{
    assign_lvar_offsets(prog);

    printf("    .globl main\n");
    printf("main:\n");

    // stack
    //-------------------------------// sp
    //              fp
    //-------------------------------// fp = sp - 8
    //              variables
    //-------------------------------// sp = sp - 8 - stack_size
    //              ...
    //-------------------------------//

    // prologue
    printf("    addi sp, sp, -8\n");
    printf("    sd fp, 0(sp)\n");
    // mv rd, rs: addi rd, rs, 0
    printf("    mv fp, sp\n");

    printf("    addi sp, sp, -%d\n", prog->stack_size);

    for (struct node *n = prog->body; n; n = n->next) {
        gen_stmt(n);
        assert(depth == 0);
    }

    // epilogue
    // mv rd, rs: addi rd, rs, 0
    printf("    mv sp, fp\n");
    printf("    ld fp, 0(sp)\n");
    printf("    addi sp, sp, 8\n");

    printf("    ret\n");
}
