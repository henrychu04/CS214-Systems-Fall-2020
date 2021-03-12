#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <pthread.h>
#include <stdbool.h>
#include <ctype.h>

// This code utilizes Menedez's sample code that he provided in his lecture notes

#define BACKLOG 5

struct connection
{
    struct sockaddr_storage addr;
    socklen_t addr_len;
    int fd;
};

int server(char *port);
void *echo(void *arg);
void error(char *msg, int fd, struct connection *c);
char *parseString(int fd, struct connection *c, int msgNum, char *buffer);
bool checkPunc(char *msg);
int numDigits(int length);
void throwPerror(char *msg);

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        printf("Usage: %s [port]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    (void)server(argv[1]);
    return EXIT_SUCCESS;
}

int server(char *port)
{
    if (atoi(port) < 5000 || atoi(port) > 65536)
    {
        fprintf(stderr, "Invalid port number\n");
        return -1;
    }

    struct addrinfo hint, *address_list, *addr;
    struct connection *con;
    int error, sfd;
    pthread_t tid;

    // initialize hints
    memset(&hint, 0, sizeof(struct addrinfo));
    hint.ai_family = AF_UNSPEC;
    hint.ai_socktype = SOCK_STREAM;
    hint.ai_flags = AI_PASSIVE;
    // setting AI_PASSIVE means that we want to create a listening socket

    // get socket and address info for listening port
    // - for a listening socket, give NULL as the host name (because the socket is on
    //   the local host)
    error = getaddrinfo(NULL, port, &hint, &address_list);
    if (error != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(error));
        return -1;
    }

    // attempt to create socket
    for (addr = address_list; addr != NULL; addr = addr->ai_next)
    {
        sfd = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);

        // if we couldn't create the socket, try the next method
        if (sfd == -1)
        {
            continue;
        }

        // if we were able to create the socket, try to set it up for
        // incoming connections;
        //
        // note that this requires two steps:
        // - bind associates the socket with the specified port on the local host
        // - listen sets up a queue for incoming connections and allows us to use accept
        if ((bind(sfd, addr->ai_addr, addr->ai_addrlen) == 0) && (listen(sfd, BACKLOG) == 0))
        {
            break;
        }

        // unable to set it up, so try the next method
        close(sfd);
    }

    if (addr == NULL)
    {
        // we reached the end of result without successfuly binding a socket
        fprintf(stderr, "Could not bind\n");
        return -1;
    }

    freeaddrinfo(address_list);

    // at this point sfd is bound and listening
    printf("Waiting for connection\n");

    for (;;)
    {
        // create argument struct for child thread
        con = malloc(sizeof(struct connection));
        con->addr_len = sizeof(struct sockaddr_storage);
        // addr_len is a read/write parameter to accept
        // we set the initial value, saying how much space is available
        // after the call to accept, this field will contain the actual address length

        // wait for an incoming connection
        con->fd = accept(sfd, (struct sockaddr *)&con->addr, &con->addr_len);
        // we provide
        // sfd - the listening socket
        // &con->addr - a location to write the address of the remote host
        // &con->addr_len - a location to write the length of the address
        //
        // accept will block until a remote host tries to connect
        // it returns a new socket that can be used to communicate with the remote
        // host, and writes the address of the remote hist into the provided location

        // if we got back -1, it means something went wrong
        if (con->fd == -1)
        {
            perror("accept");
            continue;
        }

        // spin off a worker thread to handle the remote connection
        error = pthread_create(&tid, NULL, echo, con);

        // if we couldn't spin off the thread, clean up and wait for another connection
        if (error != 0)
        {
            fprintf(stderr, "Unable to create thread: %d\n", error);
            close(con->fd);
            free(con);
            continue;
        }

        // otherwise, detach the thread and wait for the next connection request
        pthread_detach(tid);
    }

    // never reach here
    return 0;
}

