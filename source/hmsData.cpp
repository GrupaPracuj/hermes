// Copyright (C) 2017 Grupa Pracuj Sp. z o.o.
// This file is part of the "Hermes" library.
// For conditions of distribution and use, see copyright notice in license.txt.

#include "hmsData.hpp"

#include "hermes.hpp"

#if defined(__APPLE__) || defined(__linux__)
#include <dirent.h>
#include <sys/stat.h>
#endif

namespace hms
{

    /* DataBuffer */
    
    DataBuffer::DataBuffer(const DataBuffer& pOther)
    {
        char* data = nullptr;
        
        if (pOther.mData != nullptr)
        {
            data = new char[pOther.mCapacity];
            
            memcpy(data, pOther.mData, pOther.mSize);
        }
        
        mData = data;
        mSize = pOther.mSize;
        mCapacity = pOther.mCapacity;
    }
    
    DataBuffer::DataBuffer(DataBuffer&& pOther)
    {
        mData = pOther.mData;
        mSize = pOther.mSize;
        mCapacity = pOther.mCapacity;
        
        pOther.mData = nullptr;
        pOther.mSize = 0;
        pOther.mCapacity = 0;
    }
    
    DataBuffer::~DataBuffer()
    {
        delete[] mData;
    }
    
    DataBuffer& DataBuffer::operator=(const DataBuffer& pOther)
    {
        if (&pOther == this)
            return *this;
        
        char* data = nullptr;
        
        if (pOther.mData != nullptr)
        {
            data = new char[pOther.mCapacity];
            
            memcpy(data, pOther.mData, pOther.mSize);
        }

        delete[] mData;
        
        mData = data;
        mSize = pOther.mSize;
        mCapacity = pOther.mCapacity;
        
        return *this;
    }
    
    DataBuffer& DataBuffer::operator=(DataBuffer&& pOther)
    {
        if (&pOther == this)
            return *this;

        delete[] mData;
        
        mData = pOther.mData;
        mSize = pOther.mSize;
        mCapacity = pOther.mCapacity;
        
        pOther.mData = nullptr;
        pOther.mSize = 0;
        pOther.mCapacity = 0;
        
        return *this;
    }
    
    const void* DataBuffer::data() const
    {
        return mData;
    }
    
    void DataBuffer::pop_back(size_t pCount, bool pShrinkToFit)
    {
        if (pCount != 0)
        {
            mSize = pCount > mSize ? 0 : mSize - pCount;
            
            if (pShrinkToFit)
                reallocate(mSize);
        }
    }
    
    size_t DataBuffer::size() const
    {
        return mSize;
    }
    
    void DataBuffer::reallocate(const size_t pCapacity)
    {
        if (pCapacity != mCapacity)
        {
            if (pCapacity != 0)
            {
                char* data = new char[pCapacity];
                
                if (mData != nullptr)
                {
                    if (mSize > 0)
                        memcpy(data, mData, mSize);
                    
                    delete[] mData;
                }
                
                mData = data;
            }
            else
            {
                delete[] mData;
                
                mData = nullptr;
            }
            
            mCapacity = pCapacity;
        }
    }
    
    /* DataShared */
    
    DataShared::DataShared(size_t pId) : mId(pId), mCryptoMode(crypto::ECryptoMode::None)
    {
    }
    
    DataShared::~DataShared()
    {
    }

    bool DataShared::pack(std::string& pData, const std::vector<unsigned>& pUserData) const
    {
        return false;
    }
    
    bool DataShared::unpack(const std::string& pData, const std::vector<unsigned>& pUserData)
    {
        return false;
    }
    
    bool DataShared::unpack(std::string&& pData, const std::vector<unsigned>& pUserData)
    {
        return false;
    }
    
    bool DataShared::packBuffer(DataBuffer& pData, const std::vector<unsigned>& pUserData) const
    {
        return false;
    }
    
    bool DataShared::unpackBuffer(const DataBuffer& pData, const std::vector<unsigned>& pUserData)
    {
        return false;
    }
    
    bool DataShared::unpackBuffer(DataBuffer&& pData, const std::vector<unsigned>& pUserData)
    {
        return false;
    }
    
