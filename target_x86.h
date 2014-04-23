
#ifndef TARGET_X86_H

#define TARGET_X86_H

#include "array.h"
#include "main.h"

typedef struct
{
	char *name;
	
	array_t *array_param;
	array_t *array_local;

	array_t *array_line;

	array_t *array_text;
	array_t *array_data;
	array_t *array_bss;
	
	int label_count;
	int count_param;
} target_x86_t;

target_x86_t* target_x86_new();
int target_x86_load(target_x86_t *target_x86, char *filename);
int target_x86_save(target_x86_t *target_x86, char *filename);
void target_x86_dump(target_x86_t *target_x86);
int target_x86_compile(target_x86_t *target_x86);
void target_x86_destroy(target_x86_t *target_x86);

#endif
