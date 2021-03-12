#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include "mymalloc.h"

#define malloc(x) mymalloc(x, __FILE__, __LINE__)
#define free(x) myfree(x, __FILE__, __LINE__)

int accomodateNeg(int start, int end)
{
    if ((end - start) < 0)
    {
        end += 1000000;
    }

    return end;
}

/*
 * Allocating one byte and freeing it immediately 120 times
 */

int testA()
{
    struct timeval start, end;

    gettimeofday(&start, NULL);
    int returningStart = start.tv_usec;

    int i = 0;
    while (i < 120)
    {
        char *one = malloc(1);
        free(one);
        i++;
    }

    gettimeofday(&end, NULL);
    int returningEnd = end.tv_usec;

    returningEnd = accomodateNeg(returningStart, returningEnd);

    return (returningEnd - returningStart);
}

/*
 * Allocating 120 bytes individually then freeing each byte individually
 */

int testB()
{
    struct timeval start, end;
    int repetitions = 120;
    char *storage[repetitions];

    gettimeofday(&start, NULL);
    int returningStart = start.tv_usec;

    int i = 0;
    while (i < repetitions)
    {
        storage[i] = malloc(1);
        i++;
    }

    int j = 0;
    while (j < repetitions)
    {
        free(storage[j]);
        j++;
    }

    gettimeofday(&end, NULL);
    int returningEnd = end.tv_usec;

    returningEnd = accomodateNeg(returningStart, returningEnd);

    return (returningEnd - returningStart);
}

/*
 * Randomly malloc or free 1 byte until 120 bytes are allocated
 * 
 * Free all the pointers once 120 bytes are allocated
 * 
 */

int testC()
{
    struct timeval start, end;
    int repetitions = 120;
    char *storage[repetitions];
    int counter = 0;

    gettimeofday(&start, NULL);
    int returningStart = start.tv_usec;

    while (counter < repetitions)
    {
        int random = (rand() % 2);

        if (random == 0)
        {
            storage[counter] = malloc(1);
            counter++;
        }
        else
        {
            if (counter > 0)
            {
                free(storage[counter - 1]);
                counter--;
            }
        }
    }

    for (int i = 0; i < repetitions; i++)
    {
        free(storage[i]);
    }

    gettimeofday(&end, NULL);
    int returningEnd = end.tv_usec;

    returningEnd = accomodateNeg(returningStart, returningEnd);

    return (returningEnd - returningStart);
}

/*
 * 1. Allocate 120 bytes individually
 * 2. Free the second fourth of allocated memory
 * 3. Allocate 10 bytes 120 / 4 times
 * 4. Free each pointer in the order of storage
 * 
 * 
 */

int testD()
{
    struct timeval start, end;
    int repetitions = 120;
    char *storage[repetitions];

    gettimeofday(&start, NULL);
    int returningStart = start.tv_usec;

    int i = 0;
    while (i < repetitions)
    {
        storage[i] = malloc(1);
        i++;
    }

    for (int j = repetitions / 4; j < repetitions / 2; j++)
    {
        free(storage[j]);
    }

    int k = repetitions / 4;
    while (k < repetitions / 2)
    {
        storage[k] = malloc(10);
        k++;
    }

    for (int x = 0; x < repetitions; x++)
    {
        free(storage[x]);
    }

    gettimeofday(&end, NULL);
    int returningEnd = end.tv_usec;

    returningEnd = accomodateNeg(returningStart, returningEnd);

    return (returningEnd - returningStart);
}

/*
 * Allocate 120 bytes of memory then free it from the center
 * to the end then from the start to the center
 */

int testE()
{
    struct timeval start, end;
    int repetitions = 120;
    char *storage[repetitions];

    gettimeofday(&start, NULL);
    int returningStart = start.tv_usec;

    int i = 0;
    while (i < repetitions)
    {
        storage[i] = malloc(1);
        i++;
    }

    for (int j = repetitions / 2; j < repetitions; j++)
    {
        free(storage[j]);
    }

    for (int j = 0; j < repetitions / 2; j++)
    {
        free(storage[j]);
    }

    gettimeofday(&end, NULL);
    int returningEnd = end.tv_usec;

    returningEnd = accomodateNeg(returningStart, returningEnd);

    return (returningEnd - returningStart);
}

double calculateAverage(int *array, int num)
{
    int total = 0;

    for (int i = 0; i < num; i++)
    {
        total += array[i];
    }

    double average = total / num;

    return average;
}

int main()
{
    int totalRepetitions = 5;
    int finalA[totalRepetitions];
    int finalB[totalRepetitions];
    int finalC[totalRepetitions];
    int finalD[totalRepetitions];
    int finalE[totalRepetitions];

    int crnt = 0;
    while (crnt < totalRepetitions)
    {
        int totalA[120];
        int totalB[120];
        int totalC[240];
        int totalD[120];
        int totalE[120];

        int i = 0;
        while (i < 120)
        {
            totalA[i] = testA();
            totalB[i] = testB();
            totalD[i] = testD();
            totalE[i] = testE();
            i++;
        }

        i = 0;
        while (i < 240)
        {
            totalC[i] = testC();
            i++;
        }

        finalA[crnt] = calculateAverage(totalA, 120);
        finalB[crnt] = calculateAverage(totalB, 120);
        finalC[crnt] = calculateAverage(totalC, 240);
        finalD[crnt] = calculateAverage(totalD, 120);
        finalE[crnt] = calculateAverage(totalE, 120);
        crnt++;
    }

    int averageA = calculateAverage(finalA, totalRepetitions);
    int averageB = calculateAverage(finalB, totalRepetitions);
    int averageC = calculateAverage(finalC, totalRepetitions);
    int averageD = calculateAverage(finalD, totalRepetitions);
    int averageE = calculateAverage(finalE, totalRepetitions);

    printf("testA average: %d microseconds\n", averageA);
    printf("testB average: %d microseconds\n", averageB);
    printf("testC average: %d microseconds\n", averageC);
    printf("testD average: %d microseconds\n", averageD);
    printf("testE average: %d microseconds\n", averageE);
}