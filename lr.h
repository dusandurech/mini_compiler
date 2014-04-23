
#ifndef LR_H

#define LR_H

#include "textFile.h"
#include "array.h"

typedef struct
{
	int id;
	int count_nonterm;

	char *str;
	char *name;
	char **array;
	int len;
	int alloc;
	int token;
} rule_t;

typedef struct
{
	rule_t **array;
	int len;
	int alloc;
} list_rule_t;

typedef struct
{
	char **array;
	int len;
	int alloc;

	int x;
	int y;
	char ***table;
} table_t;

typedef struct tree_struct
{
	int id;
	struct tree_struct **array;
	rule_t *rule;
	int len;
	int alloc;
	array_t *array_value;
} tree_t;

typedef struct
{
	list_rule_t list_rule;
	table_t table;
} lr_t;

typedef struct
{
	char **array;
	int count;
	int alloc;
} source_t;

void lr_init(lr_t *lr);
void lr_load(lr_t *lr, char *rule_filename, char *table_filename);
void lr_gen_table(lr_t *lr, char *rule_filename, char *table_filename);
tree_t* lr_parse(lr_t *lr, textFile_t *textFile, array_t *array_token);
void lr_destroy(lr_t *lr);

void source_init(source_t *source);
void source_load(source_t *source, char *filename);
void source_destroy(source_t *source);

void tree_destroy(tree_t *tree);

#endif
