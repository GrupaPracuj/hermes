#ifndef _TASK_HPP_
#define _TASK_HPP_

#include <functional>
#include <queue>
#include <atomic>
#include <thread>
#include <condition_variable>
#include <unordered_map>
#include <cassert>

namespace hms
{

	class ThreadPool
    {
        friend class TaskManager;
        
    public:
        ThreadPool(const ThreadPool& pSource) = delete;
    
        bool push(const std::function<void()>& pTask);
        bool hasTask();
        void start();
        void stop();
        size_t threadCount();
        void performTaskIfExists();
        void clearTask();
        
    private:
        ThreadPool(int pID, size_t pThreadCount);
        ~ThreadPool();
        
        void update(size_t pID);
                   
        size_t mThreadCount = 0;
        std::thread** mThread = nullptr;
        std::atomic<uint32_t>* mHasTask {nullptr};
                   
        std::condition_variable mTaskCondition;
        std::mutex mTaskMutex;
        std::queue<std::function<void()>> mTask;

        std::atomic<uint32_t> mForceFinish {0};
        std::atomic<uint32_t> mIsActive {0};
        int mId;
    };
    
	class TaskManager
    {
    public:
		bool initialize(const std::vector<std::pair<int, size_t>> pThreadPool);
		bool terminate();

        void flush(int pThreadPoolID);
        size_t threadCountForPool(int pThreadPoolID);
        bool canContinueTask(int pThreadPoolID);
        void performTaskIfExists(int pThreadPoolID);
        void clearTask(int pThreadPoolID);
        
        template<typename M, typename... P>
		void execute(int pThreadPoolID, M&& pMethod, P&&... pParameter)
        {
            if (!mInitialized)
                return;
            
            std::function<void()> task = std::bind(std::forward<M>(pMethod), std::forward<P>(pParameter)...);
            
            if (pThreadPoolID < 0)
            {
                platformRunOnMainThread(task);
            }
            else
            {
                assert(mThreadPool.find(pThreadPoolID) != mThreadPool.end());
            
                ThreadPool* threadPool = mThreadPool[pThreadPoolID];
                threadPool->push(task);
            }
        }

    private:
        friend class Hermes;
    
        TaskManager();
        TaskManager(const TaskManager& pOther) = delete;
        TaskManager(TaskManager&& pOther) = delete;
        ~TaskManager();
        
        TaskManager& operator=(const TaskManager& pOther) = delete;
        TaskManager& operator=(TaskManager&& pOther) = delete;

        void platformRunOnMainThread(std::function<void()> pMethod);

        std::unordered_map<int, ThreadPool*> mThreadPool;
        // TODO - Use atomic + compare_exchange_strong for init
        uint32_t mInitialized = {0};
    };

}

#endif
