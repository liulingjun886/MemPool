#pragma once
#include "MemCommon.h"
#include "PageCache.h"


class CentralCache
{
public:
	static inline CentralCache* GetInstance()
	{
		return &_Inst;
	}

	uint32 FetchRangeObj(void*& start, void*& end, uint32 num, uint32 byte);
	void ReturnToCentralCache(void* start,uint32 nIndex);
private:
	CentralCache(){}
	CentralCache(const CentralCache&);
	CentralCache& operator=(const CentralCache&);
	Span* GetOneSpan(SpanList& spanlist, uint32 byte);
private:
	SpanList _spanlist[NLISTS];
	static CentralCache _Inst;
};

