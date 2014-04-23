
#ifndef COMPILER_H

#define COMPILER_H

#include "array.h"
#include "main.h"

#define STACK_MAX		256

typedef struct
{
	int top;
	int stack[STACK_MAX];
} stack_t;

typedef struct
{
	int is_cycle_set;
	char startcycle[STR_SIZE];
	char stopcycle[STR_SIZE];

	stack_t stack;
	array_t *array_out;
	int label_id;
	int var_id;
} compiler_t;

compiler_t* compiler_new();
void compiler_print(compiler_t *compiler);
int compiler_run(compiler_t *compiler, tree_t *tree);
void compiler_add_line(compiler_t *compiler, char *line);
void compiler_save(compiler_t *compiler, char *filename);
void compiler_destroy(compiler_t *compiler);

#endif