    size_t DataShared::getId() const
    {
        return mId;
    }
    
    crypto::ECryptoMode DataShared::getCryptoMode() const
    {
        return mCryptoMode;
    }
    
    void DataShared::setCryptoMode(crypto::ECryptoMode pCryptoMode)
    {
        mCryptoMode = pCryptoMode;
    }
    
    /* FileWriter */
    
    FileWriter::FileWriter(const std::string& pFilePath, bool pAppend)
    {
        std::ios_base::openmode opmode = std::ofstream::out | std::ofstream::binary;
        
        if (!pAppend)
            opmode |= std::ofstream::trunc;
        else
            opmode |= std::ofstream::app;
        
        mStream.open(pFilePath, opmode);
        if (mStream.is_open())
            mPosition = static_cast<size_t>(mStream.tellp());
    }
    
    size_t FileWriter::write(const void* pBuffer, size_t pSize)
    {
        size_t writeCount = 0;
        
        if (mStream.is_open())
        {
            mStream.write(reinterpret_cast<const char*>(pBuffer), static_cast<std::streamsize>(pSize));
            
            auto flag = mStream.rdstate();
            if (flag != std::ofstream::goodbit)
            {
                if ((flag & std::ofstream::badbit) == 0)
                    mStream.clear();
            }
            else
            {
                writeCount = pSize;
                mPosition += writeCount;
            }
        }
        
        return writeCount;
    }
    
    size_t FileWriter::write(const std::string& pText)
    {
        size_t writeCount = 0;
        
        if (mStream.is_open())
        {
            mStream << pText;
            
            auto flag = mStream.rdstate();
            if (flag != std::ofstream::goodbit)
            {
                if ((flag & std::ofstream::badbit) == 0)
                    mStream.clear();
            }
            else
            {
                writeCount = pText.size();
                mPosition += writeCount;
            }
        }
        
        return writeCount;
    }
    
    size_t FileWriter::write(const DataBuffer& pBuffer)
    {
        size_t writeCount = 0;
        
        if (mStream.is_open())
        {
            mStream.write(reinterpret_cast<const char*>(pBuffer.data()), static_cast<std::streamsize>(pBuffer.size()));
            
            auto flag = mStream.rdstate();
            if (flag != std::ofstream::goodbit)
            {
                if ((flag & std::ofstream::badbit) == 0)
                    mStream.clear();
            }
            else
            {
                writeCount = pBuffer.size();
                mPosition += writeCount;
            }
        }
        
        return writeCount;
    }
    
    bool FileWriter::seek(int64_t pPosition, bool pRelative)
    {
        bool success = false;
        
        if (mStream.is_open())
        {
            mStream.seekp(pPosition, pRelative ? std::ios::cur : std::ios::beg);
            
            auto flag = mStream.rdstate();
            if (flag != std::ofstream::goodbit)
            {
                if ((flag & std::ofstream::badbit) == 0)
                    mStream.clear();
            }
            else
            {
                int64_t position = pRelative ? static_cast<int64_t>(mPosition) + pPosition : pPosition;
                mPosition = std::max(static_cast<size_t>(0), std::min(static_cast<size_t>(position), mSize));
                success = true;
            }
        }
        
        return success;
    }
    
    bool FileWriter::flush()
    {
        bool success = true;
        
        if (mStream.is_open())
        {
            mStream.flush();
            auto flag = mStream.rdstate();
            if (flag != std::ofstream::goodbit)
            {
                if ((flag & std::ofstream::badbit) == 0)
                    mStream.clear();
                
                success = false;
            }
        }
        
        return success;
    }
    
    size_t FileWriter::getPosition() const
    {
        return mPosition;
    }
    
    bool FileWriter::isOpen() const
    {
        return mStream.is_open();
    }
    
    /* MemoryWriter */
    
    MemoryWriter::MemoryWriter(void* pMemory, size_t pSize, bool pDeleteOnClose) : mBuffer(pMemory), mSize(pSize), mDeleteBufferOnClose(pDeleteOnClose)
    {
    }
    
    MemoryWriter::~MemoryWriter()
    {
        if (mBuffer != nullptr && mDeleteBufferOnClose)
            delete [] reinterpret_cast<char*>(mBuffer);
    }
    
