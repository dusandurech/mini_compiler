CC = gcc
FLAGS = -g -O0
OBJ = array.o compiler.o lex.o lr.o main.o target_x86.o textFile.o
OUT_BIN = main
LIBS = 

$(OUT_BIN): $(OBJ)
	$(CC) $(FLAGS) -o $(OUT_BIN) $(OBJ) $(LIBS)

array.o: array.c array.h
	$(CC) $(FLAGS) -c array.c

compiler.o: compiler.c array.h lr.h textFile.h compiler.h main.h
	$(CC) $(FLAGS) -c compiler.c

lex.o: lex.c lex.h array.h textFile.h main.h
	$(CC) $(FLAGS) -c lex.c

lr.o: lr.c main.h array.h lex.h textFile.h lr.h
	$(CC) $(FLAGS) -c lr.c

main.o: main.c array.h textFile.h lex.h lr.h compiler.h main.h \
  target_x86.h
	$(CC) $(FLAGS) -c main.c

target_x86.o: target_x86.c array.h main.h target_x86.h
	$(CC) $(FLAGS) -c target_x86.c

textFile.o: textFile.c textFile.h array.h main.h
	$(CC) $(FLAGS) -c textFile.c

clean:
	rm -rf *.o $(OUT_BIN)
install:
	cp $(OUT_BIN) /usr/local/bin
uninstall:
	rm -rf /usr/local/bin/$(OUT_BIN)
