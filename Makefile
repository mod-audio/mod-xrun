CC ?= gcc

all: mod-xrun

mod-xrun: mod-xrun.c
	$(CC) $^ -std=gnu99 $(shell pkg-config --cflags --libs jack) -Wall -Wextra -Werror -o $@

clean:
	rm -f mod-xrun
