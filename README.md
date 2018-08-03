# HashMap
This is a thread-safe hash table. 

<img src="./img/1.png"/>

# Example Code

```c++
// Elements added to the hash table
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

// Vistor Mode
class CTaskVistor : public HashList::IVistor<std::shared_ptr<CTask>, ULONG_PTR>
{
public:
    virtual void Vistor(std::shared_ptr<HashList::Node<std::shared_ptr<CTask>, ULONG_PTR>> pNode, void *pData = NULL) override
    {
        pNode->GetData()->DoTask(pData);
    };
};

// Vistor hash table thread
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
```
- Add node thread and remove node thread

```c++
// Add elements Thread
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

// Remove node check function. if true, this will remove the first element that is accessed every time.
bool RemoveCallBack(std::shared_ptr<CTask> pTask)
{
    std::cout << "TaskSize: " << mapTask.GetSize() << " RemoveTask: " << pTask->GetID() << "  ThreadID: " << GetCurrentThreadId() << std::endl;
    return true;
}

// Remove task thread
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
```