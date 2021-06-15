CFLAGS=-std=c11 -g -static

main: codegen.c parse.c main.c

test: main
	bash test.sh

clean:
	rm -rf 9cc *.o *~ tmp*

.PHONY: test clean
