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
        assert(pModule == nullptr || pModule->getIndex() == std::make_pair(-1, -1));
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
        assert(pModule != nullptr && pModule.get() != mMainModule.get() && pPrimaryIndex >= 0 && pPrimaryIndex < INT32_MAX);
        
        if (static_cast<size_t>(pPrimaryIndex) >= mModule.size())
            mModule.resize(static_cast<size_t>(pPrimaryIndex) + 1);
        
        assert(mModule[static_cast<size_t>(pPrimaryIndex)].size() < INT32_MAX);
        mModule[static_cast<size_t>(pPrimaryIndex)].push_back(pModule);
        pModule->mIndex = {pPrimaryIndex, static_cast<int32_t>(mModule[static_cast<size_t>(pPrimaryIndex)].size()) - 1};
        pModule->attach(pModule);
        
        if (mOnActive != nullptr)
            mOnActive(pModule);
    }
    
    void ModuleHandler::pop(int32_t pPrimaryIndex, size_t pCount)
    {
        assert(pPrimaryIndex >= 0 && static_cast<size_t>(pPrimaryIndex) < mModule.size() && pCount > 0);
        erase(pPrimaryIndex, std::max(0, static_cast<int32_t>(mModule[static_cast<size_t>(pPrimaryIndex)].size()) - 1), pCount);
    }
    
    void ModuleHandler::erase(int32_t pPrimaryIndex, int32_t pSecondaryIndex, size_t pCount)
    {
        assert(pPrimaryIndex >= 0 && static_cast<size_t>(pPrimaryIndex) < mModule.size() && pSecondaryIndex >= 0 && pSecondaryIndex < mModule[static_cast<size_t>(pPrimaryIndex)].size() && pCount > 0);
        auto* primaryStack = &mModule[static_cast<size_t>(pPrimaryIndex)];
        const int32_t stackSize = static_cast<int32_t>(primaryStack->size());
        const int32_t eraseEnd = std::min(pSecondaryIndex + static_cast<int32_t>(pCount), stackSize);
        const int32_t eraseCount = std::max(0, eraseEnd - pSecondaryIndex);
        
        for (int32_t i = stackSize - 1; i >= eraseEnd; --i)
            (*primaryStack)[static_cast<size_t>(i)]->mIndex.second -= eraseCount;
        
        for (int32_t i = eraseEnd - 1; i >= pSecondaryIndex; --i)
        {
            (*primaryStack)[static_cast<size_t>(i)]->mIndex = {-1, -1};
            (*primaryStack)[static_cast<size_t>(i)]->detach();
        }

        if (eraseEnd > pSecondaryIndex)
            primaryStack->erase(primaryStack->begin() + pSecondaryIndex, primaryStack->begin() + eraseEnd);
        
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

        if (eraseEnd > pSecondaryIndex && mOnActive != nullptr)
            mOnActive(primaryStack->size() > 0 ? primaryStack->back() : nullptr);
    }
    
    void ModuleHandler::clear()
    {
        for (size_t i = mModule.size(); i > 0; --i)
        {
            const size_t primaryIndex = i - 1;
            for (size_t j = mModule[primaryIndex].size(); j > 0; --j)
            {
                mModule[primaryIndex][j - 1]->mIndex = {-1, -1};
                mModule[primaryIndex][j - 1]->detach();
            }
        }
        mModule.clear();
        
        if (mOnActive != nullptr)
            mOnActive(nullptr);
    }
    
    void ModuleHandler::setOnActiveCallback(std::function<void(std::shared_ptr<ModuleShared>)> pCallback)
    {
        mOnActive = std::move(pCallback);
    }
    
    ModuleHandler* ModuleHandler::getInstance()
    {
        static ModuleHandler instance;
        
        return &instance;
    }
    
}
}


