#pragma once
#include <map>

typedef unsigned char SLOT;

#define MAX_SLOTS 16
//#define USE_MAP

#define GETSLOT(key) ((SLOT)(key & 0xF))

#define LSHIFT(KEY) (KEY >> 4)

template <typename T>
class CRadixTree
{
public:
	CRadixTree():m_p(NULL)
	{
#ifdef USE_MAP
		m_mapTree.clear();
#else
		m_ArrTree = NULL;
#endif
	}

	~CRadixTree()
	{
		
#ifdef USE_MAP
		for (typename std::map<SLOT, CRadixTree<T>* >::iterator it = m_mapTree.begin(); it != m_mapTree.end(); ++it)
		{
			if (it->second)
				delete it->second;
		}
		m_mapTree.clear();
#else
		if(NULL != m_ArrTree)
		{
			for(int i = 0; i < MAX_SLOTS; ++i)
			{
				if(NULL != m_ArrTree[i])
				{
					delete m_ArrTree[i];
				}
			}
		}
		delete[] m_ArrTree;
#endif

	}
public:
	void Insert(long key, T* p)
	{
		if (0 == key)
		{
			m_p = p;
			return;
		}
		
#ifdef USE_MAP
		typename std::map<SLOT, CRadixTree<T>* >::iterator it = m_mapTree.find(GETSLOT(key));
		if (it == m_mapTree.end())
		{
			m_mapTree[GETSLOT(key)] = new CRadixTree<T>;
			it = m_mapTree.find(GETSLOT(key));
		}
		it->second->Insert(LSHIFT(key), p);
#else
		if(NULL == m_ArrTree)
		{
			m_ArrTree = new CRadixTree<T>*[MAX_SLOTS];
			memset(m_ArrTree,0,sizeof(CRadixTree<T>*)*MAX_SLOTS);
		}

		if(NULL == m_ArrTree[GETSLOT(key)])
		{
			m_ArrTree[GETSLOT(key)] = new CRadixTree<T>;
		}

		m_ArrTree[GETSLOT(key)]->Insert(LSHIFT(key), p);
#endif

	}

	T* Find(long key)
	{
		if (0 == key)
			return m_p;

#ifdef USE_MAP
		typename std::map<SLOT, CRadixTree<T>* >::iterator it = m_mapTree.find(GETSLOT(key));
		if (it != m_mapTree.end())
			return it->second->Find(LSHIFT(key));
		return NULL;
#else
		if(NULL == m_ArrTree || NULL == m_ArrTree[GETSLOT(key)])
			return NULL;
		return m_ArrTree[GETSLOT(key)]->Find(LSHIFT(key));
#endif

	}

	bool Del(long key)
	{
		
#ifdef USE_MAP
		if (0 == key)
		{
			m_p = NULL;
			return 0 == m_mapTree.size();
		}

		typename std::map<SLOT, CRadixTree<T>* >::iterator it = m_mapTree.find(GETSLOT(key));

		if (it != m_mapTree.end())
		{
			if (it->second->Del(LSHIFT(key)))
			{
				delete it->second;
				m_mapTree.erase(it);
			}
		}
		return 0 == m_mapTree.size();
#else
		if(0 == key)
		{
			m_p = NULL;
			return true;
		}
		if(NULL == m_ArrTree || NULL == m_ArrTree[GETSLOT(key)])
			return false;
		return m_ArrTree[GETSLOT(key)]->Del(LSHIFT(key));
#endif

	}
	
private:

#ifdef USE_MAP
	std::map<SLOT, CRadixTree<T>* > m_mapTree;
#else
	CRadixTree<T>** m_ArrTree;
#endif

	T* m_p;
};