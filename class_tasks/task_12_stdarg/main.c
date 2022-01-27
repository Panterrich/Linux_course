#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>


void stoi(int value);
void my_printf(char* format, ...);

int main()
{
    char c[] = "Hello world";
    char b[] = "Hello \n";
    my_printf("1234 %d %s%s %c\n", 10115, c, b, '!');

    return 0;
}

void my_printf(char* format, ...)
{
    int len = strlen(format);
    
    va_list lst = {};
    va_start(lst, format);

    for (int i = 0; i < len; i++)
    {
        if (format[i] == '%')
        {
            if (i + 1 == len)
            {
                break;
            }

            switch (format[i + 1])
            {
                case 'd':
                    stoi(va_arg(lst, int));
                    i++;
                    break;
                case 's':
                    char* buffer = va_arg(lst, char*);
                    write(STDOUT_FILENO,  buffer, strlen(buffer));
                    i++;
                    break;
                case 'c':
                    char c = va_arg(lst, int);
                    write(STDOUT_FILENO, &c, 1);
                    i++;
                    break;

                default:
                    printf("ERROR: undefined behavior");
                    break;
            }
        }

        else
        {
            write(STDOUT_FILENO, format + i, 1);
        }
    }

    va_end(lst);
}

void stoi(int value)
{
    static char minus = '-';
   // printf("\n%d!!!!\n", value);

    if (value < 0)
    {
        write(STDOUT_FILENO, &minus, 1);
        value = abs(value);
    }

    if (value > 9)
    {
        stoi(value / 10);
    }

    char c = value % 10 + '0';
    write(STDOUT_FILENO, &c, 1);
    
}

