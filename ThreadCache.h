#pragma once
#include "MemCommon.h"
#include "CentralCache.h"

//ThreadCache是一个线程独有的资源，直接与本线程交互
class ThreadCache
{
public:
	ThreadCache();
	~ThreadCache();
public:
	//ThreadCache提供两个接口，一个为申请内存，另一个为释放内存
	void* Allocate(uint32 size);
	void Deallocate(void* ptr);
private:
	//向CentralCache申请内存块的接口,返回一块内存块，将剩下的挂载在ThreadCache的对应处
	void* FetchFromCentralCache(uint32 index, uint32 byte);
	void ReturnToCentralCache(FreeList &freelist,uint32 nIndex);
private:
	FreeList _freelist[NLISTS];
};

