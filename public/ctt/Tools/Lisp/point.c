#include <stdio.h>
#include <stdlib.h>

//计算非数组type的大小
#define sizeofb(type)  (size_t)((type*)0+1)
//计算非数组type的大小
#define sizeofb1(type)  (size_t)(&((type*)0)[1])
//计算数组type的大小
#define sizeofa(var)  (size_t)((size_t)(&var+1)-(size_t)(&var))
//计算member在type中的位置
#define offsetof(type, member)  (size_t)(&((type*)0)->member)
//根据member的地址获取type的起始地址
#define containerof(ptr, type, member) ({ const typeof(((type*)0)->member)* __mptr = (ptr); (type*)((char*)__mptr - offsetof(type, member)); })

struct test {
	char a;
	short b;
	char k;
	char s;
};


int main(int argc, char** argv)
{
	int arr[10]; //int a = 0;
	int* point = malloc(10*sizeof(int));
	printf("%d\n", ({int a = 3 , b = 3 > 2; a = 4; b;}));
	printf("%zu\n", sizeofb(struct test));
	printf("%zu\n", sizeofb(int));
	printf("%zu\n", sizeofb(int*));
	printf("%zu\n", sizeofb1(int*));
	printf("%zu\n", sizeofa(arr));
	printf("%zu\n", sizeofa(point));
	printf("%zu\n", offsetof(struct test, k));
	struct test* asd = malloc(sizeof(struct test));
	printf("%p, %p\n", asd, containerof(&asd->k, struct test, k));
	free(asd);
	free(point);
	return 0;
}