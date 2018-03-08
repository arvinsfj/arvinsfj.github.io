#include <stdio.h>

int new_strlen(const char *_s)
{
    printf("%s\n", _s);
    return 9;
}

int main(int argc, const char * argv[])
{
    char *str = "hellolazy";
    printf("%d\n", strlen(str));
    return 0;
}
