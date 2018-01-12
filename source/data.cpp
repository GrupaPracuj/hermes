// Copyright (C) 2017 Grupa Pracuj Sp. z o.o.
// This file is part of the "Hermes" library.
// For conditions of distribution and use, see copyright notice in license.txt.

#include "data.hpp"

#include "hermes.hpp"
#include "tools.hpp"

#include <fstream>

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
    
    DataShared::DataShared(size_t pId) : mId(pId), mCryptoMode(crypto::EDataCrypto::None)
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
    
    bool DataShared::readFromFile(const std::string& pFilePath, const std::vector<unsigned>& pUserData, EDataSharedType pType)
    {
        using namespace crypto;
        
        bool status = false;
        
        std::ifstream file(pFilePath, std::ifstream::in | std::ifstream::binary);
        
        if (file.is_open())
        {
            switch (pType)
            {
            case EDataSharedType::Text:
                {
                    std::stringstream ss;
                    ss << file.rdbuf();
                    std::string data = ss.str();
                    
                    if (Hermes::getInstance()->getDataManager()->mCipher != nullptr && mCryptoMode != EDataCrypto::None)
                    {
                        std::string key, iv;
                        Hermes::getInstance()->getDataManager()->mCipher(key, iv);
                        data = decrypt(data.data(), data.size(), key, iv, mCryptoMode);
                        
                        // TODO find reliable way to clear strings without compiler optimizing it away
                        key.clear();
                        iv.clear();

                        size_t eofCount = 0;
                        auto it = data.rbegin();
                        for (; it != data.rend() && *(it) == '\x03'; it++) // check for end of text character at the end
                            eofCount++;

                        if (eofCount != 0)
                            data.erase(it.base(), data.end());
                    }
                    
                    status = unpack(data, pUserData);
                }
                break;
            case EDataSharedType::Binary:
                {
                    std::vector<char> data((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
                    
                    DataBuffer dataBuffer;
                    dataBuffer.push_back<char>(data.data(), data.size());

                    status = unpackBuffer(std::move(dataBuffer), pUserData);
                }
                break;
            default:
                // wrong access type
                assert(false);
                break;
            }
            
            file.close();
        }
        else
        {
            Hermes::getInstance()->getLogger()->print(ELogLevel::Warning, "File not found: \"%\".", pFilePath);
        }
        
        return status;
    }
    
    bool DataShared::writeToFile(const std::string& pFilePath, const std::vector<unsigned>& pUserData, EDataSharedType pType, bool pClearContent) const
    {
        using namespace crypto;
        
        bool status = false;
        
        std::ios_base::openmode opmode = std::fstream::in | std::fstream::out | std::fstream::binary;
        
        if (pClearContent)
            opmode |= std::fstream::trunc;
        else
            opmode |= std::fstream::app;
        
        std::ofstream file(pFilePath, opmode);
        
        if (file.is_open())
        {
            switch (pType)
            {
            case EDataSharedType::Text:
                {
                    std::string data = "";
                    status = pack(data, pUserData);
                    
                    if (status)
                    {
                        if (Hermes::getInstance()->getDataManager()->mCipher != nullptr && mCryptoMode != EDataCrypto::None)
                        {
                            const size_t blockLength = 16;
                            const size_t fillCount = data.length() % blockLength;
                            if (fillCount != 0)
                                data.insert(data.end(), blockLength - fillCount, '\x03');
                            
                            std::string key, iv;
                            Hermes::getInstance()->getDataManager()->mCipher(key, iv);
                            data = encrypt(data.data(), data.size(), key, iv, mCryptoMode);
                            
                            // TODO find reliable way to clear strings without compiler optimizing it away
                            key.clear();
                            iv.clear();
                        }
                        
                        file << data;
                    }
                }
                break;
            case EDataSharedType::Binary:
                {
                    DataBuffer dataBuffer;
                    status = packBuffer(dataBuffer, pUserData);
                        
                    if (status)
                        file.write(static_cast<const char*>(dataBuffer.data()), static_cast<int>(dataBuffer.size()));
                }
                break;
            default:
                // wrong access type
                assert(false);
                break;
            }
            
            file.close();
        }
        else
        {
            Hermes::getInstance()->getLogger()->print(ELogLevel::Warning, "Could not write to file: \"%\".", pFilePath);
        }
        
        return status;
    }
    
    size_t DataShared::getId() const
    {
        return mId;
    }
    
    crypto::EDataCrypto DataShared::getCryptoMode() const
    {
        return mCryptoMode;
    }
    
    void DataShared::setCryptoMode(crypto::EDataCrypto pCryptoMode)
    {
        mCryptoMode = pCryptoMode;
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
    
}
