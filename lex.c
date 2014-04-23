
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lex.h"
#include "array.h"
#include "main.h"

#define RESIZE_ALIGMENT		8

#define AUTOMAT_START_CHAR	' '
#define AUTOMAT_STOP_CHAR	'~'

static stat_t* stat_new()
{
	stat_t *stat;
	int i;
	int j;

	stat = (stat_t *) malloc( sizeof(stat_t) );

	stat->fin = 0;

	for(i = 0; i < STAT_MAX_TRANS; i++)
	{
		stat->det[i] = -1;
	}

	for(i = 0; i < STAT_COUNT; i++)
	{
		for(j = 0; j < STAT_MAX_TRANS; j++)
		{
			stat->trans[i][j] = -1;
		}
	}

	return stat;
}

static void stat_destroy(stat_t *stat)
{
	free(stat);
}

static void stat_clean_trans(stat_t *stat, int n)
{
	int i;

	for(i = 0; i < STAT_MAX_TRANS; i++)
	{
		stat->trans[n][i] = -1;
	}
}

static void stat_set_fin(stat_t *stat, int fin)
{
	stat->fin = fin;
}

static int stat_get_fin(stat_t *stat)
{
	return stat->fin;
}

static void stat_add_det(stat_t *stat, int next_stat)
{
	int i;

	for(i = 0; i < STAT_MAX_TRANS; i++)
	{
		if( stat->det[i] == next_stat )
		{
			return;
		}

		if( stat->det[i] == -1 )
		{
			stat->det[i] = next_stat;
			return;
		}
	}

	printf("det is full\n");
}

int rec = 0;

static void stat_add_trans(stat_t *stat, char trans, int next_stat)
{
	int i;

	for(i = 0; i < STAT_MAX_TRANS; i++)
	{
		int n = (unsigned char) trans;

		if( stat->trans[n][i] == next_stat )
		{
			return;
		}

		if( stat->trans[n][i] == -1 )
		{
			if( i > rec ) rec = i;
			stat->trans[n][i] = next_stat;
			return;
		}
	}

	printf("trans is full\n");
}

static void stat_add_interval_trans(stat_t *stat, char trans_a, char trans_b, int next_stat)
{
	int i;

	for(i = trans_a; i <= trans_b; i++)
	{
		stat_add_trans(stat, i, next_stat);
	}
}

static void* resize(void *p, int n, int *count, int *alloc)
{
	if( *count+1 > *alloc )
	{
		(*alloc) += RESIZE_ALIGMENT;
		p = realloc(p, (*alloc) * n);
	}

	return p;
}

static void automat_init(automat_t *automat)
{
	automat->stat = NULL;
	automat->alloc = 0;
	automat->count = 0;
	automat->current = 0;
	automat->fin_count = 0;
}

static void automat_reset(automat_t *automat)
{
	automat->current = 0;
}

static int automat_step(automat_t *automat, char c)
{
	if( automat->current != -1 )
	{
		stat_t *stat;
		int next;

		stat = automat->stat[automat->current];
		next = stat->trans[c][0];
		automat->current = next;
	}

	return automat->current;
}

static int automat_is_fin(automat_t *automat)
{
	if( automat->current != -1 )
	{
		stat_t *stat;

		stat = automat->stat[automat->current];
		return stat_get_fin(stat);
	}

	return 0;
}

static int automat_get_count_stat(automat_t *automat)
{
	return automat->count;
}

static void automat_add_stat(automat_t *automat, stat_t *stat)
{
	automat->stat = resize(automat->stat, sizeof(stat_t *), &automat->count, &automat->alloc);
	automat->stat[automat->count] = stat;
	automat->count++;
}

static stat_t* automat_get_stat(automat_t *automat, int stat)
{
	if( stat < 0 || stat >= automat->count )
	{
		return 0;
	}

	return automat->stat[stat];
}

static stat_t* automat_last_stat(automat_t *automat)
{
	return automat_get_stat(automat, automat->count-1);
}

