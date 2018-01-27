#include <stdio.h>
#include <stdlib.h>
#include <string.h>


static char k = 'c';

void mia();

void miba()
{
    mia();
}

void mib()
{
	*(&k+1) = 'd';
}

void mia()
{
	 static char vk = 'a';
	 printf("%d\n", vk);
}


void mic()
{
    void* sc = "\x55\x48\x89\xe5\xe8\xf5\xfd\xff\xff\x5d\xc3\x0f\x1f\x44\x00\x00";
    ((void (*)())sc)();
}

void mid()
{
    char kc = 'a';
    void* sc = "\xe8\x43\xfe\xff\xff";
    ((void (*)())sc)();
}

void mie()
{
    char kc = 'a';
    void* sc = "\xe8\x43\xfe\xff\xff";
    for(int i = 0; i < 7; i++){
        *(&kc+17+i) = ((unsigned char*)sc)[i];
    }
    *((long*)(&kc+9)) = (long)(&kc+17);
    
    //char kc = 'a';
    /*
     for(int i = 0; i < mib-miba; i++){
     printf("\\x%02x", ((unsigned char*)miba)[i]);
     }*/
}

void mik()
{
	char* a = malloc(1);
	*a = 'A';
	//free(a);
	printf("%d\n", *a); 
	char* b = malloc(1);
	printf("%d\n", *(b-16)); 
}

void min()
{
	char a = 'a';
}

void max()
{
	char b;
	printf("%d\n", b); 
}

int main(int argc, char** argv)
{
	max();
	min();
	max();
	mik();
	mib();
	mia();
    mic();
	mid();
    
	return 0;
}
