
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "main.h"
#include "array.h"
#include "lex.h"
#include "lr.h"

#define RESIZE_ALIGMENT		8

static void* resize(void *p, int n, int *count, int *alloc)
{
	if( *count+1 > *alloc )
	{
		(*alloc) += RESIZE_ALIGMENT;
		p = realloc(p, (*alloc) * n);
	}

	return p;
}

rule_t* rule_new(char *name)
{
	static int id = 0;
	rule_t *rule;

	rule = (rule_t *) malloc( sizeof(rule_t) );
	memset(rule, 0, sizeof(rule_t));

	rule->id = id++;
	rule->name = strdup(name);

	return rule;
}

void rule_append(rule_t *rule, char *name)
{
	rule->array = resize(rule->array, sizeof(char *), &rule->len, &rule->alloc);
	rule->array[rule->len++] = strdup(name);
}

rule_t* rule_clone(rule_t *rule)
{
	rule_t *clone;
	int i;

	clone = (rule_t *) malloc( sizeof(rule_t) );
	memset(clone, 0, sizeof(rule_t));

	clone->name = strdup(rule->name);
	clone->token = rule->token;
	clone->id = rule->id;

	for(i = 0; i < rule->len; i++)
	{
		rule_append(clone, rule->array[i]);
	}

	return clone;
}

int rule_cmp(rule_t *rule_a, rule_t *rule_b)
{
	rule_t *clone;
	int i;

	if( rule_a->len != rule_b->len ||
	    rule_a->token != rule_b->token ||
	    rule_a->id != rule_b->id )
	{
		return 0;
	}

	return 1;
}

void rule_print(rule_t *rule)
{
	int i;

	printf("%s -> ", rule->name);

	for(i = 0; i < rule->len; i++)
	{
		if( rule->token == i )
		{
			printf(" # ");
		}

		printf("%s ", rule->array[i]);
	}

	if( rule->token == i )
	{
		printf(" # ");
	}

	putchar('\n');
}

void rule_destroy(rule_t *rule)
{
	free(rule->name);

	if( rule->str != NULL )
	{
		free(rule->str);
	}

	if( rule->array != NULL )
	{
		int i;

		for(i = 0; i < rule->len; i++)
		{
			free(rule->array[i]);
		}

		free(rule->array);
	}

	free(rule);
}

void list_rule_init(list_rule_t *list_rule)
{
	memset(list_rule, 0, sizeof(list_rule_t));
}

void list_rule_add_rule(list_rule_t *list_rule, rule_t *rule)
{
	list_rule->array = resize(list_rule->array, sizeof(rule_t *), &list_rule->len, &list_rule->alloc);
	list_rule->array[list_rule->len++] = rule;
}

void list_rule_print(list_rule_t *list_rule)
{
	int i;

	for(i = 0; i < list_rule->len; i++)
	{
		rule_t *rule = list_rule->array[i];

		rule_print(rule);
	}
}

int list_rule_is_nonterm(list_rule_t *list_rule, char *name)
{
	int i;

	for(i = 0; i < list_rule->len; i++)
	{
		rule_t *rule = list_rule->array[i];

		if( strcmp(rule->name, name) == 0 )
		{
			return 1;
		}
	}

	return 0;
}

void list_rule_work(list_rule_t *list_rule)
{
	int i;

	for(i = 0; i < list_rule->len; i++)
	{
		rule_t *rule = list_rule->array[i];
		int j;

		for(j = 0; j < rule->len; j++)
		{
			if( list_rule_is_nonterm(list_rule, rule->array[j]) )
			{
				rule->count_nonterm++;
			}
		}
	}
}

rule_t* list_rule_get(list_rule_t *list_rule, char *name)
{
	int i;

	for(i = 0; i < list_rule->len; i++)
	{
		rule_t *rule = list_rule->array[i];

		if( strcmp(rule->name, name) == 0 )
		{
			return rule;
		}
	}

	return NULL;
}

char* list_rule_get_main_nonterm(list_rule_t *list_rule)
{
	rule_t *rule = list_rule->array[0];
	return rule->name;
}

void list_rule_destroy(list_rule_t *list_rule)
{
	int i;

	for(i = 0; i < list_rule->len; i++)
	{
		rule_t *rule = list_rule->array[i];

		rule_destroy(rule);
	}

	free(list_rule->array);
}