static void automat_del_last_stat(automat_t *automat)
{
	stat_t *stat;

	stat = automat_last_stat(automat);
	stat_destroy(stat);

	automat->stat[automat->count-1] = NULL;
	automat->count--;
}

static int stat_regexp(automat_t *automat, char *str_regexp)
{
	stat_t *stat;
	int len;
	int i;

	len = strlen(str_regexp);

	if( automat_get_count_stat(automat) == 0 )
	{
		stat = stat_new();
		automat_add_stat(automat, stat);
	}

	for(i = 0; i < len; i++)
	{
		int j;
		char c;
		int count;
		c = str_regexp[i];

 		//printf("parse %c\n", c);
		count = automat_get_count_stat(automat);
	
		switch( c )
		{
			case '?' :
				if( i == 0 )
				{
					stat = automat_get_stat(automat, 0);
				}
				else
				{
					stat = automat_last_stat(automat);
				}

				for(j = AUTOMAT_START_CHAR; j < AUTOMAT_STOP_CHAR; j++)
				{
					stat_add_trans(stat, j, count);
				}

				automat_add_stat(automat, stat_new() );
			break;

			case '*' :
				if( i == 0 )
				{
					stat = automat_get_stat(automat, 0);
				}
				else
				{
					stat = automat_last_stat(automat);
				}

				for(j = AUTOMAT_START_CHAR; j < AUTOMAT_STOP_CHAR; j++)
				{
					stat_add_trans(stat, j, count-1);
				}
			break;

			case '+' :
				if( automat->count >= 1 )
				{
					stat_t *stat2 = automat_get_stat(automat, automat->count-2);
					stat = automat_get_stat(automat, automat->count-1);
	
					for(j = AUTOMAT_START_CHAR; j < AUTOMAT_STOP_CHAR; j++)
					{
						if( stat2->trans[j][0] != -1 )
						{
							stat_add_trans(stat, j, count-1);
						}
					}
				}
			break;

			case '.' :
				if( automat->count >= 2 )
				{
					stat = automat_get_stat(automat, automat->count-2);
	
					for(j = AUTOMAT_START_CHAR; j < AUTOMAT_STOP_CHAR; j++)
					{
						int k;

						for(k = 0; k < STAT_MAX_TRANS; k++)
						{
							if( stat->trans[j][k] != -1 )
							{
								stat->trans[j][k] = count-2;
							}
						}
					}

					automat_del_last_stat(automat);
				}
			break;

			case '[' :
			{
				char mask[256];
				char neg;
				int j;

				memset(mask, 0, 256);

				if( i == 0 )
				{
					stat = automat_get_stat(automat, 0);
				}
				else
				{
					stat = automat_last_stat(automat);
				}

				i++;
				neg = str_regexp[i];

				if( neg != '!' )
				{
					neg = '\0';
				}
				else
				{
					i++;
				}

				//printf("neg = %c\n", neg);

				while( str_regexp[i] != ']' )
				{
					if( str_regexp[i+1] == '-' )
					{
						char a = str_regexp[i];
						char b = str_regexp[i+2];
						char c;

						//printf("%c..%c\n", a, b);
	
						for(c = a; c <= b; c++)
						{
							mask[c] = 1;
						}

						i += 3;
					}
					else
					{
						char c;

						c = str_regexp[i];
						mask[c] = 1;
						//printf("%c\n", str_regexp[i]);
						i++;
					}
				}

				for(j = AUTOMAT_START_CHAR; j < AUTOMAT_STOP_CHAR; j++)
				{
					if( neg == '!' )
					{
						mask[j] = ! mask[j];
					}

					if( mask[j] )
					{
						stat_add_trans(stat, j, count);
					}
				}

				automat_add_stat(automat, stat_new() );
			}
			break;

			case '\\' :
				if( i == 0 )
				{
					stat = automat_get_stat(automat, 0);
				}
				else
				{
					stat = automat_last_stat(automat);
				}

				i++;
				c = str_regexp[i];
				stat_add_trans(stat, c, count);
				automat_add_stat(automat, stat_new() );
			break;

			default :
				if( i == 0 )
				{
					stat = automat_get_stat(automat, 0);
				}
				else
				{
					stat = automat_last_stat(automat);
				}

				stat_add_trans(stat, c, count);
				automat_add_stat(automat, stat_new() );
			break;
		}
	}

	automat->fin_count++;
	stat = automat_last_stat(automat);
	stat_set_fin(stat, automat->fin_count);

	return automat->fin_count;
}

