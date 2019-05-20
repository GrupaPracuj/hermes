// Copyright (C) 2017-2019 Grupa Pracuj Sp. z o.o.
// This file is part of the "Hermes" library.
// For conditions of distribution and use, see copyright notice in license.txt.

#include "hmsModule.hpp"

#include <limits>

namespace hms
{
namespace ext
{

    /* ModuleShared::Callbacks */
    
    void ModuleShared::Callbacks::attach(std::function<void(std::shared_ptr<ModuleShared>)> pCallback)
    {
        assert(pCallback != nullptr);
        mAttach = std::move(pCallback);
    }

    void ModuleShared::Callbacks::detach(std::function<void(std::pair<size_t, size_t>)> pCallback)
    {
        assert(pCallback != nullptr);
        mDetach = std::move(pCallback);
    }
    
    /* ModuleShared */

    const size_t ModuleShared::npos = std::numeric_limits<size_t>::max();
    
    ModuleShared::ModuleShared() : mIndices({npos, npos}), mCallbacks(std::make_unique<Callbacks>())
    {
    }
    
    bool ModuleShared::attached() const
    {
        return mIndices.first != npos && mIndices.second != npos;
    }
    
    std::pair<size_t, size_t> ModuleShared::indices() const
    {
        return mIndices;
    }
    
    /* ModuleShared::Callbacks */
    
    void ModuleHandler::Callbacks::stackChanged(std::function<void(size_t, std::shared_ptr<ModuleShared>)> pCallback)
    {
        assert(pCallback != nullptr);
        mStackChanged = std::move(pCallback);
    }
    
    /* ModuleHandler */
    
    ModuleHandler::ModuleHandler() : mCallbacks(std::make_unique<Callbacks>())
    {
    }

    std::shared_ptr<ModuleShared> ModuleHandler::main() const
    {
        return mMain;
    }
    
    void ModuleHandler::main(std::shared_ptr<ModuleShared> pModule)
    {
        assert(pModule == nullptr || pModule->indices() == std::make_pair(ModuleShared::npos, ModuleShared::npos));
        mMain = pModule;
    }
    
    size_t ModuleHandler::countPrimary() const
    {
        return mModules.size();
    }
    
    size_t ModuleHandler::countSecondary(size_t pPrimaryIndex) const
    {
        assert(pPrimaryIndex < mModules.size());
        return mModules[pPrimaryIndex].size();
    }
    
    std::shared_ptr<ModuleShared> ModuleHandler::get(const std::pair<size_t, size_t>& pIndices) const
    {
        assert(pIndices.first < mModules.size() && pIndices.second < mModules[pIndices.first].size());
        return mModules[pIndices.first][pIndices.second];
    }
    
    void ModuleHandler::push(size_t pIndexPrimary, std::shared_ptr<ModuleShared> pModule)
    {
        assert(pModule != nullptr && pModule.get() != mMain.get() && pIndexPrimary < ModuleShared::npos);
        
        if (pIndexPrimary >= mModules.size())
            mModules.resize(pIndexPrimary + 1);
        
        assert(mModules[pIndexPrimary].size() < ModuleShared::npos);
        mModules[pIndexPrimary].push_back(pModule);
        pModule->mIndices = {pIndexPrimary, mModules[pIndexPrimary].size() - 1};
        pModule->callbacks<ModuleShared::Callbacks>().mAttach(pModule);
        
        mCallbacks->mStackChanged(pIndexPrimary, pModule);
    }
    
    void ModuleHandler::pop(size_t pIndexPrimary, size_t pCount)
    {
        assert(pIndexPrimary < mModules.size() && pCount > 0);
        
        const size_t count = pCount > mModules[pIndexPrimary].size() ? mModules[pIndexPrimary].size() : pCount;
        erase({pIndexPrimary, mModules[pIndexPrimary].size() - count}, count);
    }
    
    void ModuleHandler::erase(const std::pair<size_t, size_t>& pIndices, size_t pCount)
    {
        assert(pIndices.first < mModules.size() && pIndices.second < mModules[pIndices.first].size() && pCount > 0);
        auto* stackPrimary = &mModules[pIndices.first];
        const size_t stackSize = stackPrimary->size();
        const size_t eraseEnd = std::min(pIndices.second + pCount, stackSize);
        const size_t eraseCount = std::max<size_t>(0, eraseEnd - pIndices.second);
        
        for (size_t i = stackSize; i > eraseEnd; --i)
            (*stackPrimary)[i - 1]->mIndices.second -= eraseCount;
        
        for (size_t i = eraseEnd, j = 1; i > pIndices.second; --i, j++)
        {
            (*stackPrimary)[i - 1]->mIndices = {ModuleShared::npos, ModuleShared::npos};
            (*stackPrimary)[i - 1]->callbacks<ModuleShared::Callbacks>().mDetach({eraseCount, j});
        }

        std::shared_ptr<ModuleShared> activeModule = nullptr;
        if (eraseEnd > pIndices.second)
        {
            stackPrimary->erase(stackPrimary->begin() + static_cast<std::ptrdiff_t>(pIndices.second), stackPrimary->begin() + static_cast<std::ptrdiff_t>(eraseEnd));
            if (stackPrimary->size() > 0)
                activeModule = stackPrimary->back();
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

        mCallbacks->mStackChanged(pIndices.first, pIndices.first < mModules.size() ? mModules[pIndices.first].size() > 0 ? mModules[pIndices.first].back() : nullptr : nullptr);
    }
    
    void ModuleHandler::clear()
    {
        std::vector<bool> stacks(mModules.size());
        for (size_t i = 0; i < mModules.size(); ++i)
            stacks[i] = mModules[i].size() > 0;
    
        for (size_t i = mModules.size(); i > 0; --i)
        {
            const size_t primaryIndex = i - 1;
            const size_t count = mModules[primaryIndex].size();
            for (size_t j = mModules[primaryIndex].size(), k = 1; j > 0; --j, k++)
            {
                mModules[primaryIndex][j - 1]->mIndices = {-1, -1};
                mModules[primaryIndex][j - 1]->callbacks<ModuleShared::Callbacks>().mDetach({count, k});
            }
        }
        mModules.clear();
        
        for (size_t i = stacks.size(); i > 0; ++i)
        {
            if (stacks[i - 1])
                mCallbacks->mStackChanged(i - 1, nullptr);
        }
    }
    
    ModuleHandler& ModuleHandler::instance()
    {
        static ModuleHandler instance;        
        return instance;
    }
    
}
}