    size_t MemoryWriter::write(const void* pBuffer, size_t pSize)
    {
        size_t writeCount = 0;
        
        if (mBuffer != nullptr)
        {
            writeCount = mPosition + pSize > mSize ? mSize - mPosition : pSize;
            if (writeCount > 0)
            {
                memcpy(reinterpret_cast<char*>(mBuffer) + mPosition, pBuffer, writeCount);
                mPosition += writeCount;
            }
        }
        
        return writeCount;
    }
    
    size_t MemoryWriter::write(const std::string& pText)
    {
        size_t writeCount = 0;
        
        if (mBuffer != nullptr)
        {
            writeCount = mPosition + pText.size() > mSize ? mSize - mPosition : pText.size();
            if (writeCount > 0)
            {
                memcpy(reinterpret_cast<char*>(mBuffer) + mPosition, pText.data(), writeCount);
                mPosition += writeCount;
            }
        }
        
        return writeCount;
    }
    
    size_t MemoryWriter::write(const DataBuffer& pBuffer)
    {
        size_t writeCount = 0;
        
        if (mBuffer != nullptr)
        {
            writeCount = mPosition + pBuffer.size() > mSize ? mSize - mPosition : pBuffer.size();
            if (writeCount > 0)
            {
                memcpy(reinterpret_cast<char*>(mBuffer) + mPosition, pBuffer.data(), writeCount);
                mPosition += writeCount;
            }
        }
        
        return writeCount;
    }
    
    bool MemoryWriter::seek(int64_t pPosition, bool pRelative)
    {
        bool success = false;
        
        if (mBuffer != nullptr)
        {
            int64_t position = pRelative ? static_cast<int64_t>(mPosition) + pPosition : pPosition;
            mPosition = std::max(static_cast<size_t>(0), std::min(static_cast<size_t>(position), mSize));
            success = true;
        }
        
        return success;
    }
    
    bool MemoryWriter::flush()
    {
        return mBuffer != nullptr;
    }
    
    size_t MemoryWriter::getPosition() const
    {
        return mPosition;
    }
    
    bool MemoryWriter::isOpen() const
    {
        return mBuffer != nullptr;
    }
    
    /* FileReader */
    
    FileReader::FileReader(const std::string& pFilePath)
    {
        mStream.open(pFilePath, std::ifstream::in | std::ifstream::binary);
        
        if (mStream.is_open())
        {
            mStream.seekg(0, std::ios::end);
            mSize = static_cast<size_t>(mStream.tellg());
            mStream.seekg(0, std::ios::beg);
        }
    }
    
    size_t FileReader::read(void* pBuffer)
    {
        return read(pBuffer, mSize - mPosition);
    }
    
    size_t FileReader::read(void* pBuffer, size_t pSize)
    {
        size_t readCount = 0;
        
        if (mStream.is_open())
        {
            mStream.read(reinterpret_cast<char*>(pBuffer), static_cast<std::streamsize>(pSize));
            
            readCount = static_cast<size_t>(mStream.gcount());
            mPosition += readCount;
            
            auto flag = mStream.rdstate();
            if ((flag & std::ifstream::badbit) == 0 && (flag & std::ifstream::failbit) != 0)
                mStream.clear(flag & ~std::ifstream::failbit);
        }
        
        return readCount;
    }
    
    void FileReader::read(std::string& pText)
    {
        read(pText, mSize - mPosition);
    }
    
    void FileReader::read(std::string& pText, size_t pSize)
    {
        if (mStream.is_open())
        {
            size_t readCount = mPosition + pSize > mSize ? mSize - mPosition : pSize;
            size_t offset = pText.size();
            
            pText.resize(offset + readCount, 0);
            mStream.read(&pText[0], static_cast<std::streamsize>(readCount));
            
            std::streamsize read = mStream.gcount();
            if (read < readCount)
                pText.resize(offset + static_cast<size_t>(read));
            
            mPosition += pText.size() - offset;
            
            auto flag = mStream.rdstate();
            if ((flag & std::ifstream::badbit) == 0 && (flag & std::ifstream::failbit) != 0)
                mStream.clear(flag & ~std::ifstream::failbit);
        }
    }
    
