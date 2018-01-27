#include <stdio.h>

void mia()
{
	 static char vk = 'a';
	 printf("%d\n", vk);
}

void miba()
{
    mia();
}

void mic()
{
    /*
    for (int i = 0; i < mic-miba; i++) {
        printf("\\x%02x", ((unsigned char*)miba)[i]);
    }*/
    void* sc = "\x55\x48\x89\xe5\xe8\x49\xff\xff\xff\x5d\xc3\x0f\x1f\x44\x00\x00";
    ((void (*)())sc)();
}

int main(int argc, char** argv)
{
	mia();
    mic();
	return 0;
}
