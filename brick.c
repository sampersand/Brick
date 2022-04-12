#include "token.h"
#include "compile.h"
#include <errno.h>
#include <string.h>
#include "ast.h"
#include "env.h"
#include "shared.h"
#include <unistd.h>

char *read_file(const char *filename) {
	char* buf = 0;
	size_t n = 0;
	// lol imagine checking for errors
	FILE* fp = fopen(filename, "r");
	getdelim(&buf,&n,EOF,fp);
	fclose(fp);
	return buf;
}

void run_declaration(ast_declaration*, env*);

void run(tokenizer *tzr) {
	static env e;
	ast_declaration *d;
	while ((d = next_declaration(tzr)))
		run_declaration(d, &e);

	value v;
	if ((v = lookup_var(&e, "main")) == VUNDEF)
		die("you must define a `main` function");
	call_value(v, 0, 0, &e);
}

void compile(char *in, char *out, int use_out) {
	static compiler c;
	tokenizer tzr = new_tokenizer(in);

	char *tmpfile = "_tmpout.c";
	if (!(c.out = fopen(tmpfile, "w"))) die("unable to create out file: %s", out);
	init_compiler(&c);

	ast_declaration *d;
	while ((d = next_declaration(&tzr)))
		compile_declaration(d, &c);

	if (fclose(c.out))
		die("unable to close temp file: '%s': %s\n", tmpfile, strerror(errno));
	
	int child, _ignore;
	if (!(child = fork()))
		// for some reason `gcc -w` doesnt work so lets just close stderr instead.
		execlp("gcc", "gcc",
			"-Wno-parentheses-equality",
			"-Wno-incompatible-library-redeclaration",
			"-Wno-builtin-requires-header",
			"-Wno-int-conversion",
			"-xc", tmpfile, "-o", out, use_out ? "-c" : 0, 0);
	waitpid(child, &_ignore, 0);
	// remove(tmpfile);
}

int main(int argc, char **argv) {
	char *contents, *outfile;

	if (argc != 2 && argc != 3 && argc != 4)
	usage:
		die("usage: %s (file [outfile=a.out] [-c]", argv[0]);

	compile(read_file(argv[1]), argc > 2 ? argv[2] : "a.out", argc == 4);
}