static int find_euals_stat(automat_t *automat, stat_t *arg_stat, int arg_trans)
{
	int det[STAT_MAX_TRANS];
	int count;
	int i;

	count = 0;

	for(i = 0; i < STAT_MAX_TRANS; i++)
	{
		det[i] = -1;
	}

	for(i = 0; i < STAT_MAX_TRANS; i++)
	{
		if( arg_stat->trans[arg_trans][i] != -1 )
		{
			det[count] = arg_stat->trans[arg_trans][i];
			count++;
		}
	}

	for(i = 0; i < automat->count; i++)
	{
		stat_t *stat = automat->stat[i];

		if( memcmp(stat->det, det, STAT_MAX_TRANS*sizeof(int)) == 0 )
		{
			return i;
		}
	}

	return -1;
}

static void determinate_state(automat_t *automat, stat_t *arg_stat, int stat_num, int arg_trans)
{
	stat_t *stat_alternative;
	int stat_n;
	int count;
	int i;

	stat_alternative = stat_new();
	automat_add_stat(automat, stat_alternative );

	count = 0;

	for(i = 0; i < STAT_MAX_TRANS; i++)
	{
		if( arg_stat->trans[arg_trans][i] != -1 )
		{
			stat_add_det(stat_alternative, arg_stat->trans[arg_trans][i]);
		}
	}

	for(i = 0; i < STAT_MAX_TRANS; i++)
	{
		if( stat_alternative->det[i] != -1 )
		{
			stat_t *stat;

			stat = automat_get_stat(automat, stat_alternative->det[i]);
		
			if( stat_get_fin(stat) )
			{
				stat_set_fin(stat_alternative, stat_get_fin(stat));
			}
		}
	}

	for(i = 0; i < STAT_MAX_TRANS; i++)
	{
		if( stat_alternative->det[i] != -1 )
		{
			stat_t *stat;
			int j;

			stat = automat_get_stat(automat, stat_alternative->det[i]);

			for(j = 0; j < 256; j++)
			{
				int k;

				for(k = 0; k < STAT_MAX_TRANS; k++)
				{
					if( stat->trans[j][k] != -1 )
					{
						stat_add_trans(stat_alternative, j, stat->trans[j][k]);
					}
				}
			}

		}
	}
}

static void automat_reconfigure(automat_t *automat)
{
	int i;
	int n;

	n = automat->count;

	for(i = 0; i < automat->count; i++)
	{
		stat_t *stat = automat->stat[i];
		int j;

		for(j = 0; j < 256; j++)
		{
			int k;
			int n;

			n = 0;

			for(k = 0; k < STAT_MAX_TRANS; k++)
			{
				if( stat->trans[j][k] != -1 )
				{
					n++;
				}
			}

			if( n > 1 && find_euals_stat(automat, stat, j) == -1 )
			{
				determinate_state(automat, stat, i, j);
			}
		}
	}

	for(i = 0; i < automat->count; i++)
	{
		stat_t *stat = automat->stat[i];
		int j;

		for(j = 0; j < 256; j++)
		{
			int k;
			int n;
			int m;

			n = 0;

			for(k = 0; k < STAT_MAX_TRANS; k++)
			{
				if( stat->trans[j][k] != -1 )
				{
					n++;
				}
			}

			m = find_euals_stat(automat, stat, j);

			if( n > 1 && m != -1 )
			{
				stat_clean_trans(stat, j);
				stat_add_trans(stat, j, m);
			}
		}
	}
}

