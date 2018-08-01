/*****************************************************************************
*  @file     List.h
*  @brief    HashList.
*  Details.                                                                 
*                                                                           
*  @author   IDeath
*  @email    IDeath@operatorworld.com
*  @version  0.0.0.1														
*  @date	 2017-07-18 10:20:05
*****************************************************************************/
#pragma once
#include <string>
#include <memory>
#include <windows.h>

namespace HashList
{
	template<typename T, typename HashKEY>
	class Node : std::enable_shared_from_this<Node<T, HashKEY>>
	{
	public:
		typedef void(*ReleaseCallBack)(T Data);
		Node()
			: m_bLock(FALSE)
			, Data()
			, m_bIsRemoving(FALSE)
			, m_pCallBack(NULL)
		{

		}

		Node(T Data)
			: m_bLock(FALSE)
			, Data(Data)
			, m_bIsRemoving(FALSE)
			, m_pCallBack(NULL)
		{

		}

		Node(T Data, ReleaseCallBack CallBack)
			: m_bLock(FALSE)
			, Data(Data)
			, m_bIsRemoving(FALSE)
			, m_pCallBack(CallBack)
		{

		}

		~Node()
		{
			if (NULL != m_pCallBack)
			{
				m_pCallBack(Data);
			}
		}

		std::shared_ptr<Node> GetNext()
		{
			return m_pNext;
		}

		void SetNext(std::shared_ptr<Node> pNext)
		{
			m_pNext = pNext;
		}

		void SetKey(HashKEY Key)
		{
			m_HashKey = Key;
		}

		T GetData()
		{
			return Data;
		}

		void SetData(T& rData)
		{
			Data = rData;
		}

		HashKEY GetKey()
		{
			return m_HashKey;
		}

		void SetNodeIsRemoving()
		{
			InterlockedExchange(&m_bIsRemoving, TRUE);
		}

		bool IsRemoving()
		{
			return m_bIsRemoving == TRUE;
		}

		void LockNode(long nLockCount = 4000)
		{
			long lWaitCount = 0;
			while (InterlockedExchange(&m_bLock, TRUE) == TRUE)
			{
				Sleep(0);
				if (nLockCount == INFINITE || lWaitCount++ > nLockCount)
				{
					Sleep(1);
					lWaitCount = 0;
					OutputDebugStringA("The conflict waits for too much time\n");
				}
			}
		}

		void UnLockNode()
		{
			InterlockedExchange(&m_bLock, FALSE);
		}

	private:
		std::shared_ptr<Node> m_pNext;
		HashKEY m_HashKey;
		T Data;
		volatile long m_bLock;
		volatile long m_bIsRemoving;
		ReleaseCallBack m_pCallBack;
		Node(const Node& Other) = delete;
		Node& operator=(const Node& Other) = delete;
	};

	template<typename T, typename HashKEY>
	class CLockHelper
	{
	public:
		CLockHelper(std::shared_ptr<Node<T, HashKEY>> pNode)
			: m_pNode(pNode)
		{
			if (NULL != m_pNode)
			{
				m_pNode->LockNode();
			}
		}

		~CLockHelper()
		{
			if (NULL != m_pNode)
			{
				m_pNode->UnLockNode();
			}
		}

	private:
		std::shared_ptr<Node<T, HashKEY>> m_pNode;
	};

	template<typename T, typename HashKEY>
	class IVistor
	{
	public:

		virtual void Vistor(std::shared_ptr<Node<T, HashKEY>> pNode, void *pData = NULL) = 0;
	};

	template<typename T, typename HashKEY>
	class CList
	{
	public:
		CList()
		{
			m_pFNode = std::make_shared<Node<T, HashKEY>>();
		}

		~CList()
		{
			FreeAllNode();
			m_pFNode = NULL;
		}

