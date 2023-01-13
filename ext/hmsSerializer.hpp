// Copyright (C) 2017-2023 Grupa Pracuj S.A.
// This file is part of the "Hermes" library.
// For conditions of distribution and use, see copyright notice in license.txt.

#ifndef _HMS_SERIALIZER_HPP_
#define _HMS_SERIALIZER_HPP_

#include <functional>
#include <string>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

#include "json/json.h"

namespace hms
{
namespace ext
{
    enum class ESerializerMode : uint32_t
    {
        Serialize = 1,
        Deserialize = 1 << 1,
        All = Serialize | Deserialize
    };
    
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
        
        Property(std::string pName, T C::* pPtr, ESerializerMode pMode = ESerializerMode::All, std::function<bool(const C&, const T&)> pSerializeCondition = nullptr, std::function<void(C&, T&)> pDeserialized = nullptr) : mMode(pMode), mName(std::move(pName)), mSerializeCondition(std::move(pSerializeCondition)), mDeserialized(std::move(pDeserialized)), mPtr(pPtr) {}

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
        
        ESerializerMode getMode() const
        {
            return mMode;
        }
        
        bool shouldSerialize(const C& pObj) const
        {
            return mSerializeCondition == nullptr ? true : mSerializeCondition(pObj, get(pObj));
        }
        
        void deserialized(C& pObj) const
        {
            if (mDeserialized != nullptr)
                mDeserialized(pObj, get(pObj));
        }
        
    private:
        ESerializerMode mMode;
        std::string mName;
        std::function<bool(const C&, const T&)> mSerializeCondition;
        std::function<void(C&, T&)> mDeserialized;
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
        template <typename T>
        struct is_serializable_class : std::false_type{};
    
        bool convert(const Json::Value& pSource, std::string& pDestination);
        bool convert(const std::string& pSource, Json::Value& pDestination);
        
        template <typename T>
        bool assignOp(const Json::Value* pSource, T& pDestination)
        {
            bool status = false;
            
            if constexpr(is_serializable_class<T>::value)
                status = pDestination.deserialize(*pSource);
            
            return status;
        }
        
        template <>
        bool assignOp<bool>(const Json::Value* pSource, bool& pDestination);
        
        template <>
        bool assignOp<int32_t>(const Json::Value* pSource, int32_t& pDestination);
        
        template <>
        bool assignOp<int64_t>(const Json::Value* pSource, int64_t& pDestination);
        
        template <>
        bool assignOp<uint32_t>(const Json::Value* pSource, uint32_t& pDestination);
        
        template <>
        bool assignOp<uint64_t>(const Json::Value* pSource, uint64_t& pDestination);
        
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
                    if (pSource.isObject() && pSource.isMember(pKey))
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
    
        template <typename K, typename V>
        class Map
        {
        public:
            Map(std::string pKey, std::string pValue) : mKey(std::move(pKey)), mValue(std::move(pValue))
            {
            }
            
            using iterator = typename std::unordered_map<K, V>::iterator;
            iterator begin()
            {
                return mMap.begin();
            }
            
            iterator end()
            {
                return mMap.end();
            }
            
            using const_iterator = typename std::unordered_map<K, V>::const_iterator;
            const_iterator cbegin() const
            {
                return mMap.cbegin();
            }
            
            const_iterator cend() const
            {
                return mMap.cend();
            }
            
            const_iterator find(const std::string& pKey) const
            {
                return mMap.find(pKey);
            }
            
            size_t count() const
            {
                return mMap.size();
            }
            
            bool deserialize(const Json::Value& pRoot)
            {
                bool deserialized = false;
                if (pRoot.isArray())
                {
                    for (const auto& v : pRoot)
                    {
                        K key{};
                        V value{};
                        if (safeAs<K>(v, key, mKey) && safeAs<V>(v, value, mValue))
                        {
                            deserialized = true;
                            mMap[key] = value;
                        }
                    }
                }
                
                return deserialized;
            }

            Json::Value serialize() const
            {
                Json::Value array = Json::arrayValue;
                for (auto it = mMap.cbegin(); it != mMap.cend(); it++)
                {
                    Json::Value value;
                    value[mKey] = it->first;
                    value[mValue] = it->second;

                    array.append(std::move(value));
                }

                return array;
            }

