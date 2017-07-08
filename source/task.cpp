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
    
    void Spinlock::lock()
    {
        while (mFlag.test_and_set(std::memory_order_acquire)) {}
    }
    
    void Spinlock::unlock()
    {
        mFlag.clear(std::memory_order_release);
    }
    
    /* ThreadPool */
    
    ThreadPool::ThreadPool(int pID, size_t pThreadCount) : mThreadCount(pThreadCount), mId(pID)
    {
        mThread = new std::thread*[mThreadCount];
        mHasTask = new std::atomic<uint32_t>[mThreadCount];
        
        for (size_t i = 0; i < mThreadCount; ++i)
        {
            mHasTask[i] = 0;
            mThread[i] = new std::thread(&ThreadPool::update, this, i);
        }
        
        mIsActive.store(1);
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
        delete[] mHasTask;
    }

    bool ThreadPool::push(const std::function<void()>& pTask)
    {
		bool status = mIsActive.load() != 0;

		if (status)
		{
			std::lock_guard<std::mutex> lock(mTaskMutex);
			mTask.push(pTask);
			mTaskCondition.notify_one();
		}

		return status;
    }
    
    bool ThreadPool::hasTask()
    {
        bool status = false;
        
        for (size_t i = 0; i < mThreadCount; ++i)
            status |= (mHasTask[i].load() > 0);

		return status;
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
                task = mTask.front();
                mTask.pop();
            }
        }
        
        if (task != nullptr)
            task();
    }
    
    void ThreadPool::clearTask()
    {
        std::lock_guard<std::mutex> lock(mTaskMutex);
        std::queue<std::function<void()>>().swap(mTask);
    }
    
    void ThreadPool::update(size_t pID)
    {
        std::function<void()> task = nullptr;
        
        while (true)
        {
            std::unique_lock<std::mutex> lock(mTaskMutex);
            mTaskCondition.wait(lock, [this]
            {
                return !mTask.empty() || mForceFinish.load() > 0;
            });
            
            task = nullptr;

			bool lastTask = true;
            
            if (!mTask.empty())
            {
                mHasTask[pID].store(1);
                task = mTask.front();
                mTask.pop();

				lastTask = mTask.empty();
            }
            
            lock.unlock();
            
            if (task != nullptr)
                task();

			if (lastTask)
				mHasTask[pID].store(0);

            if (mForceFinish.load() > 0)
                break;
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

	bool TaskManager::initialize(const std::vector<std::pair<int, size_t>> pThreadPool)
    {
        if (mInitialized != 0)
            return false;
        
        mInitialized = 1;

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
            currentPool.second->stop();

        for (auto& currentPool : mThreadPool)
            delete currentPool.second;
        
        mThreadPool.clear();

#if defined(ANDROID) || defined(__ANDROID__)
        mLooperAndroid = nullptr;
#endif

        mInitialized = 0;
        
        return true;
    }
    
    void TaskManager::flush(int pThreadPoolID)
    {
        if (pThreadPoolID < 0)
            return;
    
        assert(mThreadPool.find(pThreadPoolID) != mThreadPool.end());
            
        ThreadPool* threadPool = mThreadPool[pThreadPoolID];
        threadPool->stop();
        
        while (threadPool->hasTask())
            std::this_thread::sleep_for(std::chrono::milliseconds(250));
        
        threadPool->start();
    }
    
    size_t TaskManager::threadCountForPool(int pThreadPoolID)
    {
        if (pThreadPoolID < 0)
            return 0;
        
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
            return;
        
        std::unordered_map<int, ThreadPool*>::const_iterator thread = mThreadPool.find(pThreadPoolID);
        assert(thread != mThreadPool.cend());
        
        thread->second->clearTask();
    }

    void TaskManager::enqueueMainThreadTask(std::function<void()> pMethod)
    {
#if defined(__APPLE__)
        if ([NSThread isMainThread])
        {
            pMethod();
        }
        else
        {
            {
                std::lock_guard<Spinlock> lock(mMainThreadSpinlock);
                mMainThreadTask.push(std::move(pMethod));
            }

            dispatch_async(dispatch_get_main_queue(), ^
            {
                dequeueMainThreadTask();
            });
        }
#elif defined(ANDROID) || defined(__ANDROID__)
        {
            std::lock_guard<Spinlock> lock(mMainThreadSpinlock);
            mMainThreadTask.push(std::move(pMethod));
        }

        if (mLooperAndroid != nullptr)
            ALooper_addFd(mLooperAndroid, mMessagePipeAndroid[0], 0, ALOOPER_EVENT_INPUT, TaskManager::messageHandlerAndroid, this);

        int eventId = 0;
        write(mMessagePipeAndroid[1], &eventId, sizeof(eventId));
#elif defined(__linux__)
        // TO-DO
        pMethod();
#endif
    }

    void TaskManager::dequeueMainThreadTask()
    {
        bool hasMainThreadTask = false;

        {
            std::lock_guard<Spinlock> lock(mMainThreadSpinlock);
            hasMainThreadTask = !mMainThreadTask.empty();
        }

        while (hasMainThreadTask)
        {
            std::function<void()> task = nullptr;

            {
                std::lock_guard<Spinlock> lock(mMainThreadSpinlock);
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
