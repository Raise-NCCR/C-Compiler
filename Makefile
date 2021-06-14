CFLAGS=-std=c11 -g -static

9cc: 9cc.c

test: 9cc
	./test.sh

clean:
	rm -rf 9cc *.o *~ tmp*

.PHONY: test clean
