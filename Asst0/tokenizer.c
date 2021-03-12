#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

typedef struct T
{
    char *token;
    int crnt;
    int length;
} T;

enum type
{
    WORD,
    DECIMAL,
    OCTAL,
    HEX,
    FLOAT,
    FLOATPOINT,
    OPERATOR,
    SIZEOF
};

/*
 * Helper functions that are necessary for the if
 * statement conditionals in the parse, parseOp, 
 * and check functions.
 * 
 * 
 * hexChar:
 * Verifies that the char input is a hex character.
 * If not, it returns false.
 * 
 * checkOp:
 * Verifies that the char input is one of the valid
 * operators given. If not, it returns false.
 * 
 * checkInvalid:
 * Verifies if the char input is not an alpha character
 * or a digit, and checks with checkOp. If it does not
 * meet these conditions, it returns true. Otherwise,
 * it returns false.
 * 
 * checkSizeof:
 * Checks if either the next token in the string or the
 * token passed in is a sizeof token. If it is true, 
 * it will return true. Otherwise, it will return false.
 */

bool hexChar(char c)
{
    if (isdigit(c))
    {
        return true;
    }

    if (c >= 'a' && c <= 'f')
    {
        return true;
    }

    if (c >= 'A' && c <= 'F')
    {
        return true;
    }

    return false;
}

