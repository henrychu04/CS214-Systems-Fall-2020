#ifndef MYMALLOC_H_
#define MYMALLOC_H_

#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>

#define malloc(x) mymalloc(x, __FILE__, __LINE__)
#define free(x) myfree(x, __FILE__, __LINE__)
#define FULL_SIZE 4096

void *mymalloc(size_t bytes, char *file, int line);
void myfree(void *p, char *file, int line);
unsigned short inUse(unsigned char *memchunk);
unsigned short numBytes(unsigned char *memchunk);
unsigned short sizeOfChunk(unsigned char *memchunk);
void setChunk(unsigned char *memchunk, int inuse, int size);
void freeChunk(unsigned char *memchunk);

#endif