void *echo(void *arg)
{
    char host[100], port[10];
    struct connection *c = (struct connection *)arg;
    int err, nread;
    int r = -1;

    // find out the name and port of the remote host
    err = getnameinfo((struct sockaddr *)&c->addr, c->addr_len, host, 100, port, 10, NI_NUMERICSERV);
    // we provide:
    // the address and its length
    // a buffer to write the host name, and its length
    // a buffer to write the port (as a string), and its length
    // flags, in this case saying that we want the port as a number, not a service name
    if (err != 0)
    {
        fprintf(stderr, "getnameinfo: %s", gai_strerror(err));
        close(c->fd);
        return NULL;
    }

    printf("[%s:%s] connection\n", host, port);

    // Actual code starts here

    // Initial 'Knock, knock.' write

    r = write(c->fd, "REG|13|Knock, knock.|", 21);

    if (r < 0)
    {
        throwPerror("Error writing to socket");
    }

    // Initializing all the flags and counters for input checks

    char ch;
    char buf[4];
    int i = 0;
    int j = 0;
    int bar = 0;
    int numRe = 0;
    int msgNum = 1;
    int numLength = 0;
    int numValue = 0;
    int stringLength = 0;
    bool reg = false;
    bool numSet = false;

    // Allocating space for a new string to be read

    char *newString = (char *)malloc(sizeof(char) * 15);

    while ((nread = read(c->fd, &ch, 1)) > 0)
    {
        // buf keeps track of the 3 most recent characters read in
        // buf is used to verify if the new string has a valid 'REG' or 'ERR' at the beginning

        if (i == 3)
        {
            buf[0] = buf[1];
            buf[1] = buf[2];
            buf[2] = ch;
            buf[3] = '\0';
        }
        else if (i == 2)
        {
            buf[2] = ch;
            buf[3] = '\0';
            i++;
        }
        else if (i == 1)
        {
            buf[1] = ch;
            buf[2] = '\0';
            i++;
        }
        else if (i == 0)
        {
            buf[0] = ch;
            buf[1] = '\0';
            i++;
        }

        // If the length of the current word is greater than 15 characters, newString allocates more memory

        if (j > (15 * numRe) - 2)
        {
            numRe++;
            newString = (char *)realloc(newString, sizeof(char) * (j + 15));

            if (newString == NULL)
            {
                printf("Error: Failed to realloc string, exiting\n");
                exit(0);
            }
        }

        if (ch != '\n')
        {
            newString[j] = ch;
            newString[j + 1] = '\0';
            j++;
        }

        // Verifies that the string starts with 'REG' or 'ERR'

        if (strcmp(buf, "REG") == 0 || strcmp(buf, "ERR") == 0)
        {
            if (reg && bar != 3)
            {
                char num = msgNum + '0';
                char errString[] = {'M', num, 'C', 'T', '\0'};
                error(errString, c->fd, c);
                break;
            }

            if (strcmp(buf, "ERR") == 0)
            {
                break;
            }

            reg = true;
        }

        // Counts the number of bars in the string

        if (ch == '|')
        {
            bar++;
        }

        if (ch != '|' && bar == 1)
        {
            numLength++;
        }
        else if (bar == 2 && !numSet)
        {
            // Parses the number that is inbetween the first and second bar

            numSet = true;
            char numString[numLength + 1];
            int k = 4;
            int l = 0;

            while (newString[k] != '|')
            {
                numString[l] = newString[k];
                k++;
                l++;
            }

            numString[l] = '\0';

            numValue = atoi(numString);
        }
        else if (bar == 2)
        {
            // Counts the total number of characters of the actual string

            stringLength++;
        }

        // Once a valid string is read in with 3 bars, verifies that the lenght is correct and parses the string

        if (bar == 3 && reg && (strlen(newString) - 6 - numLength) == numValue)
        {
            newString[j] = '\0';

            char *parsedString = parseString(c->fd, c, msgNum, newString);

            if (parsedString == NULL)
            {
                break;
            }

            printf("parsed string is '%s'\n", parsedString);

            // Verifies that the first string is 'Who's there?'

            if (msgNum == 1 && strcmp(parsedString, "Who's there?") == 0)
            {
                msgNum += 2;
                r = write(c->fd, "REG|6|Kanga.|", 13);

                if (r < 0)
                {
                    throwPerror("Error writing to socket");
                }

                free(parsedString);
            }

            // Verifies that the second string is 'Kanga, who?'

            else if (msgNum == 3 && strcmp(parsedString, "Kanga, who?") == 0)
            {
                msgNum += 2;
                r = write(c->fd, "REG|24|Actually, it's kangaroo!|", 32);

                if (r < 0)
                {
                    throwPerror("Error writing to socket");
                }

                free(parsedString);
            }

            // Verifies that the third string ends with a punctuation

            else if (msgNum == 5 && checkPunc(parsedString))
            {
                msgNum += 2;
                free(parsedString);
                break;
            }

            // Incorrect format

            else
            {
                char num = msgNum + '0';
                char errString[] = {'M', num, 'C', 'T', '\0'};

                error(errString, c->fd, c);
                break;
            }

            // Resets all flags and counters

            j = 0;
            newString[j] = '\0';

            reg = false;
            bar = 0;
            numSet = false;
            numLength = 0;
        }

        // if the length of the string does not match the number given, an error is thrown

        else if (bar == 3 && reg && (strlen(newString) - 6 - numLength) != numValue)
        {
            char num = msgNum + '0';
            char errString[] = {'M', num, 'L', 'N', '\0'};

            error(errString, c->fd, c);
            break;
        }

        // if the format of the string is incorrect, an error is thrown

        else if (!reg && bar == 3)
        {
            char num = msgNum + '0';
            char errString[] = {'M', num, 'C', 'T', '\0'};

            error(errString, c->fd, c);
            break;
        }
    }

    printf("[%s:%s] got EOF\n", host, port);

    free(newString);

    close(c->fd);
    free(c);
    return NULL;
}

