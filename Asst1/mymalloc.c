#include <stdio.h>
#include <stdlib.h>
#include "mymalloc.h"

static char myblock[FULL_SIZE];

/*
 * mymalloc takes in a size_t paramater to determine how many bytes of memory to allocate.
 * 
 * mymalloc first checks how many bytes of data the metadata will take up by checking if the
 * number of bytes passed in is greater than 63. If it is, the metadata will take two bytes.
 * If it is not, then the metadata will take up one byte. It iterates through the entire memory
 * until a free chunk that has greater than or equal size is found. The chunk will be split in
 * two, the first being used and with the corresponding size, and the second is free, with the
 * rest of the size of the original chunk. At the end, it returns the value of the bytes of 
 * the allocated chunk.
 * 
 * 
 * Error handling:
 * If the number of bytes that are passed in is 0, it returns an error.
 * 
 * If there is no more memory to be allocated, it returns an error.
 * 
 */

void *mymalloc(size_t bytes, char *file, int line)
{
    if (bytes == 0 || NULL)
    {
        printf("Error: NULL input in line %d, %s\n", line, file);
        return NULL;
    }

    // Calculate the allocated memory bytes that needs to be stored
    unsigned int allocatedSize = bytes + 1;
    if (allocatedSize > 63)
    {
        allocatedSize++;
    }

    unsigned int i = 0;

    while (i < FULL_SIZE)
    {
        unsigned char *crnt = (unsigned char *)&myblock[i];

        // Determining if the crnt chunk of memory is in use
        if ((*crnt & 1) == 0)
        {
            unsigned short crntSize = sizeOfChunk(crnt);

            // Determining if the crnt chunk of memory has space
            // crntSize will be 0 if there is either no more memory to be allocated or if
            // the while loop has reached the last chunk of free memory
            if (crntSize == 0)
            {
                // Checking if the crnt chunk of free memory actually has space
                if (i + allocatedSize <= FULL_SIZE)
                {
                    setChunk(crnt, 1, allocatedSize);     // Allocating the crnt chunk to the requested size
                    setChunk(crnt + allocatedSize, 0, 0); // Adjusting the next chunk of memory to be completely empty

                    return crnt + numBytes(crnt) + 1; // Returning the value of the bytes of the allocated chunk of memory
                }
                else
                {
                    // If there is no more memory to be allocated, break out of the while loop
                    break;
                }
            }
            else
            {
                // Checking if the crnt block has enough memory for the requested size
                if (allocatedSize <= crntSize)
                {
                    setChunk(crnt, 1, allocatedSize); // Allocating the crnt chunk to the requested size
                    if (crntSize - allocatedSize > 0)
                    {
                        // If the next chunk of memory is not being used, set it to free with the corresponding free values
                        setChunk(crnt + allocatedSize, 0, crntSize - allocatedSize);
                    }

                    return crnt + numBytes(crnt) + 1; // Returning the pointer to the crnt chunk of memory
                }
            }
        }

        i += sizeOfChunk(crnt);
    }

    // If there is no more memory to be allocated, the following error will print
    printf("Error: No more memory to allocate in line %d, %s\n", line, file);
    return NULL;
}

/*
 * myfree takes in a void parameter to determine what ptr to free. 
 * 
 * myfree iterates through all the chunks in memory and compares it to the ptr that
 * has been passed in. If crnt chunk matches, then it will free that chunk and set the
 * corresponding metadata values and size to zero. At the same time, it keeps a prev
 * pointer to the previous chunk of memory visited. If two free chunks of memory are next
 * to each other, then it will merge the two chunks.
 * 
 * 
 * 
 * Error handling:
 * 
 * If the ptr is NULL, it returns an error.
 * 
 * If the ptr has already been free, it returns an error.
 * 
 * If the ptr has not been allocated before, it returns an error. 
 * 
 */