void load_rule(char *filename, list_rule_t *list_rule)
{
	FILE *file;
	char orig_line[STR_LINE_SIZE];
	char line[STR_LINE_SIZE];
	rule_t *rule;

	file = fopen(filename, "rt");

	while( fgets(line, STR_LINE_SIZE-1, file) != NULL )
	{
		char *s;
		int n;

		strcpy(orig_line, line);

		s = strstr(orig_line, "\n");

		if( s != NULL )
		{
			*s = '\0';
		}

		if( strstr(line, "->") == NULL )
		{
			continue;
		}

		s = strtok(line, " \n");
		n = 0;

		while( s != NULL )
		{
			if( n == 0 )
			{
				rule = rule_new(s);
				rule->str = strdup(orig_line);

			}
			else if( n != 1 )
			{
				rule_append(rule, s);
			}

			s = strtok(NULL, " \n");
			n++;
		}

		list_rule_add_rule(list_rule, rule);
		rule_print(rule);
	}

	fclose(file);
}

typedef struct state_struct
{
	rule_t **array_rule;
	int len_rule;
	int alloc_rule;

	struct state_struct **next_state;
	char **next_token;

	int len_token_next;
	int alloc_token_next;

	int len_state_next;
	int alloc_state_next;
} state_t;

state_t* state_new()
{
	state_t *state;

	state = (state_t *) malloc( sizeof(state_t) );

	memset(state, 0, sizeof(state_t));

	return state;
}

void state_print(state_t *state)
{
	int i;
	
	printf("----------------\n");
	printf("state = %p\n", state);

	for(i = 0; i < state->len_rule; i++)
	{
		rule_t *rule;

		rule = (rule_t *) state->array_rule[i];
		rule_print(rule);
	}

	for(i = 0; i < state->len_token_next; i++)
	{
		printf("next %s -> %p\n", state->next_token[i], state->next_state[i]);
	}
}

int state_is_next_token(state_t *state, char *str_token)
{
	int i;

	for(i = 0; i < state->len_token_next; i++)
	{
		if( strcmp(state->next_token[i], str_token) == 0 )
		{
			return 1;
		}
	}

	return 0;
}

state_t* state_get_next_state(state_t *state, char *str_token)
{
	int i;

	for(i = 0; i < state->len_token_next; i++)
	{
		if( strcmp(state->next_token[i], str_token) == 0 )
		{
			return state->next_state[i];
		}
	}

	return NULL;
}

int state_add_next_state(state_t *state, char *str_token, state_t *state_n)
{
	if( state_is_next_token(state, str_token) == 0 )
	{
		state->next_token = resize(state->next_token, sizeof(char *), &state->len_token_next, &state->alloc_token_next);
		state->next_token[state->len_token_next++] = strdup(str_token);

		state->next_state = resize(state->next_state, sizeof(state_t *), &state->len_state_next, &state->alloc_state_next);
		state->next_state[state->len_state_next++] = state_n;

		return 1;
	}

	return 0;
}

int state_is_in_rule(state_t *state, rule_t *rule)
{
	int i;

	for(i = 0; i < state->len_rule; i++)
	{
		rule_t *rule_n;

		rule_n = (rule_t *) state->array_rule[i];

		if( rule_cmp(rule, rule_n) )
		{
			return 1;
		}
	}

	return 0;
}

int state_cmp(state_t *state_a, state_t *state_b)
{
	int i;
	
	if( state_a->len_rule != state_b->len_rule )
	{
		return 0;
	}

	for(i = 0; i < state_a->len_rule; i++)
	{
		rule_t *rule;

		rule = (rule_t *) state_a->array_rule[i];
		
		if( state_is_in_rule(state_b, rule) == 0 )
		{
			return 0;
		}
	}

	return 1;
}