    void FileReader::read(DataBuffer& pBuffer)
    {
        read(pBuffer, mSize - mPosition);
    }
    
    void FileReader::read(DataBuffer& pBuffer, size_t pSize)
    {
        if (mStream.is_open())
        {
            size_t offset = pBuffer.size();
            
            const size_t readMaxSize = 1024;
            char buff[readMaxSize];
            size_t read = 0;
            do
            {
                mStream.read(buff, static_cast<std::streamsize>(std::min(pSize - read, readMaxSize)));
                pBuffer.push_back(buff, static_cast<size_t>(mStream.gcount()));
                read += static_cast<size_t>(mStream.gcount());
                
                if (!mStream.good())
                    break;
            }
            while (read < pSize);
            
            mPosition += pBuffer.size() - offset;
            
            auto flag = mStream.rdstate();
            if ((flag & std::ifstream::badbit) == 0 && (flag & std::ifstream::failbit) != 0)
                mStream.clear(flag & ~std::ifstream::failbit);
        }
    }
    
    bool FileReader::seek(int64_t pPosition, bool pRelative)
    {
        bool success = false;
        
        if (mStream.is_open())
        {
            mStream.seekg(pPosition, pRelative ? std::ios::cur : std::ios::beg);
            
            auto flag = mStream.rdstate();
            if (flag != std::ifstream::goodbit)
            {
                if ((flag & std::ifstream::badbit) == 0)
                    mStream.clear();
            }
            else
            {
                int64_t position = pRelative ? static_cast<int64_t>(mPosition) + pPosition : pPosition;
                mPosition = std::max(static_cast<size_t>(0), std::min(static_cast<size_t>(position), mSize));
                success = true;
            }
        }
        
        return success;
    }
    
    size_t FileReader::getPosition() const
    {
        return mPosition;
    }
    
    size_t FileReader::getSize() const
    {
        return mSize;
    }
    
    bool FileReader::isOpen() const
    {
        return mStream.is_open();
    }
    
    /* MemoryReader */
    
    MemoryReader::MemoryReader(const void* pMemory, size_t pSize, bool pDeleteOnClose) : mBuffer(pMemory), mSize(pSize), mDeleteBufferOnClose(pDeleteOnClose)
    {
    }
    
    MemoryReader::~MemoryReader()
    {
        if (mBuffer != nullptr && mDeleteBufferOnClose)
            delete [] reinterpret_cast<const char*>(mBuffer);
    }
    
    size_t MemoryReader::read(void* pBuffer)
    {
        return read(pBuffer, mSize - mPosition);
    }
    
    size_t MemoryReader::read(void* pBuffer, size_t pSize)
    {
        size_t readCount = 0;
        
        if (mBuffer != nullptr)
        {
            readCount = mPosition + pSize > mSize ? mSize - mPosition : pSize;
            if (readCount > 0)
            {
                memcpy(pBuffer, reinterpret_cast<const char*>(mBuffer) + mPosition, readCount);
                mPosition += readCount;
            }
        }
        
        return readCount;
    }
    
    void MemoryReader::read(std::string& pText)
    {
        read(pText, mSize - mPosition);
    }
    
    void MemoryReader::read(std::string& pText, size_t pSize)
    {
        if (mBuffer != nullptr)
        {
            size_t readCount = mPosition + pSize > mSize ? mSize - mPosition : pSize;
            if (readCount > 0)
            {
                size_t offset = pText.size();
                pText.resize(offset + readCount, 0);
                memcpy(&pText[offset], reinterpret_cast<const char*>(mBuffer) + mPosition, readCount);
                mPosition += readCount;
            }
        }
    }
    
    void MemoryReader::read(DataBuffer& pBuffer)
    {
        read(pBuffer, mSize - mPosition);
    }
    
    void MemoryReader::read(DataBuffer& pBuffer, size_t pSize)
    {
        if (mBuffer != nullptr)
        {
            size_t readCount = mPosition + pSize > mSize ? mSize - mPosition : pSize;
            if (readCount > 0)
            {
                pBuffer.push_back(reinterpret_cast<const char*>(mBuffer) + mPosition, readCount);
                mPosition += readCount;
            }
        }
    }
    
