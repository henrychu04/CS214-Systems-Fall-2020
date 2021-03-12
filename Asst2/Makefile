CC = gcc
CFLAGS = -Wall -g -Werror -fsanitize=address -pthread
# CFLAGS = -Wall -g -Werror -pthread

all: detector

detector: Asst2.c
	$(CC) $(CFLAGS) Asst2.c -o detector -lm

clean:
	rm -f detector