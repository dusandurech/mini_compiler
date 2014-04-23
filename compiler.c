
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "array.h"
#include "lr.h"
#include "compiler.h"

static void stack_init(stack_t *stack)
{
	stack->top = 0;
}

static void stack_push(stack_t *stack, int n)
{
	stack->top++;
	stack->stack[stack->top] = n;
}

static void stack_clear(stack_t *stack)
{
	stack->top = 0;
}

static int stack_pop(stack_t *stack)
{
	int n;
	
	n = stack->stack[stack->top];
	stack->top--;
	
	return n;
}

static void stack_quit(stack_t *stack)
{
}

void compiler_add_line(compiler_t *compiler, char *line)
{
	array_add(compiler->array_out, strdup(line));
}

compiler_t* compiler_new()
{
	compiler_t *compiler;

	compiler = (compiler_t *) malloc( sizeof(compiler_t) );
	memset(compiler, 0, sizeof(compiler_t));

	stack_init(&compiler->stack);
	compiler->array_out = array_new();
	compiler->label_id = 0;
		
	return compiler;
}

int compiler_next_var_id(compiler_t *compiler)
{
	return compiler->var_id++;
}

void compiler_reset_var_id(compiler_t *compiler)
{
	compiler->var_id = 0;
}

int compiler_get_label_id(compiler_t *compiler)
{
	return compiler->label_id;
}

void compiler_next_label_id(compiler_t *compiler)
{
	compiler->label_id++;
}

int compiler_set_cycle_start(compiler_t *compiler, char *strstart)
{
	compiler->is_cycle_set = 1;

	strcpy(compiler->startcycle, strstart);
}

int compiler_set_cycle_stop(compiler_t *compiler, char *strstop)
{
	compiler->is_cycle_set = 1;

	strcpy(compiler->stopcycle, strstop);
}

void compiler_unset_cycle(compiler_t *compiler)
{
	compiler->is_cycle_set = 0;

	compiler->startcycle[0] = '\0';
	compiler->startcycle[0] = '\0';
}

void compiler_print(compiler_t *compiler)
{
	int i;

	for(i = 0; i < compiler->array_out->count; i++)
	{
		char *str = (char *) array_get(compiler->array_out, i);
		printf("%s\n", str);
	}
}

void compiler_save(compiler_t *compiler, char *filename)
{
	FILE *file;
	int i;

	file = fopen(filename, "wt");

	for(i = 0; i < compiler->array_out->count; i++)
	{
		char *str = (char *) array_get(compiler->array_out, i);
		fprintf(file, "%s\n", str);
	}
	
	fclose(file);
}

void compiler_destroy(compiler_t *compiler)
{
	array_destroy_item(compiler->array_out, free);
	stack_quit(&compiler->stack);
	
	free(compiler);
}

void compiler_run_un_op(compiler_t *compiler, tree_t *tree, char *op)
{
	char line[STR_LINE_SIZE];
	int n1;
	int n2;
				
	compiler_run(compiler, tree->array[0]);
	n1 = stack_pop(&compiler->stack);
	
	n2 = compiler_next_var_id(compiler);
	stack_push(&compiler->stack, n2);
				
	sprintf(line, "let h%d = %s h%d", n2, op, n1);
	compiler_add_line(compiler, line);
}

void compiler_run_un_op_ex(compiler_t *compiler, tree_t *tree, char *op)
{
	char line[STR_LINE_SIZE];
	int n1;
	int n2;
				
	compiler_run(compiler, tree);
	n1 = stack_pop(&compiler->stack);
	
	n2 = compiler_next_var_id(compiler);
	stack_push(&compiler->stack, n2);
				
	sprintf(line, "let h%d = %s h%d", n2, op, n1);
	compiler_add_line(compiler, line);
}

void compiler_run_bin_op(compiler_t *compiler, tree_t *tree, char *op)
{
	char line[STR_LINE_SIZE];
	int a;
	int b;
	int c;
			
	compiler_run(compiler, tree->array[1]);
	compiler_run(compiler, tree->array[0]);
			
	a = stack_pop(&compiler->stack);
	b = stack_pop(&compiler->stack);
	c = compiler_next_var_id(compiler);
	stack_push(&compiler->stack, c);
			
	sprintf(line, "let h%d = h%d %s h%d", c, a, op, b);
	compiler_add_line(compiler, line);
}