        private:
            std::unordered_map<K, V> mMap;
            std::string mKey;
            std::string mValue;
        };
        
        template <typename T>
        class Field
        {
        public:
            Field(std::string pKey = "name") : mKey(std::move(pKey))
            {
            }
            
            const T& value() const
            {
                return mValue;
            }
            
            bool deserialize(const Json::Value& pRoot)
            {
                return safeAs<T>(pRoot, mValue, mKey);
            }

            Json::Value serialize() const
            {
                Json::Value value;
                value[mKey] = mValue;

                return value;
            }

        private:
            T mValue{};
            std::string mKey;
        };
        
        template <typename K, typename V>
        struct is_serializable_class<Map<K, V>> : std::true_type{};
        
        template <typename T>
        struct is_serializable_class<Field<T>> : std::true_type{};
        
        template <typename C, typename = std::enable_if_t<!hasRegisteredProperties<C>() && !std::is_pointer<C>::value && (!is_array<C>::value && !is_vector<C>::value)>, typename = void>
        void serialize(Json::Value& pDestination, const C& pSource, const std::string& pName = "");
        
        template <typename C, typename = std::enable_if_t<!hasRegisteredProperties<C>() && !std::is_pointer<C>::value && (is_array<C>::value || is_vector<C>::value)>, typename = void, typename = void>
        void serialize(Json::Value& pDestination, const C& pSource, const std::string& pName = "");
        
        template <typename C, typename = std::enable_if_t<std::is_pointer<C>::value>>
        void serialize(Json::Value& pDestination, C pSource, const std::string& pName = "");
        
        template <typename C, typename = std::enable_if_t<hasRegisteredProperties<C>() && !std::is_pointer<C>::value>>
        void serialize(Json::Value& pDestination, const C& pSource, const std::string& pName = "");
        
        template <typename C, typename = std::enable_if_t<!std::is_pointer<C>::value>>
        Json::Value serialize(const C& pSource, const std::string& pName = "");
        
        template <typename C, typename = std::enable_if_t<!hasRegisteredProperties<C>() && !std::is_pointer<C>::value && (!is_array<C>::value && !is_vector<C>::value)>, typename = void>
        bool deserialize(C& pDestination, const Json::Value& pSource, const std::string& pName = "");
        
        template <typename C, typename = std::enable_if_t<!hasRegisteredProperties<C>() && !std::is_pointer<C>::value && is_array<C>::value>, typename = void, typename = void>
        bool deserialize(C& pDestination, const Json::Value& pSource, const std::string& pName = "");
        
        template <typename C, typename = std::enable_if_t<!hasRegisteredProperties<C>() && !std::is_pointer<C>::value && is_vector<C>::value>, typename = void, typename = void, typename = void>
        bool deserialize(C& pDestination, const Json::Value& pSource, const std::string& pName = "");
        
        template <typename C, typename = std::enable_if_t<std::is_pointer<C>::value>>
        bool deserialize(C pDestination, const Json::Value& pSource, const std::string& pName = "");
        
        template <typename C, typename = std::enable_if_t<hasRegisteredProperties<C>() && !std::is_pointer<C>::value>>
        bool deserialize(C& pDestination, const Json::Value& pSource, const std::string& pName = "");
        
        template <typename C, typename = std::enable_if_t<!std::is_pointer<C>::value>>
        C deserialize(const Json::Value& pSource, const std::string& pName = "");
        
        template <typename C, typename, typename>
        void serialize(Json::Value& pDestination, const C& pSource, const std::string& pName)
        {
            if (pName.length() == 0)
            {
                if constexpr(is_serializable_class<C>::value)
                    pDestination.append(pSource.serialize());
                else
                    pDestination.append(pSource);
            }
            else
            {
                if constexpr(is_serializable_class<C>::value)
                    pDestination[pName] = pSource.serialize();
                else
                    pDestination[pName] = pSource;
            }
        }
        
