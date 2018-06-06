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
    
    class TaskManager;

    class ThreadPool
    {
    public:
        ThreadPool(const ThreadPool& pSource) = delete;
    
        void push(std::pair<std::function<int32_t()>, std::function<void()>> pTask);
        bool hasTask() const;
        void flush();
        void performTaskIfExists();
        
    private:
        friend TaskManager;
    
        ThreadPool(int32_t pId, size_t pThreadCount, TaskManager* pTaskManager);
        ~ThreadPool();
        
        void update(size_t pId);
                   
        size_t mThreadCount = 0;
        std::thread** mThread = nullptr;
                   
        std::condition_variable mTaskCondition;
        mutable std::mutex mTaskMutex;
        std::queue<std::pair<std::function<int32_t()>, std::function<void()>>> mTask;
        std::atomic<uint32_t> mProcessingTaskCount {0};
        std::atomic<uint32_t> mTerminate {0};
        int32_t mId;
        
        TaskManager* mTaskManager;
    };
    
    class TaskManager
    {
    public:
        bool initialize(const std::vector<std::pair<int32_t, size_t>> pThreadPool, std::function<void(std::function<void()>)> pMainThreadHandler = nullptr);
        bool terminate();

        bool hasTask(int32_t pThreadPoolId) const;
        void flush(int32_t pThreadPoolId);
        size_t threadCountForPool(int32_t pThreadPoolId) const;
        bool canContinueTask(int32_t pThreadPoolId) const;
        void performTaskIfExists(int32_t pThreadPoolId) const;
        
        template<typename M, typename... P>
        void execute(int32_t pThreadPoolId, M&& pMethod, P&&... pParameter)
        {
            executeDelayed(pThreadPoolId, 0, pMethod, pParameter...);
        }
        
        template<typename M, typename... P>
        void executeDelayed(int32_t pThreadPoolId, uint64_t pDelayMs, M&& pMethod, P&&... pParameter)
        {
            if (!mInitialized)
                return;
            
            std::function<int32_t()> condition = createCondition(pThreadPoolId, pDelayMs);
            std::function<void()> task = std::bind(std::forward<M>(pMethod), std::forward<P>(pParameter)...);

            if (pThreadPoolId < 0)
            {
                enqueueMainThreadTask(std::move(task));
            }
            else
            {
                auto threadPool = mThreadPool.find(pThreadPoolId);
                assert(threadPool != mThreadPool.cend());
                threadPool->second->push(std::make_pair(std::move(condition), std::move(task)));
            }
        }

    private:
        friend class Hermes;
        friend class ThreadPool;
    
        TaskManager();
        TaskManager(const TaskManager& pOther) = delete;
        TaskManager(TaskManager&& pOther) = delete;
        ~TaskManager();
        
        TaskManager& operator=(const TaskManager& pOther) = delete;
        TaskManager& operator=(TaskManager&& pOther) = delete;

        void enqueueMainThreadTask(std::function<void()> pTask);
        void dequeueMainThreadTask();
        std::function<int32_t()> createCondition(int32_t& pThreadPoolId, uint64_t pDelayMs) const;

#if defined(ANDROID) || defined(__ANDROID__)
        static int messageHandlerAndroid(int pFd, int pEvent, void* pData);
#endif

        std::unordered_map<int32_t, ThreadPool*> mThreadPool;

        std::queue<std::function<void()>> mMainThreadTask;
        mutable std::mutex mMainThreadMutex;

#if defined(ANDROID) || defined(__ANDROID__)
        int mMessagePipeAndroid[2] = {0, 0};
        ALooper* mLooperAndroid = nullptr;
#endif

        std::function<void(std::function<void()>)> mMainThreadHandler;

        // TODO - Use atomic + compare_exchange_strong for init
        uint32_t mInitialized = {0};
    };

}

#endif
