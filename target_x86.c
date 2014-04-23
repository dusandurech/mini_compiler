
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "array.h"
#include "main.h"

#include "target_x86.h"

typedef struct
{
	char *name;
	char *addr;
} var_t;

static var_t* var_new(char *name, char *addr)
{
	var_t *var;

	var = (var_t *) malloc( sizeof(var_t) );
	memset(var, 0, sizeof(var_t));

	var->name = strdup(name);
	var->addr = strdup(addr);

	return var;
}

static void var_destroy(var_t *var)
{
	if( var->name != NULL )
	{
		free(var->name);
	}

	if( var->addr != NULL )
	{
		free(var->addr);
	}

	free(var);
}

#define TARGET_TEXT		1
#define TARGET_DATA		2
#define TARGET_BSS		3

target_x86_t* target_x86_new()
{
	target_x86_t *target_x86;

	target_x86 = (target_x86_t *) malloc( sizeof(target_x86_t) );
	memset(target_x86, 0, sizeof(target_x86_t));

	target_x86->array_param = array_new();
	target_x86->array_local = array_new();
	target_x86->array_line = array_new();
	target_x86->array_text = array_new();
	target_x86->array_data = array_new();
	target_x86->array_bss = array_new();

	return target_x86;
}

int target_x86_load(target_x86_t *target_x86, char *filename)
{
	FILE *file;
	char line[STR_LINE_SIZE];

	file = fopen(filename, "rt");

	while( fgets(line, STR_LINE_SIZE-1, file) != NULL )
	{
		char *s;

		s = strstr(line, "\n");

		if( s != NULL )
		{
			*s = '\0';
		}

		array_add(target_x86->array_line, strdup(line));
	}

	fclose(file);

	return 0;
}

int target_x86_save(target_x86_t *target_x86, char *filename)
{
	FILE *file;
	int i;

	file = fopen(filename, "wt");

	fprintf(file, "\nSECTION .text\n\n");
	
	fprintf(file,
		"BITS 32\n"
		"\n"
		"extern printf\n"
		"extern scanf\n"
		"extern exit\n"
		"\n"
		"global main\n"
		"\n");
	
	for(i = 0; i < target_x86->array_text->count; i++)
	{
		char *str = (char *) array_get(target_x86->array_text, i);
		fprintf(file, "%s\n", str);
	}
	
	fprintf(file, "\nSECTION .data\n\n");
	
	for(i = 0; i < target_x86->array_data->count; i++)
	{
		char *str = (char *) array_get(target_x86->array_data, i);
		fprintf(file, "%s\n", str);
	}

	fprintf(file, "\nSECTION .bss\n\n");

	for(i = 0; i < target_x86->array_bss->count; i++)
	{
		char *str = (char *) array_get(target_x86->array_bss, i);
		fprintf(file, "%s\n", str);
	}
	
	fclose(file);
	
	return 0;
}

void add_line(target_x86_t *target_x86, int type, char *line)
{
	if( type == TARGET_TEXT )
	{
		array_add(target_x86->array_text, strdup(line));
	}
}

void target_x86_dump(target_x86_t *target_x86)
{
	int i;

	for(i = 0; i < target_x86->array_text->count; i++)
	{
		char *line = (char *) array_get(target_x86->array_text, i);
		printf("%s\n", line);
	}
}

static void add_data_string(target_x86_t *target_x86, char *name, char *val)
{
	char line[STR_LINE_SIZE];
	int i;
	
	for(i = 0; i < target_x86->array_data->count; i++)
	{
		char *str = (char *) array_get(target_x86->array_data, i);
		
		if( strstr(str, name) != NULL )
		{
			return;
		}
	}
	
	sprintf(line, "%s : db \"%s\", 10, 0", name, val);
	array_add(target_x86->array_data, strdup(line));
}

static int target_x86_clean_function(target_x86_t *target_x86)
{
	if( target_x86->name != NULL )
	{
		free(target_x86->name);
		target_x86->name = NULL;
	}
	
	arrray_do_empty_item(target_x86->array_param, var_destroy);
	arrray_do_empty_item(target_x86->array_local, var_destroy);
}

static int target_x86_print_function(target_x86_t *target_x86)
{
	int i;
	
	printf("function name = %s\n", target_x86->name);
	
	for(i = 0; i < target_x86->array_param->count; i++)
	{
		var_t *var = array_get(target_x86->array_param, i);
		
		printf("param %d %s %s\n", i, var->name, var->addr);
	}

	for(i = 0; i < target_x86->array_local->count; i++)
	{
		var_t *var = array_get(target_x86->array_local, i);
		
		printf("local %d %s %s\n", i, var->name, var->addr);
	}

}

