#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <stdbool.h>
#include <math.h>

struct wordNode
{
    char *word;
    float occurrence;
    struct wordNode *next;
};

struct meanNode
{
    char *fileName1;
    char *fileName2;
    int total;
    float KLD1;
    float KLD2;
    float JSD;
    struct meanNode *next;
};

struct fileNode
{
    pthread_t id;
    char *fileName;
    bool folder;
    int total;
    struct wordNode *wordHead;
    struct fileNode *next;
};

struct args
{
    char *baseDir;
    struct fileNode *fileHead;
    struct meanNode *meanHead;
    pthread_mutex_t lock;
};

// Initializing functions first for better readablity

struct fileNode *findThread(struct args *argsParam);
void iterate(struct fileNode *filePtr, int fd);
void *tokenize(struct args *argsParam);
void *readDirectory(struct args *argsParam);
struct fileNode *mergeSortedList(struct fileNode *a, struct fileNode *b);
void split(struct fileNode *source, struct fileNode **frontRef, struct fileNode **backRef);
void mergeSort(struct fileNode **headRef);
void anal(struct args *argsParam);
void printing(struct args *argsParam);
void freeing(struct args *argsParam);
int main(int argc, char *argv[]);

/*
 * main() is the driver function that calls each function in the program in order to calculate
 * the Jensen-Shannon Distance.
 * 
 * The function first checks if the paramters are valid, including a directory that can be opened.
 * It will then initialize the initial args struct so that each thread created can use the shared
 * linked lists. After each file is read, the function will sort the file linked list in decreasing
 * number of tokens. It will analyze the files and calculate the Jensen-Shannon Distance between each
 * file and put the result in the mean linked list. It will then sort the mean linked list by increasing
 * number of tokens. Finally, it will print out each Jensen-Shannon Distance color-coded and then free all
 * the allocated memory.
 * 
 * @param int argc and char *argv[] as terminal inputs
 * 
 */

int main(int argc, char *argv[])
{
    // Verifying correct number of arguments passed in

    if (argc != 2)
    {
        printf("Error: No arguments passed in\nPlease enter a base directory\n");
        exit(0);
    }

    // Verifying that the passed in directory can be opened

    DIR *dir;
    dir = opendir(argv[1]);

    if (dir == NULL)
    {
        printf("Error: Base directory cannot be opened, exiting\n");
        exit(0);
    }

    closedir(dir);

    // Initial args struct initialization

    struct args *initialArgs = (struct args *)malloc(sizeof(struct args));
    initialArgs->fileHead = NULL;
    initialArgs->meanHead = NULL;
    initialArgs->baseDir = argv[1];

    // Initializing the mutex in the args struct

    if (pthread_mutex_init(&initialArgs->lock, NULL) != 0)
    {
        printf("Error: Mutex initialization failed, exiting");
        exit(0);
    }

    // Calling readDirectory() to read all the files and create the necessary threads

    readDirectory(initialArgs);

    // Verifying that there is data written to the file linked list

    struct fileNode *ptr = initialArgs->fileHead;

    if (ptr == NULL)
    {
        printf("Error: No data, exiting\n");
        exit(0);
    }

    // Joining the threads so that all threads can finish

    struct fileNode *filePtr = initialArgs->fileHead;

    while (filePtr != NULL)
    {
        pthread_join(filePtr->id, NULL);
        filePtr = filePtr->next;
    }

    // Soring the file nodes in decreasing order

    mergeSort(&initialArgs->fileHead);

    // Analyzing each file and calculating the Jensen-Shannon Distance between each file

    anal(initialArgs);

    // Printing the results

    printing(initialArgs);

    // Freeing the allocated memory

    freeing(initialArgs);

    return 0;
}

/*
 * readDirectory() goes through the specified directory, finds all the files, 
 * and creates the necessary threads.
 * 
 * The function first finds the matching directory that the current thread is
 * working with. It attempts to open the directory, and returns an error if the
 * directory cannot be opened. It will then loop through the directory, and create
 * new threads for all the valid files found and add a new file node with the
 * corresponding thread id.
 * 
 * @param struct args with the shared linked lists
 * 
 */

