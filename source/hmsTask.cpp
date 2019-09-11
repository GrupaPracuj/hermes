// Copyright (C) 2017-2019 Grupa Pracuj Sp. z o.o.
// This file is part of the "Hermes" library.
// For conditions of distribution and use, see copyright notice in license.txt.

#include "hmsTask.hpp"

#include <chrono>
#include <list>
#if defined(__APPLE__)
#import <Foundation/Foundation.h>
#elif defined(ANDROID) || defined(__ANDROID__)
#include <android/looper.h>
#include <fcntl.h>
#include <unistd.h>
#endif

namespace hms
{
    
    /* Spinlock */
    
    void Spinlock::lock() noexcept
    {
        while (mFlag.test_and_set(std::memory_order_acquire))
            std::this_thread::yield();
    }
    
    void Spinlock::unlock() noexcept
    {
        mFlag.clear(std::memory_order_release);
    }
    
    bool Spinlock::try_lock() noexcept
    {
        return !mFlag.test_and_set(std::memory_order_acquire);
    }
    
    /* TaskManager::ThreadPool */
    
    TaskManager::ThreadPool::ThreadPool(int32_t pId, size_t pThreadCount, TaskManager* pTaskManager) : mId(pId), mTaskManager(pTaskManager)
    {
        assert(pThreadCount > 0);
        mThreads.reserve(pThreadCount);
        for (size_t i = 0; i < pThreadCount; ++i)
        {
            mThreads.push_back({std::thread(), 0});
            mThreads[i].first = std::thread(&ThreadPool::update, this, i);
        }
    }
    
    TaskManager::ThreadPool::~ThreadPool()
    {
        for (size_t i = 0; i < mThreads.size(); ++i)
            mThreads[i].first.join();
    }
    
    void TaskManager::ThreadPool::flush(std::function<void()> pCallback)
    {
        for (size_t i = 0; i < mThreads.size(); ++i)
            mThreads[i].second.mValue.store(1);
        
        {
            std::lock_guard<std::mutex> lock(mMutex);
            mFlushCallback = std::move(pCallback);
        }
        
        mCondition.notify_all();
    }
    
    void TaskManager::ThreadPool::push(std::pair<std::function<int32_t()>, std::function<void()>> pTask)
    {
        if (mTerminate.load() == 0)
        {
            {
                std::lock_guard<std::mutex> lock(mMutex);
                mTask.push(std::move(pTask));
            }

            mCondition.notify_one();
        }
    }
    
    void TaskManager::ThreadPool::pushContinuous(std::pair<std::function<bool()>, std::function<void()>> pTask)
    {
        if (mTerminate.load() == 0)
        {
            {
                std::lock_guard<std::mutex> lock(mMutex);
                mTaskContinuous.push(std::move(pTask));
            }

            mCondition.notify_one();
        }
    }
    
    void TaskManager::ThreadPool::update(size_t pIndex)
    {
        std::list<std::pair<std::function<int32_t()>, std::function<void()>>> taskPaused;
        std::list<std::pair<std::function<bool()>, std::function<void()>>> taskContinuous;
        std::function<void()> flushCallback = nullptr;

        while (mTerminate.load() == 0)
        {
            std::unique_lock<std::mutex> lock(mMutex);

            if (taskPaused.size() == 0)
            {
                mCondition.wait(lock, [this, pIndex, &taskContinuous]
                {
                    return !mTask.empty() || !mTaskContinuous.empty() || !taskContinuous.empty() || mThreads[pIndex].second.mValue.load() > 0 || mTerminate.load() > 0;
                });
            }
            else
            {
                int32_t delay = 0;
                for (auto it = taskPaused.begin(); it != taskPaused.end(); ++it)
                {
                    int32_t result = it->first();
                    if (result == -1)
                    {
                        mTask.push({[]() -> int32_t { return -1; }, std::move(it->second)});
                        it = taskPaused.erase(it);
                    }
                    else if (delay == 0 || result < delay)
                    {
                        delay = result;
                    }
                }
            
                const auto timeout = std::chrono::steady_clock::now() + std::chrono::milliseconds(delay);
                mCondition.wait_until(lock, timeout, [this, pIndex, &taskContinuous]
                {
                    return !mTask.empty() || !mTaskContinuous.empty() || !taskContinuous.empty() || mThreads[pIndex].second.mValue.load() > 0 || mTerminate.load() > 0;
                });
            }
            
            if (mThreads[pIndex].second.mValue.load() > 0)
            {
                std::queue<std::pair<std::function<int32_t()>, std::function<void()>>>().swap(mTask);
                std::queue<std::pair<std::function<bool()>, std::function<void()>>>().swap(mTaskContinuous);
                taskPaused.clear();
                taskContinuous.clear();
                mThreads[pIndex].second.mValue.store(0);
                
                bool executeCallback = true;
                
                for (size_t i = 0; i < mThreads.size(); ++i)
                    executeCallback &= mThreads[i].second.mValue.load() == 0;
                
                if (executeCallback)
                {
                    flushCallback = std::move(mFlushCallback);
                    mFlushCallback = nullptr;
                }
            }

            int32_t condition = 1;
            std::function<void()> task = nullptr;

            if (!mTask.empty())
            {
                condition = mTask.front().first == nullptr ? 0 : mTask.front().first();
                if (condition <= 0)
                {
                    task = std::move(mTask.front().second);
                }
                else
                {
                    taskPaused.push_back(std::move(mTask.front()));
                }
                mTask.pop();
            }
            
            while (!mTaskContinuous.empty())
            {
                taskContinuous.push_back(std::move(mTaskContinuous.front()));
                mTaskContinuous.pop();
            }

            lock.unlock();

            if (condition == 0)
            {
                task();
            }
            else if (condition < 0)
            {
                mTaskManager->enqueueMainThreadTask(std::move(task));
            }
            
            for (auto it = taskContinuous.begin(); it != taskContinuous.end(); ++it)
            {
                if (!it->first())
                    it->second();
                else
                    it = taskContinuous.erase(it);
            }
            
            if (flushCallback != nullptr)
            {
                flushCallback();
                flushCallback = nullptr;
            }
        }
    }
    
