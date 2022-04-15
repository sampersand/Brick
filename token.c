#include "token.h"
#include "shared.h"

#include <ctype.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

tokenizer new_tokenizer(const char *stream) {
	return (tokenizer) {
		.stream = stream,
		.lineno = 1
	};
}

#define parse_error(tzr, msg, ...) (die(\
	"invalid syntax at %d: " msg, tzr->lineno, __VA_ARGS__))

static char peek(tokenizer *tzr) {
	return tzr->stream[0];
}

static void advance(tokenizer *tzr) {
	if (*tzr->stream++ == '\n')
		++tzr->lineno;
}

static token parse_integer(tokenizer *tzr, bool is_negative) {
	token tkn = { .kind = TK_INT };
	long long num = 0;

	char c;
	for (; isdigit(c = peek(tzr)); advance(tzr))
		num = num*10 + (c - '0');

	if (isalpha(c) || c == '_')
		parse_error(tzr, "bad character '%c' after integer literal", c);

	tkn.num = is_negative ? -num : num;

	return tkn;
}

static token parse_identifier(tokenizer *tzr) {
	const char *start = tzr->stream;

	char c;
	for(; isalnum(c = peek(tzr)) || c == '_'; advance(tzr));

	int len = tzr->stream - start;

	#define CHECK_FOR_KEYWORD(str_, kind_) \
		if (!strncmp(start, str_, strlen(str_))) return (token) {.kind= kind_};
	CHECK_FOR_KEYWORD("true", TK_TRUE)
	CHECK_FOR_KEYWORD("false", TK_FALSE)
	CHECK_FOR_KEYWORD("null", TK_NULL)
	CHECK_FOR_KEYWORD("global", TK_GLOBAL)
	CHECK_FOR_KEYWORD("extern", TK_EXTERN)
	CHECK_FOR_KEYWORD("local", TK_LOCAL)
	CHECK_FOR_KEYWORD("function", TK_FUNCTION)
	CHECK_FOR_KEYWORD("if", TK_IF)
	CHECK_FOR_KEYWORD("else", TK_ELSE)
	CHECK_FOR_KEYWORD("while", TK_WHILE)
	CHECK_FOR_KEYWORD("break", TK_BREAK)
	CHECK_FOR_KEYWORD("continue", TK_CONTINUE)
	CHECK_FOR_KEYWORD("return", TK_RETURN)

	return (token) { .kind=TK_IDENT, .str = strndup(start, tzr->stream - start) };
}

static int parse_hex(tokenizer *tzr, char c) {
	if (isdigit(c)) return c - '0';
	if ('a' <= c && c <= 'f') return c - 'a' + 10;
	if ('A' <= c && c <= 'F') return c - 'F' + 10;
	parse_error(tzr, "unknown hex digit '%c'", c);
}

static token parse_string(tokenizer *tzr) {
	char quote = peek(tzr);
	advance(tzr);

	const char *start = tzr->stream;
	int starting_line = tzr->lineno;
	bool was_anything_escaped = false;

	char c;
	while ((c = peek(tzr)) != quote) {
		if (c == '\0')
			parse_error(tzr, "unterminated quote encountered started on %d", starting_line);

		advance(tzr);

		if (c == '\\')  {
			c = peek(tzr);

			// if (quote == '\"' || (c == '\\' || c == '\'' || c == '\"'))
			was_anything_escaped = true, advance(tzr);
		}
	}

	int length = tzr->stream - start;
	advance(tzr);

	char *str;

	// simple case, just return the original string.
	if (!was_anything_escaped) {
		str = strndup(start, length);
		goto after_escaping;
	}

	// well, something was escaped, so we now need to deal with that.
	str = malloc(length); // note not `+1`, as we're removing at least 1 slash.
	int i = 0, stridx = 0;

	while (i < length) {
		if (start[i] != '\\') {
			str[stridx++] = start[i++];
			continue;
		}

		char c = start[++i];

		if (quote == '\'' && 0) {
			if (c != '\\' && c != '\"' && c != '\'')
				str[stridx++] = '\\';
		} else {
			switch (c) {
			case '\'': case '\"': case '\\': break;
			case 'n': c = '\n'; break;
			case 't': c = '\t'; break;
			case 'r': c = '\r'; break;
			case 'f': c = '\f'; break;
			case '0': c = '\0'; break;
			case 'x':
				i += 3;
				c = (parse_hex(tzr, start[i-2]) << 4) + parse_hex(tzr, start[i-1]);
				break;
			default:
				parse_error(tzr, "unknown escape character '%c'", c);
			}
		}

		str[stridx++] = c;
		i++;
	}

	str[stridx] = '\0';

after_escaping:
	// later addition because i figured having char literals is useful.
	if (quote == '\'') {
		uint64_t n = 0;
		switch (strlen(str)) {
		default:
			die("char literals may only be 8 characters max, not %lu", strlen(str));
		case 8: n = (n << 8) + str[7];
		case 7: n = (n << 8) + str[6];
		case 6: n = (n << 8) + str[5];
		case 5: n = (n << 8) + str[4];
		case 4: n = (n << 8) + str[3];
		case 3: n = (n << 8) + str[2];
		case 2: n = (n << 8) + str[1];
		case 1: n = (n << 8) + str[0];
		case 0:
			return (token) { .kind = TK_INT, .num = n };
		}
	}

	return (token) { .kind = TK_STR, .str = str };
}

