// Copyright (C) 2017-2020 Grupa Pracuj Sp. z o.o.
// This file is part of the "Hermes" library.
// For conditions of distribution and use, see copyright notice in license.txt.

#include "hmsTools.hpp"

#include "hermes.hpp"
#include "aes.h"

#include <climits>
#include <random>
#include <iomanip>
#include <algorithm>

#if defined(_WIN32)
#include <stdlib.h>
#define bswap_16(X) _byteswap_ushort(X)
#define bswap_32(X) _byteswap_ulong(X)
#define bswap_64(X) _byteswap_uint64(X)
#elif __APPLE__
#include <libkern/OSByteOrder.h>
#define bswap_16(X) OSReadSwapInt16(&X,0)
#define bswap_32(X) OSReadSwapInt32(&X,0)
#define bswap_64(X) OSReadSwapInt64(&X,0)
#else
#define bswap_16(X) ((((X)&0xFF) << 8) | (((X)&0xFF00) >> 8))
#define bswap_32(X) ((((X)&0x000000FF) << 24) | (((X)&0xFF000000) >> 24) | (((X)&0x0000FF00) << 8) | (((X) &0x00FF0000) >> 8))
#define bswap_64(X) ((((X)&0x00000000000000FF) << 56) | (((X)&0xFF00000000000000) >> 56) | (((X)&0x000000000000FF00) << 40) | (((X)&0x00FF000000000000) >> 40) | (((X)&0x0000000000FF0000) << 24) | (((X)&0x0000FF0000000000) >> 24) | (((X)&0x00000000FF000000) << 8) | (((X) &0x000000FF00000000) >> 8))
#endif

namespace hms
{
namespace tools
{
    uint16_t byteSwap16(uint16_t pX)
    {
        return bswap_16(pX);
    }
    
    uint32_t byteSwap32(uint32_t pX)
    {
        return bswap_32(pX);
    }
    
    uint64_t byteSwap64(uint64_t pX)
    {
        return bswap_64(pX);
    }
    
    // URLTool
    
    URLTool::URLTool(const std::string& pURL)
    {
        const size_t protocolOffset = 3;
        
        mURL = pURL;
        
        if (mURL.length() == 0) return;
        
        const size_t protocolPos = mURL.find("://");
        
        if (protocolPos != std::string::npos)
        {
            mProtocol = mURL.data();
            mProtocolLength = protocolPos;
            
            mSecure = (mProtocolLength == 3 && mURL.compare(0, mProtocolLength, "wss")) == 0 || (mProtocolLength == 5 && mURL.compare(0, mProtocolLength, "https") == 0);
        }
        
        size_t hostEndPos = mURL.find('/', mProtocolLength + protocolOffset);
        
        if (hostEndPos == std::string::npos)
        {
            mURL += '/';
            hostEndPos = mURL.length() - 1;
        }
        
        size_t portPos = mURL.find(':', mProtocolLength + protocolOffset);
        
        if (portPos != std::string::npos)
        {
            auto port = mURL.substr(portPos + 1, hostEndPos - (portPos + 1));
            
            try
            {
                mPort = static_cast<uint16_t>(std::stoul(port));
                mPortLength = hostEndPos - portPos;
            }
            catch (std::exception& e)
            {
                Hermes::getInstance()->getLogger()->print(ELogLevel::Error, "Url parsing port conversion fail: '%'", mURL);
            }
        }
        else
        {
            portPos = hostEndPos;
        }
        
        size_t parameterPos = mURL.find('?', hostEndPos);
        
        if (parameterPos != std::string::npos)
        {
            mParameter = mURL.data() + parameterPos;
            mParameterLength = mURL.length() - parameterPos;
        }
        
        mHost = mURL.data() + (mProtocolLength != 0 ? mProtocolLength + protocolOffset : 0);
        mHostLength = portPos - (mProtocolLength != 0 ? mProtocolLength + protocolOffset : 0);
        
        mPath = mURL.data() + hostEndPos;
        mPathLength = mURL.length() - mParameterLength - hostEndPos;
    }
    
