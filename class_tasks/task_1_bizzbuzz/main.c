#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>

//===============================================================================

#define ERROR_CASE(error) case error: fprintf(file, "\n\n" #error "\n\n"); break;

enum ERRORS 
{
    ERROR_NULLPTR_WORD = 1,
};

//===============================================================================

int print_word(char* word);

void print_error(int error, FILE* file);

//===============================================================================


int main(int argc, char* argv[])
{
    if (argc < 2) return 0;

    int count_words = argc - 1;
    int error = 0;

    for (int i = 0; i < count_words; ++i)
    {
        error = print_word(argv[i + 1]);
        if (error)
        {
            print_error(error, stderr);
            return i;
        }
    }

    printf("\n");
    return 0;
}

//===============================================================================

int print_word(char* word)
{
    if (!word) return ERROR_NULLPTR_WORD;

    char* begin_word = word;
    int   len        = strlen(word);

    if (len == 0) return 0;
    if (*word == '-' && len == 1) 
    {
        printf("- ");
        return 0;
    }

    int num_mod_3  = 0;
    int last_digit = 1;

    if (*word == '-') word++;

    while (isdigit(*word))
    {
        last_digit = *word - '0';
        num_mod_3  = (last_digit + num_mod_3) % 3;

        word++;
    }

    if (*word == '.')
    {
        ++word;
        while (*word == '0') ++word;
    }

         if (*word != '\0')                         printf("%s ", begin_word);
    else if (num_mod_3 == 0 && last_digit % 5 == 0) printf("bizzbuzz ");
    else if (num_mod_3 == 0)                        printf("bizz ");
    else if                   (last_digit % 5 == 0) printf("buzz ");
    else                                            printf("%s ", begin_word);

    return 0;
}

void print_error(int error, FILE* file)
{
    assert(file);

    switch (error)
    {
    ERROR_CASE(ERROR_NULLPTR_WORD);
    
    default:
        fprintf(file, "Unknown error!\n\n");
        break;
    }
}