		bool AddNode(std::shared_ptr<Node<T, HashKEY>> pNode)
		{
			std::shared_ptr<Node<T, HashKEY>> pPrvNode = m_pFNode;
			do
			{
				CLockHelper<T, HashKEY> HelperP(pPrvNode);
				std::shared_ptr<Node<T, HashKEY>> pNextNode = pPrvNode->GetNext();
				if (NULL == pNextNode)
				{
					pPrvNode->SetNext(pNode);
				}
				else
				{
					CLockHelper<T, HashKEY> HelperN(pNextNode);
					pNode->SetNext(pNextNode);
					pPrvNode->SetNext(pNode);
				}
			} while (false);
			return true;
		}

		bool RemoveNode(std::shared_ptr<Node<T, HashKEY>> pNode)
		{
			std::shared_ptr<Node<T, HashKEY>> pPrvNode = m_pFNode;
			CLockHelper<T, HashKEY> LockFNode(m_pFNode);
			bool bResult = false;
			do
			{
				std::shared_ptr<Node<T, HashKEY>> pNextNode = pPrvNode->GetNext();
				if (NULL == pNextNode)
				{
					break;
				}

				CLockHelper<T, HashKEY> HelperN(pNextNode);
				if (pNextNode == pNode)
				{
					pNode->SetNodeIsRemoving();
					pPrvNode->SetNext(pNextNode->GetNext());
					bResult = true;
					break;
				}

				pPrvNode = pNextNode;
			} while (true);
			return bResult;
		}

		void VistorNode(IVistor<T, HashKEY>& Vistor, void *pData = NULL)
		{
			std::shared_ptr<Node<T, HashKEY>> pPrvNode = m_pFNode;
			do
			{
				CLockHelper<T, HashKEY> HelperP(pPrvNode);
				std::shared_ptr<Node<T, HashKEY>> pNextNode = pPrvNode->GetNext();
				if (NULL == pNextNode)
				{
					break;
				}

				CLockHelper<T, HashKEY> HelperN(pNextNode);
				if (!pNextNode->IsRemoving())
				{
					Vistor.Vistor(pNextNode, pData);
				}
				pPrvNode = pNextNode;
			} while (true);
		}

		void VistorNode(HashKEY Key, IVistor<T, HashKEY>& Vistor, void *pData = NULL)
		{
			std::shared_ptr<Node<T, HashKEY>> pPrvNode = m_pFNode;
			do
			{
				CLockHelper<T, HashKEY> HelperP(pPrvNode);
				std::shared_ptr<Node<T, HashKEY>> pNextNode = pPrvNode->GetNext();
				if (NULL == pNextNode)
				{
					break;
				}

				CLockHelper<T, HashKEY> HelperN(pNextNode);
				if (pNextNode->GetKey() == Key && !pNextNode->IsRemoving())
				{
					Vistor.Vistor(pNextNode, pData);
				}
				pPrvNode = pNextNode;
			} while (true);
		}

		void FreeAllNode()
		{
			std::shared_ptr<Node<T, HashKEY>> pNextNode;
			{
				CLockHelper<T, HashKEY> LockFNode(m_pFNode);
				pNextNode = m_pFNode->GetNext();
				m_pFNode->SetNext(std::shared_ptr<Node<T, HashKEY>>());
			}

			std::shared_ptr<Node<T, HashKEY>> pPrvNode = pNextNode;
			do
			{
				if (NULL == pPrvNode)
				{
					break;
				}

				CLockHelper<T, HashKEY> HelperP(pPrvNode);
				pPrvNode->SetNodeIsRemoving();
				std::shared_ptr<Node<T, HashKEY>> pNextNode = pPrvNode->GetNext();
				if (NULL == pNextNode)
				{
					break;
				}

				pPrvNode->SetNext(std::shared_ptr<Node<T, HashKEY>>());
				pPrvNode = pNextNode;
			} while (true);
		}

	private:
		std::shared_ptr<Node<T, HashKEY>> m_pFNode;
	};
}