void *readDirectory(struct args *argsParam)
{
    // Finds the current directory and thread that the current thread is working with

    struct fileNode *tempFile = findThread(argsParam);

    // Copies the directory name so that the method knows what directory it is working with

    int dirLength = 0;

    if (tempFile != NULL)
    {
        dirLength = strlen(tempFile->fileName) + 1;
    }
    else
    {
        dirLength = strlen(argsParam->baseDir) + 1;
    }

    char baseDir[dirLength];

    if (tempFile != NULL)
    {
        strcpy(baseDir, tempFile->fileName);
    }
    else
    {
        strcpy(baseDir, argsParam->baseDir);
    }

    // Verify that the current directory can be opened
    // Initializes the dirent and dir variables so that all files in the directory can be found

    struct dirent *dirent;
    DIR *dir;
    dir = opendir(baseDir);

    if (dir == NULL)
    {
        printf("Error: [%s] directory cannot be opened, returning\n", baseDir);
        return 0;
    }

    // Looping through all the file in the current directory

    while ((dirent = readdir(dir)) != NULL)
    {
        char *fileName = dirent->d_name;

        // Skips the "." and ".." files in each directory

        if (strcmp(fileName, ".") != 0 && strcmp(fileName, "..") != 0)
        {
            // Skips all files that are not a directory or files with ASCII text

            if (dirent->d_type == DT_DIR || dirent->d_type == DT_REG)
            {
                // Initializing a new file node

                struct fileNode *newFile = (struct fileNode *)malloc(sizeof(struct fileNode));
                newFile->total = 0;
                newFile->wordHead = NULL;
                newFile->folder = false;
                newFile->next = NULL;

                newFile->fileName = (char *)malloc(sizeof(char) * (strlen(baseDir) + strlen(fileName) + 2));

                strcpy(newFile->fileName, baseDir);
                strcat(newFile->fileName, "/");
                strcat(newFile->fileName, fileName);

                pthread_mutex_lock(&argsParam->lock);

                if (argsParam->fileHead == NULL)
                {
                    argsParam->fileHead = newFile;
                }
                else
                {
                    struct fileNode *ptr = argsParam->fileHead;

                    while (ptr->next != NULL)
                    {
                        ptr = ptr->next;
                    }

                    ptr->next = newFile;
                }

                pthread_mutex_unlock(&argsParam->lock);

                if (dirent->d_type == DT_DIR)
                {
                    // If the file found is a directory, a new thread is created and recurses

                    newFile->folder = true;

                    void *(*readDir)();
                    readDir = readDirectory;

                    int err = pthread_create(&newFile->id, NULL, readDir, argsParam);

                    if (err != 0)
                    {
                        printf("Error: Creating thread unsuccessful, exiting\n");
                        exit(0);
                    }
                }
                else if (dirent->d_type == DT_REG)
                {
                    // If the file found is a ASCII text file, a new thread is created and calls tokenize() to read file contents

                    void *(*token)();
                    token = tokenize;

                    int err = pthread_create(&newFile->id, NULL, token, argsParam);

                    if (err != 0)
                    {
                        printf("Error: Creating thread unsuccessful, exiting\n");
                        exit(0);
                    }
                }

                // locking the mutex so that the file node can be inserted in the list
            }
        }
    }

    closedir(dir);

    return 0;
}

/*
 * tokenize() finds the parameters that iterate() requires, and calls iterate().
 * 
 * The function uses the function findThread() to find the current directory
 * that the thread is working with. It will attempt to open the file found,
 * and if the file is not openable, it will return an error. Then, the function
 * will find the matching file node node that the thread is working with, so that
 * new word nodes can be added to the file node. It will then call iterate() to 
 * read the file.
 * 
 * @param struct args with the shared linked lists
 * 
 */