void automat_save(automat_t *automat, char *filename)
{
	FILE *file;
	int i;

	file = fopen(filename, "wt");

	for(i = 0; i < automat->count; i++)
	{
		stat_t *stat = automat->stat[i];
		int isPrint;
		int j;

		isPrint = 0;

		for(j = 0; j < 256; j++)
		{
			int n;

			n = 0;

			while( stat->trans[j+n][0] != -1 && stat->trans[j+n][0] == stat->trans[j][0] )
			{
				n++;
			}

			if( n <= 1 && stat->trans[j][0] != -1 )
			{
				fprintf(file, "%d %d %d %d %d\n", i, stat->trans[j][0], (int)j, (int)j, stat_get_fin(stat));
				isPrint = 1;
			}
			else if( n > 1 )
			{
				fprintf(file, "%d %d %d %d %d\n", i, stat->trans[j][0], (int)j, (int)(j+n-1), stat_get_fin(stat));
				j += n-1;
				isPrint = 1;
			}
		}

		if( ! isPrint )
		{
			fprintf(file, "%d -1 -1 -1 %d\n", i, stat_get_fin(stat));
		}
	}

	fclose(file);
}

void automat_load(automat_t *automat, char *filename)
{
	FILE *file;
	char line[STR_LINE_SIZE];

	file = fopen(filename, "rt");

	while( fgets(line, STR_LINE_SIZE-1, file) != NULL )
	{
		stat_t *stat;
		int num_stat;
		int trans;
		int a;
		int b;
		int fin;

		sscanf(line, "%d %d %d %d %d", &num_stat, &trans, &a, &b, &fin);

		stat = automat_get_stat(automat, num_stat);

		if( stat == NULL )
		{
			stat = stat_new();
			automat_add_stat(automat, stat);
		}

		stat_add_interval_trans(stat, a, b, trans);
		stat_set_fin(stat, fin);
	}

	fclose(file);
}

static void automat_print_graphviz(automat_t *automat)
{
	int i;

	printf("digraph G\n{\n");

	for(i = 0; i < automat->count; i++)
	{
		stat_t *stat = automat->stat[i];
		int j;

		if( stat_get_fin(stat) )
		{
			printf("\t\"%d\" [style=filled, fillcolor=\"red\"];\n", i);
		}

		for(j = 0; j < 256; j++)
		{
			int k;

			for(k = 0; k < STAT_MAX_TRANS; k++)
			{
				if( stat->trans[j][k] != -1 )
				{
					printf("\t\"%d\" -> \"%d\" [label=\"%c\"];\n", i, stat->trans[j][k], (char)j);
				}
			}
		}
	}

	printf("}\n");
}

static void automat_print(automat_t *automat)
{
	int i;

	printf("\t\t");

	for(i = 'a'; i < 'f'; i++)
	{
		printf("%c\t", i);
	}

	printf("\n");

	for(i = 0; i < automat->count; i++)
	{
		stat_t *stat = automat->stat[i];
		int j;

		printf("%d|\t", i);

		if( stat->det[0] == -1 )
		{
			printf("{%d}\t", i);
		}
		else
		{
			printf("{");

			for(j = 0; j < STAT_MAX_TRANS; j++)
			{
				if( stat->det[j] != -1 )
				{
					printf("%d ", stat->det[j]);
				}
			}

			printf("}\t");
		}


		for(j = 'a'; j < 'f'; j++)
		{
			int k;

			for(k = 0; k < STAT_MAX_TRANS; k++)
			{
				if( stat->trans[j][k] != -1 )
				{
					printf("%d ", stat->trans[j][k]);
				}
			}

			printf("\t");
		}

		if( stat_get_fin(stat) )
		{
			printf("\tfin");
		}

		printf("\n");
	}
}