void myfree(void *ptr, char *file, int line)
{
    // Handling NULL inputs
    if (ptr == NULL)
    {
        printf("Error: NULL input in line %d, %s\n", line, file);
        return;
    }

    int removed = 0;
    unsigned int i = 0;
    unsigned char *prev = NULL;

    while (i < FULL_SIZE)
    {
        unsigned char *crnt = (unsigned char *)&myblock[i];

        // Determining if the crnt chunk of memory is in use
        if ((*crnt & 1) == 0)
        {
            unsigned short crntSize = sizeOfChunk(crnt);

            if (crntSize == 0)
            {
                if (prev != NULL)
                {
                    // Set prev to 0 values, since we are at the end of myblock
                    setChunk(prev, 0, 0);
                }

                break;
            }
            else
            {
                // Verify that the pointer doesn't refer here. If it does, we know that the location has already been deallocated; therefore, we know that someone attempted to refree a freed up memory slot
                if (crnt + 1 + numBytes(crnt) == ptr && removed == 0)
                {
                    printf("Error: Memory freed already in line %d, %s\n", line, file);
                    return;
                }

                // If prev is not NULL, then we merge the two empty blocks prev and crnt
                if (prev != NULL)
                {
                    int newSize = sizeOfChunk(prev) + sizeOfChunk(crnt);
                    setChunk(prev, 0, newSize);
                }

                i += sizeOfChunk(crnt);
            }

            if (prev == NULL)
            {
                prev = crnt;
            }
        }
        else
        {
            // Checking to see if crnt is equal to ptr
            if (crnt + 1 + numBytes(crnt) == ptr)
            {
                setChunk(crnt, 0, sizeOfChunk(crnt));
                removed++;
            }
            else
            {
                prev = NULL;
                i += sizeOfChunk(crnt);
            }
        }
    }

    // If no blocks of memory have been freed, the following error will print
    if (removed == 0)
    {
        printf("Error: Free error - invalid pointer in line %d, %s\n", line, file);
    }
}

/*
 * numBytes takes in the crnt block of memory that is being evaluated
 * and determines how many bytes of information it uses
 * 
 * With the way that the metadata is set up, it checks the second bit
 * and returns the value of it.
 * 
 * If the block of memory allocates 63 or less bytes, then it will
 * return true.
 * If the block of memory allocates 64 or more bytes, then it will
 * return false.
 * 
 */

unsigned short numBytes(unsigned char *crnt)
{
    return (*crnt >> 1) & 1;
}

/*
 * sizeOfChunk takes in the crnt block of memory that is being evaluated
 * and determines if it is in use or not.
 * 
 * With the way that the metadata is set up, it first shifts over the first
 * bit, which only tells us if the block is in use or not. So, we skip over
 * the first bit. The second bit tells us how large the block of memory is.
 * If the memory allocated is greater than 63 bytes, the second bit is 
 * true, since one byte can only store values less than 64.
 * 
 * If the chunk size is less than or equal to 63, then the function would return 
 * the crnt chunk shifted over by two bits.
 * If the chunk size is greater than or equal to 64, then the function would
 * return the size value of the crnt chunk
 * 
 */

unsigned short sizeOfChunk(unsigned char *crnt)
{
    unsigned short bytesize = (*crnt >> 1) & 1;

    if (bytesize == 0)
    {
        return (*crnt >> 2);
    }
    else
    {
        return (*(unsigned short *)crnt) >> 2;
    }
}

/*
 * setChunk takes in the crnt chunk, an int inuse to set the chunk to allocated
 * or freed, and the size of the new crnt chunk.
 * 
 * setChunk sets the chunk passed in to the values of the parameters passed in. This
 * function is used to both allocate and free chunks depending on the value of the
 * inuse parameter.
 * 
 */

void setChunk(unsigned char *crnt, int inuse, int bytes)
{
    if (bytes < 64)
    {
        *(crnt) = (bytes << 2) + inuse;
    }
    else
    {
        *((short *)crnt) = (bytes << 2) + inuse + 2;
    }
}