token next_token(tokenizer *tzr) {
	char c;

	// Strip whitespace and comments.
	for (; (c = peek(tzr)); advance(tzr)) {
		if (c == '/' && tzr->stream[1] == '/' || c == '#')
			do {
				advance(tzr);
			} while ((c = peek(tzr)) && c != '\n');
		else if (c=='/' && tzr->stream[1] == '*') {
			int n = 0;
			do {
				if (c == '/' && tzr->stream[1] == '*') ++n;
				if (c == '*' && tzr->stream[1] == '/') --n;
				advance(tzr);
			} while (n && (c = peek(tzr)));
			if (n) die("unterminated `/*` commented. (%d)", n);
			continue;
		}

		if (!isspace(c))
			break;
	}

	// For simple tokens, just return them.
	switch (c) {
	case '=': case '!': case '<': case '>':
		if (tzr->stream[1] == '=')
			tzr->stream++, c += 0x10;
		goto normal;

	case '+': case '-': 
		if (isdigit(tzr->stream[1]))
			return advance(tzr), parse_integer(tzr, c == '-');
		// fallthru

	case '(': case ')': case '[': case ']': case '{': case '}':
	case ',': case ';': case '*': case '/': case '%':
	normal:
		advance(tzr);
		// fallthru

	case '\0':
		return (token) { .kind = c };
	}

	// for more complicated ones, defer to their functions.
	if (isdigit(c)) return parse_integer(tzr, false);
	if (isalpha(c) || c == '_') return parse_identifier(tzr);
	if (c == '\'' || c == '\"') return parse_string(tzr);

	parse_error(tzr, "unknown token start: '%c'", c);
}


void dump_token(FILE *out, token tkn) {
	switch(tkn.kind) {
	case TK_EOF: fputs("EOF", out); break;
	case TK_NULL: fputs("Null", out); break;
	case TK_TRUE: fputs("True", out); break;
	case TK_FALSE: fputs("False", out); break;
	case TK_INT: fprintf(out, "Integer(%lld)", tkn.num); break;
	case TK_STR: fprintf(out, "Str(%s)", tkn.str); break;
	case TK_IDENT: fprintf(out, "Ident(%s)", tkn.str); break;
	case TK_GLOBAL: fputs("Token(global)", out); break;
	case TK_FUNCTION: fputs("Token(function)", out); break;
	case TK_EXTERN: fputs("Token(extern)", out); break;
	case TK_LOCAL: fputs("Token(local)", out); break;
	case TK_IF: fputs("Token(if)", out); break;
	case TK_ELSE: fputs("Token(else)", out); break;
	case TK_WHILE: fputs("Token(while)", out); break;
	case TK_BREAK: fputs("Token(break)", out); break;
	case TK_CONTINUE: fputs("Token(continue)", out); break;
	case TK_RETURN: fputs("Token(return)", out); break;
	case TK_LPAREN: fputs("Token[(]", out); break;
	case TK_RPAREN: fputs("Token[)]", out); break;
	case TK_LBRACKET: fputs("Token([)", out); break;
	case TK_RBRACKET: fputs("Token(])", out); break;
	case TK_LBRACE: fputs("Token({)", out); break;
	case TK_RBRACE: fputs("Token(})", out); break;
	case TK_ASSIGN: fputs("Token(=)", out); break;
	case TK_COMMA: fputs("Token(,)", out); break;
	case TK_SEMICOLON: fputs("Token(;)", out); break;
	case TK_ADD: fputs("Token(+)", out); break;
	case TK_SUB: fputs("Token(-)", out); break;
	case TK_MUL: fputs("Token(*)", out); break;
	case TK_DIV: fputs("Token(/)", out); break;
	case TK_MOD: fputs("Token(%%)", out); break;
	case TK_NOT: fputs("Token(!)", out); break;
	case TK_LTH: fputs("Token(<)", out); break;
	case TK_GTH: fputs("Token(>)", out); break;
	case TK_LEQ: fputs("Token(<=)", out); break;
	case TK_GEQ: fputs("Token(>=)", out); break;
	case TK_EQL: fputs("Token(==)", out); break;
	case TK_NEQ: fputs("Token(!=)", out); break;
	default: fprintf(out, "Token(<%d>)", tkn.kind); break;
	}
}
