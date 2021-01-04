#include "ThreadCache.h"
#include <assert.h>

ThreadCache::ThreadCache()
{
	
}

ThreadCache::~ThreadCache()
{
	for(int i = 0; i < NLISTS; ++i)
	{
		FreeList& freelist = _freelist[i];
		if(!freelist.Empty())
		{
			ReturnToCentralCache(freelist,i);
		}
	}
}

//申请空间
void* ThreadCache::Allocate(uint32 size)
{
	//预防措施，防止将要给出的内存大于可以给出的最大大小
	assert(size <= MAXBYTES);

	//根据用户需要size的大小计算ThreadCache应该给出的内存块大小
	size = ClassSize::Roundup(size);

	//根据内存块的大小计算出内存块所在自由链表的下标
	uint32 index = ClassSize::Index(size);
	FreeList& freelist = _freelist[index];
	if (!freelist.Empty())
	{
		//表示该处的自由链表下含有可用的内存块
		return freelist.Pop();
	}

	//走到该处及说明该自由链表没有可用的内存块
	return FetchFromCentralCache(index, size);
}

//向Centralcache申请一定数量的目标内存块
void* ThreadCache::FetchFromCentralCache(uint32 index, uint32 byte)
{
	assert(byte <= MAXBYTES);
	FreeList& freelist = _freelist[index];	//拿到是哪个freelist想要获取内存块
	uint32 num = FETCH_NUM;	//想要从CentralCache拿到的内存块的个数

	void *start, *end;	//标记拿到的内存	fetchnum表示真实拿到的内存块个数
	uint32 fetchnum = CentralCache::GetInstance()->FetchRangeObj(start, end, num, byte);
	if (fetchnum == 1)
	{
		//如果只从CentralCache拿到一块就不用将剩余的内存块挂载在自由链表下
		return start;
	}

	freelist.PushRange(NEXT_OBJ(start), end, fetchnum - 1);
	return start;
}

//释放内存块
void ThreadCache::Deallocate(void* ptr)
{
	//拿到管理ptr指针的span
	Span* span = PageCache::GetInstance()->MapObjectToSpan(ptr);
	assert(span != NULL);
	if (span->_objsize > MAXBYTES)
	{
		//如果要返还的Span大于最大字节大小，则直接返还给PageCache
		PageCache::GetInstance()->TakeSpanToPageCache(span);
		return;
	}
	FreeList& freelist = _freelist[ClassSize::Index(span->_objsize)];

	//将内存块头插
	freelist.Push(ptr);

	//超过10个即返还 开始将内存返还到中心CentralCache
	if (freelist.Size() >= freelist.MaxSize())
	{
		ReturnToCentralCache(freelist,ClassSize::Index(span->_objsize));
	}
}

void ThreadCache::ReturnToCentralCache(FreeList &freelist,uint32 nIndex)
{
	void* start = freelist.Clear();
	CentralCache::GetInstance()->ReturnToCentralCache(start,nIndex);
}
