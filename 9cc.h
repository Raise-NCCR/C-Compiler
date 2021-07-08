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
    ND_ASSIGN,
    ND_LVAR,
    ND_RETURN
} NodeKind;

typedef struct Node Node;

struct Node
{
    NodeKind kind;
    Node *lhs;
    Node *rhs;
    int val;
    int offset;
};

Node *code[100];

typedef enum
{
    TK_RESERVED,
    TK_IDENT,
    TK_NUM,
    TK_RETURN,
    TK_EOF,
} TokenKind;

typedef struct Token Token;

struct Token
{
    TokenKind kind;
    Token *next;
    int val;
    char *str;
    int len;
};

Token *token;

typedef struct LVar LVar;

struct LVar {
    struct LVar *next;
    char *name;
    int len;
    int offset;
};

LVar *locals;

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
bool consume_kind(TokenKind kind);
Token *consume_ident();
void expect(char *op);
int expect_number();
bool at_eof();
Token *new_token(TokenKind kind, Token *cur, char *str, int len);
Token *tokenize();
LVar *find_lvar(Token *tok);
int is_alnum(char c);

#endif