void *tokenize(struct args *argsParam)
{
    // Finds the current directory and thread that the current thread is working with

    struct fileNode *filePtr = findThread(argsParam);

    // Verifies that the current file can be opened

    int fd = open(filePtr->fileName, O_RDONLY);

    if (fd == -1)
    {
        printf("Error: File [%s] is not accessible, returning\n", filePtr->fileName);
        return 0;
    }

    // Iterates throught the contents of the file and adds word nodes to corresponding file node

    iterate(filePtr, fd);

    return 0;
}

/*
 * iterate() is a recursive function that goes through the current file by character.
 * The function only counts alphabetical characters and dashes (-) as valid characters.
 * 
 * THe function initially allocates a string that can store 15 characters. If the string
 * in the file is more than 15 characters, it reallocates space for another 15 characters.
 * If it encounters a whitespace, it stops reading from the file and inserts the new 
 * token into the corresponding file node's word linked list. If the file already contains
 * the new token, it will free the new token and increment the matched word node. If the file
 * does not contain the token, then the function will insert the new word node into the file's
 * word linked list alphabetically automatically. The function will recurse if it encounters a
 * whitespace, since there are more tokens to be read in the file.
 * 
 * @param struct fileNode as a pointer to the next fileNode
 * @param int fd as the file descriptor number
 * 
 */

void iterate(struct fileNode *filePtr, int fd)
{
    // Allocates space in the memory for a new token

    char *newTok = (char *)malloc(sizeof(char) * 15);

    char c;
    int bytes;
    int i = 0;
    int numRe = 1;
    bool whitespace = false;

    // Reads the data in the current file

    while ((bytes = read(fd, &c, sizeof(c))) > 0)
    {
        if (i > (15 * numRe) - 1)
        {
            // If the length of the current token is greater than 15 characters, newTok allocates more memory

            numRe++;
            newTok = (char *)realloc(newTok, sizeof(char) * (i + 15));

            if (newTok == NULL)
            {
                printf("Error: Failed to realloc string, exiting\n");
                exit(0);
            }
        }

        if (isspace(c))
        {
            // If a whitespace is found, the loop has reached the end of the word and breaks out of the loop

            whitespace = true;
            break;
        }
        else if (isalpha(c) || c == '-')
        {
            // Only alphabetical characters and dashes count as valid charaters

            newTok[i] = tolower(c);
            i++;
        }
    }

    newTok[i] = '\0';

    if (i == 0)
    {
        // If no data has been read, it frees the token malloced and nothing will happen

        free(newTok);
    }
    else
    {
        // Adds one to the file node total since a new token has been found

        filePtr->total++;

        struct wordNode *newWord = (struct wordNode *)malloc(sizeof(struct wordNode));
        newWord->word = newTok;
        newWord->occurrence = 1;
        newWord->next = NULL;

        // Determines if the new token already exists or adds a new word node in the correct place in the word linked list alphabetically

        if (filePtr->wordHead == NULL)
        {
            filePtr->wordHead = newWord;
        }
        else
        {
            struct wordNode *wordPtr = filePtr->wordHead;
            struct wordNode *prev = NULL;
            bool exist = false;
            bool less = false;

            while (wordPtr != NULL)
            {
                if (strcmp(wordPtr->word, newTok) > 0)
                {
                    less = true;
                    break;
                }
                else if (strcmp(wordPtr->word, newTok) == 0)
                {
                    free(newTok);
                    free(newWord);
                    wordPtr->occurrence++;
                    exist = true;
                    break;
                }
                else if (wordPtr->next == NULL)
                {
                    break;
                }

                prev = wordPtr;
                wordPtr = wordPtr->next;
            }

            if (less)
            {
                newWord->next = wordPtr;

                if (prev == NULL)
                {
                    filePtr->wordHead = newWord;
                }
                else
                {
                    prev->next = newWord;
                }
            }
            else if (!exist)
            {
                wordPtr->next = newWord;
            }
        }
    }

    // If a whitespace is found, the method recurses since there is more data to be read

    if (whitespace)
    {
        iterate(filePtr, fd);
    }
}

