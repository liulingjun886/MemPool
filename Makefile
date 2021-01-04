SRC = $(wildcard *.cpp)

.PHONY:test

test:$(SRC)
	rm -f core.*
	g++ -g -o0 $^ -o $@ -lpthread