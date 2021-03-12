CC = gcc
CFLAGS1 = -Wall -pthread -fsanitize=address

all: KKJserver

KKJserver: Asst3.c
	$(CC) $(CFLAGS1) Asst3.c -o KKJserver

clean:
	rm -f KKJserver