static int automat_run(automat_t *automat, char *str)
{
	int len;
	stat_t *stat;
	int state;
	int next;
	int i;

	len = strlen(str);
	state = 0;

	for(i = 0; i < len; i++)
	{
		char c;

		c = str[i];
		stat = automat->stat[state];

		next = stat->trans[c][0];

		if( next == -1 )
		{
			return 0;
		}

		state = next;
	}

	stat = automat->stat[state];

	if( stat_get_fin(stat) )
	{
		return stat_get_fin(stat);
	}

	return 0;
}

static void automat_destroy(automat_t *automat)
{
	int i;

	for(i = 0; i < automat->count; i++)
	{
		stat_t *stat = automat->stat[i];
		stat_destroy(stat);
	}

	free(automat->stat);
}

static token_t* token_new()
{
	token_t *token;

	token = (token_t *) malloc( sizeof(token_t) );
	memset(token, 0, sizeof(token_t));

	return token;
}

static void token_print(token_t *token)
{
	printf("%s\t%s\t%d\t%d\n", token->str, token->name, token->line, token->offset);
}

static void token_destroy(token_t *token)
{
	if( token->str != NULL )
	{
		free(token->str);
	}

	if( token->name != NULL )
	{
		free(token->name);
	}

	free(token);
}

static dict_t* dict_new()
{
	dict_t *dict;

	dict = (dict_t *) malloc( sizeof(dict_t) );
	memset(dict, 0, sizeof(dict_t));

	return dict;
}

static void dict_print(dict_t *dict)
{
	printf("%s %s %d\n", dict->reg, dict->name, dict->id);
}

static void dict_destroy(dict_t *dict)
{
	if( dict->name != NULL )
	{
		free(dict->name);
	}

	if( dict->reg != NULL )
	{
		free(dict->reg);
	}

	free(dict);
}

void lex_init(lex_t *lex)
{
	//automat_t pokus;

	memset(lex, 0, sizeof(lex_t));

	automat_init(&lex->automat);
	lex->array_dict = array_new();

	//automat_init(&pokus);
	//stat_regexp(&pokus, "\\!=");
	//automat_reconfigure(&pokus);
	//automat_print_graphviz(&pokus);
}

void lex_gen_automat(lex_t *lex, char *filename)
{
	FILE *file;
	char line[STR_LINE_SIZE];
	char reg[STR_LINE_SIZE];
	char name[STR_LINE_SIZE];

	file = fopen(filename, "rt");

	while( fgets(line, STR_LINE_SIZE-1, file) != NULL )
	{
		dict_t *dict;
		int res;

		sscanf(line, "%s %s", reg, name);
		res = stat_regexp(&lex->automat, reg);

		dict = dict_new();
		dict->id = res;
		dict->reg = strdup(reg);
		dict->name = strdup(name);

		array_add(lex->array_dict, dict);
	}

	fclose(file);

	automat_reconfigure(&lex->automat);
	//automat_print_graphviz(&lex->automat);
}

void lex_load(lex_t *lex, char *filename)
{
	FILE *file;
	char line[STR_LINE_SIZE];
	char reg[STR_LINE_SIZE];
	char name[STR_LINE_SIZE];
	int count;

	file = fopen(filename, "rt");
	count = 1;

	while( fgets(line, STR_LINE_SIZE-1, file) != NULL )
	{
		dict_t *dict;

		sscanf(line, "%s %s", reg, name);

		dict = dict_new();
		dict->id = count;
		dict->reg = strdup(reg);
		dict->name = strdup(name);

		count++;
		array_add(lex->array_dict, dict);
	}

	fclose(file);
}

void lex_print_reg(lex_t *lex)
{
	int i;

	for(i = 0; i < lex->array_dict->count; i++)
	{
		dict_t *dict = (dict_t *) array_get(lex->array_dict, i);
		dict_print(dict);
	}
}

