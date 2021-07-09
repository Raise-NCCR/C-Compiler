#include "./9cc.h"

static int jump_number = 0;
static int arg_count = 0;
static char *arg_register[] = {"rdi", "rsi", "rdx", "rcx", "r8"};
static LVar *func;

Node *new_node(NodeKind kind, Node *lhs, Node *rhs)
{
    Node *node = calloc(1, sizeof(Node));
    node->kind = kind;
    node->lhs = lhs;
    node->rhs = rhs;
    return node;
}

Node *new_node_num(int val)
{
    Node *node = calloc(1, sizeof(Node));
    node->kind = ND_NUM;
    node->val = val;
    return node;
}

void program()
{
    int i = 0;
    locals = calloc(1, sizeof(LVar));
    while (!at_eof())
        code[i++] = stmt();
    code[i] = NULL;
}

Node *stmt()
{
    Node *node;

    if (consume_kind(TK_RETURN))
    {
        node = calloc(1, sizeof(Node));
        node->kind = ND_RETURN;
        node->lhs = expr();
    }
    else if (consume("{"))
    {
        node = stmt();
        for (;;)
        {
            if (consume("}"))
                return node;
            else
                node = new_node(ND_BLOCK, node, stmt());
        }
    }
    else if (consume_kind(TK_IF))
    {
        expect("(");
        node = expr();
        expect(")");
        node = new_node(ND_IF, node, stmt());
        for (;;)
        {
            if (consume_kind(TK_ELSE))
                node = new_node(ND_ELSE, node, stmt());
            else
                return node;
        }
    }
    else if (consume_kind(TK_WHILE))
    {
        expect("(");
        node = expr();
        expect(")");
        node = new_node(ND_WHILE, node, stmt());
        return node;
    }
    else if (consume_kind(TK_FOR))
    {
        expect("(");
        int flag = 0;
        for (;;)
        {

            if (consume(";"))
                continue;
            else if (consume(")"))
            {
                node = new_node(ND_FOR, node, stmt());
                return node;
            }
            else
            {
                if (!flag++)
                    node = expr();
                else
                    node = new_node(ND_FOR, node, expr());
            }
        }
    }
    else
    {
        node = expr();
    }

    if (!consume(";"))
        error_at(token->str, "';'ではないトークンです");
    return node;
}

Node *expr()
{
    return assign();
}

Node *assign()
{
    Node *node = equality();

    if (consume("="))
        node = new_node(ND_ASSIGN, node, assign());
    return node;
}

Node *equality()
{
    Node *node = relational();

    for (;;)
    {
        if (consume("=="))
            node = new_node(ND_EQU, node, relational());
        else if (consume("!="))
            node = new_node(ND_NEQ, node, relational());
        else
            return node;
    }
}

Node *relational()
{
    Node *node = add();

    for (;;)
    {
        if (consume("<"))
            node = new_node(ND_LES, node, add());
        else if (consume("<="))
            node = new_node(ND_LEQ, node, add());
        else if (consume(">"))
            node = new_node(ND_LES, add(), node);
        else if (consume(">="))
            node = new_node(ND_LEQ, add(), node);
        else
            return node;
    }
}

Node *add()
{
    Node *node = mul();

    for (;;)
    {
        if (consume("+"))
            node = new_node(ND_ADD, node, mul());
        else if (consume("-"))
            node = new_node(ND_SUB, node, mul());
        else
            return node;
    }
}

Node *mul()
{
    Node *node = unary();

    for (;;)
    {
        if (consume("*"))
            node = new_node(ND_MUL, node, unary());
        else if (consume("/"))
            node = new_node(ND_DIV, node, unary());
        else
            return node;
    }
}

Node *unary()
{
    if (consume("+"))
        return primary();
    else if (consume("-"))
        return new_node(ND_SUB, new_node_num(0), unary());
    return primary();
}

Node *primary()
{
    Token *tok = consume_ident();
    if (tok)
    {
        Node *node = calloc(1, sizeof(Node));
        if (consume("("))
        {
            node->kind = ND_FUNC;
            int flag = 0;
            for (;;)
            {
                if (consume(","))
                    continue;
                else if (consume(")"))
                    break ;
                else
                    node = new_node(ND_FUNC, node, primary());
            }
        }
        else
            node->kind = ND_LVAR;

        LVar *lvar = find_lvar(tok);
        if (lvar)
        {
            node->offset = lvar->offset;
        }
        else
        {
            lvar = calloc(1, sizeof(LVar));
            lvar->next = locals;
            lvar->name = tok->str;
            lvar->len = tok->len;
            lvar->offset = locals->offset + 8;
            node->offset = lvar->offset;
            locals = lvar;
        }
        return node;
    }

    if (consume("("))
    {
        Node *node = expr();
        expect(")");
        return node;
    }

    return new_node_num(expect_number());
}

