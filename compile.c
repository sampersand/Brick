 #include "compile.h"
#include "ast.h"
#include "shared.h"
#include <stdbool.h>
#include <string.h>

#define FPRINTF(...) (fprintf(c->out, __VA_ARGS__) >= 0 ? (void) 0 : die("cannot fprintf"))
#define FPUTS(str) (fputs(str, c->out) >= 0 ? (void) 0 : die("cannot fputs"))
#define FPUTC(chr) (fputc(chr, c->out) >= 0 ? (void) 0 : die("cannot fputc"))

#define compile(thing) (_Generic(thing,\
	ast_expression*: compile_expression, \
	ast_primary*: compile_primary\
	)(thing, c))
void compile_block(ast_block *b, compiler *c, bool add_return_zero);
void compile_expression(ast_expression *e, compiler *c);
void compile_primary(ast_primary *p, compiler *c) {
	FPUTC('(');
	switch (p->kind) {
	case AST_PAREN: compile(p->expr); break;

	case AST_INDEX:
		FPUTS("((bint*)");
		compile(p->prim);
		FPUTS(")[");
		compile(p->expr);
		FPUTC(']');
		break;

	// // this is bad. 
	case AST_FNCALL:
		compile(p->prim);
		FPUTC('(');
		for (int i = 0; i < p->amnt; ++i) {
			if (i) FPUTC(',');
			compile(p->args[i]);
		}
		FPUTC(')');
		break;

	case AST_NEG: FPUTC('-'); compile(p->prim); break;
	case AST_NOT: FPUTC('!'); compile(p->prim); break;

	case AST_ARY:;
		FPRINTF("_brick_make_ary(%d", p->amnt);
		for (int i = 0; i < p->amnt; ++i) {
			FPUTC(',');
			compile(p->args[i]);
		}
		FPUTC(')');
		break;

	case AST_VAR: FPUTS(p->str); break;
	case AST_INT: FPRINTF("%lld", p->num); break;
	case AST_STR:
		FPUTS("(bint) \"");
		for (int i = 0; p->str[i]; ++i) FPRINTF("\\x%02x", p->str[i]);
		FPUTC('"');
		break;
	case AST_TRUE: FPUTS("1"); break;
	case AST_FALSE: FPUTS("0"); break;
	case AST_NULL: FPUTS("0"); break;
	}

	FPUTC(')');
}

void compile_expression(ast_expression *e, compiler *c) {
	FPUTC('(');
	switch (e->kind) {
	case AST_ASSIGN:
		FPRINTF("%s=", e->name);
		compile(e->rhs);
		break;

	case AST_IDX_ASSIGN:
		FPUTS("((bint*)");
		compile(e->prim);
		FPUTS(")[");
		compile(e->index);
		FPUTS("]=");
		compile(e->rhs);
		break;

	case AST_PRIM:
		compile(e->prim);
		break;

	case AST_BINOP:
		FPUTC('(');
		compile(e->prim);
		FPUTC(')');
		switch (e->binop) {
		case TK_ADD: case TK_SUB: case TK_MUL: case TK_DIV: case TK_MOD:
		case TK_NOT: case TK_LTH: case TK_GTH:
			FPUTC(e->binop);
			break;
		case TK_LEQ:
		case TK_GEQ:
		case TK_EQL:
		case TK_NEQ:
			FPUTC(e->binop - 0x10);
			FPUTC('=');
			break;
		default:
			die("bad binop: %d", e->binop);
		}
		compile(e->rhs);
	}
	FPUTC(')');
}

void compile_statement(ast_statement *s, compiler *c) {
	switch (s->kind) {
	case AST_RETURN:
		FPUTS("return ");
		if (s->expr) compile(s->expr);
		FPUTS(";\n");
		break;

	case AST_LOCAL:
		FPRINTF("auto bint %s=", s->varname);
		if (s->expr) compile(s->expr);
		else FPUTC('0');
		FPUTS(";\n");
		break;

	case AST_IF:
		FPUTS("if(");
		compile(s->expr);
		FPUTC(')');
		compile_block(s->body, c, false);
		if (s->else_body) {
			FPUTS("else");
			compile_block(s->else_body, c, false);
		}
		break;

	case AST_WHILE:
		FPUTS("while(");
		compile(s->expr);
		FPUTC(')');
		compile_block(s->body, c, false);
		break;

	case AST_BREAK:
		FPUTS("break;\n");
		break;

	case AST_CONTINUE:
		FPUTS("continue;\n");
		break;

	case AST_EXPR:
		compile(s->expr);
		FPUTS(";\n");
	}
}


void compile_block(ast_block *block, compiler *c, bool add_return_zero) {
	FPUTC('{');

	for (int i = 0; i < block->amnt; ++i)
		compile_statement(block->stmts[i], c);

	if (add_return_zero) FPUTS("return 0;");
	FPUTS("}\n");
}


void init_compiler(compiler *c) {
	FPUTS("\
#include <stdarg.h>\n\
typedef long long bint;\n\
/* these are needed because brick doesnt allow for va fns */\n\
extern int sprintf(bint,bint,...);\n\
extern int printf(bint,...);\n\
extern int fprintf(bint,bint,...);\n\
static bint itoa(bint a){\n\
	extern void*malloc(unsigned long);\n\
	char*c=malloc(45);sprintf((bint)c,(bint)\"%lld\",a);\n\
	return(bint)c;}\n\
static bint strget(bint s,bint i){return ((char*)s)[(int)i];}\n\
static bint strset(bint s,bint i,bint val){return((char*)s)[i]=(char)val;}\n\
extern void*fdopen(int,char*);\n\
static _Noreturn bint die(bint fmt,...){\n\
	extern int fputc(int,void*);\n\
	extern int vfprintf(void*,char*,va_list);\n\
	extern _Noreturn void exit(int);\n\
	va_list ap;va_start(ap,fmt);vfprintf(fdopen(2, \"w\"),fmt,ap);va_end(ap);\n\
	fputc('\\n',fdopen(2,\"w\"));exit(1);}\n\
static bint _brick_make_ary(int len,...){\n\
	extern void*malloc(unsigned long);\n\
	bint *m=malloc(sizeof(bint)*len);\n\
	va_list l;va_start(l,len);for(int i=0;i<len;i++)m[i]=va_arg(l, bint);va_end(l);\n\
	return (bint) m;}\n\n/*user-defined code*/\n\
");
}

void compile_declaration(ast_declaration *d, compiler *c) {
	switch (d->kind) {
	case AST_EXTERN:
	case AST_EXTERNF:
		FPUTS("extern ");
	case AST_GLOBAL:
		FPRINTF("bint %s", d->name);
		if (d->kind == AST_EXTERNF) FPUTS("()");
		FPUTS(";\n");
		break;

	case AST_FUNCTION:
		if (!strcmp(d->name, "main"))
			FPUTS("int main(int __argc, char **__argv) { bint argc=__argc, argv=(bint)__argv;\n");
		else {
			FPRINTF("bint %s(", d->name);
			for (int i = 0; i < d->argc; ++i) {
				if (i != 0) FPUTC(',');
				FPRINTF("bint %s", d->args[i]);
			}
			FPUTC(')');
		}

		compile_block(d->block, c, true);
		if (!strcmp(d->name, "main")) FPUTC('}');
		// do nothing
	}
}
