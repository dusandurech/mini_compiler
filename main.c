
#include <stdio.h>
#include <stdlib.h>

#include "array.h"
#include "textFile.h"
#include "lex.h"
#include "lr.h"
#include "compiler.h"
#include "target_x86.h"
#include "main.h"

#define LEX			"lex"
#define GRAMA			"grama"
#define TABLE			"table.txt"
#define AUTOMAT			"automat.txt"

int compile(char *src_filename, char *dst_filename)
{
	textFile_t *textFile;
	array_t *array_lex;
	lex_t lex;
	lr_t lr;
	tree_t *tree;

	textFile = textFile_new();
	textFile_load(textFile, src_filename);

	lex_init(&lex);

	lex_load_automat(&lex, AUTOMAT);
	lex_load(&lex, LEX);
	lex_print_reg(&lex);

	array_lex = lex_analyze(&lex, textFile);
	lex_print_token(array_lex);

	lex_destroy(&lex);

	lr_init(&lr);
	lr_load(&lr, GRAMA, TABLE);
	tree = lr_parse(&lr, textFile, array_lex);

	if( tree != NULL )
	{
		compiler_t *compiler;
 
		//tree_print_graphviz(tree, "out.dot");

		compiler = compiler_new();
		compiler_run(compiler, tree);
		compiler_save(compiler, dst_filename);
		compiler_destroy(compiler);

		tree_destroy(tree);
	}

	textFile_destroy(textFile);
	lex_destroy_token(array_lex);
	lr_destroy(&lr);

	return 0;
}

int gen_parse_table(char *src_filename, char *dst_filename)
{
	lr_t lr;

	lr_init(&lr);
	lr_gen_table(&lr, src_filename, dst_filename);
	lr_destroy(&lr);

	return 0;
}

int gen_lex_automat(char *src_filename, char *dst_filename)
{
	lex_t lex;

	lex_init(&lex);

	lex_gen_automat(&lex, src_filename);
	lex_print_reg(&lex);
	lex_save_automat(&lex, dst_filename);

	lex_destroy(&lex);

	return 0;
}

int target(char *src_filename, char *dst_filename)
{
	target_x86_t *target;

	target = target_x86_new();
	target_x86_load(target, src_filename);
	target_x86_compile(target);
	//target_x86_dump(target);
	target_x86_save(target, dst_filename);
	target_x86_destroy(target);

	return 0;
}

int all(char *src_filename, char *dst_filename)
{
	textFile_t *textFile;
	array_t *array_lex;
	lex_t lex;
	lr_t lr;
	tree_t *tree;

	textFile = textFile_new();
	textFile_load(textFile, src_filename);

	lex_init(&lex);

	lex_gen_automat(&lex, LEX);
	lex_print_reg(&lex);

	array_lex = lex_analyze(&lex, textFile);
	lex_print_token(array_lex);

	lex_destroy(&lex);

	lr_init(&lr);
	lr_gen_table(&lr, GRAMA, "table.txt");
	tree = lr_parse(&lr, textFile, array_lex);

	if( tree != NULL )
	{
		compiler_t *compiler;
		
		//tree_print_graphviz(tree, "out.dot");

		compiler = compiler_new();
		compiler_run(compiler, tree);
		compiler_save(compiler, dst_filename);
		compiler_destroy(compiler);

		tree_destroy(tree);
	}

	textFile_destroy(textFile);
	lex_destroy_token(array_lex);
	lr_destroy(&lr);

	return 0;
}

int main(int argc, char **argv)
{
	if( argc >= 3 && strcmp(argv[1], "-a") == 0 )
	{
		char *src_filename = argv[2];
		char *dst_filename = argv[3];

		all(src_filename, dst_filename);
	}

	if( argc >= 3 && strcmp(argv[1], "-c") == 0 )
	{
		char *src_filename = argv[2];
		char *dst_filename = argv[3];

		compile(src_filename, dst_filename);
	}

	if( argc >= 3 && strcmp(argv[1], "-lr") == 0 )
	{
		char *src_filename = argv[2];
		char *dst_filename = argv[3];

		gen_parse_table(src_filename, dst_filename);
	}

	if( argc >= 3 && strcmp(argv[1], "-lex") == 0 )
	{
		char *src_filename = argv[2];
		char *dst_filename = argv[3];

		gen_lex_automat(src_filename, dst_filename);
	}

	if( argc >= 3 && strcmp(argv[1], "-t") == 0 )
	{
		char *src_filename = argv[2];
		char *dst_filename = argv[3];

		target(src_filename, dst_filename);
	}

	return 0;
}
