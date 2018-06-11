#ifndef _HMS_MODULE_HPP_
#define _HMS_MODULE_HPP_

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
        template <typename T, typename = typename std::enable_if<std::is_base_of<ModuleShared, T>::value>::type, typename... U>
        static std::shared_ptr<T> create(U&&... pArgument)
        {
            std::shared_ptr<T> module(new T(std::forward<U>(pArgument)...), std::bind(&ModuleShared::invokeDelete<T>, std::placeholders::_1));
            module->mThis = module;
            
            return module;
        }
        
        void setOnAttachCallback(std::function<void(std::shared_ptr<ModuleShared> lpModule)> pCallback);
        
        /*
         * lpInfo.first - how many modules will be erased
         * lpInfo.second - order in erase sequence of this module
        */
        void setOnDetachCallback(std::function<void(std::pair<size_t, size_t> lpInfo)> pCallback);

        std::pair<int32_t, int32_t> getIndex() const;
        
    protected:
        ModuleShared() = default;
        virtual ~ModuleShared() = default;
        
        void attach(std::shared_ptr<ModuleShared> pThis);
        void detach(std::pair<size_t, size_t> pInfo);
        
        std::weak_ptr<ModuleShared> mThis;
        std::pair<int32_t, int32_t> mIndex = {-1, -1};
        
    private:
        ModuleShared(const ModuleShared& pOther) = delete;
        ModuleShared(ModuleShared&& pOther) = delete;
        
        ModuleShared& operator=(const ModuleShared& pOther) = delete;
        ModuleShared& operator=(ModuleShared&& pOther) = delete;
        
        template <typename T>
        static void invokeDelete(T* pObject)
        {
            delete pObject;
        }
        
        std::function<void(std::shared_ptr<ModuleShared>)> mOnAttach;
        std::function<void(std::pair<size_t, size_t>)> mOnDetach;
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
        
        void setOnActiveCallback(std::function<void(std::shared_ptr<ModuleShared>)> pCallback);
        
        static ModuleHandler* getInstance();
        
    private:
        ModuleHandler() = default;
        ~ModuleHandler() = default;
        ModuleHandler(const ModuleHandler& pOther) = delete;
        ModuleHandler(ModuleHandler&& pOther) = delete;
        
        ModuleHandler& operator=(const ModuleHandler& pOther) = delete;
        ModuleHandler& operator=(ModuleHandler&& pOther) = delete;

        std::shared_ptr<ModuleShared> mMainModule;
        std::vector<std::vector<std::shared_ptr<ModuleShared>>> mModule;
        
        std::function<void(std::shared_ptr<ModuleShared>)> mOnActive;
    };
    
}
}

#endif