    bool MemoryReader::seek(int64_t pPosition, bool pRelative)
    {
        bool success = false;
        
        if (mBuffer != nullptr)
        {
            int64_t position = pRelative ? static_cast<int64_t>(mPosition) + pPosition : pPosition;
            mPosition = std::max(static_cast<size_t>(0), std::min(static_cast<size_t>(position), mSize));
            success = true;
        }
        
        return success;
    }
    
    size_t MemoryReader::getPosition() const
    {
        return mPosition;
    }
    
    size_t MemoryReader::getSize() const
    {
        return mSize;
    }
    
    bool MemoryReader::isOpen() const
    {
        return mBuffer != nullptr;
    }

    /* DirectoryReader */
    
    class DirectoryReader : public DataStorageReader
    {
    public:
        DirectoryReader() = delete;
        DirectoryReader(const DirectoryReader& pOther) = delete;
        DirectoryReader(DirectoryReader&& pOther) = delete;
        
        DirectoryReader& operator=(const DirectoryReader& pOther) = delete;
        DirectoryReader& operator=(DirectoryReader&& pOther) = delete;
        
        virtual ~DirectoryReader() = default;
        virtual std::unique_ptr<DataReader> openFile(const std::string& pName) const override
        {
            std::unique_ptr<DataReader> reader = nullptr;
            
            for (auto& name : mFileName)
            {
                if (mPrefix + name == pName)
                {
                    reader = std::unique_ptr<DataReader>(new FileReader(mPath + name));
                    break;
                }
            }
            
            if (reader == nullptr)
            {
                for (auto it = mSubdirectory.cbegin(); it != mSubdirectory.cend(); it++)
                {
                    if ((reader = it->get()->openFile(pName)) != nullptr)
                        break;
                }
            }
            
            return reader;
        }
        virtual bool clearStorage(const std::string& pPath) override
        {
            bool found = false;
            
            std::string path = pPath.back() != '/' ? pPath + '/' : pPath;
            if (path == mPath)
            {
                found = true;
                for (const auto& v : mFileName)
                    std::remove((mPath + v).c_str());
                
                mFileName.clear();
            }
            else
            {
                for (auto& v : mSubdirectory)
                {
                    if ((found = v->clearStorage(path)))
                        break;
                }
            }
            
            return found;
        }
        
    private:
        friend class DirectoryLoader;
        
        std::vector<std::string> mFileName;
        std::vector<std::unique_ptr<DirectoryReader>> mSubdirectory;
        std::string mPath;
        
        DirectoryReader(const std::string& pPath, const std::string& pPrefix) : DataStorageReader(pPrefix)
        {
#if defined(__APPLE__) || defined(__linux__)
            DIR* dir = NULL;
            if ((dir = opendir(pPath.c_str())) != NULL)
            {
                const size_t reserveSize = 10;
                
                mPath = pPath.back() != '/' ? pPath + '/' : pPath;
                auto isDir = [](const std::string& lpFilePath) -> bool
                {
                    struct stat stat_buf;
                    stat(lpFilePath.c_str(), &stat_buf);
                    return S_ISDIR(stat_buf.st_mode);
                };
                
                struct dirent *file;
                while ((file = readdir(dir)) != NULL)
                {
                    std::string name{file->d_name};
                    if (name != "." && name != "..")
                    {
                        std::string fullPath = mPath + name;
                        if (!isDir(fullPath))
                        {
                            if (mFileName.capacity() == mFileName.size())
                                mFileName.reserve(mFileName.capacity() + reserveSize);
                            
                            mFileName.push_back(std::move(name));
                        }
                        else
                        {
                            if (name.back() != '/')
                                name += '/';
                            
                            if (mSubdirectory.capacity() == mSubdirectory.size())
                                mSubdirectory.reserve(mSubdirectory.capacity() + reserveSize);
                            
                            mSubdirectory.push_back(std::unique_ptr<DirectoryReader>(new DirectoryReader(fullPath, pPrefix + name)));
                        }
                    }
                }
                closedir(dir);
                mFileName.shrink_to_fit();
                mSubdirectory.shrink_to_fit();
            }
#endif
        }
    };
    
