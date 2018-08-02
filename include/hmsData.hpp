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
        virtual ~DataWriter() = default;
        
        virtual size_t write(const void* pBuffer, size_t pSize) = 0;
        virtual size_t write(const std::string& pText) = 0;
        virtual size_t write(const DataBuffer& pBuffer) = 0;
        
        virtual bool seek(int64_t pPosition, bool pRelative = false) = 0;
        virtual bool flush() = 0;
        
        virtual size_t getPosition() const = 0;
        virtual bool isOpen() const = 0;
        
    protected:
        DataWriter() = default;
        
    };
    
    class FileWriter : public DataWriter
    {
    public:
        FileWriter(const std::string& pFilePath, bool pAppend = false);
        virtual ~FileWriter() = default;
        
        virtual size_t write(const void* pBuffer, size_t pSize) override;
        virtual size_t write(const std::string& pText) override;
        virtual size_t write(const DataBuffer& pBuffer) override;
        
        virtual bool seek(int64_t pPosition, bool pRelative = false) override;
        virtual bool flush() override;
        
        virtual size_t getPosition() const override;
        virtual bool isOpen() const override;
        
    private:
        std::ofstream mStream;
        size_t mPosition = 0;
        size_t mSize = 0;
    };
        
    class MemoryWriter : public DataWriter
    {
    public:
        MemoryWriter(void* pBuffer, size_t pSize, bool pDeleteOnClose = false);
        virtual ~MemoryWriter();
        
        virtual size_t write(const void* pBuffer, size_t pSize) override;
        virtual size_t write(const std::string& pText) override;
        virtual size_t write(const DataBuffer& pBuffer) override;
        
        virtual bool seek(int64_t pPosition, bool pRelative = false) override;
        virtual bool flush() override;
        
        virtual size_t getPosition() const override;
        virtual bool isOpen() const override;
        
    private:
        void* mBuffer = nullptr;
        bool mDeleteBufferOnClose = false;
        size_t mPosition = 0;
        size_t mSize = 0;
    };
        
    class DataReader
    {
    public:
        virtual ~DataReader() = default;
        
        virtual size_t read(void* pBuffer) = 0;
        virtual size_t read(void* pBuffer, size_t pSize) = 0;
        virtual void read(std::string& pText) = 0;
        virtual void read(std::string& pText, size_t pSize) = 0;
        virtual void read(DataBuffer& pBuffer) = 0;
        virtual void read(DataBuffer& pBuffer, size_t pSize) = 0;
        
        virtual bool seek(int64_t pPosition, bool pRelative = false) = 0;
        
        virtual size_t getPosition() const = 0;
        virtual size_t getSize() const = 0;
        virtual bool isOpen() const = 0;
        
    protected:
        DataReader() = default;
        
    };
        
    class FileReader : public DataReader
    {
    public:
        FileReader(const std::string& pFilePath);
        virtual ~FileReader() = default;
        
        virtual size_t read(void* pBuffer) override;
        virtual size_t read(void* pBuffer, size_t pSize) override;
        virtual void read(std::string& pText) override;
        virtual void read(std::string& pText, size_t pSize) override;
        virtual void read(DataBuffer& pBuffer) override;
        virtual void read(DataBuffer& pBuffer, size_t pSize) override;
        
        virtual bool seek(int64_t pPosition, bool pRelative = false) override;
        
        virtual size_t getPosition() const override;
        virtual size_t getSize() const override;
        virtual bool isOpen() const override;
        
    private:
        std::ifstream mStream;
        size_t mSize = 0;
        size_t mPosition = 0;
    };
        
    class MemoryReader : public DataReader
    {
    public:
        MemoryReader(const void* pMemory, size_t pSize, bool pDeleteOnClose = false);
        virtual ~MemoryReader();
        
        virtual size_t read(void* pBuffer) override;
        virtual size_t read(void* pBuffer, size_t pSize) override;
        virtual void read(std::string& pText) override;
        virtual void read(std::string& pText, size_t pSize) override;
        virtual void read(DataBuffer& pBuffer) override;
        virtual void read(DataBuffer& pBuffer, size_t pSize) override;
        
        virtual bool seek(int64_t pPosition, bool pRelative = false) override;
        
        virtual size_t getPosition() const override;
        virtual size_t getSize() const override;
        virtual bool isOpen() const override;
        
    private:
        const void* mBuffer = nullptr;
        bool mDeleteBufferOnClose = false;
        size_t mSize = 0;
        size_t mPosition = 0;
    };
        
    class DataStorageReader
    {
    public:
        virtual ~DataStorageReader() = default;
        virtual std::unique_ptr<DataReader> openFile(const std::string& pName) const = 0;
        virtual bool clearStorage(const std::string& pPath) { return false; };
        
    protected:
        std::string mPrefix;
        
        DataStorageReader(const std::string& pPrefix = "") : mPrefix(pPrefix) {};
    };
        
    class DataStorageLoader
    {
    public:
        virtual ~DataStorageLoader() = default;
        virtual bool isLoadable(const std::string& pPath) const = 0;
        virtual bool isLoadable(DataReader& pReader) const = 0;
        virtual std::unique_ptr<DataStorageReader> createStorageReader(const std::string& pPath, const std::string& pPrefix = "") const = 0;
        virtual std::unique_ptr<DataStorageReader> createStorageReader(DataReader& pReader, const std::string& pPrefix = "") const = 0;
        
    protected:
        DataStorageLoader() = default;
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
        
        bool readFile(const std::string& pFileName, DataShared* pData, const std::vector<unsigned>& pUserData, EDataSharedType pType = EDataSharedType::Text);
        bool readFile(const std::string& pFileName, std::string& pText) const;
        bool readFile(const std::string& pFileName, DataBuffer& pDataBuffer) const;
        
        bool writeFile(const std::string& pFileName, const DataShared* pData, const std::vector<unsigned>& pUserData, EDataSharedType pType = EDataSharedType::Text, bool pClearContent = true) const;
        bool writeFile(const std::string& pFileName, const std::string& pText, bool pClearContent = true) const;
        bool writeFile(const std::string& pFileName, const DataBuffer& pDataBuffer, bool pClearContent = true) const;
        
        std::unique_ptr<DataReader> getDataReader(const std::string& pFileName) const;
        std::unique_ptr<DataWriter> getDataWriter(const std::string& pFileName, bool pAppend = false) const;
        
        void addStorageLoader(std::unique_ptr<DataStorageLoader> pLoader);
        void addStorageReader(std::unique_ptr<DataStorageReader> pReader);
        bool addStorageReader(const std::string& pName, const std::string& pPrefix = "", bool pRelativeToWorkspace = true);
        bool addStorageReader(DataReader& pReader, const std::string& pPrefix = "");
        
        void clearDirectory(const std::string& pPath);
        
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
        std::vector<std::unique_ptr<DataStorageLoader>> mLoader;
        std::function<void(std::string& lpKey, std::string& lpIV)> mCipher = nullptr;
        std::string mWorkingDirectory;
        bool mInitialized = false;
    };

}

#endif
