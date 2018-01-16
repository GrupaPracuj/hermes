// Copyright (C) 2017 Grupa Pracuj Sp. z o.o.
// This file is part of the "Hermes" library.
// For conditions of distribution and use, see copyright notice in license.txt.

#ifndef _TOOLS_HPP_
#define _TOOLS_HPP_

#include "json/json.h"
#include "data.hpp"

#include <string>

namespace hms
{    
namespace tools
{
    uint16_t byteSwap16(uint16_t pX);
    uint32_t byteSwap32(uint32_t pX);
    uint64_t byteSwap64(uint64_t pX);
    
    inline bool isLittleEndian()
    {
        uint16_t word = 0x0001;
        char* byte = (char*)&word;
        return byte[0];
    };
    
    class URLTool
    {
    public:
        URLTool(const std::string& pURL);
        URLTool(const URLTool& pTool) : URLTool(pTool.getURL()) {}
        ~URLTool() { mProtocol = mHost = mPath = mParameter = nullptr; }
        
        const std::string& getURL() const;
        const char* getProtocol(size_t& pLength) const;
        const char* getHost(size_t& pLength, bool pIncludePort = false) const;
        const char* getPath(size_t& pLength, bool pIncludeParameter = false) const;
        const char* getParameter(size_t& pLength) const;
        uint16_t getPort() const;
        bool isSecure() const;
        
        std::string getHttpURL(bool pBase = false) const;
        
    private:
        URLTool() = delete;
        URLTool(URLTool&& pTool) = delete;
        
        std::string mURL;
        const char* mProtocol = nullptr;
        const char* mHost = nullptr;
        const char* mPath = nullptr;
        const char* mParameter = nullptr;
        uint16_t mPort = 0;
        size_t mProtocolLength = 0;
        size_t mHostLength = 0;
        size_t mPathLength = 0;
        size_t mPortLength = 0;
        size_t mParameterLength = 0;
        bool mSecure = false;
    };
}
    
namespace json
{
    bool convertJSON(const Json::Value& pSource, std::string& pDestination);
    bool convertJSON(const std::string& pSource, Json::Value& pDestination);
    
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
    
}

namespace crypto
{
    enum class ECryptoMode : int
    {
        AES_256_OFB = 0,
        AES_256_CBC,
        None
    };
    
    std::string encodeBase64(unsigned char const* bytes_to_encode, size_t in_len);
    std::string decodeBase64(std::string const& encoded_string);
        
    std::string getSHA1Digest(const std::string& pData, bool pNetworkOrder = true);
    std::string getRandomCryptoBytes(size_t pLength);
        

    std::string encrypt(const std::string& pData, const std::string& pKey, const std::string& pIV, ECryptoMode pMode);
    DataBuffer encrypt(const DataBuffer& pData, const std::string& pKey, const std::string& pIV, ECryptoMode pMode);
    std::string decrypt(const std::string& pData, const std::string& pKey, const std::string& pIV, ECryptoMode pMode);
    DataBuffer decrypt(const DataBuffer& pData, const std::string& pKey, const std::string& pIV, ECryptoMode pMode);
}
}

#endif
