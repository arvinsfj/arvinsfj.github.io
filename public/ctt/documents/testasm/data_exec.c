#include <stdio.h>
#include <string.h>

static unsigned char sc[16] = {'a'};

//./test `perl -e 'print "\x55\x48\x89\xe5\xe8\xd7\xfe\xff\xff\x5d\xc3\x0f\x1f\x44\x00\x00"'`

void mia()
{
	 printf("%d\n", 'a');
}

void mic()
{
    ((void (*)())sc)();
}

int main(int argc, char** argv)
{
    memcpy(sc, argv[1], 16);
	mia();
    mic();
	return 0;
}
