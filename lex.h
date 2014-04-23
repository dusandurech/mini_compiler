
#ifndef LEX_H

#define LEX_H

#include "array.h"
#include "textFile.h"

#define STAT_COUNT		256
#define STAT_MAX_TRANS		4

typedef struct
{
	unsigned char fin;
	int det[STAT_MAX_TRANS];
	int trans[STAT_COUNT][STAT_MAX_TRANS];
} stat_t;

typedef struct
{
	stat_t **stat;
	int fin_count;
	int alloc;
	int count;
	int current;
} automat_t;

typedef struct
{
	char *str;
	char *name;
	int line;
	int offset;
} token_t;

typedef struct
{
	int id;
	char *reg;
	char *name;
} dict_t;

typedef struct
{
	array_t *array_dict;
	int count;

	automat_t automat;
} lex_t;

void lex_init(lex_t *lex);
void lex_load(lex_t *lex, char *filename);
void lex_gen_automat(lex_t *lex, char *filename);
void lex_save_automat(lex_t *lex, char *filename);
void lex_load_automat(lex_t *lex, char *filename);
void lex_print_reg(lex_t *lex);
void lex_print_token(array_t *array);
void lex_destroy_token(array_t *array);
array_t* lex_analyze(lex_t *lex, textFile_t *textFile);
array_t* lex_fake(textFile_t *textFile);
void lex_destroy(lex_t *lex);

#endif