static var_t* target_x86_get_param_var(target_x86_t *target_x86, char *name)
{
	int i;
	
	for(i = 0; i < target_x86->array_param->count; i++)
	{
		var_t *var = array_get(target_x86->array_param, i);
		
		if( strcmp(name, var->name) == 0 )
		{
			return var;
		}
	}
	
	return NULL;
}

static int target_x86_add_param_var(target_x86_t *target_x86, char *name)
{
	var_t *var;
	char addr[STR_SIZE];
	int count;
	
	if( target_x86_get_param_var(target_x86, name) != NULL )
	{
		return -1;
	}
	
	count = target_x86->array_param->count;
	sprintf(addr, "[ebp+0x%x]", 8+4*count);
	
	var = var_new(name, addr);
	array_add(target_x86->array_param, var);
	
	return 0;
}

static var_t* target_x86_get_local_var(target_x86_t *target_x86, char *name)
{
	int i;
	
	for(i = 0; i < target_x86->array_local->count; i++)
	{
		var_t *var = array_get(target_x86->array_local, i);
		
		if( strcmp(name, var->name) == 0 )
		{
			return var;
		}
	}
	
	return NULL;
}

static int target_x86_add_local_var(target_x86_t *target_x86, char *name)
{
	var_t *var;
	char addr[STR_SIZE];
	int count;
	
	if( target_x86_get_local_var(target_x86, name) != NULL )
	{
		return -1;
	}
	
	count = target_x86->array_local->count;
	sprintf(addr, "[ebp-0x%x]", 4+4*count);
	
	var = var_new(name, addr);
	array_add(target_x86->array_local, var);
	
	return 0;
}

static var_t* target_x86_get_var(target_x86_t *target_x86, char *name)
{
	var_t *var;
	
	var = target_x86_get_param_var(target_x86, name);
	
	if( var == NULL )
	{
		var = target_x86_get_local_var(target_x86, name);
	}
	
	return var;
}

