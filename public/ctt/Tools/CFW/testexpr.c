
#include <stdio.h>
#include <stdlib.h>

char* expr(char* text)
{
    int textlen = 0;
    while (text[textlen++] != '\0');
    //
    char* result = malloc(textlen);
    int curi = 0;
    //
    char stack[255] = {'\0'};
    int sp = 1;
    //
    char c;
    int pos = 0;
    while ((c = text[pos++]) != '\0') {
        if (c == '(') {
            stack[sp++] = c;
        } else if (c == ')') {
            while (stack[--sp] != '(') result[curi++] = stack[sp];
        } else if (c == '+' || c == '-' || c == '*' || c == '/' || c == '^') {
            stack[sp++] = c;
        } else {
            result[curi++] = c;
        }
    }
    while (sp >= 0) result[curi++] = stack[--sp];
    for (; curi >= 0; curi--) text[curi] = result[curi];
    free(result);
    return text;
}

int main(int argc, char** argv)
{
    char text[] = "((a+b)*(c/d))";
    printf("%s\n", expr(text));
    return 0;
}