    /* DirectoryLoader */
    
    class DirectoryLoader : public DataStorageLoader
    {
    public:
        DirectoryLoader() = default;
        virtual ~DirectoryLoader() = default;
        
        virtual bool isLoadable(const std::string& pPath) const override
        {
            bool loadable = false;
            
#if defined(__APPLE__) || defined(__linux__)
            DIR* dir = NULL;
            if ((dir = opendir(pPath.c_str())) != NULL)
            {
                loadable = true;
                closedir(dir);
            }
#endif
            
            return loadable;
        }
        
        virtual bool isLoadable(DataReader& pReader) const override
        {
            return false;
        }
        
        virtual std::unique_ptr<DataStorageReader> createStorageReader(const std::string& pPath, const std::string& pPrefix = "") const override
        {
            return std::unique_ptr<DirectoryReader>(new DirectoryReader(pPath, pPrefix));
        }
        
        virtual std::unique_ptr<DataStorageReader> createStorageReader(DataReader& pReader, const std::string& pPrefix = "") const override
        {
            return nullptr;
        }
    };
    
    /* DataManager */
    
    DataManager::DataManager()
    {
        addStorageLoader(std::unique_ptr<DirectoryLoader>(new DirectoryLoader()));
    }
    
    DataManager::~DataManager()
    {
        terminate();
    }
    
    bool DataManager::initialize()
    {
        if (mInitialized)
            return false;

        mInitialized = true;
        
        return true;
    }
    
    bool DataManager::terminate()
    {
        if (!mInitialized)
            return false;
        
        mData.clear();

        mInitialized = false;
        
        return true;
    }
    
    bool DataManager::add(std::shared_ptr<DataShared> pData)
    {
        bool status = false;

        if (pData.get() != nullptr)
        {
            const size_t id = pData.get()->getId();
            
            // resize array if required
            if (id >= mData.size())
                mData.resize(id + 1);
            
            // check if Id is in use
            assert(mData[id].get() == nullptr);
            
            mData[id] = pData;
            
            status = true;
        }
        
        return status;
    }
    
    size_t DataManager::count() const
    {
        return mData.size();
    }
    
    std::shared_ptr<DataShared> DataManager::get(size_t pID) const
    {
        // check if Id has a valid value
        assert(pID < mData.size());
        
        return mData[pID];
    }
    
    void DataManager::setCipher(std::function<void(std::string& lpKey, std::string& lpIV)> pCipher)
    {
        mCipher = std::move(pCipher);
    }
    
    void DataManager::setWorkingDirectory(std::string pWorkingDirectory)
    {
        mWorkingDirectory = std::move(pWorkingDirectory);
        
        if (mWorkingDirectory.length() != 0 && mWorkingDirectory.back() != '/')
            mWorkingDirectory += '/';
    }
    
    std::string DataManager::getWorkingDirectory() const
    {
        return mWorkingDirectory;
    }
    