static int is_variable(char *str)
{
	char c;
	
	c = str[0];
	
	if( (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') )
	{
		return 1;
	}
	
	return 0;
}

#define LET_BIN		1
#define LET_UN		2
#define LET_AS		3
#define LET_ERR		4

static int analye_op_let(char *line, char *param_dst, char *param_src1, char *param_src2, char *param_op)
{
	char cmd[STR_SIZE];
	char str[STR_SIZE];
	int res;
			
	res = sscanf(line, "%s %s %s %s %s %s", cmd, param_dst, str, param_src1, param_op, param_src2);

	if( res == 6 )
	{
		return LET_BIN;
	}

	res = sscanf(line, "%s %s %s %s %s", cmd, param_dst, str, param_op, param_src1);

	if( res == 5 )
	{
		return LET_UN;
	}

	res = sscanf(line, "%s %s %s %s", cmd, param_dst, str, param_src1);

	if( res == 4 )
	{
		return LET_AS;
	}
	
	return LET_ERR;
}

static int target_x86_prepare_function(target_x86_t *target_x86, int n)
{
	int i;
	
	for(i = n; i < target_x86->array_line->count; i++)
	{
		char *line = (char *) array_get(target_x86->array_line, i);
		
		if( strncmp(line, "begin_function", 14) == 0 )
		{
			char cmd[STR_SIZE];
			char param[STR_SIZE];
			
			sscanf(line, "%s %s", cmd, param);
			
			target_x86->name = strdup(param);
		}
		
		if( strncmp(line, "param", 5) == 0 )
		{
			char cmd[STR_SIZE];
			char param[STR_SIZE];
		
			sscanf(line, "%s %s", cmd, param);
			target_x86_add_param_var(target_x86, param);
		}

		if( strncmp(line, "let", 3) == 0 )
		{
			char cmd[STR_SIZE];
			char param1[STR_SIZE];
			char param2[STR_SIZE];
			char param3[STR_SIZE];
			char op[STR_SIZE];
			int res;
			
			res = analye_op_let(line, param1, param2, param3, op);
			
			if( res == LET_BIN )
			{
				if( is_variable(param1) )
				{
					target_x86_add_local_var(target_x86, param1);
				}
					
				if( is_variable(param2) )
				{
					target_x86_add_local_var(target_x86, param2);
				}
				
				if( is_variable(param3) )
				{
					target_x86_add_local_var(target_x86, param3);
				}
			}
	
			if( res == LET_UN || res == LET_AS )
			{
				if( is_variable(param1) )
				{
					target_x86_add_local_var(target_x86, param1);
				}
					
				if( is_variable(param2) )
				{
					target_x86_add_local_var(target_x86, param2);
				}
			}
		}

		if( strncmp(line, "end_function", 12) == 0 )
		{
			break;
		}
	}	
}

typedef struct
{
	char *name;
	int len;
	int (*f)(target_x86_t *target_x86, int num);
} op_t;

int op_begin_function(target_x86_t *target_x86, int num)
{
	char line[STR_LINE_SIZE];
	int size;
	int i;
	
	target_x86_clean_function(target_x86);
	target_x86_prepare_function(target_x86, num);
	//target_x86_print_function(target_x86);
	
	add_line(target_x86, TARGET_TEXT, "");
	sprintf(line, "%s:", target_x86->name);
	add_line(target_x86, TARGET_TEXT, line);
	add_line(target_x86, TARGET_TEXT, "");

	add_line(target_x86, TARGET_TEXT, ";");
	add_line(target_x86, TARGET_TEXT, "; param :");
	add_line(target_x86, TARGET_TEXT, ";");
	
	for(i = 0; i < target_x86->array_param->count; i++)
	{
		var_t *var = array_get(target_x86->array_param, i);
		
		sprintf(line, "; %s\t%s", var->name, var->addr);
		add_line(target_x86, TARGET_TEXT, line);
	}

	add_line(target_x86, TARGET_TEXT, ";");
	add_line(target_x86, TARGET_TEXT, "; local var :");
	add_line(target_x86, TARGET_TEXT, ";");

	for(i = 0; i < target_x86->array_local->count; i++)
	{
		var_t *var = array_get(target_x86->array_local, i);
		
		sprintf(line, "; %s\t%s", var->name, var->addr);
		add_line(target_x86, TARGET_TEXT, line);
	}
	
	add_line(target_x86, TARGET_TEXT, ";");
	add_line(target_x86, TARGET_TEXT, "");

	add_line(target_x86, TARGET_TEXT, "push ebp");
	add_line(target_x86, TARGET_TEXT, "mov ebp, esp");

	size = 4+4*target_x86->array_local->count;
	sprintf(line, "sub esp, 0x%x", size);
	add_line(target_x86, TARGET_TEXT, line);
	add_line(target_x86, TARGET_TEXT, "");
	
	return 0;
}

int op_end_function(target_x86_t *target_x86, int num)
{
	char line[STR_LINE_SIZE];
	
	//sprintf(line, "; %d", num);
	//add_line(target_x86, TARGET_TEXT, line);
		
	add_line(target_x86, TARGET_TEXT, "");
	add_line(target_x86, TARGET_TEXT, "mov esp, ebp");
	add_line(target_x86, TARGET_TEXT, "pop ebp");
	add_line(target_x86, TARGET_TEXT, "ret");
	add_line(target_x86, TARGET_TEXT, "");

	return 0;
}

int op_let_assign(target_x86_t *target_x86, char *str_dst, char *str_src)
{
	char line[STR_LINE_SIZE];
	var_t *var;

	if( is_variable(str_src) )
	{
		var_t *var;
		
		var = target_x86_get_var(target_x86, str_src);
		str_src = var->addr;
	}
	
	var = target_x86_get_var(target_x86, str_dst);
	str_dst = var->addr;

	sprintf(line, "mov eax, %s", str_src);
	add_line(target_x86, TARGET_TEXT, line);
	
	sprintf(line, "mov %s, eax", str_dst);
	add_line(target_x86, TARGET_TEXT, line);
	
	return 0;
}

int op_let_un_math(target_x86_t *target_x86, char *str_dst, char *str_src, char *op)
{
	char line[STR_LINE_SIZE];
	var_t *var;

	if( is_variable(str_src) )
	{
		var_t *var;
		
		var = target_x86_get_var(target_x86, str_src);
		str_src = var->addr;
	}
	
	var = target_x86_get_var(target_x86, str_dst);
	str_dst = var->addr;

	sprintf(line, "mov eax, %s", str_src);
	add_line(target_x86, TARGET_TEXT, line);

	if( strcmp(op, "-") == 0 )
	{
		add_line(target_x86, TARGET_TEXT, "neg eax");
	}
	
	if( strcmp(op, "not") == 0 )
	{
		add_line(target_x86, TARGET_TEXT, "neg eax");
	}

	if( strcmp(op, "neg") == 0 )
	{
		add_line(target_x86, TARGET_TEXT, "mov ecx, 0");

		add_line(target_x86, TARGET_TEXT, "cmp eax, 0");
		sprintf(line, "jne .label_%d", target_x86->label_count);
		add_line(target_x86, TARGET_TEXT, line);
		
		add_line(target_x86, TARGET_TEXT, "mov ecx, 1");

		sprintf(line, ".label_%d:", target_x86->label_count);
		add_line(target_x86, TARGET_TEXT, line);
		
		add_line(target_x86, TARGET_TEXT, "mov eax, ecx");

		target_x86->label_count++;
	}
	
	sprintf(line, "mov %s, eax", str_dst);
	add_line(target_x86, TARGET_TEXT, line);
	
	return 0;
}

int op_let_bin_cmp(target_x86_t *target_x86, char *str_jmp)
{
	char line[STR_LINE_SIZE];

	add_line(target_x86, TARGET_TEXT, "mov ecx, 1");
	add_line(target_x86, TARGET_TEXT, "cmp eax, ebx");

	sprintf(line, "%s .label_%d", str_jmp, target_x86->label_count);
	add_line(target_x86, TARGET_TEXT, line);
	
	add_line(target_x86, TARGET_TEXT, "mov ecx, 0");
	
	sprintf(line, ".label_%d:", target_x86->label_count);
	add_line(target_x86, TARGET_TEXT, line);
	
	add_line(target_x86, TARGET_TEXT, "mov eax, ecx");

	target_x86->label_count++;
	
	return 0;
}

int op_let_bin_math(target_x86_t *target_x86, char *str_dst, char *str_src_a, char *str_src_b, char *op)
{
	char line[STR_LINE_SIZE];
	var_t *var;

	if( is_variable(str_src_a) )
	{
		var_t *var;
		
		var = target_x86_get_var(target_x86, str_src_a);
		str_src_a = var->addr;
	}
	
	if( is_variable(str_src_b) )
	{
		var_t *var;
		
		var = target_x86_get_var(target_x86, str_src_b);
		str_src_b = var->addr;
	}
	
	var = target_x86_get_var(target_x86, str_dst);
	str_dst = var->addr;

	sprintf(line, "mov eax, %s", str_src_a);
	add_line(target_x86, TARGET_TEXT, line);
	
	sprintf(line, "mov ebx, %s", str_src_b);
	add_line(target_x86, TARGET_TEXT, line);

	if( strcmp(op, "+") == 0 )
	{
		add_line(target_x86, TARGET_TEXT, "add eax, ebx");
	}
	
	if( strcmp(op, "-") == 0 )
	{
		add_line(target_x86, TARGET_TEXT, "sub eax, ebx");
	}

	if( strcmp(op, "*") == 0 )
	{
		add_line(target_x86, TARGET_TEXT, "imul ebx");
	}
	
	if( strcmp(op, "/") == 0 )
	{
		add_line(target_x86, TARGET_TEXT, "mov edx, 0");
		add_line(target_x86, TARGET_TEXT, "div ebx");
	}

	if( strcmp(op, "%") == 0 )
	{
		add_line(target_x86, TARGET_TEXT, "mov edx, 0");
		add_line(target_x86, TARGET_TEXT, "div ebx");
		add_line(target_x86, TARGET_TEXT, "mov eax, edx");
	}

	if( strcmp(op, "and") == 0 )
	{
		add_line(target_x86, TARGET_TEXT, "mov ecx, 0");

		add_line(target_x86, TARGET_TEXT, "cmp eax, 1");
		sprintf(line, "jne .label_%d", target_x86->label_count);
		add_line(target_x86, TARGET_TEXT, line);
		
		add_line(target_x86, TARGET_TEXT, "cmp ebx, 1");
		sprintf(line, "jne .label_%d", target_x86->label_count);
		add_line(target_x86, TARGET_TEXT, line);

		add_line(target_x86, TARGET_TEXT, "mov ecx, 1");

		sprintf(line, ".label_%d:", target_x86->label_count);
		add_line(target_x86, TARGET_TEXT, line);
		
		add_line(target_x86, TARGET_TEXT, "mov eax, ecx");

		target_x86->label_count++;
	}

	if( strcmp(op, "or") == 0 )
	{
		add_line(target_x86, TARGET_TEXT, "mov ecx, 0");

		add_line(target_x86, TARGET_TEXT, "cmp eax, 1");
		sprintf(line, "je .label_%d", target_x86->label_count);
		add_line(target_x86, TARGET_TEXT, line);
		
		add_line(target_x86, TARGET_TEXT, "cmp ebx, 1");
		sprintf(line, "je .label_%d", target_x86->label_count);
		add_line(target_x86, TARGET_TEXT, line);

		sprintf(line, "jmp .label_%d", target_x86->label_count+1);
		add_line(target_x86, TARGET_TEXT, line);


		sprintf(line, ".label_%d:", target_x86->label_count);
		add_line(target_x86, TARGET_TEXT, line);
		
		add_line(target_x86, TARGET_TEXT, "mov ecx, 1");

		sprintf(line, ".label_%d:", target_x86->label_count+1);
		add_line(target_x86, TARGET_TEXT, line);

		add_line(target_x86, TARGET_TEXT, "mov eax, ecx");

		target_x86->label_count += 2;
	}

	if( strcmp(op, "==") == 0 ) op_let_bin_cmp(target_x86, "je");
	if( strcmp(op, "!=") == 0 ) op_let_bin_cmp(target_x86, "jne");
	if( strcmp(op, ">") == 0 ) op_let_bin_cmp(target_x86, "ja");
	if( strcmp(op, ">=") == 0 ) op_let_bin_cmp(target_x86, "jae");
	if( strcmp(op, "<") == 0 ) op_let_bin_cmp(target_x86, "jb");
	if( strcmp(op, "<=") == 0 ) op_let_bin_cmp(target_x86, "jbe");
	
	sprintf(line, "mov %s, eax", str_dst);
	add_line(target_x86, TARGET_TEXT, line);
	
	return 0;
}

int op_let(target_x86_t *target_x86, int num)
{
	char *line;
	char cmd[STR_SIZE];
	char param1[STR_SIZE];
	char param2[STR_SIZE];
	char param3[STR_SIZE];
	char op[STR_SIZE];
	int res;
		
	line = (char *) array_get(target_x86->array_line, num);
	
	res = analye_op_let(line, param1, param2, param3, op);

	if( res == LET_AS )
	{
		op_let_assign(target_x86, param1, param2);
	}

	if( res == LET_UN )
	{
		op_let_un_math(target_x86, param1, param2, op);
	}

	if( res == LET_BIN )
	{
		op_let_bin_math(target_x86, param1, param2, param3, op);
	}

	return 0;
}

int op_print(target_x86_t *target_x86, int num)
{
	char line[STR_LINE_SIZE];
	char *line_code;
	char name[STR_SIZE];
	char str[STR_SIZE];
	char cmd[STR_SIZE];
	char param[STR_SIZE];
	var_t *var;
	int res;
	
	line_code = (char *) array_get(target_x86->array_line, num);
	
	res = sscanf(line_code, "%s %s", cmd, param);
	
	var = target_x86_get_var(target_x86, param);

	sprintf(line, "push dword %s", var->addr);
	add_line(target_x86, TARGET_TEXT, line);
	
	sprintf(line, "push dword str_%s", param);
	add_line(target_x86, TARGET_TEXT, line);

	add_line(target_x86, TARGET_TEXT, "call printf");
	add_line(target_x86, TARGET_TEXT, "add esp, 2*4");

	sprintf(name, "str_%s", param);
	sprintf(str, "%s = %%d", param);
	add_data_string(target_x86, name, str);
	
	return 0;
}

int op_call(target_x86_t *target_x86, int num)
{
	char line[STR_LINE_SIZE];
	char *line_code;
	char cmd[STR_SIZE];
	char param[STR_SIZE];
	int res;
	
	line_code = (char *) array_get(target_x86->array_line, num);
	
	res = sscanf(line_code, "%s %s", cmd, param);
	
	sprintf(line, "call %s", param);
	add_line(target_x86, TARGET_TEXT, line);

	sprintf(line, "add esp, %d*4", target_x86->count_param);
	add_line(target_x86, TARGET_TEXT, line);
	target_x86->count_param = 0;
	
	return 0;
}

int op_add_param(target_x86_t *target_x86, int num)
{
	char line[STR_LINE_SIZE];
	char *line_code;
	var_t *var;
	char cmd[STR_SIZE];
	char param[STR_SIZE];
	int res;
	
	line_code = (char *) array_get(target_x86->array_line, num);
	
	res = sscanf(line_code, "%s %s", cmd, param);
	var = target_x86_get_var(target_x86, param);
	
	sprintf(line, "push dword %s", var->addr);
	add_line(target_x86, TARGET_TEXT, line);

	target_x86->count_param++;
		
	return 0;
}

int op_param(target_x86_t *target_x86, int num)
{
	return 0;
}

int op_ifjmp(target_x86_t *target_x86, int num)
{
	char line[STR_LINE_SIZE];
	char *line_code;
	char cmd[STR_SIZE];
	char param1[STR_SIZE];
	char param2[STR_SIZE];
	var_t *var;
	int res;
	
	line_code = (char *) array_get(target_x86->array_line, num);
	
	res = sscanf(line_code, "%s %s %s", cmd, param1, param2);
	
	var = target_x86_get_var(target_x86, param1);

	sprintf(line, "mov eax, %s", var->addr);
	add_line(target_x86, TARGET_TEXT, line);
	
	add_line(target_x86, TARGET_TEXT, "cmp eax, 1");
	sprintf(line, "je %s", param2);
	add_line(target_x86, TARGET_TEXT, line);

	return 0;
}

int op_jmp(target_x86_t *target_x86, int num)
{
	char line[STR_LINE_SIZE];
	char *line_code;
	char cmd[STR_SIZE];
	char param[STR_SIZE];
	int res;
	
	line_code = (char *) array_get(target_x86->array_line, num);
	
	res = sscanf(line_code, "%s %s", cmd, param);
	
	sprintf(line, "jmp %s", param);
	add_line(target_x86, TARGET_TEXT, line);

	return 0;
}

int op_label(target_x86_t *target_x86, int num)
{
	char line[STR_LINE_SIZE];
	char *line_code;
	char cmd[STR_SIZE];
	char param[STR_SIZE];
	int res;
	
	line_code = (char *) array_get(target_x86->array_line, num);
	
	res = sscanf(line_code, "%s %s", cmd, param);
	
	sprintf(line, "%s:", param);
	add_line(target_x86, TARGET_TEXT, line);

	return 0;
}

op_t list_op[] = 
{
	{	.name = "begin_function",	.len = 14,	.f = op_begin_function	},
	{	.name = "end_function",		.len = 12,	.f = op_end_function	},
	{	.name = "let",			.len = 3,	.f = op_let		},
	{	.name = "call",			.len = 4,	.f = op_call		},
	{	.name = "add_param",		.len = 9,	.f = op_add_param	},
	{	.name = "param",		.len = 5,	.f = op_param		},
	{	.name = "ifjmp",		.len = 5,	.f = op_ifjmp		},
	{	.name = "jmp",			.len = 3,	.f = op_jmp		},
	{	.name = "label",		.len = 5,	.f = op_label		},
	{	.name = "print",		.len = 5,	.f = op_print		}
};

#define COUNT_OP	sizeof(list_op) / sizeof(op_t)

op_t* find_op(char *line)
{
	char str_op[STR_SIZE];
	int i;

	sscanf(line, "%s", str_op);

	for(i = 0; i < COUNT_OP; i++)
	{
		op_t *op;

		op = &list_op[i];

		if( strncmp(str_op, op->name, op->len) == 0 )
		{
			return op;
		}
	}

	return NULL;
}

int target_x86_compile(target_x86_t *target_x86)
{
	int i;

	for(i = 0; i < target_x86->array_line->count; i++)
	{
		char *line = (char *) array_get(target_x86->array_line, i);
		op_t *op;
		
		op = find_op(line);
		
		if( op != NULL )
		{
			char str[STR_LINE_SIZE];
			
			sprintf(str, "; %s", line);
			add_line(target_x86, TARGET_TEXT, str);
			
			op->f(target_x86, i);
		}
	}

	return 0;
}

void target_x86_destroy(target_x86_t *target_x86)
{
	if( target_x86->name != NULL )
	{
		free(target_x86->name);
	}
	
	array_destroy_item(target_x86->array_param, var_destroy);
	array_destroy_item(target_x86->array_local, var_destroy);

	array_destroy_item(target_x86->array_line, free);
	array_destroy_item(target_x86->array_text, free);
	array_destroy_item(target_x86->array_data, free);
	array_destroy_item(target_x86->array_bss, free);

	free(target_x86);
}
