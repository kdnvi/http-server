.PHONY: run clean
run: bin/server
	./bin/server
bin/server: main.c | mkbin
	cc -Wall -Wextra -Wpedantic -g -o bin/server main.c
clean:
	rm -rf bin
mkbin:
	mkdir -p bin
