#pragma once
#include <stdio.h>
#include <stdlib.h>

#define die(...) (fprintf(stderr, __VA_ARGS__), fputc('\n', stderr), exit(1))
