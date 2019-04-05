// Copyright (C) 2017-2019 Grupa Pracuj Sp. z o.o.
// This file is part of the "Hermes" library.
// For conditions of distribution and use, see copyright notice in license.txt.

#include "hmsModule.hpp"

namespace hms
{
namespace ext
{

    /* ModuleShared::Callbacks */
    
    void ModuleShared::Callbacks::setAttach(std::function<void(std::shared_ptr<ModuleShared>)> pCallback)
    {
        assert(pCallback != nullptr);
        mAttach = std::move(pCallback);
    }

    void ModuleShared::Callbacks::setDetach(std::function<void(std::pair<size_t, size_t>)> pCallback)
    {
        assert(pCallback != nullptr);
        mDetach = std::move(pCallback);
    }
    
    /* ModuleShared */
    
    ModuleShared::ModuleShared() : mIndices({-1, -1}), mCallbacks(std::make_unique<Callbacks>())
    {
    }
    
    std::pair<int32_t, int32_t> ModuleShared::getIndices() const
    {
        return mIndices;
    }
    
    /* ModuleHandler */

    std::shared_ptr<ModuleShared> ModuleHandler::getMain() const
    {
        return mMainModule;
    }
    
    void ModuleHandler::setMain(std::shared_ptr<ModuleShared> pModule)
    {
        assert(pModule == nullptr || pModule->getIndices() == std::make_pair(-1, -1));
        mMainModule = pModule;
    }
    
    int32_t ModuleHandler::countPrimary() const
    {
        return static_cast<int32_t>(mModules.size());
    }
    
    int32_t ModuleHandler::countSecondary(int32_t pPrimaryIndex) const
    {
        assert(pPrimaryIndex >= 0 && static_cast<size_t>(pPrimaryIndex) < mModules.size());
        
        return static_cast<int32_t>(mModules[static_cast<size_t>(pPrimaryIndex)].size());
    }
    
    std::weak_ptr<ModuleShared> ModuleHandler::get(std::pair<int32_t, int32_t> pIndex) const
    {
        assert(pIndex.first >= 0 && static_cast<size_t>(pIndex.first) < mModules.size() && pIndex.second >= 0 && static_cast<size_t>(pIndex.second) < mModules[static_cast<size_t>(pIndex.first)].size());
        
        return mModules[static_cast<size_t>(pIndex.first)][static_cast<size_t>(pIndex.second)];
    }
    
    void ModuleHandler::push(int32_t pPrimaryIndex, std::shared_ptr<ModuleShared> pModule)
    {
        assert(pModule != nullptr && pModule.get() != mMainModule.get() && pPrimaryIndex >= 0 && pPrimaryIndex < INT32_MAX);
        
        if (static_cast<size_t>(pPrimaryIndex) >= mModules.size())
            mModules.resize(static_cast<size_t>(pPrimaryIndex) + 1);
        
        assert(mModules[static_cast<size_t>(pPrimaryIndex)].size() < INT32_MAX);
        mModules[static_cast<size_t>(pPrimaryIndex)].push_back(pModule);
        pModule->mIndices = {pPrimaryIndex, static_cast<int32_t>(mModules[static_cast<size_t>(pPrimaryIndex)].size()) - 1};
        pModule->getCallbacks<ModuleShared::Callbacks>()->mAttach(pModule);
        
        if (mStackChangedCallback != nullptr)
            mStackChangedCallback(pPrimaryIndex, pModule);
    }
    
    void ModuleHandler::pop(int32_t pPrimaryIndex, size_t pCount)
    {
        assert(pPrimaryIndex >= 0 && static_cast<size_t>(pPrimaryIndex) < mModules.size() && pCount > 0);
        
        const size_t count = pCount > mModules[static_cast<size_t>(pPrimaryIndex)].size() ? mModules[static_cast<size_t>(pPrimaryIndex)].size() : pCount;
        erase(pPrimaryIndex, static_cast<int32_t>(mModules[static_cast<size_t>(pPrimaryIndex)].size() - count), count);
    }
    
    void ModuleHandler::erase(int32_t pPrimaryIndex, int32_t pSecondaryIndex, size_t pCount)
    {
        assert(pPrimaryIndex >= 0 && static_cast<size_t>(pPrimaryIndex) < mModules.size() && pSecondaryIndex >= 0 && pSecondaryIndex < mModules[static_cast<size_t>(pPrimaryIndex)].size() && pCount > 0);
        auto* primaryStack = &mModules[static_cast<size_t>(pPrimaryIndex)];
        const int32_t stackSize = static_cast<int32_t>(primaryStack->size());
        const int32_t eraseEnd = std::min(pSecondaryIndex + static_cast<int32_t>(pCount), stackSize);
        const int32_t eraseCount = std::max(0, eraseEnd - pSecondaryIndex);
        
        for (int32_t i = stackSize - 1; i >= eraseEnd; --i)
            (*primaryStack)[static_cast<size_t>(i)]->mIndices.second -= eraseCount;
        
        for (int32_t i = eraseEnd - 1, j = 1; i >= pSecondaryIndex; --i, j++)
        {
            (*primaryStack)[static_cast<size_t>(i)]->mIndices = {-1, -1};
            (*primaryStack)[static_cast<size_t>(i)]->getCallbacks<ModuleShared::Callbacks>()->mDetach({static_cast<size_t>(eraseCount), static_cast<size_t>(j)});
        }

        std::shared_ptr<ModuleShared> activeModule = nullptr;
        if (eraseEnd > pSecondaryIndex)
        {
            primaryStack->erase(primaryStack->begin() + pSecondaryIndex, primaryStack->begin() + eraseEnd);
            if (primaryStack->size() > 0)
                activeModule = primaryStack->back();
        }

        auto eraseIt = mModules.end();
        for (auto it = mModules.rbegin(); it != mModules.rend(); ++it)
        {
            if (it->size() == 0)
                --eraseIt;
            else
                break;
        }
        if (eraseIt != mModules.end())
            mModules.erase(eraseIt, mModules.end());

        if (mStackChangedCallback != nullptr)
            mStackChangedCallback(pPrimaryIndex, static_cast<size_t>(pPrimaryIndex) < mModules.size() ? mModules[static_cast<size_t>(pPrimaryIndex)].size() > 0 ? mModules[static_cast<size_t>(pPrimaryIndex)].back() : nullptr : nullptr);
    }
    
    void ModuleHandler::clear()
    {
        for (size_t i = mModules.size(); i > 0; --i)
        {
            const size_t primaryIndex = i - 1;
            const size_t count = mModules[primaryIndex].size();
            for (size_t j = mModules[primaryIndex].size(), k = 1; j > 0; --j, k++)
            {
                mModules[primaryIndex][j - 1]->mIndices = {-1, -1};
                mModules[primaryIndex][j - 1]->getCallbacks<ModuleShared::Callbacks>()->mDetach({count, k});
            }
        }
        mModules.clear();
        
        if (mStackChangedCallback != nullptr)
            mStackChangedCallback(-1, nullptr);
    }
    
    void ModuleHandler::setStackChangedCallback(std::function<void(int32_t, std::shared_ptr<ModuleShared>)> pCallback)
    {
        mStackChangedCallback = std::move(pCallback);
    }
    
    ModuleHandler* ModuleHandler::getInstance()
    {
        static ModuleHandler instance;
        
        return &instance;
    }
    
}
}