int compiler_run(compiler_t *compiler, tree_t *tree)
{
	rule_t *rule;

	rule = tree->rule;

	if( strcmp(rule->name, "$expr_unar") == 0 )
	{
		if( strcmp(rule->str, "$expr_unar -> variable = $expr_and_or") == 0 )
		{
			char line[STR_LINE_SIZE];
			char *str = (char *) array_get(tree->array_value, 0);
			int n;
			
			compiler_run(compiler, tree->array[0]);
			n = stack_pop(&compiler->stack);
			
			sprintf(line, "let %s = h%d", str, n);
			compiler_add_line(compiler, line);
		}
		
		if( strcmp(rule->str, "$expr_unar -> variable") == 0 )
		{
			char line[STR_LINE_SIZE];
			char *str = (char *) array_get(tree->array_value, 0);
			int n;

			n = compiler_next_var_id(compiler);
			stack_push(&compiler->stack, n);
			
			sprintf(line, "let h%d = %s", n, str);
			compiler_add_line(compiler, line);
		}

		if( strcmp(rule->str, "$expr_unar -> ( $expr_add_sub )") == 0 )
		{
			compiler_run(compiler, tree->array[0]);
		}

		if( strcmp(rule->str, "$expr_unar -> number") == 0 )
		{
			char line[STR_LINE_SIZE];
			char *str = (char *) array_get(tree->array_value, 0);
			int n;

			n = compiler_next_var_id(compiler);
			stack_push(&compiler->stack, n);
			
			sprintf(line, "let h%d = %s", n, str);
			compiler_add_line(compiler, line);
		}

		if( strcmp(rule->str, "$expr_unar -> + $expr_unar") == 0 )
		{
			compiler_run_un_op(compiler, tree, "+");
		}

		if( strcmp(rule->str, "$expr_unar -> - $expr_unar") == 0 )
		{
			compiler_run_un_op(compiler, tree, "-");
		}

		if( strcmp(rule->str, "$expr_unar -> not $expr_unar") == 0 )
		{
			compiler_run_un_op(compiler, tree, "not");

		}

		if( strcmp(rule->str, "$expr_unar -> variable ( )") == 0 )
		{
			char line[STR_LINE_SIZE];
			char *str = (char *) array_get(tree->array_value, 0);

			sprintf(line, "call %s", str);
			compiler_add_line(compiler, line);
		}

		if( strcmp(rule->str, "$expr_unar -> variable ( $call_function_arg )") == 0 )
		{
			char line[STR_LINE_SIZE];
			char *str = (char *) array_get(tree->array_value, 0);

			compiler_run(compiler, tree->array[0]);

			sprintf(line, "call %s", str);
			compiler_add_line(compiler, line);
		}
	}
	else if( strcmp(rule->name, "$expr_add_sub") == 0 )
	{
		if( strcmp(rule->str, "$expr_add_sub -> $expr_add_sub + $expr_mul_div") == 0 )
		{
			compiler_run_bin_op(compiler, tree, "+");
		}

		if( strcmp(rule->str, "$expr_add_sub -> $expr_add_sub - $expr_mul_div") == 0 )
		{
			compiler_run_bin_op(compiler, tree, "-");
		}

		if( strcmp(rule->str, "$expr_add_sub -> $expr_mul_div") == 0 )
		{
			compiler_run(compiler, tree->array[0]);
		}
	}
	else if( strcmp(rule->name, "$expr_mul_div") == 0 )
	{
		if( strcmp(rule->str, "$expr_mul_div -> $expr_mul_div * $expr_unar") == 0 )
		{
			compiler_run_bin_op(compiler, tree, "*");
		}

		if( strcmp(rule->str, "$expr_mul_div -> $expr_mul_div / $expr_unar") == 0 )
		{
			compiler_run_bin_op(compiler, tree, "/");
		}

		if( strcmp(rule->str, "$expr_mul_div -> $expr_mul_div % $expr_unar") == 0 )
		{
			compiler_run_bin_op(compiler, tree, "%");
		}

		if( strcmp(rule->str, "$expr_mul_div -> $expr_unar") == 0 )
		{
			compiler_run(compiler, tree->array[0]);
		}
	}
	else if( strcmp(rule->name, "$expr_cmp") == 0 )
	{
		if( strcmp(rule->str, "$expr_cmp -> $expr_cmp == $expr_add_sub") == 0 )
		{
			compiler_run_bin_op(compiler, tree, "==");
		}

		if( strcmp(rule->str, "$expr_cmp -> $expr_cmp != $expr_add_sub") == 0 )
		{
			compiler_run_bin_op(compiler, tree, "!=");
		}

		if( strcmp(rule->str, "$expr_cmp -> $expr_cmp > $expr_add_sub") == 0 )
		{
			compiler_run_bin_op(compiler, tree, ">");
		}

		if( strcmp(rule->str, "$expr_cmp -> $expr_cmp >= $expr_add_sub") == 0 )
		{
			compiler_run_bin_op(compiler, tree, ">=");
		}

		if( strcmp(rule->str, "$expr_cmp -> $expr_cmp < $expr_add_sub") == 0 )
		{
			compiler_run_bin_op(compiler, tree, "<");
		}

		if( strcmp(rule->str, "$expr_cmp -> $expr_cmp <= $expr_add_sub") == 0 )
		{
			compiler_run_bin_op(compiler, tree, "<=");
		}

		if( strcmp(rule->str, "$expr_cmp -> $expr_add_sub") == 0 )
		{
			compiler_run(compiler, tree->array[0]);
		}
	}
	else if( strcmp(rule->name, "$expr_and_or") == 0 )
	{
		if( strcmp(rule->str, "$expr_and_or -> $expr_and_or and $expr_cmp") == 0 )
		{
			compiler_run_bin_op(compiler, tree, "and");
		}

		if( strcmp(rule->str, "$expr_and_or -> $expr_and_or or $expr_cmp") == 0 )
		{
			compiler_run_bin_op(compiler, tree, "or");
		}

		if( strcmp(rule->str, "$expr_and_or -> $expr_cmp") == 0 )
		{
			compiler_run(compiler, tree->array[0]);
		}
	}
	else if( strcmp(rule->name, "$cmd") == 0 )
	{
		if( strcmp(rule->str, "$cmd -> $expr ;") == 0 )
		{
			compiler_run(compiler, tree->array[0]);
		}

		if( strcmp(rule->str, "$cmd -> if ( $expr ) $line") == 0 )
		{
			char line[STR_LINE_SIZE];
			int n;

			stack_clear(&compiler->stack);
			compiler_reset_var_id(compiler);
			
			compiler_run_un_op(compiler, tree, "neg");
			
			n = stack_pop(&compiler->stack);
			sprintf(line, "ifjmp h%d ifjmp_l%d", n, compiler_get_label_id(compiler));
			compiler_add_line(compiler, line);

			compiler_run(compiler, tree->array[1]);

			sprintf(line, "label ifjmp_l%d", compiler_get_label_id(compiler));
			compiler_add_line(compiler, line);

			compiler_next_label_id(compiler);
		}

		if( strcmp(rule->str, "$cmd -> if ( $expr ) $line else $line") == 0 )
		{
			char line[STR_LINE_SIZE];
			int id1;
			int id2;
			int n;

			stack_clear(&compiler->stack);
			compiler_reset_var_id(compiler);
			
			id1 = compiler_get_label_id(compiler);
			compiler_next_label_id(compiler);

			id2 = compiler_get_label_id(compiler);
			compiler_next_label_id(compiler);

			compiler_run(compiler, tree->array[0]);

			n = stack_pop(&compiler->stack);
			sprintf(line, "ifjmp h%d ifelsejmp_l%d", n, id1);
			compiler_add_line(compiler, line);

			compiler_run(compiler, tree->array[2]);

			sprintf(line, "jmp ifendjmp_l%d", id2);
			compiler_add_line(compiler, line);

			sprintf(line, "label ifelsejmp_l%d", id1);
			compiler_add_line(compiler, line);

			compiler_run(compiler, tree->array[1]);

			sprintf(line, "label ifendjmp_l%d", id2);
			compiler_add_line(compiler, line);
		}

		if( strcmp(rule->str, "$cmd -> while ( $expr ) $line") == 0 )
		{
			char line[STR_LINE_SIZE];
			int id1;
			int id2;
			int n;
			
			stack_clear(&compiler->stack);
			compiler_reset_var_id(compiler);
			
			id1 = compiler_get_label_id(compiler);
			compiler_next_label_id(compiler);

			id2 = compiler_get_label_id(compiler);
			compiler_next_label_id(compiler);

			sprintf(line, "whilestart_jmp_l%d", id1);
			compiler_set_cycle_start(compiler, line);

			sprintf(line, "whilestop_jmp_l%d", id2);
			compiler_set_cycle_stop(compiler, line);

			sprintf(line, "label whilestart_jmp_l%d", id1);
			compiler_add_line(compiler, line);

			compiler_run_un_op(compiler, tree, "neg");
			n = stack_pop(&compiler->stack);
			sprintf(line, "ifjmp h%d whilestop_jmp_l%d", n, id2);
			compiler_add_line(compiler, line);

			compiler_run(compiler, tree->array[1]);

			sprintf(line, "jmp whilestart_jmp_l%d", id1);
			compiler_add_line(compiler, line);

			sprintf(line, "label whilestop_jmp_l%d", id2);
			compiler_add_line(compiler, line);

			compiler_unset_cycle(compiler);
		}

		if( strcmp(rule->str, "$cmd -> do $line while ( $expr ) ;") == 0 )
		{
			char line[STR_LINE_SIZE];
			int id1;
			int id2;
			int n;
			
			stack_clear(&compiler->stack);
			compiler_reset_var_id(compiler);
			
			id1 = compiler_get_label_id(compiler);
			compiler_next_label_id(compiler);

			id2 = compiler_get_label_id(compiler);
			compiler_next_label_id(compiler);

			sprintf(line, "dostart_jmp_l%d", id1);
			compiler_set_cycle_start(compiler, line);

			sprintf(line, "dostop_jmp_l%d", id2);
			compiler_set_cycle_stop(compiler, line);

			sprintf(line, "label dostart_jmp_l%d", id1);
			compiler_add_line(compiler, line);

			compiler_run(compiler, tree->array[0]);
			compiler_run(compiler, tree->array[1]);

			n = stack_pop(&compiler->stack);
			sprintf(line, "ifjmp h%d dostart_jmp_l%d", n, id1);
			compiler_add_line(compiler, line);

			sprintf(line, "label dostop_jmp_l%d", id2);
			compiler_add_line(compiler, line);

			compiler_unset_cycle(compiler);
		}

		if( strcmp(rule->str, "$cmd -> for ( $expr ; $expr ; $expr ) $line") == 0 )
		{
			char line[STR_LINE_SIZE];
			int id1;
			int id2;
			int id3;
			int n;
			
			stack_clear(&compiler->stack);
			compiler_reset_var_id(compiler);
			
			id1 = compiler_get_label_id(compiler);
			compiler_next_label_id(compiler);

			id2 = compiler_get_label_id(compiler);
			compiler_next_label_id(compiler);

			id3 = compiler_get_label_id(compiler);

			sprintf(line, "forstart_jmp_l%d", id1);
			compiler_set_cycle_start(compiler, line);

			sprintf(line, "forstop_jmp_l%d", id2);
			compiler_set_cycle_stop(compiler, line);

			compiler_run(compiler, tree->array[0]);

			sprintf(line, "label forloop_jmp_l%d", id3);
			compiler_add_line(compiler, line);

			compiler_run_un_op_ex(compiler, tree->array[1], "neg");

			n = stack_pop(&compiler->stack);
			sprintf(line, "ifjmp h%d forstop_jmp_l%d", n, id2);
			compiler_add_line(compiler, line);

			compiler_run(compiler, tree->array[3]);
			
			sprintf(line, "label forstart_jmp_l%d", id1);
			compiler_add_line(compiler, line);

			compiler_run(compiler, tree->array[2]);

			sprintf(line, "jmp forloop_jmp_l%d", id3);
			compiler_add_line(compiler, line);

			sprintf(line, "label forstop_jmp_l%d", id2);
			compiler_add_line(compiler, line);

			compiler_unset_cycle(compiler);
		}

		if( strcmp(rule->str, "$cmd -> return ;") == 0 )
		{
			char line[STR_LINE_SIZE];

			sprintf(line, "ret");
			compiler_add_line(compiler, line);
		}

		if( strcmp(rule->str, "$cmd -> return  $expr ;") == 0 )
		{
			char line[STR_LINE_SIZE];

			compiler_run(compiler, tree->array[0]);

			sprintf(line, "ret");
			compiler_add_line(compiler, line);
		}

		if( strcmp(rule->str, "$cmd -> break ;") == 0 )
		{
			if( compiler->is_cycle_set )
			{
				char line[STR_LINE_SIZE];

				sprintf(line, "jmp %s", compiler->stopcycle);
				compiler_add_line(compiler, line);
			}
			else
			{
				printf("compiler error: not use break\n");
			}
		}

		if( strcmp(rule->str, "$cmd -> continue ;") == 0 )
		{
			if( compiler->is_cycle_set )
			{
				char line[STR_LINE_SIZE];

				sprintf(line, "jmp %s", compiler->startcycle);
				compiler_add_line(compiler, line);
			}
			else
			{
				printf("compiler error: not use continue\n");
			}
		}

		if( strcmp(rule->str, "$cmd -> print variable ;") == 0 )
		{
			char line[STR_LINE_SIZE];
			char *str = (char *) array_get(tree->array_value, 1);

			sprintf(line, "print %s", str);
			compiler_add_line(compiler, line);
		}
	}
	else if( strcmp(rule->name, "$function") == 0 )
	{
		if( strcmp(rule->str, "$function -> function variable ( ) $line") == 0 )
		{
			char line[STR_LINE_SIZE];
			char *str = (char *) array_get(tree->array_value, 1);

			stack_clear(&compiler->stack);
			compiler_reset_var_id(compiler);
			
			sprintf(line, "\nbegin_function %s", str);
			compiler_add_line(compiler, line);

			compiler_run(compiler, tree->array[0]);

			sprintf(line, "end_function");
			compiler_add_line(compiler, line);
		}

		if( strcmp(rule->str, "$function -> function variable ( $list_arg ) $line") == 0 )
		{
			char line[STR_LINE_SIZE];
			char *str = (char *) array_get(tree->array_value, 1);

			stack_clear(&compiler->stack);
			compiler_reset_var_id(compiler);
			
			sprintf(line, "\nbegin_function %s", str);
			compiler_add_line(compiler, line);

			compiler_run(compiler, tree->array[0]);
			compiler_run(compiler, tree->array[1]);

			sprintf(line, "end_function", str);
			compiler_add_line(compiler, line);
		}
	}
	else if( strcmp(rule->name, "$call_function_arg") == 0 )
	{
		if( strcmp(rule->str, "$call_function_arg -> $expr") == 0 )
		{
			char line[STR_LINE_SIZE];
			int n;
			
			compiler_run(compiler, tree->array[0]);
			
			n = stack_pop(&compiler->stack);
			sprintf(line, "add_param h%d", n);
			compiler_add_line(compiler, line);
		}

		if( strcmp(rule->str, "$call_function_arg -> $expr , $call_function_arg") == 0 )
		{
			char line[STR_LINE_SIZE];
			int n;

			compiler_run(compiler, tree->array[1]);

			compiler_run(compiler, tree->array[0]);
			
			n = stack_pop(&compiler->stack);
			sprintf(line, "add_param h%d", n);
			compiler_add_line(compiler, line);
		}
	}
	else if( strcmp(rule->name, "$list_arg") == 0 )
	{
		if( strcmp(rule->str, "$list_arg -> variable") == 0 )
		{
			char line[STR_LINE_SIZE];
			char *str = (char *) array_get(tree->array_value, 0);

			sprintf(line, "param %s", str);
			compiler_add_line(compiler, line);
		}

		if( strcmp(rule->str, "$list_arg -> variable , $list_arg") == 0 )
		{
			char line[STR_LINE_SIZE];
			char *str = (char *) array_get(tree->array_value, 0);

			sprintf(line, "param %s", str);
			compiler_add_line(compiler, line);

			compiler_run(compiler, tree->array[0]);
		}
	}
	else
	{
		int i;

		for(i = 0; i < tree->len; i++)
		{
			compiler_run(compiler, tree->array[i]);
		}
	}
}