void lex_print_token(array_t *array)
{
	int i;

	printf("token\tname\tline\toffset\n");

	for(i = 0; i < array->count; i++)
	{
		token_t *token = (token_t *) array_get(array, i);
		token_print(token);
	}
}

void lex_save_automat(lex_t *lex, char *filename)
{
	automat_save(&lex->automat, filename);
}

void lex_load_automat(lex_t *lex, char *filename)
{
	automat_load(&lex->automat, filename);
}

void lex_destroy_token(array_t *array)
{
	array_destroy_item(array, token_destroy);
}

static char* lex_get_name(lex_t *lex, int id)
{
	int i;

	for(i = 0; i < lex->array_dict->count; i++)
	{
		dict_t *dict = (dict_t *) array_get(lex->array_dict, i);

		if( dict->id == id )
		{
			return dict->name;
		}
	}

	return NULL;
}

static int sub_str(char *str, char *dst, int offset, int len)
{
	int i;

	for(i = offset; i < len; i++)
	{
		dst[i - offset] = str[i];
	}

	dst[i - offset] = '\0';

	return 0;
}

static int lex_analyze_line(lex_t *lex, array_t *array, int line, char *str)
{
	int offset;
	int len;
	int fin;
	int i;

	len = strlen(str);
	fin = 0;
	offset = 0;

	automat_reset(&lex->automat);

	//printf("line = >%s<\n", str);

	for(i = 0; i <= len; i++)
	{
		int res;
		char c;

		c = str[i];
		automat_step(&lex->automat, c);
		res = automat_is_fin(&lex->automat);

		//printf("%c -> %d %d\n", c, res, fin);

		if( res == 0 )
		{
			if( fin != 0 )
			{
				char str_token[STR_SIZE];
				token_t *token;

				token = token_new();

				sub_str(str, str_token, offset, i);

				token->str = strdup(str_token);
				token->name = strdup(lex_get_name(lex, fin));
				token->offset = offset;
				token->line = line;

				array_add(array, token);
				i--;

				//printf("str_token = %s\n", str_token);
			}

			automat_reset(&lex->automat);
			fin = 0;
			offset = i+1;
		}

		fin = res;
	}

	return -1;
}

array_t* lex_analyze(lex_t *lex, textFile_t *textFile)
{
	array_t *array;
	int count;
	int i;

	count = 1;
	array = array_new();

	for(i = 0; i < textFile->array->count; i++)
	{
		char *str = (char *) array_get(textFile->array, i);
		lex_analyze_line(lex, array, count, str);
		count++;
	}

	for(i = 0; i < 2; i++)
	{
		token_t *token;

		token = token_new();

		token->str = strdup("$");
		token->name = strdup("eof");
		token->offset = -1;
		token->line = -1;

		array_add(array, token);
	}

	return array;
}

array_t* lex_fake(textFile_t *textFile)
{
	array_t *array;
	int count;
	int i;

	count = 1;

	array = array_new();

	for(i = 0; i < textFile->array->count; i++)
	{
		char *str = (char *) array_get(textFile->array, i);
		char *s;
		int n;

		s = strtok(str, " \t\n");
		n = 0;

		while( s != NULL )
		{
			token_t *token;
	
			token = token_new();
	
			token->str = strdup(s);
			token->name = strdup("token");
			token->offset = 0;
			token->line = 0;
	
			array_add(array, token);

			s = strtok(NULL, " \n");
		}

		count++;
	}

	for(i = 0; i < 2; i++)
	{
		token_t *token;

		token = token_new();

		token->str = strdup("$");
		token->name = strdup("eof");
		token->offset = 0;
		token->line = 0;

		array_add(array, token);
	}
}

void lex_destroy(lex_t *lex)
{
	int i;

	printf("rec = %d\n", rec);

	array_destroy_item(lex->array_dict, dict_destroy);
	automat_destroy(&lex->automat);
}

