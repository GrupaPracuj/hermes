// Copyright (C) 2017 Grupa Pracuj Sp. z o.o.
// This file is part of the "Hermes" library.
// For conditions of distribution and use, see copyright notice in license.txt.

#include "task.hpp"

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
        while (mFlag.test_and_set(std::memory_order_acquire)) {}
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
    
    ThreadPool::ThreadPool(int pID, size_t pThreadCount) : mThreadCount(pThreadCount), mId(pID)
    {
        mThread = new std::thread*[mThreadCount];

        for (size_t i = 0; i < mThreadCount; ++i)
            mThread[i] = new std::thread(&ThreadPool::update, this, i);
    }
    
    ThreadPool::~ThreadPool()
    {
        mForceFinish.store(1);
        mIsActive.store(0);
        mTaskCondition.notify_all();
        
        for (size_t i = 0; i < mThreadCount; ++i)
        {
            mThread[i]->join();
            delete mThread[i];
        }
        
        delete[] mThread;
    }

    bool ThreadPool::push(std::function<void()>& pTask)
    {
        bool status = mIsActive.load() != 0;

        if (status)
        {
            {
                std::lock_guard<std::mutex> lock(mTaskMutex);
                mTask.push(std::move(pTask));
            }

            mTaskCondition.notify_one();
        }

        return status;
    }
    
    bool ThreadPool::hasTask()
    {
        size_t taskCount = 0;
        
        {    
            std::lock_guard<std::mutex> lock(mTaskMutex);
            taskCount = mTask.size();
        }
        
        return taskCount > 0 || mProcessingTaskCount.load() > 0;
    }
    
    void ThreadPool::start()
    {
        if (mForceFinish.load() == 0)
            mIsActive.store(1);
    }
    
    void ThreadPool::stop()
    {
        mIsActive.store(0);
    }
    
    size_t ThreadPool::threadCount()
    {
        return mThreadCount;
    }
    
    void ThreadPool::performTaskIfExists()
    {
        std::function<void()> task = nullptr;
        {
            std::lock_guard<std::mutex> lock(mTaskMutex);
            if (!mTask.empty())
            {
                mProcessingTaskCount++;
                task = std::move(mTask.front());
                mTask.pop();
            }
        }
        
        if (task != nullptr)
        {
            task();
            mProcessingTaskCount--;
        }
    }
    
    void ThreadPool::clearTask()
    {
        std::lock_guard<std::mutex> lock(mTaskMutex);
        std::queue<std::function<void()>>().swap(mTask);
    }
    
    void ThreadPool::update(size_t pID)
    {
        std::function<void()> task = nullptr;
        
        while (mForceFinish.load() == 0)
        {
            std::unique_lock<std::mutex> lock(mTaskMutex);
            mTaskCondition.wait(lock, [this]
            {
                return !mTask.empty() || mForceFinish.load() > 0;
            });
            
            task = nullptr;

            if (!mTask.empty())
            {
                mProcessingTaskCount++;
                task = std::move(mTask.front());
                mTask.pop();
            }
            
            lock.unlock();
            
            if (task != nullptr)
            {
                task();
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
            // check if ID is in use
            assert(mThreadPool[currentPool.first] == nullptr);
            
            mThreadPool[currentPool.first] = new ThreadPool(currentPool.first, currentPool.second);
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
            currentPool.second->stop();
            currentPool.second->clearTask();
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
        std::queue<std::function<void()>>().swap(mMainThreadTask);

        mInitialized = 0;
        
        return true;
    }
    
    void TaskManager::flush(int pThreadPoolID)
    {
        if (pThreadPoolID < 0)
        {
            dequeueMainThreadTask();
        }
        else
        {
            assert(mThreadPool.find(pThreadPoolID) != mThreadPool.end());
                
            ThreadPool* threadPool = mThreadPool[pThreadPoolID];
            threadPool->stop();
            
            while (threadPool->hasTask())
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            
            threadPool->start();
        }
    }
    
    size_t TaskManager::threadCountForPool(int pThreadPoolID)
    {
        if (pThreadPoolID < 0)
            return 1;
        
        std::unordered_map<int, ThreadPool*>::const_iterator thread = mThreadPool.find(pThreadPoolID);
        assert(thread != mThreadPool.cend());
        
        return thread->second->threadCount();
    }
    
    bool TaskManager::canContinueTask(int pThreadPoolID)
    {
        if (pThreadPoolID < 0)
            return false;
        
        std::unordered_map<int, ThreadPool*>::const_iterator thread = mThreadPool.find(pThreadPoolID);
        assert(thread != mThreadPool.cend());
        
        return thread->second->mIsActive.load() != 0 && thread->second->mForceFinish.load() == 0;
    }
    
    void TaskManager::performTaskIfExists(int pThreadPoolID)
    {
        if (pThreadPoolID < 0)
            return;
        
        std::unordered_map<int, ThreadPool*>::const_iterator thread = mThreadPool.find(pThreadPoolID);
        assert(thread != mThreadPool.cend());
        
        thread->second->performTaskIfExists();
    }
    
    void TaskManager::clearTask(int pThreadPoolID)
    {
        if (pThreadPoolID < 0)
        {
            std::lock_guard<std::mutex> lock(mMainThreadMutex);
            std::queue<std::function<void()>>().swap(mMainThreadTask);
        }
        else
        {
            std::unordered_map<int, ThreadPool*>::const_iterator thread = mThreadPool.find(pThreadPoolID);
            assert(thread != mThreadPool.cend());
            
            thread->second->clearTask();
        }
    }

    void TaskManager::enqueueMainThreadTask(std::function<void()> pTask)
    {
#if defined(__APPLE__)
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
#elif defined(ANDROID) || defined(__ANDROID__)
        {
            std::lock_guard<std::mutex> lock(mMainThreadMutex);
            mMainThreadTask.push(std::move(pTask));
        }

        if (mLooperAndroid != nullptr)
            ALooper_addFd(mLooperAndroid, mMessagePipeAndroid[0], ALOOPER_POLL_CALLBACK, ALOOPER_EVENT_INPUT, TaskManager::messageHandlerAndroid, this);

        int eventId = 0;
        write(mMessagePipeAndroid[1], &eventId, sizeof(eventId));
#else
        if (mMainThreadHandler != nullptr)
            mMainThreadHandler(std::move(pTask));
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

#if defined(ANDROID) || defined(__ANDROID__)
    int TaskManager::messageHandlerAndroid(int pFd, int pEvent, void* pData)
    {
        TaskManager* taskManager = static_cast<TaskManager*>(pData);
        taskManager->dequeueMainThreadTask();

        return 0;
    }
#endif

}
