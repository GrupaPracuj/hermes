// Copyright (C) 2017-2018 Grupa Pracuj Sp. z o.o.
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

    class TaskManager
    {
    public:
        bool initialize(const std::vector<std::pair<int32_t, size_t>> pThreadPool, std::function<void(std::function<void()>)> pMainThreadHandler = nullptr);
        bool terminate();

        void flush(int32_t pThreadPoolId, std::function<void()> pCallback);
        
        template<typename M, typename... P>
        void execute(int32_t pThreadPoolId, M&& pMethod, P&&... pParameter)
        {
            if (!mInitialized)
                return;
        
            std::function<void()> task = std::bind(std::forward<M>(pMethod), std::forward<P>(pParameter)...);

            if (pThreadPoolId < 0)
            {
                enqueueMainThreadTask(std::move(task));
            }
            else
            {
                auto threadPool = mThreadPool.find(pThreadPoolId);
                assert(threadPool != mThreadPool.cend());
                threadPool->second->push(std::make_pair(nullptr, std::move(task)));
            }
        }
        
        template<typename M, typename... P>
        void executeContinuous(int32_t pThreadPoolId, std::function<bool()> pTerminateCondition, M&& pMethod, P&&... pParameter)
        {
            assert(pThreadPoolId >= 0 && pTerminateCondition != nullptr);
        
            if (!mInitialized)
                return;
        
            std::function<void()> task = std::bind(std::forward<M>(pMethod), std::forward<P>(pParameter)...);

            auto threadPool = mThreadPool.find(pThreadPoolId);
            assert(threadPool != mThreadPool.cend());
            threadPool->second->pushContinuous(std::make_pair(std::move(task), std::move(pTerminateCondition)));
        }
        
        template<typename M, typename... P>
        void executeOnMainThreadDelayed(int32_t pOffloadThreadPoolId, int32_t pDelayMs, M&& pMethod, P&&... pParameter)
        {
            assert(pOffloadThreadPoolId >= 0 && pDelayMs > 0);
            
            if (!mInitialized)
                return;

            auto threadPool = mThreadPool.find(pOffloadThreadPoolId);
            assert(threadPool != mThreadPool.cend());
            threadPool->second->push(std::make_pair(createCondition(pDelayMs), std::bind(std::forward<M>(pMethod), std::forward<P>(pParameter)...)));
        }

    private:
        friend class Hermes;
        
        template <typename T>
        struct AtomicWrapper
        {
            AtomicWrapper() : mValue()
            {
            }
            
            AtomicWrapper(T pValue) : mValue(pValue)
            {
            }

            AtomicWrapper(const std::atomic<T>& pOther) : mValue(pOther.load())
            {
            }

            AtomicWrapper(const AtomicWrapper& pOther) : mValue(pOther.mValue.load())
            {
            }

            AtomicWrapper &operator=(const AtomicWrapper& pOther)
            {
                mValue.store(pOther.mValue.load());
            }
            
            std::atomic<T> mValue;
        };

        class ThreadPool
        {
        public:
            ThreadPool(int32_t pId, size_t pThreadCount, TaskManager* pTaskManager);
            ThreadPool(const ThreadPool& pOther) = delete;
            ThreadPool(TaskManager&& pOther) = delete;
            ~ThreadPool();
            
            ThreadPool& operator=(const ThreadPool& pOther) = delete;
            ThreadPool& operator=(ThreadPool&& pOther) = delete;

            void flush(std::function<void()> pCallback);
            
            void push(std::pair<std::function<int32_t()>, std::function<void()>> pTask);
            void pushContinuous(std::pair<std::function<void()>, std::function<bool()>> pTask);
            
            void update(size_t pIndex);
            void terminate();
            
        private:
            std::vector<std::pair<std::thread, AtomicWrapper<uint32_t>>> mThreads;
            std::condition_variable mCondition;
            mutable std::mutex mMutex;
            std::queue<std::pair<std::function<int32_t()>, std::function<void()>>> mTask;
            std::queue<std::pair<std::function<void()>, std::function<bool()>>> mTaskContinuous;
            std::atomic<uint32_t> mTerminate {0};
            std::function<void()> mFlushCallback;
            int32_t mId;
            
            TaskManager* mTaskManager;
        };
    
        TaskManager();
        TaskManager(const TaskManager& pOther) = delete;
        TaskManager(TaskManager&& pOther) = delete;
        ~TaskManager();
        
        TaskManager& operator=(const TaskManager& pOther) = delete;
        TaskManager& operator=(TaskManager&& pOther) = delete;

        void enqueueMainThreadTask(std::function<void()> pTask);
        void dequeueMainThreadTask();
        std::function<int32_t()> createCondition(int32_t pDelayMs) const;

#if defined(ANDROID) || defined(__ANDROID__)
        static int32_t messageHandlerAndroid(int32_t pFd, int32_t pEvent, void* pData);
#endif

        std::unordered_map<int32_t, ThreadPool*> mThreadPool;

        std::queue<std::function<void()>> mMainThreadTask;
        mutable std::mutex mMainThreadMutex;

#if defined(ANDROID) || defined(__ANDROID__)
        int32_t mMessagePipeAndroid[2] = {0, 0};
        ALooper* mLooperAndroid = nullptr;
#endif

        std::function<void(std::function<void()>)> mMainThreadHandler;

        // TODO - Use atomic + compare_exchange_strong for init
        uint32_t mInitialized = {0};
    };

}

#endif
