FLAGS=-ggdb
INCLUDE=-I../tokenizer-stable

all : test

test : unittest.o langdef.o
	g++ unittest.o langdef.o -o test

unittest.o : uLanguageDefinition2.h uLangDef2Test.h uLangDef2Test.cpp
	g++ ${INCLUDE} ${FLAGS} uLangDef2Test.cpp -c -o unittest.o 

langdef.o : uLanguageDefinition2.h uLanguageDefinition2.cpp
	g++ ${INCLUDE} ${FLAGS} uLanguageDefinition2.cpp -c -o langdef.o 

clean :
	rm test *.o