/*
 * anal() analyzes all the file nodes, and calculates the Jensen-Shannon Distance
 * for each file.
 * 
 * The function utilizes two while loops to make sure that each node in the file
 * linked list is compared with each other. At each comparision, the function will
 * calculate the Kullbek-Leibler Divergences with each word node and get the
 * Jensen-Shannon Distance. Each value is stored in a mean node so that the program
 * can reference the values later.
 * 
 * @param struct args with the file linked list
 * 
 */

void anal(struct args *argsParam)
{
    struct fileNode *filePtr1 = argsParam->fileHead;
    struct fileNode *filePtr2 = argsParam->fileHead;

    // Loops through all the file nodes in the file linked list

    while (filePtr1 != NULL)
    {
        // If the current file is a folder, it will skip it

        if (filePtr1->folder)
        {
            filePtr1 = filePtr1->next;
            continue;
        }

        // Sets the second file pointer to filePtr1->next so that all the files will be compared with each other

        filePtr2 = filePtr1->next;

        while (filePtr2 != NULL)
        {
            // If the current file is a folder, it will skip it

            if (filePtr2->folder)
            {
                filePtr2 = filePtr2->next;
                continue;
            }

            // Initializing a new mean node

            struct meanNode *newMean = (struct meanNode *)malloc(sizeof(struct meanNode));
            newMean->fileName1 = filePtr1->fileName;
            newMean->fileName2 = filePtr2->fileName;
            newMean->total = 0;
            newMean->KLD1 = 0;
            newMean->KLD2 = 0;
            newMean->JSD = 0;
            newMean->next = NULL;

            struct wordNode *wordPtr1 = filePtr1->wordHead;
            struct wordNode *wordPtr2 = filePtr2->wordHead;

            // Loops through all the words between both files being compared and calculates the necessary data

            while (wordPtr1 != NULL || wordPtr2 != NULL)
            {
                // Calculating the data for the Jensen-Shannon Distance between both files

                if (wordPtr1 != NULL && wordPtr2 != NULL)
                {
                    if (strcmp(wordPtr1->word, wordPtr2->word) == 0)
                    {
                        float meanProb = ((wordPtr1->occurrence / filePtr1->total) + (wordPtr2->occurrence / filePtr2->total)) / 2;

                        float KLD1 = (wordPtr1->occurrence / filePtr1->total) * log10((wordPtr1->occurrence / filePtr1->total) / meanProb);

                        float KLD2 = (wordPtr2->occurrence / filePtr2->total) * log10((wordPtr2->occurrence / filePtr2->total) / meanProb);

                        newMean->KLD1 += KLD1;
                        newMean->KLD2 += KLD2;

                        wordPtr1 = wordPtr1->next;
                        wordPtr2 = wordPtr2->next;

                        newMean->total++;
                    }
                    else if (strcmp(wordPtr1->word, wordPtr2->word) > 0)
                    {
                        float meanProb = ((0) + (wordPtr2->occurrence / filePtr2->total)) / 2;

                        float KLD2 = (wordPtr2->occurrence / filePtr2->total) * log10((wordPtr2->occurrence / filePtr2->total) / meanProb);

                        newMean->KLD2 += KLD2;

                        wordPtr2 = wordPtr2->next;

                        newMean->total++;
                    }
                    else if (strcmp(wordPtr1->word, wordPtr2->word) < 0)
                    {
                        float meanProb = ((wordPtr1->occurrence / filePtr1->total) + (0)) / 2;

                        float KLD1 = (wordPtr1->occurrence / filePtr1->total) * log10((wordPtr1->occurrence / filePtr1->total) / meanProb);

                        newMean->KLD1 += KLD1;

                        wordPtr1 = wordPtr1->next;

                        newMean->total++;
                    }
                }
                else if (wordPtr1 == NULL)
                {
                    float meanProb = ((0) + (wordPtr2->occurrence / filePtr2->total)) / 2;

                    float KLD2 = (wordPtr2->occurrence / filePtr2->total) * log10((wordPtr2->occurrence / filePtr2->total) / meanProb);

                    newMean->KLD2 += KLD2;

                    wordPtr2 = wordPtr2->next;

                    newMean->total++;
                }
                else if (wordPtr2 == NULL)
                {
                    float meanProb = ((wordPtr1->occurrence / filePtr1->total) + (0)) / 2;

                    float KLD1 = (wordPtr1->occurrence / filePtr1->total) * log10((wordPtr1->occurrence / filePtr1->total) / meanProb);

                    newMean->KLD1 += KLD1;

                    wordPtr1 = wordPtr1->next;

                    newMean->total++;
                }
            }

            newMean->JSD = (newMean->KLD1 + newMean->KLD2) / 2;

            // Inserts the new mean node in the mena linked list in decreasing order

            if (argsParam->meanHead == NULL)
            {
                argsParam->meanHead = newMean;
            }
            else
            {
                struct meanNode *tempMean = argsParam->meanHead;
                struct meanNode *prevMean = NULL;

                while (tempMean != NULL)
                {
                    if (tempMean->total <= newMean->total)
                    {
                        if (prevMean == NULL)
                        {
                            newMean->next = tempMean;
                            argsParam->meanHead = newMean;
                        }
                        else
                        {
                            prevMean->next = newMean;
                            newMean->next = tempMean;
                        }

                        break;
                    }

                    prevMean = tempMean;
                    tempMean = tempMean->next;
                }

                if (tempMean == NULL)
                {
                    prevMean->next = newMean;
                }
            }

            filePtr2 = filePtr2->next;
        }

        filePtr1 = filePtr1->next;
    }
}

