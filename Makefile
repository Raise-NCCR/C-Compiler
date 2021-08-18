CFLAGS=-std=c11 -g -static
SRCS=main.c codegen.c parse.c
OBJS=$(SRCS:.c=.o)

9cc: $(OBJS)
	$(CC) -g -o 9cc $(OBJS) $(LDFLAGS)
	rm *.o

$(OBJS): 9cc.h

test: 9cc
	bash test3.sh

clean:
	rm -rf 9cc *.o *~ tmp*

.PHONY: test clean
