/*****************************************************************************
*  @file     HashMap.h
*  @brief    HashMAP
*  Details.                                                                 
*                                                                           
*  @author   IDeath
*  @email    IDeath@operatorworld.com
*  @version  0.0.0.1														
*  @date	 2017-07-18 10:22:53
*****************************************************************************/
#pragma once
#include "List.h"
#include <mutex>
#include <vector>
#include <atomic>
#include <Windows.h>

namespace HashMAP
{
	#define DEFAULT_HASH_MAP_INITSIZE 1024

	inline size_t FNVHash(std::string& strKey)
	{
		const char *pStrKey = strKey.c_str();
		register size_t sizeHash = 2166136261;
		while (size_t ch = (size_t)*pStrKey++)
		{
			sizeHash *= 16777619;
			sizeHash ^= ch;
		}
		return sizeHash;
	}

	inline size_t HashSizeT(size_t& nKey)
	{
		return FNVHash(std::to_string(nKey));
	}

	inline size_t HashSOCKET(SOCKET& nKey)
	{
		return FNVHash(std::to_string(nKey));
	}

	inline size_t HashULong(ULONG& nKey)
	{
		return FNVHash(std::to_string(nKey));
	}

	inline size_t HashULongPTR(ULONG_PTR& nKey)
	{
		return FNVHash(std::to_string(nKey));
	}
	
	template<typename T, typename HashKEY = std::string, DWORD dwInitSize = DEFAULT_HASH_MAP_INITSIZE>
	class CHashMap
	{
		typedef size_t(*HashCallBack)(HashKEY&);
		typedef void(*ReleaseCallBack)(T Data);
	public:
		class CVistor : public HashList::IVistor<T, HashKEY>
		{
		public:
			virtual void Vistor(std::shared_ptr<HashList::Node<T, HashKEY>> pNode, void *pData)
			{
				m_vResultNode.push_back(pNode);
			}

			std::vector<std::shared_ptr<HashList::Node<T, HashKEY>>> GetResult()
			{
				return m_vResultNode;
			}

		private:
			std::vector<std::shared_ptr<HashList::Node<T, HashKEY>>> m_vResultNode;
		};
		
		CHashMap()
			: m_nSize(0)
			, m_HashCallBack(NULL)
		{
			memset(m_arrayThreadIndex, 0, sizeof(m_arrayThreadIndex));
		}

		CHashMap(HashCallBack pCallBack)
			: m_nSize(0)
			, m_HashCallBack(pCallBack)
		{
			memset(m_arrayThreadIndex, 0, sizeof(m_arrayThreadIndex));
		}

		virtual ~CHashMap()
		{

		}

		size_t GetSize()
		{
			return m_nSize;
		}

		bool AddNode(std::shared_ptr<HashList::Node<T, HashKEY>> pNode)
		{
			if (NULL == pNode)
			{
				return false;
			}

			DWORD dwHashIndex = GetHashIndex(pNode->GetKey());
			if (InitHashArray(dwHashIndex)->AddNode(pNode))
			{
				m_nSize++;
				return true;
			}
			return false;
		}

		bool AddNode(HashKEY Key, T Data, ReleaseCallBack pCallBack = NULL)
		{
			std::shared_ptr<HashList::Node<T, HashKEY>> pData(new HashList::Node<T, HashKEY>(Data, pCallBack));
			pData->SetKey(Key);
			return AddNode(pData);
		}

		bool RemoveNode(std::shared_ptr<HashList::Node<T, HashKEY>> pNode)
		{
			if (NULL == pNode)
			{
				return false;
			}

			DWORD dwHashIndex = GetHashIndex(pNode->GetKey());
			if (InitHashArray(dwHashIndex)->RemoveNode(pNode))
			{
				m_nSize--;
				return true;
			}
			return false;
		}