void state_add_rule(state_t *state, list_rule_t *list_rule, rule_t *rule)
{
	if( state_is_in_rule(state, rule) )
	{
		return;
	}

	state->array_rule = resize(state->array_rule, sizeof(rule_t *), &state->len_rule, &state->alloc_rule);
	state->array_rule[state->len_rule++] = rule;

	if( rule->token < rule->len )
	{
		char *main_token;

		main_token = rule->array[rule->token];
	
		if( list_rule_is_nonterm(list_rule, main_token) )
		{
			int i;
		
			for(i = 0; i < list_rule->len; i++)
			{
				rule_t *rule_n = list_rule->array[i];
		
				if( strcmp(rule_n->name, main_token) == 0 && state_is_in_rule(state, rule_n) == 0 )
				{
					rule_t *clone;

					clone = rule_clone(rule_n);
					state_add_rule(state, list_rule, clone);
				}
			}
		}
	}
}

void state_destroy(state_t *state)
{
	int i;

	for(i = 0; i < state->len_rule; i++)
	{
		rule_t *rule;

		rule = (rule_t *) state->array_rule[i];
		rule_destroy(rule);
	}

	if( state->array_rule != NULL )
	{
		free(state->array_rule);
	}

	for(i = 0; i < state->len_token_next; i++)
	{
		free(state->next_token[i]);
	}

	if( state->next_token != NULL )
	{
		free(state->next_token);
	}

	if( state->next_state != NULL )
	{
		free(state->next_state);
	}

	free(state);
}

typedef struct
{
	state_t **array;
	int len;
	int alloc;
} lr_automat_t;

void automat_init(lr_automat_t *automat)
{
	memset(automat, 0, sizeof(lr_automat_t));
}

void automat_print(lr_automat_t *automat)
{
	int i;

	for(i = 0; i < automat->len; i++)
	{
		state_t *state;

		state = automat->array[i];
		state_print(state);
	}

	printf("automat state %d\n", automat->len);
}

int automat_is_exists_state(lr_automat_t *automat, state_t *state)
{
	int i;

	for(i = 0; i < automat->len; i++)
	{
		state_t *state_n;

		state_n = automat->array[i];

		if( state_cmp(state_n, state) )
		{
			return 1;
		}
	}

	return 0;
}

state_t* automat_get_exists_state(lr_automat_t *automat, state_t *state)
{
	int i;

	for(i = 0; i < automat->len; i++)
	{
		state_t *state_n;

		state_n = automat->array[i];

		if( state_cmp(state_n, state) )
		{
			return state_n;
		}
	}

	return NULL;
}

int automat_get_numer_state(lr_automat_t *automat, state_t *state)
{
	int i;

	for(i = 0; i < automat->len; i++)
	{
		state_t *state_n;

		state_n = automat->array[i];

		if( state_cmp(state_n, state) )
		{
			return i;
		}
	}

	return -1;
}

int automat_add_state(lr_automat_t *automat, state_t *state)
{
	if( automat_is_exists_state(automat, state) == 0 )
	{
		automat->array = resize(automat->array, sizeof(state_t *), &automat->len, &automat->alloc);
		automat->array[automat->len++] = state;

		return 1;
	}

	return 0;
}

void automat_create(lr_automat_t *automat, list_rule_t *list_rule, char *main_token)
{
	rule_t *rule;
	state_t *state;
 	int i;

	rule = list_rule_get(list_rule, main_token);

	if( rule == NULL )
	{
		return;
	}

	state = state_new();

	rule = rule_clone(rule);
	state_add_rule(state, list_rule, rule);

	automat_add_state(automat, state);

	for(i = 0; i < automat->len; i++)
	{
		state_t *state_n;
		int j;

		state_n = automat->array[i];

		for(j = 0; j < state_n->len_rule; j++)
		{
			rule_t *rule;

			rule = (rule_t *) state_n->array_rule[j];

			if( rule->token < rule->len )
			{
				char *token;
				state_t *state_res;

				token = rule->array[rule->token];

				state = state_get_next_state(state_n, token);

				if( state == NULL )
				{
					state = state_new();
				}

				rule = rule_clone(rule);
				rule->token++;

				state_add_rule(state, list_rule, rule);
				state_add_next_state(state_n, token, state);
			}
		}

		for(j = 0; j < state_n->len_token_next; j++)
		{
			state_t *state_res;

			state_res = automat_get_exists_state(automat, state_n->next_state[j]);

			if( state_res != NULL )
			{
				state_destroy(state_n->next_state[j]);
				state_n->next_state[j] = state_res;
			}
			else
			{
				automat_add_state(automat, state_n->next_state[j]);
			}
		}
	}
}

