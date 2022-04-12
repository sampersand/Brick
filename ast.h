#pragma once
#include "value.h"
#include "token.h"

struct ast_declaration *next_declaration(tokenizer *tzr);

typedef struct ast_primary {
	enum {
		AST_PAREN, AST_INDEX, AST_FNCALL,
		AST_NEG, AST_NOT, AST_ARY, AST_VAR,
		AST_INT, AST_STR, AST_TRUE, AST_FALSE, AST_NULL,
	} kind;

	union {
		struct {
			int amnt; // use in ary literal and fncall
			struct ast_primary *prim; // used in index, fncall, and neg/not.
			struct ast_expression *expr, **args; // used in index & paren; used in ary literal and fncall
		};
		char *str; // used in var and str
		long long num; // used in int
	};
} ast_primary;

typedef struct ast_expression {
	enum { AST_ASSIGN, AST_IDX_ASSIGN, AST_BINOP, AST_PRIM } kind;

	token_kind binop;
	struct ast_expression *index, *rhs; // index used for index assign ; rhs used for both assigns and binop
	union {
		struct ast_primary *prim; // used in binop and idx assign
		char *name; // used in assign
	};
} ast_expression;

typedef struct ast_statement {
	enum { AST_RETURN, AST_IF, AST_WHILE, AST_BREAK, AST_CONTINUE, AST_LOCAL, AST_EXPR } kind;

	struct ast_expression *expr; // `return`, `if`'s condition, `while`'s condition, `expr`, and local
	struct ast_block *body, *else_body; // body is if and while body
	char *varname; // for ast_local
} ast_statement;

typedef struct ast_block {
	int amnt;
	struct ast_statement **stmts;
} ast_block;

typedef struct ast_declaration {
	enum { AST_GLOBAL, AST_FUNCTION, AST_EXTERN, AST_EXTERNF } kind;
	char *name;

	// not used for global or extern:
	char **args;
	int argc;
	struct ast_block *block;
} ast_declaration;

