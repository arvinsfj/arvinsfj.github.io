#include <stdio.h>

static long tmp;
static int k = 0;

void of_test()
{
    char a = 8;
    *((long*)(&a+9)) = tmp;
}

void of_data()
{
    char a = 2;
    tmp = *((long*)(&a+9));
    *((long*)(&a+9)) = of_test;
}

int main(int argc, const char * argv[])
{
    of_data();
    return 0;
}
