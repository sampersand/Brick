#pragma once
#include "value.h"

#ifndef STACKFRAME_LIMIT
#define STACKFRAME_LIMIT 10000
#endif

typedef struct {
	int cap, len;
	struct entry { const char *name; value v; } *entries;
} map;

typedef struct env {
	int sp;
	map globals, stackframes[STACKFRAME_LIMIT];
} env;

value lookup_var(env *, const char *);
void assign_var(env *, const char *, value);
void declare_local(env *, const char *, value);
void declare_global(env *, const char *, value);
