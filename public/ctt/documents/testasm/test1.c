#include <stdio.h>

void max(int a, int b)
{
    int k = a>=b?a:b;
    putchar(k);putchar('\n');
}

void hello()
{
    printf("hello, world!\n");
}

int main(int argc, char** argv)
{
    hello();
    max(12, 34);
    max(12, 101);
	return 0;
}
