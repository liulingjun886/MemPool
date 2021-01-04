#pragma once
#include <map>

typedef unsigned char SLOT;

#define GETSLOT(key) ((SLOT)(key & 0xF))

template <typename T>
class CRadixTree
{
public:
	CRadixTree()
	{
		m_mapTree.clear();
	}

	~CRadixTree()
	{
		for (typename std::map<SLOT, CRadixTree<T>* >::iterator it = m_mapTree.begin(); it != m_mapTree.end(); ++it)
		{
			if (it->second)
				delete it->second;
		}
		m_mapTree.clear();
	}
public:
	void Insert(long key, T* p)
	{
		if (0 == key)
		{
			m_p = p;
			return;
		}
		
		typename std::map<SLOT, CRadixTree<T>* >::iterator it = m_mapTree.find(GETSLOT(key));
		if (it == m_mapTree.end())
		{
			m_mapTree[GETSLOT(key)] = new CRadixTree<T>;
			it = m_mapTree.find(GETSLOT(key));
		}
		it->second->Insert(key >> 4, p);
	}

	T* Find(long key)
	{
		if (0 == key)
			return m_p;

		typename std::map<SLOT, CRadixTree<T>* >::iterator it = m_mapTree.find(GETSLOT(key));
		if (it != m_mapTree.end())
			return it->second->Find(key >> 4);

		return NULL;
	}

	bool Del(long key)
	{
		if (0 == key)
		{
			m_p = NULL;
			return 0 == m_mapTree.size();
		}

		typename std::map<SLOT, CRadixTree<T>* >::iterator it = m_mapTree.find(GETSLOT(key));

		if (it != m_mapTree.end())
		{
			if (it->second->Del(key >> 4))
			{
				delete it->second;
				m_mapTree.erase(it);
			}
		}
		return 0 == m_mapTree.size();
	}
	
private:
	std::map<SLOT, CRadixTree<T>* > m_mapTree;
	T* m_p;
};