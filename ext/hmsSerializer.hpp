#ifndef _HMS_SERIALIZER_HPP_
#define _HMS_SERIALIZER_HPP_

#include <tuple>
#include <utility>
#include <string>
#include <vector>

#include "json/json.h"

namespace hms
{
namespace ext
{
    template <class T>
    struct is_array
    {
        static const bool value = false;
    };
    
    template <typename T, typename std::size_t N>
    struct is_array<std::array<T, N>>
    {
        static const bool value = true;
    };
    
    template <class T>
    struct is_vector
    {
        static const bool value = false;
    };
    
    template <typename T, typename Alloc>
    struct is_vector<std::vector<T, Alloc>>
    {
        static const bool value = true;
    };
    
    template<std::size_t I = 0, typename F, typename... T>
    inline typename std::enable_if<I == sizeof...(T), void>::type
    for_each(std::tuple<T...>&, F) {}
    
    template<std::size_t I = 0, typename F, typename... T>
    inline typename std::enable_if<I < sizeof...(T), void>::type
    for_each(std::tuple<T...>& pT, F pF)
    {
        pF(std::get<I>(pT));
        for_each<I + 1, F, T...>(pT, std::forward<F>(pF));
    }
    
    template <typename C, typename T>
    class Property
    {
    public:
        using property_type = T;
        
        Property(std::string pName, T C::* pPtr) : mName(std::move(pName)), mPtr(pPtr) {}

        T& get(C& pObj) const
        {
            return pObj.*mPtr;
        }
        
        const T& get(const C& pObj) const
        {
            return pObj.*mPtr;
        }
        
        const std::string& getName() const
        {
            return mName;
        }
        
        template <typename V, typename = std::enable_if_t<std::is_constructible<T, V>::value>>
        void set(C& pObj, V&& pValue) const
        {
            pObj.*mPtr = pValue;
        }
        
    private:
        std::string mName;
        T C::* mPtr;
    };
    
    template <typename... Args>
    auto properties(Args&&... args)
    {
        return std::make_tuple(std::forward<Args>(args)...);
    }
    
    template <typename C>
    inline auto registerProperties()
    {
        return std::make_tuple();
    }
    
    template <typename C, typename T>
    struct PHolder {
        static T properties;
    };
    
    template <typename C, typename T>
    T PHolder<C, T>::properties = registerProperties<C>();
    
    template <typename C>
    constexpr bool hasRegisteredProperties()
    {
        return !std::is_same<std::tuple<>, decltype(registerProperties<C>())>::value;
    }
    
    template <typename C, typename F>
    void executeOnAllProperties(F pF)
    {
        for_each(PHolder<C, decltype(registerProperties<C>())>::properties, std::forward<F>(pF));
    }
    
    namespace json
    {
        bool convert(const Json::Value& pSource, std::string& pDestination);
        bool convert(const std::string& pSource, Json::Value& pDestination);
        
        template <typename T>
        bool assignOp(const Json::Value* pSource, T& pDestination)
        {
            return false;
        }
        
        template <>
        bool assignOp<bool>(const Json::Value* pSource, bool& pDestination);
        
        template <>
        bool assignOp<int>(const Json::Value* pSource, int& pDestination);
        
        template <>
        bool assignOp<long long int>(const Json::Value* pSource, long long int& pDestination);
        
        template <>
        bool assignOp<unsigned>(const Json::Value* pSource, unsigned& pDestination);
        
        template <>
        bool assignOp<unsigned long long int>(const Json::Value* pSource, unsigned long long int& pDestination);
        
        template <>
        bool assignOp<float>(const Json::Value* pSource, float& pDestination);
        
        template <>
        bool assignOp<double>(const Json::Value* pSource, double& pDestination);
        
        template <>
        bool assignOp<std::string>(const Json::Value* pSource, std::string& pDestination);
        
        template <typename T>
        bool safeAs(const Json::Value& pSource, T& pDestination, const std::string& pKey = "")
        {
            const Json::Value* source = nullptr;
            
            if (!pSource.empty())
            {
                if (pKey.size() > 0)
                {
                    if (pSource.isMember(pKey))
                        source = &pSource[pKey];
                }
                else
                {
                    source = &pSource;
                }
            }
            
            if (source)
            {
                if (!assignOp<T>(source, pDestination))
                    source = nullptr;
            }
            
            return (source != nullptr) ? true : false;
        }
        
        template <typename C, typename = std::enable_if_t<!hasRegisteredProperties<C>() && !std::is_pointer<C>::value && (!is_array<C>::value && !is_vector<C>::value)>, typename = void>
        void serialize(Json::Value& pDestination, const C& pSource, const std::string& pName = "");
        
        template <typename C, typename = std::enable_if_t<!hasRegisteredProperties<C>() && !std::is_pointer<C>::value && (is_array<C>::value || is_vector<C>::value)>, typename = void, typename = void>
        void serialize(Json::Value& pDestination, const C& pSource, const std::string& pName = "");
        
        template <typename C, typename = std::enable_if_t<!hasRegisteredProperties<C>() && std::is_pointer<C>::value && (!is_array<C>::value && !is_vector<C>::value)>, typename = void>
        void serialize(Json::Value& pDestination, C pSource, const std::string& pName = "");
        
