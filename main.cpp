#include <stdio.h>
#include <thread>
#include <iostream>
#include <fstream>
#include "HashMap/List.h"
#include "HashMap/HashMap.h"

class CTask;
class CTaskVistor;

HashMAP::CHashMap<std::shared_ptr<CTask>, ULONG_PTR, 4096> mapTask(HashMAP::HashULongPTR);

class CTask
{
  public:
    CTask(int nTaskID) : m_nTaskID(nTaskID){};

    void DoTask(void *pData)
    {
		std::ofstream ofil("D:\\Task.txt", std::ios::app);
		ofil << "TaskID: " << m_nTaskID << "  " << (char*)pData << "  ThreadID: " << GetCurrentThreadId() << std::endl;
		ofil.close();
    };

    int GetID()
    {
        return m_nTaskID;
    }

  private:
    int m_nTaskID;
};

class CTaskVistor : public HashList::IVistor<std::shared_ptr<CTask>, ULONG_PTR>
{
  public:
    virtual void Vistor(std::shared_ptr<HashList::Node<std::shared_ptr<CTask>, ULONG_PTR>> pNode, void *pData = NULL) override
    {
        pNode->GetData()->DoTask(pData);
    };
};

void AddTaskThread()
{
    int id = 0;
    do
    {
        std::shared_ptr<CTask> pTask(new CTask(id++));
        mapTask.AddNode(pTask->GetID(), pTask);
        long taskCount = mapTask.GetSize();
		std::ofstream ofil("D:\\Task.txt", std::ios::app);
		ofil << "AddTask: " << pTask->GetID() << " TaskSize: " << mapTask.GetSize() << "  ThreadID: " << GetCurrentThreadId() << std::endl;
		ofil.close();
        
		if (id > 10000)
		{
			id = 0;
		}

        if (taskCount > 5000)
        {
            std::this_thread::sleep_for(std::chrono::microseconds(5));
        }

        if (taskCount > 10000)
        {
            break;
        }
    } while (true);
}

bool RemoveCallBack(std::shared_ptr<CTask> pTask)
{
    std::cout << "TaskSize: " << mapTask.GetSize() << " RemoveTask: " << pTask->GetID() << "  ThreadID: " << GetCurrentThreadId() << std::endl;
    return true;
}

void RemoveTaskThreak()
{
    int id = 0;
    do
    {
        mapTask.RemoveNodeByCallBack((ULONG_PTR)id++, RemoveCallBack);
        if (id > 10000)
        {
            id = 0;
        }

		if (mapTask.GetSize() == 0)
		{
			std::cout << "Task Zero..." << "  ThreadID: " << GetCurrentThreadId() << std::endl;
			break;
		}
    } while (true);
}

void DoTaskThread()
{
    CTaskVistor Vistor;
    do
    {
        CHAR szMSG[256] = {0};
        sprintf_s(szMSG, sizeof(szMSG), "MSG = %d%d", rand(), rand());
        mapTask.VistorNode(Vistor, szMSG);

        if (mapTask.GetSize() == 0)
        {
            std::cout << "Task Zero..." << "  ThreadID: " << GetCurrentThreadId() << std::endl;
            break;
        }
    } while (true);
}

int main()
{
    srand(time(NULL));
    std::thread t1(AddTaskThread);
    std::this_thread::sleep_for(std::chrono::microseconds(500));
    std::thread t2(DoTaskThread);
    std::thread t3(RemoveTaskThreak);
    t1.join();
	t2.join();
	t3.join();
    return 0;
}
