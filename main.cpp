#include <stdio.h>
#include "ThreadCache.h"
#include <time.h>
#include <stdlib.h>
#define MAX 1024

int main()
{
	ThreadCache memPool;
	void* ptr[MAX] = {0};
	srand(time(NULL));
	for(int i = 0; i < MAX; i++)
	{
		ptr[i] = memPool.Allocate(rand()%65536);
	}
	
	printf("===========================\n");
	for(int i = 0; i < MAX; i++)
	{
		memPool.Deallocate(ptr[i]);
	}
}