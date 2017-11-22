#include <stdio.h>

int max(int a, int b)
{
    return a>=b?a:b;
}

void hello()
{
    printf("hello, world!\n");
}

int main(int argc, char** argv)
{
	printf("%d, %d\n", 1, 2);
    hello();
    max(12, 34);
	return 0;
}