void automat_destroy(lr_automat_t *automat)
{
	int i;

	for(i = 0; i < automat->len; i++)
	{
		state_t *state;

		state = automat->array[i];
		state_destroy(state);
	}

	free(automat->array);
}

void table_init(table_t *table)
{
	memset(table, 0, sizeof(table_t));
}

void table_to_array(table_t *table, char *str)
{
	int i;

	for(i = 0; i < table->len; i++)
	{
		if( strcmp(table->array[i], str) == 0 )
		{
			return;
		}
	}

	table->array = resize(table->array, sizeof(char *), &table->len, &table->alloc);
	table->array[table->len++] = strdup(str);
}

int table_get_pos(table_t *table, char *str)
{
	int i;

	for(i = 0; i < table->len; i++)
	{
		if( strcmp(table->array[i], str) == 0 )
		{
			return i;
		}
	}

	return -1;
}

void table_set(table_t *table, list_rule_t *list_rule, lr_automat_t *automat)
{
	int i;
	int j;

	for(i = 0; i < list_rule->len; i++)
	{
		rule_t *rule = list_rule->array[i];

		table_to_array(table, rule->name);
	}

	for(i = 0; i < list_rule->len; i++)
	{
		rule_t *rule = list_rule->array[i];
		int j;

		for(j = 0; j < rule->len; j++)
		{
			table_to_array(table, rule->array[j]);
		}
	}

	table->x = table->len;
	table->y = automat->len;

	table->table = (char ***) malloc( table->y * sizeof(char **) );

	for(i = 0; i < table->y; i++)
	{
		table->table[i] = (char **) malloc( table->x * sizeof(char *) );
		memset(table->table[i], 0, table->x * sizeof(char *));
	}

	for(i = 0; i < table->y; i++)
	{
		char str[STR_SIZE];
		state_t *state;
		rule_t *rule;

		state = automat->array[i];

		rule = state->array_rule[0];

		if( rule->token == rule->len )
		{
			if( rule->id == 0 )
			{
				sprintf(str, "a");
			}
			else
			{
				sprintf(str, "r%d", rule->id);
			}

			for(j = 0; j < table->x; j++)
			{
				if( table->table[i][j] == NULL && ! list_rule_is_nonterm(list_rule, table->array[j]) )
				{
					table->table[i][j] = strdup(str);
				}
			}
		}

		for(j = 0; j < state->len_token_next; j++)
		{
			state_t *state_n;
			char *token;
			int s;
			int n;

			memset(str, 0, STR_SIZE);

			state_n = state->next_state[j];
			rule = state_n->array_rule[0];
			token = state->next_token[j];
			n = table_get_pos(table, token);

			s = automat_get_numer_state(automat, state_n);

			if( s != -1 )
			{
				sprintf(str, "s%d", s);
			}

			if( table->table[i][n] != NULL )
			{
				free(table->table[i][n]);
			}

			table->table[i][n] = strdup(str);
		}

		for(j = 0; j < table->x; j++)
		{
			if( table->table[i][j] == NULL )
			{
				table->table[i][j] = strdup("");
			}
		}
	}
}

void table_print(table_t *table, FILE *stream)
{
	int i;
	int j;

	fprintf(stream, "%d %d\n", table->x, table->y);

	fprintf(stream, "\t");

	for(i = 0; i < table->len; i++)
	{
		fprintf(stream, "%s\t", table->array[i]);
	}

	fprintf(stream, "\n");

	for(i = 0; i < table->y; i++)
	{
		fprintf(stream, "%d\t", i);

		for(j = 0; j < table->x; j++)
		{
			char *s;

			if( strcmp(table->table[i][j], "") == 0 )
			{
				s = ".";
			}
			else
			{
				s = table->table[i][j];
			}

			fprintf(stream, "%s\t", s);
		}

		fprintf(stream, "\n");
	}
}

void table_save(table_t *table, char *filename)
{
	FILE *file;

	file = fopen(filename, "wt");
	table_print(table, file);
	fclose(file);
}

