#ifndef NINECC_H
#define NINECC_H

#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum
{
    ND_EQU,
    ND_NEQ,
    ND_LES,
    ND_LEQ,
    ND_ADD,
    ND_SUB,
    ND_MUL,
    ND_DIV,
    ND_NUM,
    ND_ADDR,
    ND_DEREF,
    ND_ASSIGN,
    ND_LVAR,
    ND_RETURN,
    ND_IF,
    ND_ELSE,
    ND_WHILE,
    ND_FOR,
    ND_BLOCK,
    ND_FUNC,
    ND_DECLARE,
    ND_GLOBAL,
    ND_STRING,
} NodeKind;

typedef struct Type Type;

struct Type
{
    enum
    {
        INT,
        CHAR,
        PTR,
        ARRAY
    } ty;
    struct Type *ptr_to;
    size_t array_size;
};

typedef struct Node Node;

struct Node
{
    NodeKind kind;
    Node *lhs;
    Node *rhs;
    Type *ty;
    char *func_name;
    int val;
    int offset;
};

Node *code[100];

typedef enum
{
    TK_RESERVED,
    TK_NEW_IDENT,
    TK_IDENT,
    TK_NUM,
    TK_RETURN,
    TK_IF,
    TK_ELSE,
    TK_WHILE,
    TK_FOR,
    TK_EOF,
    TK_SIZEOF,
    TK_STRING,
} TokenKind;

typedef struct Token Token;

struct Token
{
    TokenKind kind;
    Token *next;
    Type *ty;
    int val;
    char *str;
    int len;
};

Token *token;

typedef struct LVar LVar;

struct LVar
{
    struct LVar *next;
    Type *ty;
    char *name;
    int len;
    int offset;
    int size;
};

LVar *locals;
LVar *globals;
LVar *strings;

char *user_input;

Node *new_node(NodeKind kind, Node *lhs, Node *rhs);
Node *new_node_num(int val);
void program();
Node *stmt();
Node *expr();
Node *assign();
Node *equality();
Node *relational();
Node *add();
Node *mul();
Node *unary();
Node *primary();
void gen(Node *node);
void error_at(char *loc, char *fmt, ...);
bool consume(char *op);
bool judge(char *op);
bool consume_kind(TokenKind kind);
Token *consume_ident();
void expect(char *op);
int expect_number();
bool at_eof();
Token *new_token(TokenKind kind, Token *cur, char *str, int len);
Token *tokenize();
LVar *find_lvar(Token *tok);
LVar *find_global_lvar(Token *tok);
LVar *find_global_lvar_offset(int offset);
int is_alnum(char c);
bool is_global(Node *node);
bool check(char op);

#endif