    const std::string& URLTool::getURL() const
    {
        return mURL;
    }
    
    const char* URLTool::getProtocol(size_t& pLength) const
    {
        pLength = mProtocolLength;
        return mProtocol;
    }
    
    const char* URLTool::getHost(size_t& pLength, bool pIncludePort) const
    {
        pLength = mHostLength + (pIncludePort ? mPortLength : 0);
        return mHost;
    }
    
    const char* URLTool::getPath(size_t& pLength, bool pIncludeParameter) const
    {
        pLength = mPathLength + (pIncludeParameter ? mParameterLength : 0);
        return mPath;
    }
    
    const char* URLTool::getParameter(size_t& pLength) const
    {
        pLength = mParameterLength;
        return mParameter;
    }
    
    uint16_t URLTool::getPort() const
    {
        return mPort;
    }
    
    bool URLTool::isSecure() const
    {
        return mSecure;
    }
    
    std::string URLTool::getHttpURL(bool pBase) const
    {
        const size_t protocolLength = mProtocolLength == 0 ? 0 : mProtocolLength + 3;
        
        if (mURL.length() == 0)
            return "";
        else
            return (mSecure ? "https://" : "http://") + (pBase ? mURL.substr(protocolLength, mURL.length() - (mPathLength + mParameterLength + protocolLength)) : mURL.substr(protocolLength));
    }
}

namespace crypto
{
    // base64 code from http://www.adp-gmbh.ch/cpp/common/base64.html
    
    std::string encodeBase64(unsigned char const* bytes_to_encode, size_t in_len)
    {
        std::string ret;
        int i = 0;
        int j = 0;
        unsigned char char_array_3[3];
        unsigned char char_array_4[4];
        
        const std::string base64_chars =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789+/";
        
        while (in_len--)
        {
            char_array_3[i++] = *(bytes_to_encode++);
            
            if (i == 3)
            {
                char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
                char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
                char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
                char_array_4[3] = char_array_3[2] & 0x3f;
                
                for (i = 0; (i <4) ; i++)
                    ret += base64_chars[char_array_4[i]];
                
                i = 0;
            }
        }
        
        if (i)
        {
            for (j = i; j < 3; j++)
                char_array_3[j] = '\0';
            
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;
            
            for (j = 0; (j < i + 1); j++)
                ret += base64_chars[char_array_4[j]];
            
            while ((i++ < 3))
                ret += '=';
            
        }
        
        return ret;
        
    }
    
    std::string decodeBase64(std::string const& encoded_string)
    {
        const std::string base64_chars =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789+/";
        
        int in_len = (int)encoded_string.size();
        int i = 0;
        int j = 0;
        size_t in_ = 0;
        char char_array_4[4], char_array_3[3];
        std::string ret;
        
        while (in_len-- && (encoded_string[in_] != '=') && (isalnum(encoded_string[in_]) || (encoded_string[in_] == '+') || (encoded_string[in_] == '/')))
        {
            char_array_4[i++] = encoded_string[in_]; in_++;
            
            if (i ==4)
            {
                for (i = 0; i <4; i++)
                    char_array_4[i] = base64_chars.find(char_array_4[i]);
                
                char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
                char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
                char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];
                
                for (i = 0; (i < 3); i++)
                    ret += char_array_3[i];
                
                i = 0;
            }
        }
        
        if (i)
        {
            for (j = i; j <4; j++)
                char_array_4[j] = 0;
            
            for (j = 0; j <4; j++)
                char_array_4[j] = base64_chars.find(char_array_4[j]);
            
            char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
            char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
            char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];
            
            for (j = 0; (j < i - 1); j++)
                ret += char_array_3[j];
        }
        