int table_load(table_t *table, char *filename)
{
	FILE *file;
	char line[STR_LINE_SIZE];
	char str[STR_SIZE];
	int i;
	int j;

	file = fopen(filename, "rt");

	fscanf(file, "%d %d", &table->x, &table->y);

	table->table = (char ***) malloc( table->y * sizeof(char **) );

	for(i = 0; i < table->y; i++)
	{
		table->table[i] = (char **) malloc( table->x * sizeof(char *) );
		memset(table->table[i], 0, table->x * sizeof(char *));
	}

	for(i = 0; i < table->x; i++)
	{
		fscanf(file, "%s", str);
		table_to_array(table, str);
	}

	for(i = 0; i < table->y; i++)
	{
		fscanf(file, "%s", str);

		for(j = 0; j < table->x; j++)
		{
			fscanf(file, "%s", str);

			if( strcmp(str, ".") == 0 )
			{
				table->table[i][j] = strdup("");
			}
			else
			{
				table->table[i][j] = strdup(str);
			}
		}
	}

	fclose(file);

	return 0;
}

void table_destroy(table_t *table)
{
	int i;
	int j;

	for(i = 0; i < table->len; i++)
	{
		free(table->array[i]);
	}

	free(table->array);

	for(i = 0; i < table->y; i++)
	{
		for(j = 0; j < table->x; j++)
		{
			free(table->table[i][j]);
		}

		free(table->table[i]);
	}

	free(table->table);
}

char* table_get_location(table_t *table, int item_y, char *item_x)
{
	int find_x;
	int find_y;
	int i;
	int j;

	find_x = -1;
	find_y = -1;

	for(i = 1; i < table->x; i++)
	{
		if( strcmp(table->array[i], item_x) == 0 )
		{
			find_x = i;
			break;
		}
	}

	if( find_x == -1 )
	{
		return NULL;
	}

	find_y = item_y;

	if( find_x != -1 && find_y != -1 )
	{
		return table->table[find_y][find_x];
	}

	return NULL;
}

#define STACK_SIZE	256

int stack[STACK_SIZE];
int stack_pointer;

void stack_init()
{
	memset(stack, 0, STACK_SIZE*sizeof(int));
	stack_pointer = -1;
}

void stack_print()
{
	int i;

	for(i = 0; i <= stack_pointer; i++)
	{
		printf("%d ", stack[i]);
	}
}

void stack_push(int n)
{
	stack_pointer++;
	stack[stack_pointer] = n;
}

int stack_pop()
{
	int res;

	res = stack[stack_pointer];
	stack_pointer--;

	return res;
}

int stack_top()
{
	return stack[stack_pointer];
}

typedef struct
{
	int n;
} parse_log_t;

static parse_log_t* parse_log_new(n)
{
	parse_log_t *parse_log;

	parse_log = (parse_log_t *) malloc( sizeof(parse_log_t) );
	memset(parse_log, 0, sizeof(parse_log_t));

	parse_log->n = n;

	return parse_log;
}

static void parse_log_print(parse_log_t *parse_log)
{
	printf("%d\n", parse_log->n);
}

static void parse_log_destroy(parse_log_t *parse_log)
{
	free(parse_log);
}

#define PARSE_OK		-1

int parse(table_t *table, list_rule_t *list_rule, array_t *array_token, array_t *array_parse_out)
{
	int tape_offset;
	int state;
	token_t *token;

	tape_offset = 0;

	stack_init();
	stack_push(0);

	token = (token_t *) array_get(array_token, tape_offset);

	printf("\nparse:\n");

	while( 1 )
	{
		char str_state[STR_SIZE];
		char *str;
		int i;

		state = stack_top();
		sprintf(str_state, "s%d", state);

		stack_print();

		str = table_get_location(table, state, token->str);

		if( str == NULL )
		{
			str = table_get_location(table, state, token->name);
		}
		
		printf("\ttable[%s][%s] = \'%s\'", str_state, token->str, str);

		if( str == NULL )
		{
			printf("\tparse error\n");
			return tape_offset;
		}
		else if( str[0] == 's' )
		{
			int n;

			sscanf(str+1, "%d", &n);
			stack_push(n);
			printf("\tpush(%d)", n);

			tape_offset++;
			token = (token_t *) array_get(array_token, tape_offset);
			printf("\tread token %s", token->str);
		}
		else if( str[0] == 'r' )
		{
			parse_log_t *parse_log;
			rule_t *rule;
			char *str_nonterm;
			int len;
			int n;
			int i;

			sscanf(str+1, "%d", &n);

			parse_log = parse_log_new(n);
			array_add(array_parse_out, parse_log);

			rule = list_rule->array[n];

			for(i = 0; i < rule->len; i++)
			{
				printf("\tpop()");
				stack_pop();
			}

			str_nonterm = rule->name;

			state = stack_top();
			sprintf(str_state, "s%d", state);

			str = table_get_location(table, state, str_nonterm);
			printf("\ttable[%s][%s] = \'%s\'", str_state, str_nonterm, str);

			if( str == NULL || str[0] != 's' )
			{
				printf("parse reduced error\n");
				return tape_offset;
			}
			else
			{
				sscanf(str+1, "%d", &n);
				printf("\tpush(%d)", n);
				stack_push(n);
			}
		}
		else if( str[0] == '\0' )
		{
			printf("\tparse error\n");
			return tape_offset;
		}
		else if( str[0] == 'a' )
		{
			printf("\taccept\n");
			return PARSE_OK;
		}
		else
		{
			printf("\terror table\n");
			return tape_offset;
		}

		putchar('\n');
	}

	return PARSE_OK;
}

