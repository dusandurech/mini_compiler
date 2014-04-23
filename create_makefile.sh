#!/bin/bash

OUTPUT_MAKEFILE="Makefile"
PREFIX="/usr/local/bin"

rm -rf $OUTPUT_MAKEFILE

SOURCE_CODE=`echo *.c`

echo 'CC = gcc' >> $OUTPUT_MAKEFILE
echo 'FLAGS = -g -O0' >> $OUTPUT_MAKEFILE
echo 'OBJ =' ${SOURCE_CODE//".c"/".o"} >> $OUTPUT_MAKEFILE
echo 'OUT_BIN = main' >> $OUTPUT_MAKEFILE

echo "LIBS = $TERMCAP" >> $OUTPUT_MAKEFILE
echo >> $OUTPUT_MAKEFILE
echo '$(OUT_BIN): $(OBJ)' >> $OUTPUT_MAKEFILE
echo -e '\011$(CC) $(FLAGS) -o $(OUT_BIN) $(OBJ) $(LIBS)' >> $OUTPUT_MAKEFILE
echo >> $OUTPUT_MAKEFILE

for FILE in $SOURCE_CODE
do
	gcc -MM $FILE >> $OUTPUT_MAKEFILE
	echo -e '\011$(CC) $(FLAGS) -c '$FILE'\n' >> $OUTPUT_MAKEFILE
done

echo 'clean:' >> $OUTPUT_MAKEFILE
echo -e '\011rm -rf *.o $(OUT_BIN)' >> $OUTPUT_MAKEFILE

echo 'install:' >> $OUTPUT_MAKEFILE
echo -e '\011cp $(OUT_BIN) '$PREFIX >> $OUTPUT_MAKEFILE

echo 'uninstall:' >> $OUTPUT_MAKEFILE
echo -e '\011rm -rf '$PREFIX'/$(OUT_BIN)' >> $OUTPUT_MAKEFILE
