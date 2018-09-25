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
        
            void setAttach(std::function<void(std::shared_ptr<ModuleShared> lpModule)> pCallback);
            
            /*
             * lpInfo.first - how many modules will be erased
             * lpInfo.second - order in erase sequence of this module
            */
            void setDetach(std::function<void(std::pair<size_t, size_t> lpInfo)> pCallback);

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
            module->postCreate();
            
            return module;
        }

        std::pair<int32_t, int32_t> getIndices() const;
        
        template <typename T, typename = typename std::enable_if<std::is_base_of<Callbacks, T>::value>::type>
        T* getCallbacks() const
        {
            assert(mCallbacks != nullptr);
            return static_cast<T*>(mCallbacks.get());
        }
        
    protected:
        ModuleShared();
        virtual ~ModuleShared() = default;

        std::pair<int32_t, int32_t> mIndices;
        std::unique_ptr<Callbacks> mCallbacks;
        std::weak_ptr<ModuleShared> mThis;
        
    private:
        ModuleShared(const ModuleShared& pOther) = delete;
        ModuleShared(ModuleShared&& pOther) = delete;
        
        ModuleShared& operator=(const ModuleShared& pOther) = delete;
        ModuleShared& operator=(ModuleShared&& pOther) = delete;

        void postCreate()
        {
        }
    };
    
    class ModuleHandler
    {
    public:
        std::shared_ptr<ModuleShared> getMain() const;
        template <typename T, typename = typename std::enable_if<std::is_base_of<ModuleShared, T>::value>::type>
        std::shared_ptr<T> getMain() const
        {
            return std::static_pointer_cast<T>(getMain());
        }
        
        void setMain(std::shared_ptr<ModuleShared> pModule);
        template <typename T, typename = typename std::enable_if<std::is_base_of<ModuleShared, T>::value>::type>
        void setMain(std::shared_ptr<T> pModule)
        {
            return setMain(std::static_pointer_cast<ModuleShared>(pModule));
        }
        
        int32_t countPrimary() const;
        int32_t countSecondary(int32_t pPrimaryIndex) const;
        
        std::weak_ptr<ModuleShared> get(std::pair<int32_t, int32_t> pIndex) const;
        template <typename T, typename = typename std::enable_if<std::is_base_of<ModuleShared, T>::value>::type>
        std::weak_ptr<T> get(std::pair<int32_t, int32_t> pIndex) const
        {
            return std::static_pointer_cast<T>(get(pIndex));
        }
        
        void push(int32_t pPrimaryIndex, std::shared_ptr<ModuleShared> pModule);
        void pop(int32_t pPrimaryIndex, size_t pCount = 1);
        void erase(int32_t pPrimaryIndex, int32_t pSecondaryIndex, size_t pCount = 1);
        void clear();
        
        void setStackChangedCallback(std::function<void(int32_t pPrimaryStack, std::shared_ptr<ModuleShared> lpLastModule)> pCallback);
        
        static ModuleHandler* getInstance();
        
    private:
        ModuleHandler() = default;
        ~ModuleHandler() = default;
        ModuleHandler(const ModuleHandler& pOther) = delete;
        ModuleHandler(ModuleHandler&& pOther) = delete;
        
        ModuleHandler& operator=(const ModuleHandler& pOther) = delete;
        ModuleHandler& operator=(ModuleHandler&& pOther) = delete;

        std::shared_ptr<ModuleShared> mMainModule;
        std::vector<std::vector<std::shared_ptr<ModuleShared>>> mModules;
        
        std::function<void(int32_t, std::shared_ptr<ModuleShared>)> mStackChangedCallback;
    };
}
}

#endif