char *parseString(int fd, struct connection *c, int msgNum, char *msgString)
{
    // Sets the number in the error message to the correct number

    char num = msgNum + '0';

    char LN[] = {'M', num, 'L', 'N', '\0'};
    char FT[] = {'M', num, 'F', 'T', '\0'};

    int i = 0;
    int j = 0;
    int numRe = 1;
    char *newNum = (char *)malloc(sizeof(char) * 15);

    for (i = 4; i < strlen(msgString); i++)
    {
        // If the length of the current token is greater than 15 characters, newNum allocates more memory

        if (j > (15 * numRe) - 1)
        {
            numRe++;
            newNum = (char *)realloc(newNum, sizeof(char) * (j + 15));

            if (newNum == NULL)
            {
                printf("Error: Failed to realloc string, exiting\n");
                exit(0);
            }
        }

        if (!isdigit(msgString[i]) && msgString[i] != '|')
        {
            free(newNum);
            error(LN, fd, c);
            return NULL;
        }

        if (msgString[i] == '|')
        {
            i++;
            break;
        }

        newNum[j] = msgString[i];
        j++;
    }

    newNum[j] = '\0';
    int newNumParsed = atoi(newNum);

    if (newNumParsed == 0)
    {
        free(newNum);
        error(FT, fd, c);
        return NULL;
    }

    free(newNum);

    numRe = 1;
    j = 0;
    char *newString = (char *)malloc((sizeof(char) * newNumParsed) + 1);

    for (; i < strlen(msgString); i++)
    {
        // If the length of the current word is greater than the given length, newString allocates more memory

        if (j > (newNumParsed * numRe) - 1)
        {
            numRe++;
            newString = (char *)realloc(newString, sizeof(char) * (j + newNumParsed));

            if (newString == NULL)
            {
                printf("Error: Failed to realloc string, exiting\n");
                exit(0);
            }
        }

        if (msgString[i] == '|')
        {
            i++;
            break;
        }

        newString[j] = msgString[i];
        j++;
    }

    if (j != newNumParsed)
    {
        free(newString);
        error(LN, fd, c);
        return NULL;
    }

    newString[j] = '\0';

    return newString;
}

// checks if the end of the input string is a punctuation character

bool checkPunc(char *msg)
{
    if (msg[strlen(msg) - 1] == '.' || msg[strlen(msg) - 1] == '!' || msg[strlen(msg) - 1] == '?' || msg[strlen(msg) - 1] == ';' || msg[strlen(msg) - 1] == ',' || msg[strlen(msg) - 1] == ':')
    {
        return true;
    }
    else
    {
        return false;
    }
}

// Throws an error accordingly and writes to the client

void error(char *msg, int fd, struct connection *c)
{
    char error[10] = "ERR|";
    strcat(error, msg);
    strcat(error, "|");
    int r = write(fd, error, 9);

    if (r < 0)
    {
        throwPerror("Error writing to socket");
    }
}

// Counts the number of digits in a given number

int numDigits(int num)
{
    long long n = num;
    int count = 0;

    while (n != 0)
    {
        n /= 10;
        count++;
    }

    return count;
}

// Throws an error

void throwPerror(char *msg)
{
    perror(msg);

    exit(0);
}