    bool DataManager::readFile(const std::string& pFileName, DataShared* pData, const std::vector<unsigned>& pUserData, EDataSharedType pType)
    {
        using namespace crypto;
                
        assert(pData != nullptr && pFileName.length() != 0);

        bool status = false;
        std::unique_ptr<DataReader> reader = nullptr;
        
        for (auto it = mStorage.cbegin(); it != mStorage.cend(); it++)
        {
            reader = it->get()->openFile(pFileName);
            if (reader != nullptr)
                break;
        }
        
        if (reader == nullptr)
            reader = getDataReader(pFileName);
        
        if (reader->isOpen())
        {
            auto decryptCallback = [this](std::string* lpString, DataBuffer* lpBuffer, ECryptoMode lpMode) -> void
            {
                if (mCipher != nullptr && lpMode != ECryptoMode::None)
                {
                    std::string key, iv;
                    mCipher(key, iv);
                    
                    if (lpString != nullptr)
                    {
                        *lpString = decrypt(*lpString, key, iv, lpMode);
                        
                        size_t eofCount = 0;
                        auto it = lpString->rbegin();
                        for (; it != lpString->rend() && *(it) == '\x03'; it++) // check for end of text character at the end
                            eofCount++;
                        
                        if (eofCount != 0)
                            lpString->erase(it.base(), lpString->end());
                    }
                    else if (lpBuffer != nullptr)
                    {
                        *lpBuffer = decrypt(*lpBuffer, key, iv, lpMode);
                        
                        auto data = reinterpret_cast<const char*>(lpBuffer->data());
                        size_t eofCount = 0;
                        for (size_t i = lpBuffer->size(); i > 0 && data[i] == '\x03'; i--)
                            eofCount++;
                        
                        if (eofCount != 0)
                            lpBuffer->pop_back(eofCount);
                    }
                    
                    // TODO find reliable way to clear strings without compiler optimizing it away
                    key.clear();
                    iv.clear();
                }
            };
            
            switch (pType)
            {
                case EDataSharedType::Text:
                {
                    std::string text;
                    reader->read(text);
                    
                    decryptCallback(&text, nullptr, pData->getCryptoMode());
                    
                    status = pData->unpack(text, pUserData);
                }
                    break;
                case EDataSharedType::Binary:
                {
                    DataBuffer dataBuffer;
                    reader->read(dataBuffer);
                    
                    decryptCallback(nullptr, &dataBuffer, pData->getCryptoMode());
                    
                    status = pData->unpackBuffer(std::move(dataBuffer), pUserData);
                }
                    break;
                default:
                    // wrong access type
                    assert(false);
                    break;
            }
        }
        else
        {
            Hermes::getInstance()->getLogger()->print(ELogLevel::Warning, "DataShared file not found: \"%\".", mWorkingDirectory + pFileName);
        }
        
        return status;
    }
    
    bool DataManager::readFile(const std::string& pFileName, std::string& pText) const
    {
        assert(pFileName.length() != 0);
        
        bool status = false;
        std::unique_ptr<DataReader> reader = nullptr;
                
        for (auto it = mStorage.cbegin(); it != mStorage.cend(); it++)
        {
            reader = it->get()->openFile(pFileName);
            if (reader != nullptr)
                break;
        }
        
        if (reader == nullptr)
            reader = getDataReader(pFileName);
        
        if (reader->isOpen())
        {
            reader->read(pText);
            status = true;
        }
        
        return status;
    }
    
    bool DataManager::readFile(const std::string& pFileName, DataBuffer& pDataBuffer) const
    {
        assert(pFileName.length() != 0);
        
        bool status = false;
        std::unique_ptr<DataReader> reader = nullptr;
        
        for (auto it = mStorage.cbegin(); it != mStorage.cend(); it++)
        {
            reader = it->get()->openFile(pFileName);
            if (reader != nullptr)
                break;
        }
        
        if (reader == nullptr)
            reader = getDataReader(pFileName);
        
        if (reader->isOpen())
        {
            reader->read(pDataBuffer);
            status = true;
        }
        
        return status;
    }
    
