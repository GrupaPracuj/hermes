// Copyright (C) 2017 Grupa Pracuj Sp. z o.o.
// This file is part of the "Hermes" library.
// For conditions of distribution and use, see copyright notice in license.txt.

#ifndef _DATA_HPP_
#define _DATA_HPP_

#include <cassert>
#include <cstring>
#include <string>
#include <functional>
#include <memory>
#include <vector>

namespace hms
{
    namespace crypto
    {
        enum class EDataCrypto : int;
    }

    enum class EDataSharedType : int
    {
        Text = 0,
        Binary
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
        
        void pop_back(size_t pCount, bool pShrinkToFit = false);
        
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
        crypto::EDataCrypto getCryptoMode() const;
        
        void setCryptoMode(crypto::EDataCrypto pCryptoMode);
        
    private:
        size_t mId;
        crypto::EDataCrypto mCryptoMode;
    };
    
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
        
        void setCipher(std::function<void(std::string& lpKey, std::string& lpIV)> pCipher);
        
    private:
        friend class Hermes;
        friend class DataShared;
        
        DataManager();
        DataManager(const DataManager& pOther) = delete;
        DataManager(DataManager&& pOther) = delete;
        ~DataManager();
        
        DataManager& operator=(const DataManager& pOther) = delete;
        DataManager& operator=(DataManager&& pOther) = delete;
        
        std::vector<std::shared_ptr<DataShared>> mData;
        std::function<void(std::string& lpKey, std::string& lpIV)> mCipher = nullptr;
        bool mInitialized = false;
    };

}

#endif
