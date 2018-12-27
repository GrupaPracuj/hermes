// Copyright (C) 2017-2018 Grupa Pracuj Sp. z o.o.
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
    
    /* ThreadPool */
    
    ThreadPool::ThreadPool(int32_t pId, size_t pThreadCount, TaskManager* pTaskManager) : mThreadCount(pThreadCount), mId(pId), mTaskManager(pTaskManager)
    {
        mThread = new std::thread*[mThreadCount];

        for (size_t i = 0; i < mThreadCount; ++i)
            mThread[i] = new std::thread(&ThreadPool::update, this, i);
    }
    
    ThreadPool::~ThreadPool()
    {
        mTerminate.store(1);
        flush();
        mTaskCondition.notify_all();
        
        for (size_t i = 0; i < mThreadCount; ++i)
        {
            mThread[i]->join();
            delete mThread[i];
        }
        
        delete[] mThread;
    }

    void ThreadPool::push(std::pair<std::function<int32_t()>, std::function<void()>> pTask)
    {
        if (mTerminate.load() == 0)
        {
            {
                std::lock_guard<std::mutex> lock(mTaskMutex);
                mTask.push(std::move(pTask));
            }

            mTaskCondition.notify_one();
        }
    }
    
    bool ThreadPool::hasTask() const
    {
        size_t taskCount = 0;
        
        {    
            std::lock_guard<std::mutex> lock(mTaskMutex);
            taskCount = mTask.size();
        }
        
        return taskCount > 0 || mProcessingTaskCount.load() > 0;
    }
    
    void ThreadPool::flush()
    {
        std::lock_guard<std::mutex> lock(mTaskMutex);
        std::queue<std::pair<std::function<int32_t()>, std::function<void()>>>().swap(mTask);
    }
    
    void ThreadPool::performTaskIfExists()
    {
        int32_t condition = 1;
        std::function<void()> task = nullptr;
        
        {
            std::lock_guard<std::mutex> lock(mTaskMutex);
            if (!mTask.empty())
            {
                condition = mTask.front().first == nullptr ? 0 : mTask.front().first();
                if (condition != 0)
                {
                    mProcessingTaskCount++;
                    task = std::move(mTask.front().second);
                    mTask.pop();
                }
            }
        }
        
        if (condition == 0)
        {
            task();
            mProcessingTaskCount--;
        }
        else if (condition < 0)
        {
            mTaskManager->enqueueMainThreadTask(std::move(task));
            mProcessingTaskCount--;
        }
    }
    
    void ThreadPool::update(size_t pId)
    {
        std::list<std::pair<std::function<int32_t()>, std::function<void()>>> pausedTasks;
    
        while (mTerminate.load() == 0)
        {
            std::unique_lock<std::mutex> lock(mTaskMutex);
            if (pausedTasks.size() == 0)
            {
                mTaskCondition.wait(lock, [this]
                {
                    return !mTask.empty() || mTerminate.load() > 0;
                });
            }
            else
            {
                int32_t delay = 0;
                for (auto it = pausedTasks.begin(); it != pausedTasks.end(); ++it)
                {
                    int32_t result = it->first();
                    if (result == -1)
                    {
                        mTask.push({[]() -> int32_t { return -1; }, std::move(it->second)});
                        it = pausedTasks.erase(it);
                    }
                    else if (delay == 0 || result < delay)
                    {
                        delay = result;
                    }
                }
            
                const auto timeout = std::chrono::steady_clock::now() + std::chrono::milliseconds(delay);
                mTaskCondition.wait_until(lock, timeout, [this]
                {
                    return !mTask.empty() || mTerminate.load() > 0;
                });
            }

            int32_t condition = 1;
            std::function<void()> task = nullptr;

            if (!mTask.empty())
            {
                condition = mTask.front().first == nullptr ? 0 : mTask.front().first();
                if (condition <= 0)
                {
                    mProcessingTaskCount++;
                    task = std::move(mTask.front().second);
                }
                else
                {
                    pausedTasks.push_back(std::move(mTask.front()));
                }
                mTask.pop();
            }
            
            lock.unlock();
            
            if (condition == 0)
            {
                task();
                mProcessingTaskCount--;
            }
            else if (condition < 0)
            {
                mTaskManager->enqueueMainThreadTask(std::move(task));
                mProcessingTaskCount--;
            }
        }
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
        {
            currentPool.second->mTerminate.store(1);
            currentPool.second->flush();
        }
        
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
        flush(-1);

        mInitialized = 0;
        
        return true;
    }
    
    bool TaskManager::hasTask(int32_t pThreadPoolId) const
    {
        bool hasTask = false;
    
        if (pThreadPoolId < 0)
        {
            std::lock_guard<std::mutex> lock(mMainThreadMutex);
            hasTask = !mMainThreadTask.empty();
        }
        else
        {
            auto threadPool = mThreadPool.find(pThreadPoolId);
            assert(threadPool != mThreadPool.cend());
            hasTask = threadPool->second->hasTask();
        }
        
        return hasTask;
    }
    
    void TaskManager::flush(int32_t pThreadPoolId)
    {
        if (pThreadPoolId < 0)
        {
            std::lock_guard<std::mutex> lock(mMainThreadMutex);
            std::queue<std::function<void()>>().swap(mMainThreadTask);
        }
        else
        {
            auto threadPool = mThreadPool.find(pThreadPoolId);
            assert(threadPool != mThreadPool.cend());
            threadPool->second->flush();
        }
    }
    
    size_t TaskManager::threadCountForPool(int32_t pThreadPoolId) const
    {
        if (pThreadPoolId < 0)
            return 1;
        
        auto threadPool = mThreadPool.find(pThreadPoolId);
        assert(threadPool != mThreadPool.cend());
        
        return threadPool->second->mThreadCount;
    }
    
    bool TaskManager::canContinueTask(int32_t pThreadPoolId) const
    {
        if (pThreadPoolId < 0)
            return false;
        
        auto threadPool = mThreadPool.find(pThreadPoolId);
        assert(threadPool != mThreadPool.cend());
        
        return threadPool->second->mTerminate.load() == 0;
    }
    
    void TaskManager::performTaskIfExists(int32_t pThreadPoolId) const
    {
        if (pThreadPoolId < 0)
            return;
        
        auto threadPool = mThreadPool.find(pThreadPoolId);
        assert(threadPool != mThreadPool.cend());
        
        threadPool->second->performTaskIfExists();
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
