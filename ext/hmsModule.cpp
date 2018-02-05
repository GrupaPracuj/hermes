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
    
    std::pair<int, int> ModuleShared::getIndex() const
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
    
    int ModuleHandler::countPrimary() const
    {
        return static_cast<int>(mModule.size());
    }
    
    int ModuleHandler::countSecondary(int pPrimaryIndex) const
    {
        assert(pPrimaryIndex >= 0 && static_cast<size_t>(pPrimaryIndex) < mModule.size());
        
        return static_cast<int>(mModule[static_cast<size_t>(pPrimaryIndex)].size());
    }
    
    std::shared_ptr<ModuleShared> ModuleHandler::getMain() const
    {
        return mMainModule;
    }
    
    void ModuleHandler::setMain(std::shared_ptr<ModuleShared> pModule)
    {
        mMainModule = pModule;
    }
    
    std::weak_ptr<ModuleShared> ModuleHandler::get(std::pair<int, int> pIndex) const
    {
        assert(pIndex.first >= 0 && static_cast<size_t>(pIndex.first) < mModule.size() && pIndex.second >= 0 && static_cast<size_t>(pIndex.second) < mModule[static_cast<size_t>(pIndex.first)].size());
        
        return mModule[static_cast<size_t>(pIndex.first)][static_cast<size_t>(pIndex.second)];
    }
    
    void ModuleHandler::push(int pPrimaryIndex, std::shared_ptr<ModuleShared> pModule)
    {
        assert(pPrimaryIndex >= 0 && pModule != nullptr);
        
        if (static_cast<size_t>(pPrimaryIndex) >= mModule.size())
            mModule.resize(static_cast<size_t>(pPrimaryIndex) + 1);
        
        mModule[static_cast<size_t>(pPrimaryIndex)].push_back(pModule);
        pModule->mIndex = {pPrimaryIndex, static_cast<int>(mModule[static_cast<size_t>(pPrimaryIndex)].size()) - 1};
        pModule->attach(pModule);
        
        if (mOnChangeCallback != nullptr)
            mOnChangeCallback();
    }
    
    void ModuleHandler::pop(int pPrimaryIndex, size_t pCount)
    {
        assert(pPrimaryIndex >= 0 && pPrimaryIndex < static_cast<int>(mModule.size()) && pCount > 0);
        
        const size_t stackSize = mModule[static_cast<size_t>(pPrimaryIndex)].size();
        erase(mModule[static_cast<size_t>(pPrimaryIndex)][stackSize > pCount ? stackSize - pCount : 0], pCount);
        
        if (mOnChangeCallback != nullptr)
            mOnChangeCallback();
    }
    
    void ModuleHandler::erase(std::shared_ptr<ModuleShared> pModule, size_t pCount)
    {
        assert(pModule != nullptr && pModule->getIndex().first >= 0 &&  pModule->getIndex().second >= 0 && pCount > 0);
        
        const auto index = pModule->getIndex();
        auto* primaryStack = &mModule[static_cast<size_t>(index.first)];
        const int stackSize = static_cast<int>(primaryStack->size());
        const int eraseBegin = index.second;
        const int eraseEnd = std::min(eraseBegin + static_cast<int>(pCount), stackSize);
        
        for (int i = stackSize - 1; i >= eraseEnd; --i)
            (*primaryStack)[static_cast<size_t>(i)]->mIndex.second -= static_cast<int>(pCount);
        
        for (int i = eraseEnd - 1; i >= eraseBegin; --i)
            (*primaryStack)[static_cast<size_t>(i)]->detach();
        
        if (eraseEnd > eraseBegin)
        {
            primaryStack->erase(primaryStack->begin() + eraseBegin, primaryStack->begin() + eraseEnd);
            
            if (mOnChangeCallback != nullptr)
                mOnChangeCallback();
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
    }
    
    void ModuleHandler::clear()
    {
        mModule.clear();
        
        if (mOnChangeCallback != nullptr)
            mOnChangeCallback();
    }
    
    void ModuleHandler::setOnChangeCallback(std::function<void()> pCallback)
    {
        mOnChangeCallback = std::move(pCallback);
    }
    
    ModuleHandler* ModuleHandler::getInstance()
    {
        static ModuleHandler instance;
        
        return &instance;
    }
    
}
}