        template <typename C, typename, typename, typename>
        void serialize(Json::Value& pDestination, const C& pSource, const std::string& pName)
        {
            Json::Value value = Json::arrayValue;
            for (auto& v : pSource)
                serialize<typename C::value_type>(value, v);
            
            if (pName.length() != 0)
                pDestination[pName] = std::move(value);
            else if (!pDestination.isArray())
                pDestination = std::move(value);
            else
                pDestination.append(std::move(value));
        }
        
        template <typename C, typename>
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
                if ((static_cast<uint32_t>(lpProperty.getMode()) & static_cast<uint32_t>(ESerializerMode::Serialize)) == static_cast<uint32_t>(ESerializerMode::Serialize) && lpProperty.shouldSerialize(pSource))
                    serialize<typename std::decay_t<decltype(lpProperty)>::property_type>(dest, lpProperty.get(pSource), lpProperty.getName());
            });
            
            if (pName.length() != 0)
                pDestination[pName] = std::move(value);
            else if (pDestination.type() == Json::arrayValue && value != Json::nullValue)
                pDestination.append(std::move(value));
        }
        
        template <typename C, typename>
        Json::Value serialize(const C& pSource, const std::string& pName)
        {
            Json::Value dest;
            serialize<C>(dest, pSource, pName);
            
            return dest;
        }
        
        template <typename C, typename, typename>
        bool deserialize(C& pDestination, const Json::Value& pSource, const std::string& pName)
        {
            return safeAs<C>(pSource, pDestination, pName);
        }
        
        template <typename C, typename, typename, typename> // array
        bool deserialize(C& pDestination, const Json::Value& pSource, const std::string& pName)
        {
            bool status = false;
        
            Json::Value empty;
            const Json::Value& value = pName.length() == 0 ? pSource :
                                       pSource.isObject() ? pSource[pName] :
                                       empty;
            if (value.isArray() && value.size() == pDestination.size())
            {
                status = true;
                for (size_t i = 0; i < static_cast<size_t>(value.size()); i++)
                    deserialize<typename C::value_type>(pDestination[i], value[static_cast<int>(i)], "");
            }
            
            return status;
        }
        
        template <typename C, typename, typename, typename, typename> // vector
        bool deserialize(C& pDestination, const Json::Value& pSource, const std::string& pName)
        {
            bool status = false;
        
            Json::Value empty;
            const Json::Value& value = pName.length() == 0 ? pSource :
                                       pSource.isObject() ? pSource[pName] :
                                       empty;
            if (value.isArray())
            {
                size_t offset = pDestination.size();
                pDestination.resize(offset + value.size(), typename C::value_type{});
                status = true;
                for (int i = 0; static_cast<Json::ArrayIndex>(i) < value.size(); i++)
                    deserialize<typename C::value_type>(pDestination[offset + static_cast<size_t>(i)], value[i]);
            }
            
            return status;
        }
        
        template <typename C, typename>
        bool deserialize(C pDestination, const Json::Value& pSource, const std::string& pName)
        {
            return deserialize<typename std::remove_pointer<C>::type>(*pDestination, pSource, pName);
        }
        
        template <typename C, typename>
        bool deserialize(C& pDestination, const Json::Value& pSource, const std::string& pName)
        {
            const Json::Value& source = pName.length() == 0 ? pSource : pSource[pName];
            bool deserialized = false;
            
            executeOnAllProperties<C>([&source, &pDestination, &deserialized](auto& lpProperty) -> void
            {
                if ((static_cast<uint32_t>(lpProperty.getMode()) & static_cast<uint32_t>(ESerializerMode::Deserialize)) == static_cast<uint32_t>(ESerializerMode::Deserialize) && deserialize<typename std::decay_t<decltype(lpProperty)>::property_type>(lpProperty.get(pDestination), source, lpProperty.getName()))
                {
                    deserialized = true;
                    lpProperty.deserialized(pDestination);
                }
            });
            
            return deserialized;
        }
        
        template <typename C, typename>
        C deserialize(const Json::Value& pSource, const std::string& pName)
        {
            C dest{};
            deserialize<C>(dest, pSource, pName);
            
            return dest;
        }
    }
}
}

#endif
