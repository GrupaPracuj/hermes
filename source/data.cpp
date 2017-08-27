// Copyright (C) 2017 Grupa Pracuj Sp. z o.o.
// This file is part of the "Hermes" library.
// For conditions of distribution and use, see copyright notice in license.txt.

#include "data.hpp"

#include "hermes.hpp"
#include "aes.h"

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
    
    DataShared::DataShared(size_t pId) : mId(pId)
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
                        file << data;
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
    
    void DataShared::printWarning(const std::string& pKey)
    {
        Hermes::getInstance()->getLogger()->print(ELogLevel::Warning, "Could not access \"%\" value.", (pKey.size() > 0) ? pKey : "");
    }
    
    template <>
    bool DataShared::assignOp<bool>(const Json::Value* pSource, bool& pDestination)
    {
        bool status = false;
        
        if (pSource->isBool())
        {
            pDestination = pSource->asBool();
            
            status = true;
        }
        
        return status;
    }
    
    template <>
    bool DataShared::assignOp<int>(const Json::Value* pSource, int& pDestination)
    {
        bool status = false;
        
        if (pSource->isNumeric())
        {
            pDestination = pSource->asInt();
            
            status = true;
        }
        
        return status;
    }
    
    template <>
    bool DataShared::assignOp<long long int>(const Json::Value* pSource, long long int& pDestination)
    {
        bool status = false;
        
        if (pSource->isNumeric())
        {
            pDestination = pSource->asInt64();
            
            status = true;
        }
        
        return status;
    }
    
    template <>
    bool DataShared::assignOp<unsigned>(const Json::Value* pSource, unsigned& pDestination)
    {
        bool status = false;
        
        if (pSource->isNumeric())
        {
            pDestination = pSource->asUInt();
            
            status = true;
        }
        
        return status;
    }
    
    template <>
    bool DataShared::assignOp<unsigned long long int>(const Json::Value* pSource, unsigned long long int& pDestination)
    {
        bool status = false;
        
        if (pSource->isNumeric())
        {
            pDestination = pSource->asUInt64();
            
            status = true;
        }
        
        return status;
    }
    
    template <>
    bool DataShared::assignOp<float>(const Json::Value* pSource, float& pDestination)
    {
        bool status = false;
        
        if (pSource->isNumeric())
        {
            pDestination = pSource->asFloat();
            
            status = true;
        }
        
        return status;
    }
    
    template <>
    bool DataShared::assignOp<double>(const Json::Value* pSource, double& pDestination)
    {
        bool status = false;
        
        if (pSource->isNumeric())
        {
            pDestination = pSource->asDouble();
            
            status = true;
        }
        
        return status;
    }
    
    template <>
    bool DataShared::assignOp<std::string>(const Json::Value* pSource, std::string& pDestination)
    {
        bool status = false;
        
        if (pSource->isString())
        {
            pDestination = pSource->asString();
            
            status = true;
        }
        
        return status;
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
    
    bool DataManager::convertJSON(const Json::Value& pSource, std::string& pDestination) const
    {
        Json::StyledWriter writer;
        pDestination = writer.write(pSource);

        return true;
    }

    bool DataManager::convertJSON(const std::string& pSource, Json::Value& pDestination) const
    {
        bool status = false;
        
        if (pSource.size() > 0)
        {
            Json::Reader reader;
            status = reader.parse(pSource, pDestination);
        }
        
        return status;
    }

    std::string DataManager::decrypt(const std::string& pData, std::string pKey, EDataEncryption pMode) const
    {
        std::string result;
        const size_t blockLength = 16;
        const size_t dataSize = pData.size() + 1;
        
        if (dataSize - 1 > blockLength)
        {
            switch (pMode)
            {
            case EDataEncryption::AES_256_CBC:
                {
                    if (dataSize > 2 * blockLength && (dataSize - 1) % blockLength == 0)
                    {
                        aes_decrypt_ctx context[1];
                        aes_decrypt_key256(reinterpret_cast<const unsigned char*>(pKey.c_str()), context);

                        auto inBuffer = reinterpret_cast<const unsigned char*>(pData.c_str());
                        unsigned char outBuffer[blockLength] = {0};
                        
                        size_t offset = blockLength;
                        size_t length = blockLength;
                        
                        while (true)
                        {
                            if (offset + blockLength >= dataSize)
                                length = dataSize - 1 - offset;

                            aes_decrypt(inBuffer + offset, outBuffer, context);

                            if (length == 0 || length == blockLength)
                            {
                                for (size_t i = 0; i < blockLength; ++i)
                                    outBuffer[i] ^= (inBuffer + offset - blockLength)[i];
                            }

                            offset += length;
                            result += std::string(reinterpret_cast<char*>(outBuffer), length);
                            
                            if (length != blockLength)
                                break;
                        }
                    }
                }
                break;
            case EDataEncryption::AES_256_OFB:
                {
                    aes_encrypt_ctx context[1];
                    aes_encrypt_key256(reinterpret_cast<const unsigned char*>(pKey.c_str()), context);

                    auto inBuffer = reinterpret_cast<const unsigned char*>(pData.c_str());
                    unsigned char iv[blockLength];
                    memcpy(iv, inBuffer, blockLength);
                    unsigned char outBuffer[blockLength] = {0};

                    size_t offset = blockLength;
                    size_t length = blockLength;

                    while (true)
                    {
                        if (offset + blockLength >= dataSize)
                            length = dataSize - 1 - offset;
                        
                        aes_ofb_crypt(inBuffer + offset, outBuffer, static_cast<int>(length), iv, context);
                        offset += length;
                        result += std::string(reinterpret_cast<char*>(outBuffer), length);
                        
                        if (length != blockLength)
                            break;
                    }
                }
                break;
            default:
                Hermes::getInstance()->getLogger()->print(ELogLevel::Warning, "Encryption mode not supported.");
                break;
            }
        }
        
        return result;
    }
    
}
