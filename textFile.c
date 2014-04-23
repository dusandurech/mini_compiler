
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "textFile.h"
#include "main.h"

textFile_t* textFile_new()
{
	textFile_t *textFile;

	textFile = (textFile_t *) malloc( sizeof(textFile_t) );
	textFile->array = array_new();

	return textFile;
}

void textFile_load(textFile_t *textFile, char *filename)
{
	char line[STR_LINE_SIZE];
	FILE *file;

	file = fopen(filename, "rt");

	if( file == NULL )
	{
		return;
	}

	while( fgets(line, STR_LINE_SIZE-1, file) != NULL )
	{
		array_add(textFile->array, strdup(line) );
	}

	fclose(file);
}

void textFile_print(textFile_t *textFile)
{
	int i;

	for(i = 0; i < textFile->array->count; i++)
	{
		char *str = (char *) array_get(textFile->array, i);
		printf("%s", str);
	}
}

void textFile_destroy(textFile_t *textFile)
{
	if( textFile->array != NULL )
	{
		array_destroy_item(textFile->array, free);
	}

	free(textFile);
}
