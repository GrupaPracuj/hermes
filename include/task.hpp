// Copyright (C) 2017 Grupa Pracuj Sp. z o.o.
// This file is part of the "Hermes" library.
// For conditions of distribution and use, see copyright notice in license.txt.

#ifndef _TASK_HPP_
#define _TASK_HPP_

#include <functional>
#include <queue>
#include <atomic>
#include <thread>
#include <condition_variable>
#include <unordered_map>
#include <cassert>

#if defined(ANDROID) || defined(__ANDROID__)
struct ALooper;
#endif

namespace hms
{
    class Spinlock
    {
    public:
        void lock() noexcept;
        void unlock() noexcept;
        bool try_lock() noexcept;
        
    private:
        std::atomic_flag mFlag = ATOMIC_FLAG_INIT;
    };

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
                enqueueMainThreadTask(std::move(task));
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

        void enqueueMainThreadTask(std::function<void()> pMethod);
        void dequeueMainThreadTask();

#if defined(ANDROID) || defined(__ANDROID__)
        static int messageHandlerAndroid(int pFd, int pEvent, void* pData);
#endif

        std::unordered_map<int, ThreadPool*> mThreadPool;

        std::queue<std::function<void()>> mMainThreadTask;
        std::mutex mMainThreadMutex;

#if defined(ANDROID) || defined(__ANDROID__)
        int mMessagePipeAndroid[2] = {0, 0};
        ALooper* mLooperAndroid = nullptr;
#endif

        // TODO - Use atomic + compare_exchange_strong for init
        uint32_t mInitialized = {0};
    };

}

#endif