void gen_lval(Node *node)
{
    if (node->kind != ND_LVAR)
        perror("代入の左辺値が変数ではありません");

    printf("    mov rax, rbp\n");
    printf("    sub rax, %d\n", node->offset);
    printf("    push rax\n");
}

char *find_func(Node *node)
{
    for (LVar *var = locals; var; var = var->next)
    {
        if (var->offset == node->offset)
        {
            char *func_name = (char *)calloc(var->len + 1, 1);
            return strncpy(func_name, var->name, var->len);
        }
    }
    return NULL;
}

void gen_func(Node *node)
{
    if (node->rhs == NULL)
        return ;
    if (node->lhs->lhs == NULL)
    {
        gen(node->rhs);
        printf("    pop %s\n", arg_register[arg_count++]);
        return ;
    }
    gen(node->rhs);
    printf("    pop %s\n", arg_register[arg_count++]);
    gen_func(node->lhs);
    return ;
}

void gen(Node *node)
{
    switch (node->kind)
    {
    case ND_NUM:
        printf("    push %d\n", node->val);
        return;
    case ND_LVAR:
        gen_lval(node);
        printf("    pop rax\n");
        printf("    mov rax, [rax]\n");
        printf("    push rax\n");
        return;
    case ND_FUNC:
        arg_count = 0;
        gen_func(node);
        printf("    call %s\n", find_func(node));
        return;
    case ND_ASSIGN:
        gen_lval(node->lhs);
        gen(node->rhs);

        printf("    pop rdi\n");
        printf("    pop rax\n");
        printf("    mov [rax], rdi\n");
        printf("    push rdi\n");
        return;
    case ND_RETURN:
        gen(node->lhs);

        printf("    pop rax\n");
        printf("    mov rsp, rbp\n");
        printf("    pop rbp\n");
        printf("    ret\n");
        return;
    case ND_IF:
        gen(node->lhs);

        printf("    pop rax\n");
        printf("    cmp rax, 0\n");
        printf("    je .Lend%d\n", jump_number);

        gen(node->rhs);

        printf(".Lend%d:\n", jump_number++);
        return;
    case ND_ELSE:
        gen(node->lhs->lhs);

        printf("    pop rax\n");
        printf("    cmp rax, 0\n");
        printf("    je .Lelse%d\n", jump_number);

        gen(node->lhs->rhs);

        printf("    jmp .Lend%d\n", jump_number);
        printf(".Lelse%d:\n", jump_number);

        gen(node->rhs);

        printf(".Lend%d:\n", jump_number++);
        return;
    case ND_WHILE:
        printf(".Lbegin%d:\n", jump_number);

        gen(node->lhs);

        printf("    pop rax\n");
        printf("    cmp rax, 0\n");
        printf("    je .Lend%d\n", jump_number);

        gen(node->rhs);

        printf("    jmp .Lbegin%d\n", jump_number);
        printf(".Lend%d:\n", jump_number++);
        return;
    case ND_FOR:
        gen(node->lhs->lhs->lhs);

        printf(".Lbegin%d:\n", jump_number);

        gen(node->lhs->lhs->rhs);

        printf("    pop rax\n");
        printf("    cmp rax, 0\n");
        printf("    je .Lend%d\n", jump_number);

        gen(node->rhs);
        gen(node->lhs->rhs);

        printf("    jmp .Lbegin%d\n", jump_number);
        printf(".Lend%d:\n", jump_number++);
        return;
    case ND_BLOCK:
        gen(node->lhs);

        printf("    pop rax\n");

        gen(node->rhs);
        return;
    }

    gen(node->lhs);
    gen(node->rhs);

    printf("    pop rdi\n");
    printf("    pop rax\n");

    switch (node->kind)
    {
    case ND_EQU:
        printf("    cmp rax, rdi\n");
        printf("    sete al\n");
        printf("    movzb rax, al\n");
        break;
    case ND_NEQ:
        printf("    cmp rax, rdi\n");
        printf("    setne al\n");
        printf("    movzb rax, al\n");
        break;
    case ND_LES:
        printf("    cmp rax, rdi\n");
        printf("    setl al\n");
        printf("    movzb rax, al\n");
        break;
    case ND_LEQ:
        printf("    cmp rax, rdi\n");
        printf("    setle al\n");
        printf("    movzb rax, al\n");
        break;
    case ND_ADD:
        printf("    add rax, rdi\n");
        break;
    case ND_SUB:
        printf("    sub rax, rdi\n");
        break;
    case ND_MUL:
        printf("    imul rax, rdi\n");
        break;
    case ND_DIV:
        printf("    cqo\n");
        printf("    idiv rdi\n");
        break;
    }

    printf("    push rax\n");
}