    bool DataManager::writeFile(const std::string& pFileName, const DataShared* pData, const std::vector<unsigned>& pUserData, EDataSharedType pType, bool pClearContent) const
    {
        using namespace crypto;
        
        assert(pData != nullptr && pFileName.length() != 0);
        
        bool status = false;
        auto writer = getDataWriter(pFileName, !pClearContent);
        
        if (writer->isOpen())
        {
            auto encryptCallback = [this](std::string* lpString, DataBuffer* lpBuffer, ECryptoMode lpMode) -> void
            {
                if (mCipher != nullptr && lpMode != ECryptoMode::None)
                {
                    const size_t blockLength = 16;
                    std::string key, iv;
                    
                    mCipher(key, iv);
                    
                    if (lpString != nullptr)
                    {
                        const size_t fillCount = lpString->length() % blockLength;
                        if (fillCount != 0)
                            lpString->insert(lpString->end(), blockLength - fillCount, '\x03');
                        
                        *lpString = encrypt(*lpString, key, iv, lpMode);
                    }
                    else if (lpBuffer != nullptr)
                    {
                        const size_t fillCount = lpBuffer->size() % blockLength;
                        if (fillCount != 0)
                        {
                            std::string fill(blockLength - fillCount, '\x03');
                            lpBuffer->push_back(fill.data(), fill.size());
                        }
                        
                        *lpBuffer = encrypt(*lpBuffer, key, iv, lpMode);
                    }
                    
                    // TODO find reliable way to clear strings without compiler optimizing it away
                    key.clear();
                    iv.clear();
                }
            };
            
            switch (pType)
            {
                case EDataSharedType::Text:
                {
                    std::string text = "";
                    status = pData->pack(text, pUserData);
                    
                    if (status)
                    {
                        encryptCallback(&text, nullptr, pData->getCryptoMode());
                        writer->write(text);
                    }
                }
                    break;
                case EDataSharedType::Binary:
                {
                    DataBuffer dataBuffer;
                    status = pData->packBuffer(dataBuffer, pUserData);
                    
                    if (status)
                    {
                        encryptCallback(nullptr, &dataBuffer, pData->getCryptoMode());
                        writer->write(dataBuffer);
                    }
                }
                    break;
                default:
                    // wrong access type
                    assert(false);
                    break;
            }
        }
        else
        {
            Hermes::getInstance()->getLogger()->print(ELogLevel::Warning, "DataShared write fail to: \"%\".", mWorkingDirectory + pFileName);
        }
        
        return status;
    }
    
    bool DataManager::writeFile(const std::string& pFileName, const std::string& pText, bool pClearContent) const
    {
        assert(pFileName.length() != 0);
        
        bool status = false;
        auto writer = getDataWriter(pFileName, !pClearContent);
        
        if (writer->isOpen())
        {
            writer->write(pText);
            status = true;
        }
        
        return status;
    }
    
    bool DataManager::writeFile(const std::string& pFileName, const DataBuffer& pDataBuffer, bool pClearContent) const
    {
        assert(pFileName.length() != 0);
        
        bool status = false;
        auto writer = getDataWriter(pFileName, !pClearContent);
        
        if (writer->isOpen())
        {
            writer->write(pDataBuffer);
            status = true;
        }
        
        return status;
    }
    
    std::unique_ptr<DataReader> DataManager::getDataReader(const std::string& pFileName) const
    {
        return std::unique_ptr<DataReader>(new FileReader(mWorkingDirectory + pFileName));
    }
    
    std::unique_ptr<DataWriter> DataManager::getDataWriter(const std::string& pFileName, bool pAppend) const
    {
        return std::unique_ptr<DataWriter>(new FileWriter(mWorkingDirectory + pFileName, pAppend));
    }
    
    void DataManager::addStorageLoader(std::unique_ptr<DataStorageLoader> pLoader)
    {
        assert(pLoader != nullptr);
        
        mLoader.push_back(std::move(pLoader));
    }
    
    void DataManager::addStorageReader(std::unique_ptr<DataStorageReader> pReader)
    {
        assert(pReader != nullptr);
        
        mStorage.push_back(std::move(pReader));
    }
    
    bool DataManager::addStorageReader(const std::string& pName, const std::string& pPrefix, bool pRelativeToWorkspace)
    {
        bool added = false;
        
        const std::string filePath = pRelativeToWorkspace ? mWorkingDirectory + pName : pName;
        for (auto it = mLoader.cbegin(); it != mLoader.cend(); it++)
        {
            if(it->get()->isLoadable(filePath))
            {
                added = true;
                mStorage.push_back(it->get()->createStorageReader(filePath, pPrefix));
                break;
            }
        }
        
        return added;
    }
    
    bool DataManager::addStorageReader(DataReader& pReader, const std::string& pPrefix)
    {
        bool added = false;
        
        for (auto it = mLoader.cbegin(); it != mLoader.cend(); it++)
        {
            if(it->get()->isLoadable(pReader))
            {
                added = true;
                mStorage.push_back(it->get()->createStorageReader(pReader, pPrefix));
                break;
            }
        }
        
        return added;
    }
    
    void DataManager::clearDirectory(const std::string& pPath)
    {
        for (auto& v : mStorage)
        {
            if (v->clearStorage(pPath))
                break;
        }
    }
    
}
