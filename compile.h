#pragma once
#include "env.h"
#include "ast.h"

typedef struct {
	map globals;
	FILE *out;
} compiler;

void compile_declaration(ast_declaration *d, compiler *c);
void init_compiler(compiler *c);