tree_t* tree_new()
{
	static int id = 0;
	tree_t *tree;

	tree = (tree_t *) malloc( sizeof(tree_t) );
	memset(tree, 0, sizeof(tree_t));

	tree->id = id++;
	tree->array_value = array_new();

	return tree;
}

void tree_add(tree_t *tree, tree_t *node)
{
	tree->array = resize(tree->array, sizeof(tree_t *), &tree->len, &tree->alloc);
	tree->array[tree->len++] = node;
}

void tree_add_value(tree_t *tree, char *str)
{
	array_add(tree->array_value, strdup(str));
}

void tree_print_node(FILE *stream, tree_t *tree)
{
	int i;

	fprintf(stream, "\"");

	fprintf(stream, "%d ", tree->id);

	for(i = 0; i < tree->rule->len; i++)
	{
		fprintf(stream, "%s ", tree->rule->array[i]);
	}

#if 0
	if( tree->array_value != NULL )
	{
		for(i = 0; i < tree->array_value->count; i++)
		{
			char *str;
	
			str = (char *) array_get(tree->array_value, i);
			fprintf(stream, "%s ", str);
		}
	}
#endif

	fprintf(stream, "\"");
}

void tree_print_graphviz_do(FILE *stream, tree_t *tree)
{
	int i;

	for(i = 0; i < tree->len; i++)
	{
		fprintf(stream, "\t");

		tree_print_node(stream, tree);
		fprintf(stream, " -> ");
		tree_print_node(stream, tree->array[i]);

		fprintf(stream, "\n");
	}

	for(i = 0; i < tree->len; i++)
	{
		tree_print_graphviz_do(stream, tree->array[i]);
	}
}

void tree_print_graphviz(tree_t *tree, char *filename)
{
	FILE *file;

	file = fopen(filename, "wt");

	fprintf(file, "digraph G\n\{\n");

	tree_print_graphviz_do(file, tree);

	fprintf(file, "}\n");

	fclose(file);
}

void tree_destroy(tree_t *tree)
{
	int i;

	for(i = 0; i < tree->len; i++)
	{
		tree_destroy(tree->array[i]);
	}

	if( tree->array != NULL )
	{
		free(tree->array);
	}

	if( tree->array_value != NULL )
	{
		array_destroy_item(tree->array_value, free);
	}

	free(tree);
}

void tree_set_value_do(tree_t *tree, array_t *array_token, list_rule_t *list_rule, int *offset)
{
	int i;
	int n;

	n = 0;

	for(i = 0; i < tree->rule->len; i++)
	{
		char *str = tree->rule->array[i];

		if( list_rule_is_nonterm(list_rule, str) == 0 )
		{
			token_t *token = (token_t *) array_get(array_token, *offset);
			tree_add_value(tree, token->str);

			(*offset)++;
		}
		else
		{
			tree_set_value_do(tree->array[n], array_token, list_rule, offset);
			n++;
		}
	}
}

void tree_set_value(tree_t *tree, array_t *array_token, list_rule_t *list_rule)
{
	int offset;

	offset = 0;

	tree_set_value_do(tree, array_token, list_rule, &offset);
}

#define STACK_NODE_SIZE		256

