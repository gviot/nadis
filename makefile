
all:
	@echo 'This makefile is only used with the "test" argument to unit test C code'

test:
	gcc -o tests/buffer.out tests/buffer.c src/circular_buffer.c -lcheck -lpython -std=c99
	./tests/buffer.out