bool checkOp(char c)
{
    if (c == '(' || c == ')' || c == '[' || c == ']' || c == '.' || c == '-' || c == ',' || c == ',' || c == '!' || c == '~' || c == '>' || c == '<' || c == '^' || c == '|' || c == '+' || c == '/' || c == '&' || c == '?' || c == ':' || c == '=' || c == '%' || c == '*')
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool checkInvalid(char c)
{
    if (!isalpha(c) && !isdigit(c) && !checkOp(c))
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool checkSizeof(struct T *tk, char *s)
{
    if (tk != NULL && tk->crnt < tk->length - 5)
    {
        if (tk->token[tk->crnt] == 's' && tk->token[tk->crnt + 1] == 'i' && tk->token[tk->crnt + 2] == 'z' && tk->token[tk->crnt + 3] == 'e' && tk->token[tk->crnt + 4] == 'o' && tk->token[tk->crnt + 5] == 'f')
        {
            return true;
        }
    }
    else if (s != NULL && strlen(s) > 5)
    {
        if (s[0] == 's' && s[1] == 'i' && s[2] == 'z' && s[3] == 'e' && s[4] == 'o' && s[5] == 'f')
        {
            return true;
        }
    }

    return false;
}

/*
 * parse returns the next token in the string.
 * 
 * Checks if the token is a WORD, DECIMAL INTEGER,
 * OCTAL INTEGER, HEXADECIMAL INTEGER, FLOATING POINT
 * FLOAT, SIZEOF, or OPERATOR and returns the correct amount
 * of chars in the token.
 * 
 * 
 * Error handling:
 * 
 * If an error occurs while allocating a new returning
 * string, the function will print "Error: Failed to 
 * allocate new returning, exiting" and exit the program.
 */

char *parse(T *tk)
{
    char *returning = (char *)malloc(100);

    if (returning == NULL)
    {
        printf("Error: Failed to allocate new returning, exiting\n");
        exit(0);
    }

    int i = 0;

    while (tk->crnt < tk->length)
    {
        // printf("crnt is %c\n", tk->token[tk->crnt]);
        // if the crnt pointer is a WHITESPACE, it returns the token
        if (isspace(tk->token[tk->crnt]))
        {
            tk->crnt++;
            returning[i] = '\0';
            return returning;
        }

        returning[i++] = tk->token[tk->crnt];

        // checking if the crnt pointer is a HEXADECIMAL
        if (tk->token[tk->crnt] == '0' && (tk->token[tk->crnt + 1] == 'X' || tk->token[tk->crnt + 1] == 'x'))
        {
            if (hexChar(tk->token[tk->crnt + 2]))
            {
                returning[i++] = tk->token[tk->crnt + 1];
                tk->crnt += 2;

                while (hexChar(tk->token[tk->crnt]) && tk->crnt < tk->length)
                {
                    returning[i++] = tk->token[tk->crnt];
                    tk->crnt++;
                }

                returning[i] = '\0';

                if (isspace(tk->token[tk->crnt]))
                {
                    tk->crnt++;
                }

                return returning;
            }
        }

        // checking if the crnt pointer is a DIGIT
        if (isdigit(tk->token[tk->crnt]))
        {
            tk->crnt++;
            bool e = false;
            int period = 0;

            while (tk->crnt < tk->length)
            {
                if (isspace(tk->token[tk->crnt]))
                {
                    tk->crnt++;
                    break;
                }

                if (checkInvalid(tk->token[tk->crnt]))
                {
                    break;
                }

                // checking if the crnt pointer is a FLOATING POINT
                if (checkOp(tk->token[tk->crnt]))
                {
                    if (tk->token[tk->crnt] == '.' && tk->crnt < tk->length && isdigit(tk->token[tk->crnt + 1]) && !isalpha(returning[0]))
                    {
                        period++;
                        returning[i++] = tk->token[tk->crnt];
                        tk->crnt++;
                        continue;
                    }

                    returning[i] = '\0';
                    return returning;
                }
                // checking if the crnt pointer is a FLOAT
                else if (period == 1 && tk->crnt < tk->length && (tk->token[tk->crnt] == 'e' || tk->token[tk->crnt] == 'E') && (tk->token[tk->crnt + 1] == '+' || tk->token[tk->crnt + 1] == '-'))
                {
                    e = true;
                    break;
                }
                // checking if the crnt pointer is not a FLOAT
                else if (isdigit(returning[0]) && isalpha(tk->token[tk->crnt]) && (tk->token[tk->crnt] != 'e' || tk->token[tk->crnt] != 'E'))
                {
                    returning[i] = '\0';
                    return returning;
                }

                // specifically for edge cases with two periods
                // for example, "1.1.1"
                if (period == 2)
                {
                    returning[i - 1] = '\0';
                    tk->crnt--;
                    return returning;
                }

                if (checkInvalid(tk->token[tk->crnt]))
                {
                    returning[i] = '\0';
                    return returning;
                }

                returning[i++] = tk->token[tk->crnt];
                tk->crnt++;
            }

            if (e)
            {
                // validating that the crnt pointer is actually a FLOAT
                if ((tk->token[tk->crnt + 1] == '-' || tk->token[tk->crnt + 1] == '+') && isdigit(tk->token[tk->crnt + 2]))
                {
                    returning[i++] = tk->token[tk->crnt];
                    tk->crnt++;
                    returning[i++] = tk->token[tk->crnt];
                    tk->crnt++;

                    while (isdigit(tk->token[tk->crnt]))
                    {
                        returning[i++] = tk->token[tk->crnt];
                        tk->crnt++;
                    }

                    if (isspace(tk->token[tk->crnt]))
                    {
                        tk->crnt++;
                    }
                }
            }

            returning[i] = '\0';
            return returning;
        }

        // checking if the crnt pointer is an OPERATOR
        if (checkOp(tk->token[tk->crnt]))
        {
            if (returning[0] != '\0' && !checkOp(returning[0]))
            {
                returning[i - 1] = '\0';
                return returning;
            }

            if (checkOp(tk->token[tk->crnt + 1]))
            {
                tk->crnt++;
                while (checkOp(tk->token[tk->crnt]))
                {
                    returning[i++] = tk->token[tk->crnt];
                    tk->crnt++;
                }

                tk->crnt--;
            }

            returning[i] = '\0';
            tk->crnt++;

            if (isspace(tk->token[tk->crnt]))
            {
                tk->crnt++;
            }

            return returning;
        }

        // checking if the crnt pointer is an invalid OPERATOR
        if (checkInvalid(tk->token[tk->crnt]))
        {
            if (!checkInvalid(returning[0]))
            {
                returning[i - 1] = '\0';
                return returning;
            }

            while (tk->crnt < tk->length && checkInvalid(tk->token[tk->crnt]))
            {
                if (isspace(tk->token[tk->crnt]))
                {
                    tk->crnt++;
                    break;
                }

                returning[i++] = tk->token[tk->crnt];
                tk->crnt++;
            }

            returning[i - 1] = '\0';

            if (isspace(tk->token[tk->crnt]))
            {
                tk->crnt++;
            }

            return returning;
        }

        // checking if the crnt pointer is SIZEOF
        if (i == 1 && checkSizeof(tk, NULL))
        {
            returning[i] = tk->token[tk->crnt + 1];
            returning[i + 1] = tk->token[tk->crnt + 2];
            returning[i + 2] = tk->token[tk->crnt + 3];
            returning[i + 3] = tk->token[tk->crnt + 4];
            returning[i + 4] = tk->token[tk->crnt + 5];
            returning[i + 5] = '\0';

            tk->crnt += 6;

            if (isspace(tk->token[tk->crnt]))
            {
                tk->crnt++;
            }

            return returning;
        }

        tk->crnt++;
    }

    // if none of the if statement conditions are met, a WORD is returned
    if (tk->crnt == tk->length)
    {
        returning[i] = '\0';
        return returning;
    }
    else
    {
        returning[0] = '\0';
        return returning;
    }
}

/*
 * check returns the enum value of the type of string
 * that is passed in. The main function uses the enum
 * returned to print out the correct value for the
 * crnt token.
 */

enum type check(char *tk)
{
    int i = 0;

    if (isdigit(tk[0]))
    {
        if (strlen(tk) > 2 && tk[0] == '0' && (tk[1] == 'X' || tk[1] == 'x'))
        {
            i = 2;
            while (i < strlen(tk))
            {
                if (!hexChar(tk[i]))
                {
                    return WORD;
                }

                i++;
            }

            if (i == strlen(tk))
            {
                return HEX;
            }
        }
        else
        {
            bool e = false;
            bool period = false;

            for (int j = 0; j < strlen(tk); j++)
            {
                if ((tk[j] == 'e' || tk[j] == 'E') && (tk[j + 1] == '-' || tk[j + 1] == '+'))
                {
                    i = j;
                    e = true;
                    break;
                }

                if (tk[j] == '.')
                {
                    period = true;
                }
            }

            if (e)
            {
                i += 2;

                while (isdigit(tk[i]) && i < strlen(tk))
                {
                    i++;
                }

                if (i == strlen(tk))
                {
                    return FLOAT;
                }
            }
            else if (period)
            {
                return FLOATPOINT;
            }
            else
            {
                bool octal = true;

                if (tk[i] - '0' == 0)
                {
                    while (isdigit(tk[i]))
                    {
                        if (tk[i] - '0' > 8)
                        {
                            octal = false;
                            break;
                        }

                        i++;
                    }
                }
                else
                {
                    octal = false;
                }

                if (octal)
                {
                    return OCTAL;
                }
                else if (!octal)
                {
                    return DECIMAL;
                }
            }
        }
    }
    else if (isalpha(tk[0]))
    {
        if (checkSizeof(NULL, tk))
        {
            return SIZEOF;
        }
        else
        {
            return WORD;
        }
    }

    return OPERATOR;
}

/*
 * parseOp takes in the string OPERATOR token and prints
 * out the corresponding name of the OPERATOR.
 * 
 * If any invalid operators are passed in, the function
 * will print "Error: Invalid operator: " with the 
 * corresponding operator.
 */

void parseOp(char *s)
{
    for (int i = 0; i < strlen(s); i++)
    {
        switch (s[i])
        {
        case '(':
            printf("left parenthesis: \"%c\"\n", s[i]);
            break;
        case ')':
            printf("right parenthesis: \"%c\"\n", s[i]);
            break;
        case '[':
            printf("left bracket: \"%c\"\n", s[i]);
            break;
        case ']':
            printf("right bracket: \"%c\"\n", s[i]);
            break;
        case '.':
            printf("structure member: \"%c\"\n", s[i]);
            break;
        case '~':
            printf("1s complement: \"%c\"\n", s[i]);
            break;
        case '?':
            printf("conditional true: \"%c\"\n", s[i]);
            break;
        case ':':
            printf("conditional false: \"%c\"\n", s[i]);
            break;
        case ',':
            printf("comma: \"%c\"\n", s[i]);
            break;
        case '!':
            if (i < strlen(s) && s[i + 1] == '=')
            {
                printf("inequality test: \"%c%c\"\n", s[i], s[i + 1]);
                i++;
                break;
            }
            printf("negate: \"%c\"\n", s[i]);
            break;
        case '^':
            if (i < strlen(s) && s[i + 1] == '=')
            {
                printf("bitwise XOR equals: \"%c%c\"\n", s[i], s[i + 1]);
                i++;
                break;
            }
            printf("bitwise XOR: \"%c\"\n", s[i]);
            break;
        case '/':
            if (i < strlen(s) && s[i + 1] == '=')
            {
                printf("divide equals: \"%c%c\"\n", s[i], s[i + 1]);
                i++;
                break;
            }
            printf("division: \"%c\"\n", s[i]);
            break;
        case '=':
            if (i < strlen(s) && s[i + 1] == '=')
            {
                printf("equality test: \"%c%c\"\n", s[i], s[i + 1]);
                i++;
                break;
            }
            printf("assignment: \"%c\"\n", s[i]);
            break;
        case '*':
            if (i < strlen(s) && s[i + 1] == '=')
            {
                printf("times equals: \"%c%c\"\n", s[i], s[i + 1]);
                i++;
                break;
            }
            printf("multiply/dereference operator: \"%c\"\n", s[i]);
            break;
        case '%':
            if (i < strlen(s) && s[i + 1] == '=')
            {
                printf("mod equals: \"%c%c\"\n", s[i], s[i + 1]);
                i++;
                break;
            }
            printf("Error: Invalid operator: \"%c\"\n", s[i]);
            break;
        case '|':
            if (i < strlen(s))
            {
                if (s[i + 1] == '|')
                {
                    printf("logical OR: \"%c%c\"\n", s[i], s[i + 1]);
                    i++;
                    break;
                }
                else if (s[i + 1] == '=')
                {
                    printf("bitwise OR equals: \"%c%c\"\n", s[i], s[i + 1]);
                    i++;
                    break;
                }
            }
            printf("bitwise OR: \"%c\"\n", s[i]);
            break;
        case '+':
            if (i < strlen(s))
            {
                if (s[i + 1] == '+')
                {
                    printf("increment: \"%c%c\"\n", s[i], s[i + 1]);
                    i++;
                    break;
                }
                else if (s[i + 1] == '=')
                {
                    printf("plus equals: \"%c%c\"\n", s[i], s[i + 1]);
                    i++;
                    break;
                }
            }
            printf("addition: \"%c\"\n", s[i]);
            break;
        case '&':
            if (i < strlen(s))
            {
                if (s[i + 1] == '&')
                {
                    printf("logical AND: \"%c%c\"\n", s[i], s[i + 1]);
                    i++;
                    break;
                }
                else if (s[i + 1] == '=')
                {
                    printf("bitwise AND equals: \"%c%c\"\n", s[i], s[i + 1]);
                    i++;
                    break;
                }
            }
            printf("AND/address operator: \"%c\"\n", s[i]);
            break;
        case '-':
            if (i < strlen(s))
            {
                if (s[i + 1] == '>')
                {
                    printf("structure pointer: \"%c%c\"\n", s[i], s[i + 1]);
                    i++;
                    break;
                }
                else if (s[i + 1] == '-')
                {
                    printf("decrement: \"%c%c\"\n", s[i], s[i + 1]);
                    i++;
                    break;
                }
                else if (s[i + 1] == '=')
                {
                    printf("minus equals: \"%c%c\"\n", s[i], s[i + 1]);
                    i++;
                    break;
                }
            }
            printf("minus/subtract operator: \"%c\"\n", s[i]);
            break;
        case '>':
            if (i < strlen(s))
            {
                if ((i < strlen(s) - 1) && s[i + 1] == '>' && s[i + 2] == '=')
                {
                    printf("shift right equals: \"%c%c%c\"\n", s[i], s[i + 1], s[i + 2]);
                    i += 2;
                    break;
                }
                else if (s[i + 1] == '>')
                {
                    printf("shift right: \"%c%c\"\n", s[i], s[i + 1]);
                    i++;
                    break;
                }
                else if (s[i + 1] == '=')
                {
                    printf("less than or equal test: \"%c%c\"\n", s[i], s[i + 1]);
                    i++;
                    break;
                }
            }
            printf("greater than test: \"%c\"\n", s[i]);
            break;
        case '<':
            if (i < strlen(s))
            {
                if ((i < strlen(s) - 1) && s[i + 1] == '<' && s[i + 2] == '=')
                {
                    printf("shift left equals: \"%c%c%c\"\n", s[i], s[i + 1], s[i + 2]);
                    i += 2;
                    break;
                }
                else if (s[i + 1] == '<')
                {
                    printf("shift left: \"%c%c\"\n", s[i], s[i + 1]);
                    i++;
                    break;
                }
                else if (s[i + 1] == '=')
                {
                    printf("greater than or equal test: \"%c%c\"\n", s[i], s[i + 1]);
                    i++;
                    break;
                }
            }
            printf("less than test: \"%c\"\n", s[i]);
            break;
        default:
            printf("Error: Invalid operator: \"%c\"\n", s[i]);
            break;
        }
    }
}

/*
 * load reads the string input from the command line, allocates
 * a new struct T, and sets the struct T values accordingly.
 * 
 * 
 * Error handling:
 * If the input is NULL, then the function will return NULL.
 * 
 * If an error occurs while allocating a new T struct, the
 * function will print "Error: Failed to allocate new T, exiting"
 * and exit the program.
 * 
 * If an error occurs while allocating a new token, the function
 * will print "Error: Failed to allocate new token, exiting" and
 * exit the program.
 */

T *load(char *s)
{
    if (s == NULL)
    {
        return NULL;
    }
    else
    {
        T *new = (T *)malloc(sizeof(T));

        if (new == NULL)
        {
            printf("Error: Failed to allocate new T, exiting\n");
            exit(0);
        }

        new->length = strlen(s);
        new->crnt = 0;
        new->token = (char *)malloc(new->length + 1);

        if (new->token == NULL)
        {
            printf("Error: Failed to allocate new token, exiting\n");
            exit(0);
        }

        for (int i = 0; i < new->length; i++)
        {
            new->token[i] = s[i];
        }

        new->token[new->length] = '\0';

        return new;
    }
}

/*
 * main takes in a string argument and parses the tokens
 * contained in the string. Depending on what tokens string
 * contains, it will print out its name and corresponding value.
 * 
 * 
 * Error handling:
 * If no string is passed in as an argument, or too many
 * arguments are passed in, the function will print 
 * "Incorrect format, returning" and return 0.
 * 
 * If there is an error loading the string into the T struct,
 * the function will print "Error loading, returning" and
 * return 0.
 * 
 * If there is an error in parsing a token, the function
 * will print "Error: Invalid token".
 */

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("Incorrect format, returning\n");
        return 0;
    }

    T *total = load(argv[1]);

    if (total == NULL)
    {
        printf("Error loading, returning\n");
        return 0;
    }

    while (1)
    {
        char *tk;
        tk = parse(total);

        if (strlen(tk) == 0)
        {
            free(tk);
            break;
        }
        else
        {
            switch (check(tk))
            {
            case WORD:
                printf("word: \"%s\"\n", tk);
                break;
            case HEX:
                printf("hexadecimal integer: \"%s\"\n", tk);
                break;
            case DECIMAL:
                printf("decimal integer: \"%s\"\n", tk);
                break;
            case OCTAL:
                printf("octal integer: \"%s\"\n", tk);
                break;
            case FLOATPOINT:
                // since FLOATPOINT and FLOAT are considered to be the same
                // printf("floating point: \"%s\"\n", tk);
                // break;
            case FLOAT:
                printf("float: \"%s\"\n", tk);
                break;
            case SIZEOF:
                printf("sizeof: \"%s\"\n", tk);
                break;
            case OPERATOR:
                parseOp(tk);
                break;
            default:
                printf("Error: Invalid token\n");
                break;
            }
        }

        free(tk);
    }

    free(total->token);
    free(total);
}
