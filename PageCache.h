#pragma once
#include "MemCommon.h"
#include "RadixTree.h"

/* PageCache
 * 与系统物理内存交互，一次申请128页连续内存
 */
class PageCache
{
public:
	static PageCache* GetInstance()
	{
		return &_Inst;
	}
	
	Span* NewSpan(uint32 npage);
	Span* MapObjectToSpan(void* ptr);
	void TakeSpanToPageCache(Span* span);

	~PageCache();
private:
	Span* _NewSpan(uint32 npage);
	PageCache();
	PageCache(const PageCache&);
	PageCache& operator=(const PageCache&);
	Span* AddSpan();
	void DelSpan(Span* span);
private:
	SpanList _pagelist[NPAGES];
	CRadixTree<Span> _id_span_map;
	static PageCache _Inst;
};