/*
 * findThread() iterates through all the thread nodes and returns the
 * matching current thread.
 * 
 * With the way that the structs are implemented, the current directory
 * that the current thread is working with is unknown. To find the current
 * directory, the directory name is stored in the file node. Finding
 * the matching file node will find the current directory that the 
 * current thread is working with.
 * 
 * @param struct args with the file linked list
 * 
 * @return matching file node
 * 
 */

struct fileNode *findThread(struct args *argsParam)
{
    struct fileNode *filePtr = argsParam->fileHead;

    pthread_t tid = pthread_self();

    bool broken = false;

    // Loops through all the file nodes to find the current directory that the thread is working with

    while (filePtr != NULL)
    {
        if (pthread_equal(filePtr->id, tid))
        {
            broken = true;
            break;
        }

        filePtr = filePtr->next;
    }

    if (argsParam->fileHead != NULL && !broken)
    {
        printf("Error: Thread not found, exiting\n");
        exit(0);
    }

    return filePtr;
}

/*
 * mergeSort() sorts the file linked list in decreasing order.
 * 
 * The function uses a merge sort algorithm to sort the linked list. It calls the
 * functions mergeSortedList() and split().
 * 
 * @param pointer to the head file node
 * 
 */

void mergeSort(struct fileNode **headRef)
{
    struct fileNode *head = *headRef;
    struct fileNode *ptr1;
    struct fileNode *ptr2;

    if ((head == NULL) || (head->next == NULL))
    {
        return;
    }

    split(head, &ptr1, &ptr2);

    mergeSort(&ptr1);
    mergeSort(&ptr2);

    *headRef = mergeSortedList(ptr1, ptr2);
}

struct fileNode *mergeSortedList(struct fileNode *ptr1, struct fileNode *ptr2)
{
    struct fileNode *result = NULL;

    if (ptr1 == NULL)
    {
        return (ptr2);
    }
    else if (ptr2 == NULL)
    {
        return (ptr1);
    }

    if (ptr1->total >= ptr2->total)
    {
        result = ptr1;
        result->next = mergeSortedList(ptr1->next, ptr2);
    }
    else
    {
        result = ptr2;
        result->next = mergeSortedList(ptr1, ptr2->next);
    }

    return (result);
}