		bool RemoveNode(HashKEY Key)
		{
			std::vector<std::shared_ptr<HashList::Node<T, HashKEY>>> vNode = GetKeyNode(Key);
			bool bResult = false;
			for (std::shared_ptr<HashList::Node<T, HashKEY>> pNode : vNode)
			{
				bResult = RemoveNode(pNode);
			}
			return bResult;
		}

		void FreeHashMapNode()
		{
			m_nSize = 0;

			for (DWORD dwIndex = 0; dwIndex < dwInitSize; ++dwIndex)
			{
				if (NULL != m_arrayHashData[dwIndex])
				{
					m_arrayHashData[dwIndex]->FreeAllNode();
				}
			}
		}

		void VistorNode(HashList::IVistor<T, HashKEY>& Vistor, void *pData = NULL)
		{
			for (int nIndex = 0; nIndex < dwInitSize; ++nIndex)
			{
				std::shared_ptr<HashList::CList<T, HashKEY>> pListItem = m_arrayHashData[nIndex];
				if (NULL != pListItem)
				{
					pListItem->VistorNode(Vistor, pData);
				}
			}
		}

		void VistorKeyNode(HashKEY Key, HashList::IVistor<T, HashKEY>& Vistor, void *pData = NULL)
		{
			DWORD dwHashIndex = GetHashIndex(Key);
			std::shared_ptr<HashList::CList<T, HashKEY>> pListItem = m_arrayHashData[dwHashIndex];
			if (NULL != pListItem)
			{
				pListItem->VistorNode(Key, Vistor, pData);
			}
		}

		std::vector<std::shared_ptr<HashList::Node<T, HashKEY>>> GetKeyNode(HashKEY strKey)
		{
			CVistor Vistor;
			VistorKeyNode(strKey, Vistor);
			return Vistor.GetResult();
		}

		std::shared_ptr<HashList::Node<T, HashKEY>> operator[](HashKEY Key)
		{
			CVistor Vistor;
			VistorKeyNode(Key, Vistor);
			std::vector<std::shared_ptr<HashList::Node<T, HashKEY>>> vResult = Vistor.GetResult();
			if (vResult.size())
			{
				return vResult[0];
			}
			return std::shared_ptr<HashList::Node<T, HashKEY>>();
		}

	private:
		std::shared_ptr<HashList::CList<T, HashKEY>> InitHashArray(DWORD dwHashIndex)
		{
			if (NULL == m_arrayHashData[dwHashIndex])
			{
				std::lock_guard<std::mutex> Lock(m_mutexInit);
				if (NULL == m_arrayHashData[dwHashIndex])
				{
					m_arrayHashData[dwHashIndex] = std::shared_ptr<HashList::CList<T, HashKEY>>(new HashList::CList<T, HashKEY>());
				}
			}
			return m_arrayHashData[dwHashIndex];
		}
		
		DWORD GetThreadHashIndex()
		{
			DWORD dwThreadID = GetCurrentThreadId();
			for (size_t nIndex = 0; nIndex < dwInitSize; ++nIndex)
			{
				if (0 == m_arrayThreadIndex[nIndex])
				{
					std::lock_guard<std::mutex> Lock(m_mutexInit);
					if (0 == m_arrayThreadIndex[nIndex])
					{
						m_arrayThreadIndex[nIndex] = dwThreadID;
						return nIndex;
					}
				}
				else
				{
					if (dwThreadID == m_arrayThreadIndex[nIndex])
					{
						return nIndex;
					}
				}
			}
			return 0;
		}

		inline DWORD GetHashIndex(HashKEY Key)
		{
			if (NULL == m_HashCallBack)
			{
				return GetThreadHashIndex();
			}
			else
			{
				size_t nHash = m_HashCallBack(Key);
				return nHash % dwInitSize;
			}
		}

	private:
		HashCallBack m_HashCallBack;
		std::atomic<size_t> m_nSize;
		std::mutex m_mutexInit;
		std::atomic<size_t> m_arrayThreadIndex[dwInitSize];
		std::shared_ptr<HashList::CList<T, HashKEY>> m_arrayHashData[dwInitSize];
	};
}