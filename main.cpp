#include <stdio.h>
#include "ThreadCache.h"
#include <time.h>
#include <stdlib.h>
#define MAX 5
static ThreadCache memPool;

int main()
{
	void* ptr[MAX] = {0};
	srand(time(NULL));
	for(int i = 0; i < MAX; i++)
	{
		ptr[i] = memPool.Allocate(rand()%65536);
	}
	
	printf("===========================\n");
	
	/*for(int i = 0; i < MAX; i++)
	{
		memPool.Deallocate(ptr[i]);
	}*/
}