void split(struct fileNode *source, struct fileNode **front, struct fileNode **back)
{
    struct fileNode *ptr1 = source->next;
    struct fileNode *ptr2 = source;

    while (ptr1 != NULL)
    {
        ptr1 = ptr1->next;

        if (ptr1 != NULL)
        {
            ptr2 = ptr2->next;
            ptr1 = ptr1->next;
        }
    }

    *front = source;
    *back = ptr2->next;
    ptr2->next = NULL;
}

/*
 * printing() prints the Jensen-Shannon Distance calculations for each file compared.
 * 
 * The function loops through the mean linked list and prints the calculated 
 * Jensen-Shannon Distance for each file comparison. It color codes the results accordingly.
 * 
 * The commented out code prints out each file that the program detects, and prints out
 * each word in the file.
 * 
 * @param struct args with the shared linked lists
 * 
 */

void printing(struct args *argsParam)
{
    // The following code prints out all the files found in the given directory

    // struct fileNode *filePtr = argsParam->fileHead;

    // while (filePtr != NULL)
    // {
    //     if (filePtr->fileName != NULL)
    //     {
    //         printf("file - %s\n", filePtr->fileName);
    //         printf("total - %d\n", filePtr->total);

    //         struct wordNode *wordPtr = filePtr->wordHead;

    //         while (wordPtr != NULL)
    //         {
    //             printf("\tword - %s\n", wordPtr->word);
    //             printf("\t occurrence - %f\n", wordPtr->occurrence);
    //             wordPtr = wordPtr->next;
    //         }
    //     }

    //     filePtr = filePtr->next;
    // }

    struct meanNode *meanPtr = argsParam->meanHead;

    // Loops through all the mean nodes and prints out the Jensen-Shannon Distance for each comparison

    while (meanPtr != NULL)
    {
        if (meanPtr->JSD >= 0 && meanPtr->JSD <= 0.1)
        {
            printf("\033[0;31m");
        }
        else if (meanPtr->JSD > 0.1 && meanPtr->JSD <= 0.15)
        {
            printf("\033[0;33m");
        }
        else if (meanPtr->JSD > 0.15 && meanPtr->JSD <= 0.2)
        {
            printf("\033[0;32m");
        }
        else if (meanPtr->JSD > 0.2 && meanPtr->JSD <= 0.25)
        {
            printf("\033[0;36m");
        }
        else if (meanPtr->JSD > 0.25 && meanPtr->JSD <= 0.3)
        {
            printf("\033[0;34m");
        }
        else
        {
            printf("\033[0m");
        }

        printf("%f \"%s\" and \"%s\"\n", meanPtr->JSD, meanPtr->fileName1, meanPtr->fileName2);
        printf("\033[0m");

        meanPtr = meanPtr->next;
    }
}

/*
 * freeing() frees all the allocated memory.
 * 
 * The function utilizes while loops to iterate through each linked list in the
 * shared struct args and frees all the allocated memory.
 * 
 * @param struct args with the shared linked lists
 * 
 */

void freeing(struct args *argsParam)
{
    struct fileNode *filePtr = argsParam->fileHead;
    struct fileNode *fileTemp = argsParam->fileHead;

    while (filePtr != NULL)
    {
        if (filePtr->wordHead != NULL)
        {
            struct wordNode *wordPtr = filePtr->wordHead;
            struct wordNode *wordTemp = filePtr->wordHead;

            while (wordPtr != NULL)
            {
                free(wordPtr->word);
                wordTemp = wordPtr;
                wordPtr = wordPtr->next;
                free(wordTemp);
            }
        }

        free(filePtr->fileName);

        fileTemp = filePtr;
        filePtr = filePtr->next;

        free(fileTemp);
    }

    struct meanNode *meanPtr = argsParam->meanHead;
    struct meanNode *meanTemp = argsParam->meanHead;

    while (meanPtr != NULL)
    {
        meanTemp = meanPtr;
        meanPtr = meanPtr->next;

        free(meanTemp);
    }

    free(argsParam);
}
