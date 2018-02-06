// Copyright (C) 2017 Grupa Pracuj Sp. z o.o.
// This file is part of the "Hermes" library.
// For conditions of distribution and use, see copyright notice in license.txt.

#include "data.hpp"

#include "hermes.hpp"
#include "tools.hpp"

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
    
    /* DataWriter */
    
    DataWriter::DataWriter(const std::string& pFilePath, bool pAppend)
    {
        std::ios_base::openmode opmode = std::ofstream::out | std::ofstream::binary;
        
        if (!pAppend)
            opmode |= std::ofstream::trunc;
        else
            opmode |= std::ofstream::app;
        
        mStream.open(pFilePath, opmode);
        if (mStream.is_open())
            mPosition = mStream.tellp();
    }
    
    DataWriter::DataWriter(void* pMemory, std::streamsize pSize, bool pDeleteOnClose) : mBuffer(pMemory), mSize(pSize), mDeleteBufferOnClose(pDeleteOnClose)
    {
    }
    
    DataWriter::~DataWriter()
    {
        if (mBuffer != nullptr && mDeleteBufferOnClose)
            delete [] reinterpret_cast<char*>(mBuffer);
    }
    
    std::streamsize DataWriter::write(const void* pBuffer, std::streamsize pSize)
    {
        std::streamsize writeCount = 0;
        
        if (mBuffer != nullptr)
        {
            writeCount = static_cast<std::streamsize>(mPosition) + pSize > mSize ? mSize - static_cast<std::streamsize>(mPosition) : pSize;
            if (writeCount > 0)
            {
                memcpy(reinterpret_cast<char*>(mBuffer) + static_cast<int32_t>(mPosition), pBuffer, static_cast<size_t>(writeCount));
                mPosition += writeCount;
            }
        }
        else if (mStream.is_open())
        {
            mStream.write(reinterpret_cast<const char*>(pBuffer), pSize);
            
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
    
    std::streamsize DataWriter::write(const std::string& pText)
    {
        std::streamsize writeCount = 0;
        
        if (mBuffer != nullptr)
        {
            writeCount = mPosition + static_cast<std::streamoff>(pText.size()) > mSize ? mSize - static_cast<std::streamsize>(mPosition) : static_cast<std::streamsize>(pText.size());
            if (writeCount > 0)
            {
                memcpy(reinterpret_cast<char*>(mBuffer) + static_cast<int32_t>(mPosition), pText.data(), static_cast<size_t>(writeCount));
                mPosition += writeCount;
            }
        }
        else if (mStream.is_open())
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
                writeCount = static_cast<std::streamsize>(pText.size());
                mPosition += writeCount;
            }
        }
        
        return writeCount;
    }
    
    std::streamsize DataWriter::write(const DataBuffer& pBuffer)
    {
        std::streamsize writeCount = 0;
        
        if (mBuffer != nullptr)
        {
            writeCount = mPosition + static_cast<std::streamoff>(pBuffer.size()) > mSize ? mSize - static_cast<std::streamsize>(mPosition) : static_cast<std::streamsize>(pBuffer.size());
            if (writeCount > 0)
            {
                memcpy(reinterpret_cast<char*>(mBuffer) + static_cast<int32_t>(mPosition), pBuffer.data(), static_cast<size_t>(writeCount));
                mPosition += writeCount;
            }
        }
        else if (mStream.is_open())
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
                writeCount = static_cast<std::streamsize>(pBuffer.size());
                mPosition += writeCount;
            }
        }
        
        return writeCount;
    }
    
    bool DataWriter::seek(std::streamoff pPosition, bool pRelative)
    {
        bool success = false;
        
        if (mBuffer != nullptr)
        {
            mPosition = std::max(static_cast<std::streampos>(0), std::min(pRelative ? mPosition + pPosition : static_cast<std::streampos>(pPosition), static_cast<std::streampos>(mSize)));
            success = true;
        }
        else if (mStream.is_open())
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
                mPosition = std::max(static_cast<std::streampos>(0), std::min(pRelative ? mPosition + pPosition : static_cast<std::streampos>(pPosition), static_cast<std::streampos>(mSize)));
                success = true;
            }
        }
        
        return success;
    }
    
    bool DataWriter::flush()
    {
        bool success = true;
        
        if (mBuffer == nullptr && mStream.is_open())
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
    
    std::streampos DataWriter::getPosition() const
    {
        return mPosition;
    }
    
    bool DataWriter::isOpen() const
    {
        return mBuffer == nullptr ? mStream.is_open() : true;
    }
    
    /* DataReader */
    
    DataReader::DataReader(const std::string& pFilePath)
    {
        mStream.open(pFilePath, std::ifstream::in | std::ifstream::binary);
        
        if (mStream.is_open())
        {
            mStream.seekg(0, std::ios::end);
            mSize = static_cast<std::streamsize>(mStream.tellg());
            mStream.seekg(0, std::ios::beg);
        }
    }
    
    DataReader::DataReader(const void* pMemory, std::streamsize pSize, bool pDeleteOnClose) : mBuffer(pMemory), mSize(pSize), mDeleteBufferOnClose(pDeleteOnClose)
    {
    }
    
    DataReader::~DataReader()
    {
        if (mBuffer != nullptr && mDeleteBufferOnClose)
            delete [] reinterpret_cast<const char*>(mBuffer);
    }
    
    std::streamsize DataReader::read(void* pBuffer)
    {
        return read(pBuffer, mSize - static_cast<std::streamsize>(mPosition));
    }
    
    std::streamsize DataReader::read(void* pBuffer, std::streamsize pSize)
    {
        std::streamsize readCount = 0;
        
        if (mBuffer != nullptr)
        {
            readCount = static_cast<std::streamsize>(mPosition) + pSize > mSize ? mSize - static_cast<std::streamsize>(mPosition) : pSize;
            if (readCount > 0)
            {
                memcpy(pBuffer, reinterpret_cast<const char*>(mBuffer) + static_cast<int32_t>(mPosition), static_cast<size_t>(readCount));
                mPosition += readCount;
            }
        }
        else if (mStream.is_open())
        {
            mStream.read(reinterpret_cast<char*>(pBuffer), pSize);
            mPosition += mStream.gcount();
            
            auto flag = mStream.rdstate();
            if ((flag & std::ifstream::badbit) == 0 && (flag & std::ifstream::failbit) != 0)
                mStream.clear(flag & ~std::ifstream::failbit);
        }
        
        return readCount;
    }
    
    void DataReader::read(std::string& pText)
    {
        read(pText, mSize - static_cast<std::streamsize>(mPosition));
    }
    
    void DataReader::read(std::string& pText, std::streamsize pSize)
    {
        if (mBuffer != nullptr)
        {
            size_t readCount = static_cast<size_t>(static_cast<std::streamsize>(mPosition) + pSize > mSize ? mSize - mPosition : pSize);
            if (readCount > 0)
            {
                size_t offset = pText.size();
                pText.resize(offset + readCount, 0);
                memcpy(&pText[offset], reinterpret_cast<const char*>(mBuffer) + static_cast<int32_t>(mPosition), readCount);
                mPosition += static_cast<std::streamoff>(readCount);
            }
        }
        else if (mStream.is_open())
        {
            std::streamsize readCount = static_cast<std::streamsize>(mPosition) + pSize > mSize ? mSize - static_cast<std::streamsize>(mPosition) : pSize;
            size_t offset = pText.size();
            
            pText.resize(offset + static_cast<size_t>(readCount), 0);
            mStream.read(&pText[0], readCount);
            
            std::streamsize read = mStream.gcount();
            if (read < readCount)
                pText.resize(pText.length() - (offset + static_cast<size_t>(read)));
            
            mPosition += static_cast<std::streamoff>(pText.size() - offset);
            
            auto flag = mStream.rdstate();
            if ((flag & std::ifstream::badbit) == 0 && (flag & std::ifstream::failbit) != 0)
                mStream.clear(flag & ~std::ifstream::failbit);
        }
    }
    
    void DataReader::read(DataBuffer& pBuffer)
    {
        read(pBuffer, mSize - static_cast<std::streamsize>(mPosition));
    }
    
    void DataReader::read(DataBuffer& pBuffer, std::streamsize pSize)
    {
        if (mBuffer != nullptr)
        {
            size_t readCount = static_cast<size_t>(static_cast<std::streamsize>(mPosition) + pSize > mSize ? mSize - mPosition : pSize);
            if (readCount > 0)
            {
                pBuffer.push_back(reinterpret_cast<const char*>(mBuffer) + static_cast<int32_t>(mPosition), readCount);
                mPosition += static_cast<std::streamoff>(readCount);
            }
        }
        else if (mStream.is_open())
        {
            size_t offset = pBuffer.size();
            
            const size_t readMaxSize = 1024;
            char buff[readMaxSize];
            std::streamsize read = 0;
            do
            {
                mStream.read(buff, std::min(pSize - read, static_cast<std::streamsize>(readMaxSize)));
                pBuffer.push_back(buff, static_cast<size_t>(mStream.gcount()));
                read += mStream.gcount();
                
                if (!mStream.good())
                    break;
            }
            while (read < pSize);
            
            mPosition += static_cast<std::streamoff>(pBuffer.size() - offset);
            
            auto flag = mStream.rdstate();
            if ((flag & std::ifstream::badbit) == 0 && (flag & std::ifstream::failbit) != 0)
                mStream.clear(flag & ~std::ifstream::failbit);
        }
    }
    
    bool DataReader::seek(std::streamoff pPosition, bool pRelative)
    {
        bool success = false;
        
        if (mBuffer != nullptr)
        {
            mPosition = std::max(static_cast<std::streampos>(0), std::min(pRelative ? mPosition + pPosition : static_cast<std::streampos>(pPosition), static_cast<std::streampos>(mSize)));
            success = true;
        }
        else if (mStream.is_open())
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
                mPosition = std::max(static_cast<std::streampos>(0), std::min(pRelative ? mPosition + pPosition : static_cast<std::streampos>(pPosition), static_cast<std::streampos>(mSize)));
                success = true;
            }
        }
        
        return success;
    }
    
    std::streampos DataReader::getPosition() const
    {
        return mPosition;
    }
    
    std::streamsize DataReader::getSize() const
    {
        return mSize;
    }
    
    bool DataReader::isOpen() const
    {
        return mBuffer == nullptr ? mStream.is_open() : true;
    }
    
    /* DataStorageReader */
    
    DataStorageReader::DataStorageReader(std::unique_ptr<DataReader> pSource, std::string pCallPrefix)
    {
        
    }
    
    std::unique_ptr<DataReader> DataStorageReader::openFile(const std::string& pName)
    {
        return nullptr;
    }
    
    /* DataManager */
    
    DataManager::DataManager()
    {
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
        
        if (mWorkingDirectory.back() != '/')
            mWorkingDirectory += '/';
    }
    
    std::string DataManager::getWorkingDirectory() const
    {
        return mWorkingDirectory;
    }
    
    bool DataManager::readData(std::shared_ptr<DataShared> pData, const std::string& pFileName, const std::vector<unsigned>& pUserData, EDataSharedType pType)
    {
        using namespace crypto;
        
        assert(pData != nullptr && pFileName.length() != 0);

        bool status = false;
        std::unique_ptr<DataReader> reader = nullptr;
        
        for (auto it = mStorage.cbegin(); it != mStorage.cend(); it++)
        {
            reader = (*it)->openFile(pFileName);
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
            reader = (*it)->openFile(pFileName);
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
            reader = (*it)->openFile(pFileName);
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
    
    bool DataManager::writeData(std::shared_ptr<DataShared> pData, const std::string& pFileName, const std::vector<unsigned>& pUserData, EDataSharedType pType, bool pClearContent) const
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
        return std::unique_ptr<DataReader>(new DataReader(mWorkingDirectory + pFileName));
    }
    
    std::unique_ptr<DataWriter> DataManager::getDataWriter(const std::string& pFileName, bool pAppend) const
    {
        return std::unique_ptr<DataWriter>(new DataWriter(mWorkingDirectory + pFileName, pAppend));
    }
    
}
