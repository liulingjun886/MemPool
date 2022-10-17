#pragma once
#include <assert.h>
#include <string.h>
//Common.h主要存放项目所用到的数据结构及常量

//表示自由链表中一共有240类大小的内存块
#define NLISTS 240

//表示自由链表中可以给出的最大的内存块的大小为64KB
#define MAXBYTES 65536

//表示PageCache中页种类的数量 即最大为128页 0页处存放span结构
#define NPAGES 129

//一页内存的大小
#define PAGESIZE 4096

#define FETCH_NUM 5

#define MAX_NUM 7



typedef unsigned long long uint64;
typedef long long int64;

typedef unsigned int uint32;
typedef int int32;

typedef unsigned short uint16;
typedef short int16;

typedef char int8;
typedef unsigned char uint8;

inline void*& NEXT_OBJ(void* ptr)
{
	return *((void**)ptr);
}

class FreeList
{
public:
	FreeList():_ptr(NULL),_size(0)
	{
		
	}
	uint32 Size()
	{
		return _size;
	}

	uint32 MaxSize()
	{
		return MAX_NUM;
	}

	void* Clear()
	{
		void* start = _ptr;
		_ptr = NULL;
		_size = 0;
		return start;
	}

	bool Empty()
	{
		return _ptr == NULL;
	}

	void Push(void* obj)
	{
		NEXT_OBJ(obj) = _ptr;
		_ptr = obj;
		++_size;
	}

	void PushRange(void* start, void* end, uint32 num)
	{
		NEXT_OBJ(end) = _ptr;
		_ptr = start;
		_size += num;
	}

	void* Pop()
	{
		void* obj = _ptr;
		_ptr = NEXT_OBJ(_ptr);
		--_size;
		return obj;
	}

private:
	void* _ptr;

	//记录该自由链表链接的内存块的size
	uint32 _size;
};

class ClassSize
{
public:
	//align在这里表示对齐数，该函数的作用为根据size计算出应该分配多大的内存块
	static inline uint32 _Roundup(uint32 size, uint32 align)
	{
		//return ((size + align) / align * align);
		return ((size + align - 1) & ~(align - 1));
	}

	//根据size的大小计算应该给出的内存块大小
	static inline uint32 Roundup(uint32 size)
	{
		assert(size <= MAXBYTES);

		//总体上将freelist分为四段	为什么要采用这样的对齐规则？
		//[8, 128]									8B对齐 采用STL内存池的分段规则
		//[129, 1024]							16B对齐
		//[1025, 8 * 1024]					128B对齐
		//[8 * 1024 + 1, 64 * 1024]		512B对齐
		if(size <= 128)
		{
			return _Roundup(size, 8);
		}
		else if(size <= 1024)
		{
			return _Roundup(size, 16);
		}
		else if(size <= 8 * 1024)
		{
			return _Roundup(size, 128);
		}
		else if(size <= 64 * 1024)
		{
			return _Roundup(size, 512);
		}

		//程序走到这里时说明size已经超越最大的内存块，与首部相呼应
		assert(false);
		return -1;
	}
	
	static inline uint32 _Index(uint32 size, uint32 align)
	{
		return _Roundup(size, align) / align - 1;
	}

	static inline uint32 Index(uint32 size)
	{
		assert(size <= MAXBYTES);

		if(size <= 128)
		{
			return _Index(size, 8);
		}
		else if(size <= 1024)
		{
			return _Index(size - 128, 16) + 16;
		}
		else if(size <= 8 * 1024)
		{
			return _Index(size - 1024, 128) + 16 + 56;
		}
		else if(size <= 64 * 1024)
		{
			return _Index(size - 8 * 1024, 512) + 16 + 56 + 56;
		}

		//程序到达该步骤，一定是之前某处出错了
		assert(false);
		return -1;
	}

	//计算应该给出多少个内存块，内存块数控制在[2, 512]之间
	static uint32 NumMoveSize(uint32 byte)
	{
		if (byte == 0)
		{
			return 0;
		}
		int num = (int)(MAXBYTES / byte);
		if (num < 2)
			num = 2;

		if (num > 512)
			num = 512;
		return num;
	}

	static uint32 NumMovePage(uint32 byte)
	{
		//计算应该给出多少块内存块
		uint32 num = NumMoveSize(byte);
		uint32 npage = (uint32)((num * byte) / (4 * 1024));	//根据所需要给出的内存块数计算应该需要几页的连续内存 1页=4K
		if (npage == 0)
			npage = 1;

		return npage;
	}
};

typedef unsigned long PageID;
struct Span
{
	PageID _pageid;	// 记录页号
	uint32 _npage;	// 记录该Span中一共有几页 1页=4k

	//Span为带头双向循环链表
	Span* _prev;
	Span* _next;

	void* _objlist;	// 对象自由链表
	uint32 _objsize;	//对象内存块的大小
	uint32 _usecount;	//对象内存块实用计数

	Span()
	{
		_pageid = 0;
		_npage = 0;
		_prev = NULL;
		_next = NULL;
		_objlist = NULL;
		_objsize = 0;
		_usecount = 0;
	}
};


struct SpanPool
{
	SpanPool* _next;
	uint32 _used;
	Span _span[0];

	SpanPool()
	{
		_next = NULL;
		_used = 0;
	}
};


//CentralCache的数据结构
class SpanList
{
public:
	SpanList():_head(NULL)
	{
		_head = new Span;
		_head->_next = _head;
		_head->_prev = _head;
	}
	~SpanList()
	{
		if(_head)
		{
			delete _head;
			_head = NULL;
		}
	}
	void Insert(Span* cur, Span* newspan)
	{
		assert(cur);
		Span* prev = cur->_prev;

		prev->_next = newspan;
		newspan->_prev = prev;
		newspan->_next = cur;
		cur->_prev = newspan;
	}

	void Earse(Span* cur)
	{
		assert(cur != NULL && cur != _head);

		Span* prev = cur->_prev;
		Span* next = cur->_next;

		prev->_next = next;
		next->_prev = prev;
	}

	bool Empty()
	{
		return _head->_next == _head;
	}

	Span* Begin()
	{
		return _head->_next;
	}

	Span* End()
	{
		return _head;
	}

	Span* Pop()
	{
		Span* span = Begin();
		Earse(span);
		return span;
	}

	void PushFront(Span* span)
	{
		Insert(Begin(), span);
	}
	
private:
	Span* _head;
};
