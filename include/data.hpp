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
#include <type_traits>
#include <vector>
#include <fstream>

namespace hms
{
    class DataManager;
    
    namespace crypto
    {
        enum class ECryptoMode : int;
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
        
        size_t getId() const;
        crypto::ECryptoMode getCryptoMode() const;
        
        void setCryptoMode(crypto::ECryptoMode pCryptoMode);
        
    private:
        size_t mId;
        crypto::ECryptoMode mCryptoMode;
    };
        
    class DataWriter
    {
    public:
        DataWriter(const std::string& pFilePath, bool pAppend = false);
        DataWriter(void* pMemory, std::streamsize pSize, bool pDeleteOnClose = false);
        DataWriter() = delete;
        DataWriter(const DataWriter& pOther) = delete;
        DataWriter(DataWriter&& pOther) = delete;
        ~DataWriter();
        
        DataWriter& operator=(const DataWriter& pOther) = delete;
        DataWriter& operator=(DataWriter&& pOther) = delete;
        
        std::streamsize write(const void* pBuffer, std::streamsize pSize);
        std::streamsize write(const std::string& pText);
        std::streamsize write(const DataBuffer& pBuffer);
        
        bool seek(std::streamoff pPosition, bool pRelative = false);
        bool flush();
        
        std::streampos getPosition() const;
        bool isOpen() const;
        
    private:
        std::ofstream mStream;
        void* mBuffer = nullptr;
        bool mDeleteBufferOnClose = false;
        std::streampos mPosition = 0;
        std::streamsize mSize = 0;
        
    };
        
    class DataReader
    {
    public:
        DataReader(const std::string& pFilePath);
        DataReader(const void* pMemory, std::streamsize pSize, bool pDeleteOnClose = false);
        DataReader() = delete;
        DataReader(const DataReader& pOther) = delete;
        DataReader(DataWriter&& pOther) = delete;
        ~DataReader();
        
        DataReader& operator=(const DataReader& pOther) = delete;
        DataReader& operator=(DataReader&& pOther) = delete;
        
        std::streamsize read(void* pBuffer);
        std::streamsize read(void* pBuffer, std::streamsize pSize);
        void read(std::string& pText);
        void read(std::string& pText, std::streamsize pSize);
        void read(DataBuffer& pBuffer);
        void read(DataBuffer& pBuffer, std::streamsize pSize);
        
        bool seek(std::streamoff pPosition, bool pRelative = false);
        
        std::streampos getPosition() const;
        std::streamsize getSize() const;
        bool isOpen() const;
        
    private:
        std::ifstream mStream;
        const void* mBuffer = nullptr;
        bool mDeleteBufferOnClose = false;
        std::streamsize mSize = 0;
        std::streampos mPosition = 0;
        
    };
        
    class DataStorageReader // TODO
    {
    public:
        DataStorageReader(std::unique_ptr<DataReader> pSource, std::string pCallPrefix);
        
        std::unique_ptr<DataReader> openFile(const std::string& pName);
        
    protected:
        std::vector<std::string> mName;
    };
    
    class DataManager
    {
    public:
        bool initialize();
        bool terminate();
        
        bool add(std::shared_ptr<DataShared> pData);
        
        template <typename T, typename = typename std::enable_if<std::is_base_of<DataShared, T>::value>::type>
        bool add(std::shared_ptr<T> pData)
        {
            return add(std::static_pointer_cast<DataShared>(pData));
        }
        
        size_t count() const;
        std::shared_ptr<DataShared> get(size_t pId) const;
        
        template <typename T, typename = typename std::enable_if<std::is_base_of<DataShared, T>::value>::type>
        std::shared_ptr<T> get(size_t pId) const
        {
            assert(pId < mData.size());
        
            return std::static_pointer_cast<T>(mData[pId]);
        }
        
        void setCipher(std::function<void(std::string& lpKey, std::string& lpIV)> pCipher);
        void setWorkingDirectory(std::string pWorkingDirectory);
        
        std::string getWorkingDirectory() const;
        
        bool readData(std::shared_ptr<DataShared> pData, const std::string& pFileName, const std::vector<unsigned>& pUserData, EDataSharedType pType = EDataSharedType::Text);
        bool readFile(const std::string& pFileName, std::string& pText) const;
        bool readFile(const std::string& pFileName, DataBuffer& pDataBuffer) const;
        
        bool writeData(std::shared_ptr<DataShared> pData, const std::string& pFileName, const std::vector<unsigned>& pUserData, EDataSharedType pType = EDataSharedType::Text, bool pClearContent = true) const;
        bool writeFile(const std::string& pFileName, const std::string& pText, bool pClearContent = true) const;
        bool writeFile(const std::string& pFileName, const DataBuffer& pDataBuffer, bool pClearContent = true) const;
        
        std::unique_ptr<DataReader> getDataReader(const std::string& pFileName) const;
        std::unique_ptr<DataWriter> getDataWriter(const std::string& pFileName, bool pAppend = false) const;
        
    private:
        friend class Hermes;
        
        DataManager();
        DataManager(const DataManager& pOther) = delete;
        DataManager(DataManager&& pOther) = delete;
        ~DataManager();
        
        DataManager& operator=(const DataManager& pOther) = delete;
        DataManager& operator=(DataManager&& pOther) = delete;
        
        std::vector<std::shared_ptr<DataShared>> mData;
        std::vector<std::unique_ptr<DataStorageReader>> mStorage;
        std::function<void(std::string& lpKey, std::string& lpIV)> mCipher = nullptr;
        std::string mWorkingDirectory;
        bool mInitialized = false;
    };

}

#endif