    void TaskManager::ThreadPool::terminate()
    {
        for (size_t i = 0; i < mThreads.size(); ++i)
            mThreads[i].second.mValue.store(1);
            
        mTerminate.store(1);
        mCondition.notify_all();
    }

    /* TaskManager */
    
    TaskManager::TaskManager()
    {
    }
    
    TaskManager::~TaskManager()
    {
        terminate();
    }

    bool TaskManager::initialize(const std::vector<std::pair<int, size_t>> pThreadPool, std::function<void(std::function<void()>)> pMainThreadHandler)
    {
        if (mInitialized != 0)
            return false;
        
        mInitialized = 1;
        mMainThreadHandler = std::move(pMainThreadHandler);

#if defined(ANDROID) || defined(__ANDROID__)
        pipe2(mMessagePipeAndroid, O_NONBLOCK | O_CLOEXEC);
        mLooperAndroid = ALooper_forThread();
#endif

        mThreadPool.reserve(pThreadPool.size());

        for (auto& currentPool : pThreadPool)
        {
            assert(mThreadPool[currentPool.first] == nullptr);
            mThreadPool[currentPool.first] = new ThreadPool(currentPool.first, currentPool.second, this);
        }

        mInitialized = 2;

        return true;
    }
    
    bool TaskManager::terminate()
    {
        if (mInitialized != 2)
            return false;
        
        mInitialized = 1;
        
        for (auto& currentPool : mThreadPool)
            currentPool.second->terminate();
        
        for (auto& currentPool : mThreadPool)
            delete currentPool.second;
        
        mThreadPool.clear();

#if defined(ANDROID) || defined(__ANDROID__)
        if (mLooperAndroid != nullptr)
            ALooper_removeFd(mLooperAndroid, mMessagePipeAndroid[0]);

        mMessagePipeAndroid[0] = 0;
        mMessagePipeAndroid[1] = 0;
        mLooperAndroid = nullptr;
#endif

        mMainThreadHandler = nullptr;        
        flush(-1, nullptr);

        mInitialized = 0;
        
        return true;
    }
    
    void TaskManager::flush(int32_t pThreadPoolId, std::function<void()> pCallback)
    {
        if (pThreadPoolId < 0)
        {
            std::lock_guard<std::mutex> lock(mMainThreadMutex);
            std::queue<std::function<void()>>().swap(mMainThreadTask);
            
            if (pCallback != nullptr)
                pCallback();
        }
        else
        {
            auto threadPool = mThreadPool.find(pThreadPoolId);
            assert(threadPool != mThreadPool.cend());
            threadPool->second->flush(std::move(pCallback));
        }
    }

    void TaskManager::enqueueMainThreadTask(std::function<void()> pTask)
    {
        if (mMainThreadHandler != nullptr)
        {
            mMainThreadHandler(std::move(pTask));
        }
#if defined(__APPLE__)
        else
        {
            if ([NSThread isMainThread])
            {
                pTask();
            }
            else
            {
                {
                    std::lock_guard<std::mutex> lock(mMainThreadMutex);
                    mMainThreadTask.push(std::move(pTask));
                }

                dispatch_async(dispatch_get_main_queue(), ^
                {
                    dequeueMainThreadTask();
                });
            }
        }
#elif defined(ANDROID) || defined(__ANDROID__)
        else
        {
            {
                std::lock_guard<std::mutex> lock(mMainThreadMutex);
                mMainThreadTask.push(std::move(pTask));
            }

            if (mLooperAndroid != nullptr)
                ALooper_addFd(mLooperAndroid, mMessagePipeAndroid[0], ALOOPER_POLL_CALLBACK, ALOOPER_EVENT_INPUT, TaskManager::messageHandlerAndroid, this);

            int32_t eventId = 0;
            write(mMessagePipeAndroid[1], &eventId, sizeof(eventId));
        }
#endif
    }

    void TaskManager::dequeueMainThreadTask()
    {
        bool hasMainThreadTask = false;

        {
            std::lock_guard<std::mutex> lock(mMainThreadMutex);
            hasMainThreadTask = !mMainThreadTask.empty();
        }

        while (hasMainThreadTask)
        {
            std::function<void()> task = nullptr;

            {
                std::lock_guard<std::mutex> lock(mMainThreadMutex);
                task = std::move(mMainThreadTask.front());
                mMainThreadTask.pop();
                hasMainThreadTask = !mMainThreadTask.empty();
            }

            task();
        }
    }
    
    std::function<int32_t()> TaskManager::createCondition(int32_t pDelayMs) const
    {
        return [pDelayMs, startTime = std::chrono::steady_clock::now()]() -> int32_t
        {
            auto difference = std::chrono::duration_cast<std::chrono::duration<float, std::milli>>(std::chrono::steady_clock::now() - startTime).count();
            return difference >= pDelayMs ? -1 : pDelayMs - difference;
        };
    }

#if defined(ANDROID) || defined(__ANDROID__)
    int32_t TaskManager::messageHandlerAndroid(int32_t pFd, int32_t pEvent, void* pData)
    {
        TaskManager* taskManager = static_cast<TaskManager*>(pData);
        taskManager->dequeueMainThreadTask();

        return 0;
    }
#endif

}