static tree_t* stack_node[STACK_NODE_SIZE];
static int stack_node_pointer;

static void stack_node_init()
{
	memset(stack_node, 0, STACK_NODE_SIZE*sizeof(tree_t *));
	stack_node_pointer = -1;
}

static int stack_node_is_empty()
{
	return stack_node_pointer == -1;
}

static void stack_node_push(tree_t *node)
{
	stack_node_pointer++;
	stack_node[stack_node_pointer] = node;
}

static tree_t* stack_node_pop()
{
	tree_t *res;

	assert( stack_node_pointer >= 0 );

	res = stack_node[stack_node_pointer];
	stack_node_pointer--;

	return res;
}

static tree_t* stack_node_top()
{
	assert( stack_node_pointer >= 0 );
	return stack_node[stack_node_pointer];
}

tree_t* create_tree(list_rule_t *list_rule, array_t *array_parse_out)
{
	int i;

	stack_node_init();

	for(i = 0; i < array_parse_out->count; i++)
	{
		parse_log_t *parse_log = (parse_log_t *) array_get(array_parse_out, i);
		tree_t *tree;

		tree = tree_new();
		tree->rule = list_rule->array[ parse_log->n ];

		if( tree->rule->count_nonterm == 0 )
		{
			stack_node_push(tree);
		}
		else
		{
			tree_t *array[8];
			int len;
			int j;

			len = 0;

			for(j = 0; j < tree->rule->count_nonterm; j++)
			{
				tree_t *tree_top;

				tree_top = stack_node_pop();
				array[len++] = tree_top;
			}

			while( len > 0 )
			{
				tree_add(tree, array[len-1]);
				len--;
			}

			stack_node_push(tree);
		}
	}

	return stack_node_pop();
}

static void print_error(textFile_t *textFile, int line, int offset)
{
	char *str;
	int len;
	int count;
	int i;

	if( line <= 0 )
	{
		return;
	}

	count = 0;
	str = (char *) array_get(textFile->array, line-1);
	len = strlen(str);

	printf("error at line %d\n", line);
	printf("%s", str);

	for(i = 0; i < len; i++)
	{
		if( str[i] == '\t' )
		{
			count++;
		}
	}

	offset += count*7;

	for(i = 0; i < offset; i++)
	{
		putchar('.');
	}

	putchar('^');
	putchar('\n');
}

void lr_init(lr_t *lr)
{
	list_rule_init(&lr->list_rule);
	table_init(&lr->table);
}

void lr_load(lr_t *lr, char *rule_filename, char *table_filename)
{
	lr_automat_t automat;

	load_rule(rule_filename, &lr->list_rule);
	list_rule_work(&lr->list_rule);

	table_load(&lr->table, table_filename);
}

void lr_gen_table(lr_t *lr, char *rule_filename, char *table_filename)
{
	char *str_main_nonterm;
	lr_automat_t automat;

	load_rule(rule_filename, &lr->list_rule);
	list_rule_work(&lr->list_rule);

	str_main_nonterm = list_rule_get_main_nonterm(&lr->list_rule);
	list_rule_print(&lr->list_rule);

	automat_init(&automat);
	automat_create(&automat, &lr->list_rule, str_main_nonterm );
	//automat_print(&automat);

	table_set(&lr->table, &lr->list_rule, &automat);
	table_save(&lr->table, table_filename);

	automat_destroy(&automat);
}

tree_t* lr_parse(lr_t *lr, textFile_t *textFile, array_t *array_token)
{
	array_t *array_parse_out;
	int out_len;
	tree_t *tree;
	int res;

	array_parse_out = array_new();

	res = parse(&lr->table, &lr->list_rule, array_token, array_parse_out);

	if( res == PARSE_OK )
	{
		tree = create_tree(&lr->list_rule, array_parse_out);
		tree_set_value(tree, array_token, &lr->list_rule);
	
		array_destroy_item(array_parse_out, parse_log_destroy);

		return tree;
	}
	else
	{
		token_t *token;

		array_destroy_item(array_parse_out, parse_log_destroy);
		token = (token_t *) array_get(array_token, res);
		print_error(textFile, token->line, token->offset);
	}

	return NULL;
}

void lr_destroy(lr_t *lr)
{
	list_rule_destroy(&lr->list_rule);
	table_destroy(&lr->table);
}
