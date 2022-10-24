#include "PageCache.h"
#include <sys/mman.h>

#define MAX_SPAN_NUM 64

//实例化单例对象
PageCache PageCache::_Inst;

PageCache::PageCache()
{
	AddNewSpans();
}

PageCache::~PageCache()
{
	for(int i = 1; i < NPAGES; ++i)
	{
		SpanList& spanlist = _pagelist[i];		
		while(!spanlist.Empty())
		{
			Span* pSpan = spanlist.Pop();
			munmap(pSpan->_objlist,pSpan->_npage*PAGESIZE);
		}
	}

	while(_spanpoolhead._next)
	{
		SpanPool* next = _spanpoolhead._next;
		_spanpoolhead._next = next->_next;
		munmap(next,MAX_SPAN_NUM*PAGESIZE);
	}	
}

//直接生成span的内存池
bool PageCache::AddNewSpans()
{
	SpanPool* ptr = (SpanPool*)mmap(NULL, MAX_SPAN_NUM*PAGESIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS /*MAP_HUGETLB*/, -1, 0);
	if (ptr == NULL)
	{
		return false;
	}
	ptr->_next = _spanpoolhead._next;
	_spanpoolhead._next = ptr;
	ptr->_unused = ((MAX_SPAN_NUM*PAGESIZE)-sizeof(SpanPool))/sizeof(Span);
	return true;
}

/*
 * NewSpan获取一段对应的Span,Span是整数倍的页
 * 该函数内不做页内存的切分，交给上层CentralCache做
 */
Span* PageCache::NewSpan(uint32 npage)
{
	//TODO:LOCK
	return _NewSpan(npage);
}

Span* PageCache::AddSpan()
{
	if(!_pagelist[0].Empty())
		return _pagelist[0].Pop();
	else
	{
		SpanPool* spanpool = _spanpoolhead._next;
		if(0 == spanpool->_unused)
		{
			AddNewSpans();
			spanpool = _spanpoolhead._next;
		}
		return (char*)&spanpool->_span[--spanpool->_unused];
	}
}

void PageCache::DelSpan(Span* span)
{
	assert(span != NULL);
	_pagelist[0].PushFront(span);
}


Span* PageCache::_NewSpan(uint32 npage)
{
	assert(npage < NPAGES);

	//首先在对应的PageList上查看有没有空闲的Span
	if (!_pagelist[npage].Empty())
	{
		//目标pagelist不为空
		return _pagelist[npage].Pop();
	}

	//到这里即说明目标pagelist为空，需要向下寻找，进行分割，从下一个位置开始寻找
	for (uint32 i = npage + 1; i < NPAGES; ++i)
	{
		Span* split = NULL;
		SpanList& pagelist = _pagelist[i];
		if (!pagelist.Empty())
		{
			//找到的pagelist不为空
			Span* span = pagelist.Pop();

			//split = new Span;
			split = AddSpan();
			split->_pageid = span->_pageid + span->_npage - npage;	//从后面切内存	此处存在问题	TODO	如果剩余span不足应对情况
			split->_npage = npage;

			//计算剩余的内存
			span->_npage -= npage;

			//将剩余的大块内存挂载pageCache的相应位置下
			_pagelist[span->_npage].PushFront(span);

			//只有切出去的页才需要建立新映射
			for (uint32 i = 0; i < split->_npage; ++i)
			{
				//建立页的映射关系，xxx页-xxx页都为split管理
				_id_span_map.Insert(split->_pageid + i, split);
			}
			return split;
		}
	}

	//走到这里说明下面没用可以用来分割的Pagelist，需要重新向内存申请
	//void* ptr = VirtualAlloc(NULL, (NPAGES - 1) * 4 * 1024, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);	//直接向系统申请128页的大小
    void* ptr = mmap(NULL, (NPAGES - 1) * 4 * 1024, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS /*MAP_HUGETLB*/, -1, 0);
	if (ptr == NULL)
	{
		return NULL;
	}
	//将新申请的内存挂载在最下面
	Span* maxspan = AddSpan();
	maxspan->_objlist = ptr;
	maxspan->_pageid = (PageID)ptr >> 12;
	maxspan->_npage = NPAGES - 1;
	_pagelist[NPAGES - 1].PushFront(maxspan);

	for (uint32 i = 0; i < maxspan->_npage; ++i)
	{
		//建立页的映射关系，xxx页-xxx页都为split管理
		_id_span_map.Insert(maxspan->_pageid + i, maxspan);
	}
	
	return _NewSpan(npage);
}

Span* PageCache::MapObjectToSpan(void* obj)
{ 
	PageID pageid = (PageID)obj >> 12;	//计算指针所在的页
	return _id_span_map.Find(pageid);	//查找该页对应的管理页
}

void PageCache::TakeSpanToPageCache(Span* span)
{
	assert(span != NULL);
	//TODO:LOCK
	Span* previt = _id_span_map.Find(span->_pageid - 1);
	while (previt != NULL)
	{
		//查看前一个Span
		Span* prevspan = previt;
		if (prevspan->_usecount != 0)
		{
			//前一个span正在使用中，则不能与前一个合并
			break;
		}

		//走到这里说明可以与前一个span进行合并
		if ((prevspan->_npage + span->_npage) >= NPAGES)
		{
			//说明与前一个span合并之后的页大小超过128
			break;
		}

		_pagelist[prevspan->_npage].Earse(prevspan);
		prevspan->_npage += span->_npage;
		//delete span;
		DelSpan(span);

		span = prevspan;

		//继续向前合并
		previt = _id_span_map.Find(span->_pageid - 1);

	}

	//向后合并
	Span* nextit = _id_span_map.Find(span->_pageid + span->_npage);
	while (nextit != NULL)
	{
		Span* nextspan = nextit;
		if ((span->_npage + nextspan->_npage) >= NPAGES)
		{
			//超过128不进行合并
			break;
		}

		if (nextspan->_usecount != 0)
		{
			//前一个span不空闲不合并
			break;
		}

		_pagelist[nextspan->_npage].Earse(nextspan);
		span->_npage += nextspan->_npage;
		//delete nextspan;
		DelSpan(nextspan);

		nextit = _id_span_map.Find(span->_pageid + span->_npage);
	}

	//合并完成，重新挂载，并重新建立映射
	for (uint32 i = 0; i < span->_npage; ++i)
	{
		_id_span_map.Insert(span->_pageid + i, span);
	}

	_pagelist[span->_npage].PushFront(span);

}

