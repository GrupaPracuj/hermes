// Copyright (C) 2017-2019 Grupa Pracuj Sp. z o.o.
// This file is part of the "Hermes" library.
// For conditions of distribution and use, see copyright notice in license.txt.

#ifndef _HMS_MODULE_HPP_
#define _HMS_MODULE_HPP_

#include <cassert>
#include <cstdint>
#include <functional>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

namespace hms
{
namespace ext
{
    class ModuleShared
    {
        friend class ModuleHandler;
    public:
        class Callbacks
        {
        public:
            Callbacks() = default;
            virtual ~Callbacks() = default;

            void attach(std::function<void(std::shared_ptr<ModuleShared> lpModule)> pCallback);
            
            /*
             * lpInfo.first - how many modules will be erased
             * lpInfo.second - order in erase sequence of this module
            */
            void detach(std::function<void(std::pair<size_t, size_t> lpInfo)> pCallback);

        private:
            friend class ModuleHandler;
        
            Callbacks(const Callbacks& pOther) = delete;
            Callbacks(Callbacks&& pOther) = delete;
            
            Callbacks& operator=(const Callbacks& pOther) = delete;
            Callbacks& operator=(Callbacks&& pOther) = delete;
            
            std::function<void(std::shared_ptr<ModuleShared>)> mAttach = [](std::shared_ptr<ModuleShared>) -> void {};
            std::function<void(std::pair<size_t, size_t>)> mDetach = [](std::pair<size_t, size_t>) -> void {};
        };
    
        template <typename T, typename = typename std::enable_if<std::is_base_of<ModuleShared, T>::value>::type, typename... U>
        static std::shared_ptr<T> create(U&&... pArgument)
        {
            std::shared_ptr<T> module(new T(std::forward<U>(pArgument)...), [](T* pThis) -> void { delete pThis; });
            module->mThis = module;
            module->createPost();
            
            return module;
        }

        bool attached() const;
        std::pair<size_t, size_t> indices() const;
        
        template <typename T, typename = typename std::enable_if<std::is_base_of<Callbacks, T>::value>::type>
        T& callbacks() const
        {
            assert(mCallbacks != nullptr);
            return static_cast<T&>(*mCallbacks.get());
        }
        
        static const size_t npos;
        
    protected:
        ModuleShared();
        virtual ~ModuleShared() = default;

        std::pair<size_t, size_t> mIndices;
        std::unique_ptr<Callbacks> mCallbacks;
        std::weak_ptr<ModuleShared> mThis;
        
    private:
        ModuleShared(const ModuleShared& pOther) = delete;
        ModuleShared(ModuleShared&& pOther) = delete;
        
        ModuleShared& operator=(const ModuleShared& pOther) = delete;
        ModuleShared& operator=(ModuleShared&& pOther) = delete;

        void createPost()
        {
        }
    };
    
    class ModuleHandler
    {
    public:
        class Callbacks
        {
        public:
            Callbacks() = default;
            virtual ~Callbacks() = default;
        
            void stackChanged(std::function<void(size_t lpIndexPrimary, std::shared_ptr<ModuleShared> lpModuleLast)> pCallback);

        private:
            friend class ModuleHandler;
        
            Callbacks(const Callbacks& pOther) = delete;
            Callbacks(Callbacks&& pOther) = delete;
            
            Callbacks& operator=(const Callbacks& pOther) = delete;
            Callbacks& operator=(Callbacks&& pOther) = delete;
            
            std::function<void(size_t, std::shared_ptr<ModuleShared>)> mStackChanged = [](size_t, std::shared_ptr<ModuleShared>) -> void {};
        };
    
        std::shared_ptr<ModuleShared> main() const;
        template <typename T, typename = typename std::enable_if<std::is_base_of<ModuleShared, T>::value>::type>
        std::shared_ptr<T> main() const
        {
            return std::static_pointer_cast<T>(main());
        }
        
        void main(std::shared_ptr<ModuleShared> pModule);
        template <typename T, typename = typename std::enable_if<std::is_base_of<ModuleShared, T>::value>::type>
        void main(std::shared_ptr<T> pModule)
        {
            return main(std::static_pointer_cast<ModuleShared>(pModule));
        }
        
        size_t countPrimary() const;
        size_t countSecondary(size_t pIndexPrimary) const;
        
        std::weak_ptr<ModuleShared> get(const std::pair<size_t, size_t>& pIndices) const;
        template <typename T, typename = typename std::enable_if<std::is_base_of<ModuleShared, T>::value>::type>
        std::weak_ptr<T> get(const std::pair<size_t, size_t>& pIndices) const
        {
            return std::static_pointer_cast<T>(get(pIndices).lock());
        }
        
        void push(size_t pIndexPrimary, std::shared_ptr<ModuleShared> pModule);
        void pop(size_t pIndexPrimary, size_t pCount = 1);
        void erase(const std::pair<size_t, size_t>& pIndices, size_t pCount = 1);
        void clear();

        static ModuleHandler& instance();
        
    private:
        ModuleHandler();
        ~ModuleHandler() = default;
        ModuleHandler(const ModuleHandler& pOther) = delete;
        ModuleHandler(ModuleHandler&& pOther) = delete;
        
        ModuleHandler& operator=(const ModuleHandler& pOther) = delete;
        ModuleHandler& operator=(ModuleHandler&& pOther) = delete;

        std::shared_ptr<ModuleShared> mMain;
        std::vector<std::vector<std::shared_ptr<ModuleShared>>> mModules;
        std::unique_ptr<Callbacks> mCallbacks;
    };
}
}

#endif
