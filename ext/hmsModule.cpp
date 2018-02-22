#include "hmsModule.hpp"

#include <cassert>

namespace hms
{
namespace ext
{
    
    /* ModuleShared */
    
    void ModuleShared::setOnAttachCallback(std::function<void(std::shared_ptr<ModuleShared>)> pCallback)
    {
        mOnAttach = std::move(pCallback);
    }
    
    void ModuleShared::setOnDetachCallback(std::function<void()> pCallback)
    {
        mOnDetach = std::move(pCallback);
    }
    
    std::pair<int32_t, int32_t> ModuleShared::getIndex() const
    {
        return mIndex;
    }
    
    void ModuleShared::attach(std::shared_ptr<ModuleShared> pThis)
    {
        if (mOnAttach != nullptr)
            mOnAttach(pThis);
    }
    
    void ModuleShared::detach()
    {
        if (mOnDetach != nullptr)
            mOnDetach();
    }
    
    /* ModuleHandler */

    std::shared_ptr<ModuleShared> ModuleHandler::getMain() const
    {
        return mMainModule;
    }
    
    void ModuleHandler::setMain(std::shared_ptr<ModuleShared> pModule)
    {
        mMainModule = pModule;
    }
    
    int32_t ModuleHandler::countPrimary() const
    {
        return static_cast<int32_t>(mModule.size());
    }
    
    int32_t ModuleHandler::countSecondary(int32_t pPrimaryIndex) const
    {
        assert(pPrimaryIndex >= 0 && static_cast<size_t>(pPrimaryIndex) < mModule.size());
        
        return static_cast<int32_t>(mModule[static_cast<size_t>(pPrimaryIndex)].size());
    }
    
    std::weak_ptr<ModuleShared> ModuleHandler::get(std::pair<int32_t, int32_t> pIndex) const
    {
        assert(pIndex.first >= 0 && static_cast<size_t>(pIndex.first) < mModule.size() && pIndex.second >= 0 && static_cast<size_t>(pIndex.second) < mModule[static_cast<size_t>(pIndex.first)].size());
        
        return mModule[static_cast<size_t>(pIndex.first)][static_cast<size_t>(pIndex.second)];
    }
    
    void ModuleHandler::push(int32_t pPrimaryIndex, std::shared_ptr<ModuleShared> pModule)
    {
        assert(pModule != nullptr && pPrimaryIndex >= 0 && (static_cast<size_t>(pPrimaryIndex) >= mModule.size() || mModule[static_cast<size_t>(pPrimaryIndex)].size() < INT32_MAX));
        
        if (static_cast<size_t>(pPrimaryIndex) >= mModule.size())
            mModule.resize(static_cast<size_t>(pPrimaryIndex) + 1);
        
        mModule[static_cast<size_t>(pPrimaryIndex)].push_back(pModule);
        pModule->mIndex = {pPrimaryIndex, static_cast<int32_t>(mModule[static_cast<size_t>(pPrimaryIndex)].size()) - 1};
        pModule->attach(pModule);
        
        if (mOnActiveCallback != nullptr)
            mOnActiveCallback(pModule);
    }
    
    void ModuleHandler::pop(int32_t pPrimaryIndex, size_t pCount)
    {
        assert(pPrimaryIndex >= 0 && pPrimaryIndex < static_cast<int32_t>(mModule.size()) && pCount > 0);
        
        const size_t stackSize = mModule[static_cast<size_t>(pPrimaryIndex)].size();
        erase(mModule[static_cast<size_t>(pPrimaryIndex)][stackSize > pCount ? stackSize - pCount : 0], pCount);
    }
    
    void ModuleHandler::erase(std::shared_ptr<ModuleShared> pModule, size_t pCount)
    {
        assert(pModule != nullptr && pModule->getIndex().first >= 0 &&  pModule->getIndex().second >= 0 && pCount > 0);
        
        const auto index = pModule->getIndex();
        auto* primaryStack = &mModule[static_cast<size_t>(index.first)];
        const int32_t stackSize = static_cast<int32_t>(primaryStack->size());
        const int32_t eraseBegin = index.second;
        const int32_t eraseEnd = std::min(eraseBegin + static_cast<int32_t>(pCount), stackSize);
        
        for (int32_t i = stackSize - 1; i >= eraseEnd; --i)
            (*primaryStack)[static_cast<size_t>(i)]->mIndex.second -= static_cast<int32_t>(pCount);
        
        for (int32_t i = eraseEnd - 1; i >= eraseBegin; --i)
            (*primaryStack)[static_cast<size_t>(i)]->detach();

        std::shared_ptr<ModuleShared> activeModule = mMainModule;
        
        if (eraseEnd > eraseBegin)
        {
            primaryStack->erase(primaryStack->begin() + eraseBegin, primaryStack->begin() + eraseEnd);
            const size_t newStackSize = primaryStack->size();
            
            if (newStackSize > 0)
                activeModule = (*primaryStack)[newStackSize - 1];
        }
        
        auto eraseIt = mModule.end();
        
        for (auto it = mModule.rbegin(); it != mModule.rend(); ++it)
        {
            if (it->size() == 0)
                --eraseIt;
            else
                break;
        }
        
        if (eraseIt != mModule.end())
            mModule.erase(eraseIt, mModule.end());
        
        if (eraseEnd > eraseBegin)
        {
            if (mOnActiveCallback != nullptr)
                mOnActiveCallback(activeModule);
        }
    }
    
    void ModuleHandler::clear()
    {
        mModule.clear();
        
        if (mOnActiveCallback != nullptr)
            mOnActiveCallback(mMainModule);
    }
    
    void ModuleHandler::setOnActiveCallback(std::function<void(std::shared_ptr<ModuleShared>)> pCallback)
    {
        mOnActiveCallback = std::move(pCallback);
    }
    
    ModuleHandler* ModuleHandler::getInstance()
    {
        static ModuleHandler instance;
        
        return &instance;
    }
    
}
}