        return ret;
    }
    
    /*
     sha1.cpp - source code of
     ============
     SHA-1 in C++
     ============
     100% Public Domain.
     Original C Code
     -- Steve Reid <steve@edmweb.com>
     Small changes to fit into bglibs
     -- Bruce Guenter <bruce@untroubled.org>
     Translation to simpler C++ Code
     -- Volker Grabsch <vog@notjusthosting.com>
     Safety fixes
     -- Eugene Hopkinson <slowriot at voxelstorm dot com>
     */
    
    const size_t cBlockInts = 16;  /* number of 32bit integers per SHA1 block */
    const size_t cBlockBytes = cBlockInts * 4;
    
    uint32_t rol(const uint32_t pValue, const size_t pBits)
    {
        return (pValue << pBits) | (pValue >> (32 - pBits));
    }
    
    uint32_t blk(const std::array<uint32_t, cBlockInts>& pBlock, const size_t pIndex)
    {
        return rol(pBlock[(pIndex + 13) & 15] ^ pBlock[(pIndex + 8) & 15] ^ pBlock[(pIndex + 2) & 15] ^ pBlock[pIndex], 1);
    }
    
    /*
     * (R0+R1), R2, R3, R4 are the different operations used in SHA1
     */
    
    void R0(const std::array<uint32_t, cBlockInts>& pBlock, const uint32_t pV, uint32_t &pW, const uint32_t pX, const uint32_t pY, uint32_t &pZ, const size_t pIndex)
    {
        pZ += ((pW & (pX ^ pY)) ^ pY) + pBlock[pIndex] + 0x5a827999 + rol(pV, 5);
        pW = rol(pW, 30);
    }
    
    void R1(std::array<uint32_t, cBlockInts>& pBlock, const uint32_t pV, uint32_t &pW, const uint32_t pX, const uint32_t pY, uint32_t &pZ, const size_t pIndex)
    {
        pBlock[pIndex] = blk(pBlock, pIndex);
        pZ += ((pW & (pX ^ pY)) ^ pY) + pBlock[pIndex] + 0x5a827999 + rol(pV, 5);
        pW = rol(pW, 30);
    }
    
    void R2(std::array<uint32_t, cBlockInts>& pBlock, const uint32_t pV, uint32_t &pW, const uint32_t pX, const uint32_t pY, uint32_t &pZ, const size_t pIndex)
    {
        pBlock[pIndex] = blk(pBlock, pIndex);
        pZ += (pW ^ pX ^ pY) + pBlock[pIndex] + 0x6ed9eba1 + rol(pV, 5);
        pW = rol(pW, 30);
    }
    
    void R3(std::array<uint32_t, cBlockInts>& pBlock, const uint32_t pV, uint32_t &pW, const uint32_t pX, const uint32_t pY, uint32_t &pZ, const size_t pIndex)
    {
        pBlock[pIndex] = blk(pBlock, pIndex);
        pZ += (((pW | pX) & pY) | (pW & pX)) + pBlock[pIndex] + 0x8f1bbcdc + rol(pV, 5);
        pW = rol(pW, 30);
    }
    
    void R4(std::array<uint32_t, cBlockInts>& pBlock, const uint32_t pV, uint32_t &pW, const uint32_t pX, const uint32_t pY, uint32_t &pZ, const size_t pIndex)
    {
        pBlock[pIndex] = blk(pBlock, pIndex);
        pZ += (pW ^ pX ^ pY) + pBlock[pIndex] + 0xca62c1d6 + rol(pV, 5);
        pW = rol(pW, 30);
    }
    
    /*
     * Hash a single 512-bit block. This is the core of the algorithm.
     */
    
    void transform(std::array<uint32_t, 5>& pDigest, std::array<uint32_t, cBlockInts>& pBlock, uint64_t &pTransforms)
    {
        /* Copy digest[] to working vars */
        uint32_t a = pDigest[0];
        uint32_t b = pDigest[1];
        uint32_t c = pDigest[2];
        uint32_t d = pDigest[3];
        uint32_t e = pDigest[4];
        
        /* 4 rounds of 20 operations each. Loop unrolled. */
        R0(pBlock, a, b, c, d, e,  0);
        R0(pBlock, e, a, b, c, d,  1);
        R0(pBlock, d, e, a, b, c,  2);
        R0(pBlock, c, d, e, a, b,  3);
        R0(pBlock, b, c, d, e, a,  4);
        R0(pBlock, a, b, c, d, e,  5);
        R0(pBlock, e, a, b, c, d,  6);
        R0(pBlock, d, e, a, b, c,  7);
        R0(pBlock, c, d, e, a, b,  8);
        R0(pBlock, b, c, d, e, a,  9);
        R0(pBlock, a, b, c, d, e, 10);
        R0(pBlock, e, a, b, c, d, 11);
        R0(pBlock, d, e, a, b, c, 12);
        R0(pBlock, c, d, e, a, b, 13);
        R0(pBlock, b, c, d, e, a, 14);
        R0(pBlock, a, b, c, d, e, 15);
        R1(pBlock, e, a, b, c, d,  0);
        R1(pBlock, d, e, a, b, c,  1);
        R1(pBlock, c, d, e, a, b,  2);
        R1(pBlock, b, c, d, e, a,  3);
        R2(pBlock, a, b, c, d, e,  4);
        R2(pBlock, e, a, b, c, d,  5);
        R2(pBlock, d, e, a, b, c,  6);
        R2(pBlock, c, d, e, a, b,  7);
        R2(pBlock, b, c, d, e, a,  8);
        R2(pBlock, a, b, c, d, e,  9);
        R2(pBlock, e, a, b, c, d, 10);
        R2(pBlock, d, e, a, b, c, 11);
        R2(pBlock, c, d, e, a, b, 12);
        R2(pBlock, b, c, d, e, a, 13);
        R2(pBlock, a, b, c, d, e, 14);
        R2(pBlock, e, a, b, c, d, 15);
        R2(pBlock, d, e, a, b, c,  0);
        R2(pBlock, c, d, e, a, b,  1);
        R2(pBlock, b, c, d, e, a,  2);
        R2(pBlock, a, b, c, d, e,  3);
        R2(pBlock, e, a, b, c, d,  4);
        R2(pBlock, d, e, a, b, c,  5);
        R2(pBlock, c, d, e, a, b,  6);
        R2(pBlock, b, c, d, e, a,  7);
        R3(pBlock, a, b, c, d, e,  8);
        R3(pBlock, e, a, b, c, d,  9);
        R3(pBlock, d, e, a, b, c, 10);
        R3(pBlock, c, d, e, a, b, 11);
        R3(pBlock, b, c, d, e, a, 12);
        R3(pBlock, a, b, c, d, e, 13);
        R3(pBlock, e, a, b, c, d, 14);
        R3(pBlock, d, e, a, b, c, 15);
        R3(pBlock, c, d, e, a, b,  0);
        R3(pBlock, b, c, d, e, a,  1);
        R3(pBlock, a, b, c, d, e,  2);
        R3(pBlock, e, a, b, c, d,  3);
        R3(pBlock, d, e, a, b, c,  4);
        R3(pBlock, c, d, e, a, b,  5);
        R3(pBlock, b, c, d, e, a,  6);
        R3(pBlock, a, b, c, d, e,  7);
        R3(pBlock, e, a, b, c, d,  8);
        R3(pBlock, d, e, a, b, c,  9);
        R3(pBlock, c, d, e, a, b, 10);
        R3(pBlock, b, c, d, e, a, 11);
        R4(pBlock, a, b, c, d, e, 12);
        R4(pBlock, e, a, b, c, d, 13);
        R4(pBlock, d, e, a, b, c, 14);
        R4(pBlock, c, d, e, a, b, 15);
        R4(pBlock, b, c, d, e, a,  0);
        R4(pBlock, a, b, c, d, e,  1);
        R4(pBlock, e, a, b, c, d,  2);
        R4(pBlock, d, e, a, b, c,  3);
        R4(pBlock, c, d, e, a, b,  4);
        R4(pBlock, b, c, d, e, a,  5);
        R4(pBlock, a, b, c, d, e,  6);
        R4(pBlock, e, a, b, c, d,  7);
        R4(pBlock, d, e, a, b, c,  8);
        R4(pBlock, c, d, e, a, b,  9);
        R4(pBlock, b, c, d, e, a, 10);
        R4(pBlock, a, b, c, d, e, 11);
        R4(pBlock, e, a, b, c, d, 12);
        R4(pBlock, d, e, a, b, c, 13);
        R4(pBlock, c, d, e, a, b, 14);
        R4(pBlock, b, c, d, e, a, 15);
        
        /* Add the working vars back into digest[] */
        pDigest[0] += a;
        pDigest[1] += b;
        pDigest[2] += c;
        pDigest[3] += d;
        pDigest[4] += e;
        
        /* Count the number of transformations */
        pTransforms++;
    }
    
    void bufferToBlock(const std::string& lpBuffer, std::array<uint32_t, cBlockInts>& lpBlock)
    {
        /* Convert the std::string (byte buffer) to a uint32_t array (MSB) */
        for (size_t i = 0; i < lpBlock.size(); i++)
        {
            lpBlock[i] = (static_cast<uint32_t>(lpBuffer[4 * i + 3]) & 0xff) | (static_cast<uint32_t>(lpBuffer[4 * i + 2]) & 0xff) << 8 | (static_cast<uint32_t>(lpBuffer[4 * i + 1]) & 0xff) << 16 | (static_cast<uint32_t>(lpBuffer[4 * i + 0]) & 0xff) << 24;
        }
    };
    
    std::string getSHA1Digest(const std::string& pData, bool pNetworkOrder)
    {
        if (pData.size() == 0) return "";
        
        std::string buffer;
        uint64_t transformCount = 0;
        std::array<uint32_t, 5> digest = {
            0x67452301,
            0xefcdab89,
            0x98badcfe,
            0x10325476,
            0xc3d2e1f0
        };
        std::array<uint32_t, cBlockInts> block = {
            0,
            0,
            0,
            0,
            0
        };
        size_t readOffset = 0;
        const char* data = pData.c_str();
        
        buffer.reserve(cBlockBytes);
        
        do
        {
            size_t readCount = readOffset + cBlockBytes > pData.size() ? pData.size() - readOffset : cBlockBytes;
            buffer.append(data + readOffset, readCount);
            readOffset += readCount;
            
            if (readCount != cBlockBytes)
                break;
            
            bufferToBlock(buffer, block);
            transform(digest, block, transformCount);
            buffer.clear();
        }
        while (true);
        
        /* Total number of hashed bits */
        uint64_t totalBits = (transformCount * cBlockBytes + buffer.size()) * 8;
        
        /* Padding */
        buffer += static_cast<char>(0x80);
        
        size_t originalSize = buffer.size();
        
        while (buffer.size() < cBlockBytes)
            buffer += static_cast<char>(0x00);
        
        bufferToBlock(buffer, block);
        
        if (originalSize > cBlockBytes - 8)
        {
            transform(digest, block, transformCount);
            for (size_t i = 0; i < cBlockInts - 2; i++)
                block[i] = 0;
            
        }
        
        /* Append total_bits, split this uint64_t into two uint32_t */
        block[cBlockInts - 1] = static_cast<uint32_t>(totalBits);
        block[cBlockInts - 2] = (totalBits >> 32);
        transform(digest, block, transformCount);
        
        const bool isLittleEndian = tools::isLittleEndian();
        const size_t resultSize = digest.size() * sizeof(uint32_t);
        char bytes[resultSize];
        
        for (size_t i = 0; i < digest.size(); i++)
        {
            if (pNetworkOrder && isLittleEndian)
                digest[i] = tools::byteSwap32(digest[i]);
            
            memcpy(bytes + i * sizeof(uint32_t), &digest[i], sizeof(uint32_t));
        }
        
        std::string result(bytes, resultSize);
        return result;
    }
    
    std::string getRandomCryptoBytes(size_t pLength)
    {
        // TODO: make proper crypto random generator
        std::string data(pLength, 0);
        
        std::independent_bits_engine<std::default_random_engine, CHAR_BIT, unsigned char> engine;
        engine.seed(std::chrono::high_resolution_clock::now().time_since_epoch().count());
        
        std::generate(data.begin(), data.end(), std::ref(engine));
        
        return data;
    }
    
    std::vector<unsigned char> cbc_encrypt(const unsigned char* pData, size_t pDataSize, const std::string& pKey, const std::string& pIV)
    {
        std::vector<unsigned char> result;
        const size_t blockLength = 16;
        
        if (pDataSize % blockLength == 0)
        {
            aes_encrypt_ctx context[1];
            aes_encrypt_key256(reinterpret_cast<const unsigned char*>(pKey.c_str()), context);
            
            auto currBuffer = pData;
            unsigned char outBuffer[blockLength] = {0};
            size_t offset = 0;
            
            for (size_t i = 0; i < blockLength; ++i)
                outBuffer[i] = static_cast<unsigned char>(pIV[i]);
            
            while (offset + blockLength <= pDataSize)
            {
                for (size_t i = 0; i < blockLength; ++i)
                    outBuffer[i] ^= currBuffer[i];
                
                aes_encrypt(outBuffer, outBuffer, context);
                
                offset += blockLength;
                currBuffer = pData + offset;
                result.insert(result.end(), outBuffer, outBuffer + blockLength);
            }
        }
        else
        {
            Hermes::getInstance()->getLogger()->print(ELogLevel::Warning, "CBC encryption: data size must be divisible by %", blockLength);
        }
        
        return result;
    }
    
    std::vector<unsigned char> cbc_decrypt(const unsigned char* pData, size_t pDataSize, const std::string pKey, const std::string pIV)
    {
        std::vector<unsigned char> result;
        const size_t blockLength = 16;
        
        if (pDataSize % blockLength == 0)
        {
            aes_decrypt_ctx context[1];
            aes_decrypt_key256(reinterpret_cast<const unsigned char*>(pKey.c_str()), context);
            
            auto prevBuffer = reinterpret_cast<const unsigned char*>(pIV.c_str());
            unsigned char outBuffer[blockLength] = {0};
            size_t offset = 0;
            result.reserve(pDataSize);
            
            while (offset + blockLength <= pDataSize)
            {
                aes_decrypt(pData + offset, outBuffer, context);
                
                for (size_t i = 0; i < blockLength; ++i)
                    outBuffer[i] ^= prevBuffer[i];
                
                prevBuffer = pData + offset;
                offset += blockLength;
                result.insert(result.end(), outBuffer, outBuffer + blockLength);
            }
        }
        else
        {
            Hermes::getInstance()->getLogger()->print(ELogLevel::Warning, "CBC decryption: data size must be divisible by %", blockLength);
        }
        
        return result;
    }
    
    std::vector<unsigned char> ofb_crypt(const unsigned char* pData, size_t pDataSize, const std::string pKey, const std::string pIV)
    {
        std::vector<unsigned char> result;
        const size_t blockLength = 16;
        
        if (pDataSize >= blockLength)
        {
            aes_encrypt_ctx context[1];
            aes_encrypt_key256(reinterpret_cast<const unsigned char*>(pKey.c_str()), context);
            
            unsigned char outBuffer[blockLength] = {0};
            unsigned char iv[blockLength];
            memcpy(iv, pIV.c_str(), blockLength);
            result.reserve(pDataSize);
            
            size_t offset = 0;
            size_t length = blockLength;
            
            while (true)
            {
                if (offset + blockLength >= pDataSize)
                    length = pDataSize - offset;
                
                aes_ofb_crypt(pData + offset, outBuffer, static_cast<int>(length), iv, context);
                offset += length;
                result.insert(result.end(), outBuffer, outBuffer + length);
                
                if (length != blockLength)
                    break;
            }
        }
        else
        {
            Hermes::getInstance()->getLogger()->print(ELogLevel::Warning, "OFB encryption: data size must be bigger than %.", blockLength);
        }
        
        return result;
    }
    
    std::vector<unsigned char> encrypt(const unsigned char* pData, size_t pDataSize, const std::string& pKey, const std::string& pIV, ECryptoMode pMode)
    {
        std::vector<unsigned char> result;
        const size_t blockLength = 16;
        
        if (pKey.size() == 32 && pIV.size() == 16 && pDataSize >= blockLength)
        {
            switch (pMode)
            {
                case ECryptoMode::AES_256_CBC:
                    result = cbc_encrypt(pData, pDataSize, pKey, pIV);
                    break;
                case ECryptoMode::AES_256_OFB:
                    result = ofb_crypt(pData, pDataSize, pKey, pIV);
                    break;
                default:
                    Hermes::getInstance()->getLogger()->print(ELogLevel::Warning, "Encryption mode not supported.");
                    break;
            }
        }
        else
        {
            Hermes::getInstance()->getLogger()->print(ELogLevel::Warning, "Invalid key/iv/data size. Encryption");
            assert(pDataSize == 0);
        }
        
        return result;
    }
    
    std::vector<unsigned char> decrypt(const unsigned char* pData, size_t pDataSize, const std::string& pKey, const std::string& pIV, ECryptoMode pMode)
    {
        std::vector<unsigned char> result;
        const size_t blockLength = 16;
        
        if (pKey.size() == 32 && pIV.size() == 16 && pDataSize >= blockLength)
        {
            switch (pMode)
            {
                case ECryptoMode::AES_256_CBC:
                    result = cbc_decrypt(pData, pDataSize, pKey, pIV);
                    break;
                case ECryptoMode::AES_256_OFB:
                    result = ofb_crypt(pData, pDataSize, pKey, pIV);
                    break;
                default:
                    Hermes::getInstance()->getLogger()->print(ELogLevel::Warning, "Decryption mode not supported.");
                    break;
            }
        }
        else
        {
            Hermes::getInstance()->getLogger()->print(ELogLevel::Warning, "Invalid key/iv/data size. Decryption");
            assert(pDataSize == 0);
        }
        
        return result;
    }
    
    std::string encrypt(const std::string& pData, const std::string& pKey, const std::string& pIV, ECryptoMode pMode)
    {
        auto result = encrypt(reinterpret_cast<const unsigned char*>(pData.data()), pData.size(), pKey, pIV, pMode);
        return std::string(reinterpret_cast<char*>(result.data()), result.size());
    }
    
    DataBuffer encrypt(const DataBuffer& pData, const std::string& pKey, const std::string& pIV, ECryptoMode pMode)
    {
        auto result = encrypt(reinterpret_cast<const unsigned char*>(pData.data()), pData.size(), pKey, pIV, pMode);
        DataBuffer buffer;
        buffer.push_back(result.data(), result.size());
        return buffer;
    }
    
    std::string decrypt(const std::string& pData, const std::string& pKey, const std::string& pIV, ECryptoMode pMode)
    {
        auto result = decrypt(reinterpret_cast<const unsigned char*>(pData.data()), pData.size(), pKey, pIV, pMode);
        return std::string(reinterpret_cast<char*>(result.data()), result.size());
    }
    
    DataBuffer decrypt(const DataBuffer& pData, const std::string& pKey, const std::string& pIV, ECryptoMode pMode)
    {
        auto result = decrypt(reinterpret_cast<const unsigned char*>(pData.data()), pData.size(), pKey, pIV, pMode);
        DataBuffer buffer;
        buffer.push_back(result.data(), result.size());
        return buffer;
    }
}
}
