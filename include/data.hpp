// Copyright (C) 2017 Grupa Pracuj Sp. z o.o.
// This file is part of the "Hermes" library.
// For conditions of distribution and use, see copyright notice in license.txt.

#ifndef _DATA_HPP_
#define _DATA_HPP_

#include "json/json.h"

#include <cassert>
#include <cstring>
#include <functional>
#include <memory>
#include <vector>

namespace hms
{

    enum class EDataSharedType : int
    {
        Text = 0,
        Binary
    };
    
    enum class EDataEncryption : int
    {
        AES_256_OFB = 0,
        AES_256_CBC
    };

    class DataBuffer
    {
    public:
        DataBuffer() = default;
        DataBuffer(const DataBuffer& pOther);
        DataBuffer(DataBuffer&& pOther);
        ~DataBuffer();
        
        DataBuffer& operator=(const DataBuffer& pOther);
        DataBuffer& operator=(DataBuffer&& pOther);
        
        const void* data() const;
        
        template <typename T>
        void push_back(const T* pData, size_t pCount)
        {
            if (pData != nullptr && pCount > 0)
            {
                if (mCapacity < mSize + pCount)
                    reallocate(mSize + pCount);
                
                const size_t copySize = sizeof(T) * pCount;
                
                memcpy(mData + mSize, pData, sizeof(T) * pCount);
                
                mSize += copySize;
            }
        }
        
        size_t size() const;
        
    private:
        void reallocate(const size_t pCapacity);
        
        char* mData = nullptr;
        size_t mSize = 0;
        size_t mCapacity = 0;
    };
    
    class DataShared
    {
    public:
        DataShared(size_t pId);
        virtual ~DataShared();
        
        // Pack elements to request string.
        virtual bool pack(std::string& pData, const std::vector<unsigned>& pUserData) const;

        // Unpack elements from response string.
        virtual bool unpack(const std::string& pData, const std::vector<unsigned>& pUserData);
        virtual bool unpack(std::string&& pData, const std::vector<unsigned>& pUserData);
        
        // Pack elements to request raw data.
        virtual bool packBuffer(DataBuffer& pData, const std::vector<unsigned>& pUserData) const;
        
        // Unpack elements from response raw data.
        virtual bool unpackBuffer(const DataBuffer& pData, const std::vector<unsigned>& pUserData);
        virtual bool unpackBuffer(DataBuffer&& pData, const std::vector<unsigned>& pUserData);
        
        virtual bool readFromFile(const std::string& pFilePath, const std::vector<unsigned>& pUserData, EDataSharedType pType = EDataSharedType::Text);
        virtual bool writeToFile(const std::string& pFilePath, const std::vector<unsigned>& pUserData, EDataSharedType pType = EDataSharedType::Text, bool pClearContent = true) const;
        
        size_t getId() const;
        
        void setTextCipherPair(std::function<void(std::string& lpKey, std::string& lpIV)> pTextCipherPair);

        template <typename T>
        static bool safeAs(const Json::Value& pSource, T& pDestination, const std::string& pKey = "")
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
            
            if (source == nullptr)
                printWarning(pKey);
            
            return (source != nullptr) ? true : false;
        }
        
    private:
        template <typename T>
        static bool assignOp(const Json::Value* pSource, T& pDestination)
        {
            return false;
        }
        
        static void printWarning(const std::string& pKey);
        
        size_t mId;
        std::function<void(std::string& lpKey, std::string& lpIV)> mTextCipherPair = nullptr;
    };
    
    template <>
    bool DataShared::assignOp<bool>(const Json::Value* pSource, bool& pDestination);
    
    template <>
    bool DataShared::assignOp<int>(const Json::Value* pSource, int& pDestination);
    
    template <>
    bool DataShared::assignOp<long long int>(const Json::Value* pSource, long long int& pDestination);
    
    template <>
    bool DataShared::assignOp<unsigned>(const Json::Value* pSource, unsigned& pDestination);
    
    template <>
    bool DataShared::assignOp<unsigned long long int>(const Json::Value* pSource, unsigned long long int& pDestination);
    
    template <>
    bool DataShared::assignOp<float>(const Json::Value* pSource, float& pDestination);
    
    template <>
    bool DataShared::assignOp<double>(const Json::Value* pSource, double& pDestination);
    
    template <>
    bool DataShared::assignOp<std::string>(const Json::Value* pSource, std::string& pDestination);
    
    class DataManager
    {
    public:
        bool initialize();
        bool terminate();
        
        bool add(std::shared_ptr<DataShared> pData);
        size_t count() const;
        std::shared_ptr<DataShared> get(size_t pId) const;
        
        template <typename T>
        std::shared_ptr<T> get(size_t pId) const
        {
            // check if Id has a valid value
            assert(pId < mData.size());
        
            return std::static_pointer_cast<T>(mData[pId]);
        }

        bool convertJSON(const Json::Value& pSource, std::string& pDestination) const;
        bool convertJSON(const std::string& pSource, Json::Value& pDestination) const;

        std::string decrypt(const std::string& pData, std::string pKey, std::string pIV, EDataEncryption pMode) const;
        std::string encrypt(const std::string& pData, std::string pKey, std::string pIV, EDataEncryption pMode) const;
        
    private:
        friend class Hermes;
        
        DataManager();
        DataManager(const DataManager& pOther) = delete;
        DataManager(DataManager&& pOther) = delete;
        ~DataManager();
        
        DataManager& operator=(const DataManager& pOther) = delete;
        DataManager& operator=(DataManager&& pOther) = delete;
        
        std::vector<std::shared_ptr<DataShared>> mData;
        bool mInitialized = false;
    };

}

#endif