        template <typename C, typename = std::enable_if_t<hasRegisteredProperties<C>() && !std::is_pointer<C>::value>>
        void serialize(Json::Value& pDestination, const C& pSource, const std::string& pName = "");
        
        template <typename C, typename = std::enable_if_t<hasRegisteredProperties<C>() && std::is_pointer<C>::value>>
        void serialize(Json::Value& pDestination, C pSource, const std::string& pName = "");
        
        template <typename C, typename = std::enable_if_t<!hasRegisteredProperties<C>() && !std::is_pointer<C>::value && (!is_array<C>::value && !is_vector<C>::value)>, typename = void>
        void deserialize(C& pDestination, const Json::Value& pSource, const std::string& pName = "");
        
        template <typename C, typename = std::enable_if_t<!hasRegisteredProperties<C>() && !std::is_pointer<C>::value && is_array<C>::value>, typename = void, typename = void>
        void deserialize(C& pDestination, const Json::Value& pSource, const std::string& pName = "");
        
        template <typename C, typename = std::enable_if_t<!hasRegisteredProperties<C>() && !std::is_pointer<C>::value && is_vector<C>::value>, typename = void, typename = void, typename = void>
        void deserialize(C& pDestination, const Json::Value& pSource, const std::string& pName = "");
        
        template <typename C, typename = std::enable_if_t<!hasRegisteredProperties<C>() && std::is_pointer<C>::value && (!is_array<C>::value && !is_vector<C>::value)>, typename = void>
        void deserialize(C pDestination, const Json::Value& pSource, const std::string& pName = "");
        
        template <typename C, typename = std::enable_if_t<hasRegisteredProperties<C>()>>
        void deserialize(C& pDestination, const Json::Value& pSource, const std::string& pName = "");
        
        template <typename C, typename, typename>
        void serialize(Json::Value& pDestination, const C& pSource, const std::string& pName)
        {
            if (pName.length() == 0)
                pDestination.append(pSource);
            else
                pDestination[pName] = pSource;
        }
        
        template <typename C, typename, typename, typename>
        void serialize(Json::Value& pDestination, const C& pSource, const std::string& pName)
        {
            Json::Value value = Json::arrayValue;
            for (auto& v : pSource)
                serialize<typename C::value_type>(value, v);
            
            if (pName.length() == 0)
                pDestination.append(value);
            else
                pDestination[pName] = value;
        }
        
        template <typename C, typename, typename>
        void serialize(Json::Value& pDestination, C pSource, const std::string& pName)
        {
            serialize<typename std::remove_pointer<C>::type>(pDestination, *pSource, pName);
        }

        template <typename C, typename>
        void serialize(Json::Value& pDestination, const C& pSource, const std::string& pName)
        {
            Json::Value value{};
            Json::Value& dest = pName.length() != 0 || pDestination.type() == Json::arrayValue ? value : pDestination;
            executeOnAllProperties<C>([&dest, &pSource](auto& lpProperty) -> void
            {
                serialize<typename std::decay_t<decltype(lpProperty)>::property_type>(dest, lpProperty.get(pSource), lpProperty.getName());
            });
            
            if (pName.length() != 0)
                pDestination[pName] = value;
            else if (pDestination.type() == Json::arrayValue)
                pDestination.append(value);
        }
        
        template <typename C, typename, typename>
        void deserialize(C& pDestination, const Json::Value& pSource, const std::string& pName)
        {
            safeAs<C>(pSource, pDestination, pName);
        }
        
        template <typename C, typename, typename, typename> // array
        void deserialize(C& pDestination, const Json::Value& pSource, const std::string& pName)
        {
            const Json::Value& value = pName.length() == 0 ? pSource : pSource[pName];
            if (value.isArray() && value.size() == pDestination.size())
            {
                for (size_t i = 0; i < static_cast<size_t>(value.size()); i++)
                    deserialize<typename C::value_type>(pDestination[i], value[static_cast<int>(i)], "");
            }
        }
        
        template <typename C, typename, typename, typename, typename> // vector
        void deserialize(C& pDestination, const Json::Value& pSource, const std::string& pName)
        {
            const Json::Value& value = pName.length() == 0 ? pSource : pSource[pName];
            if (value.isArray())
            {
                size_t offset = pDestination.size();
                pDestination.resize(offset + value.size(), typename C::value_type{});
                
                for (int i = 0; i < value.size(); i++)
                    deserialize<typename C::value_type>(pDestination[offset + i], value[i]);
            }
        }
        
        template <typename C, typename, typename>
        void deserialize(C pDestination, const Json::Value& pSource, const std::string& pName)
        {
            deserialize<typename std::remove_pointer<C>::type>(*pDestination, pSource, pName);
        }
        
        template <typename C, typename>
        void deserialize(C& pDestination, const Json::Value& pSource, const std::string& pName)
        {
            const Json::Value& source = pName.length() == 0 ? pSource : pSource[pName];
            executeOnAllProperties<C>([&source, &pDestination](auto& lpProperty) -> void
            {
                deserialize<typename std::decay_t<decltype(lpProperty)>::property_type>(lpProperty.get(pDestination), source, lpProperty.getName());
            });
        }
    }
}
}

#endif
