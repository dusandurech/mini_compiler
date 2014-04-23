
#ifndef TEXTFILE_H

#define TEXTFILE_H

#include "array.h"

typedef struct
{
	array_t *array;
} textFile_t;

extern textFile_t* textFile_new();
extern void textFile_load(textFile_t *textFile, char *filename);
extern void textFile_print(textFile_t *textFile);
extern void textFile_destroy(textFile_t *textFile);

#endif
