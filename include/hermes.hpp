// Copyright (C) 2017 Grupa Pracuj Sp. z o.o.
// This file is part of the "Hermes" library.
// For conditions of distribution and use, see copyright notice in license.txt.

#ifndef _HERMES_HPP_
#define _HERMES_HPP_

#include "data.hpp"
#include "network.hpp"
#include "task.hpp"
#include "logger.hpp"

#include <array>

namespace hms
{

    namespace tools
    {
        std::string encodeBase64(unsigned char const* bytes_to_encode, size_t in_len);
        std::string decodeBase64(std::string const& encoded_string);
        std::string getRandomCryptoBytes(size_t pLength);
        std::string getSHA1Digest(const std::string& pData, bool pNetworkOrder = true);
        
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
            ~URLTool() { mProtocol = mHost = mPath = nullptr; }
            
            const std::string& getURL() const;
            const char* getProtocol(size_t& pLength) const;
            const char* getHost(size_t& pLength, bool pIncludePort = false) const;
            const char* getPath(size_t& pLength) const;
            uint16_t getPort() const;
            bool isSecure() const;
            
            std::string getHttpURL(bool pIncludePath = true) const;
            
        private:
            URLTool() = delete;
            URLTool(URLTool&& pTool) = delete;
            
            std::string mURL;
            const char* mProtocol = nullptr;
            const char* mHost = nullptr;
            const char* mPath = nullptr;
            uint16_t mPort = 0;
            size_t mProtocolLength = 0;
            size_t mHostLength = 0;
            size_t mPathLength = 0;
            size_t mPortLength = 0;
            bool mSecure = false;
        };
    }

    class Hermes
    {
    public:
        DataManager* getDataManager() const;
        NetworkManager* getNetworkManager() const;
        TaskManager* getTaskManager() const;
        Logger* getLogger() const;
        
        void getVersion(unsigned& pMajor, unsigned& pMinor, unsigned& pPatch) const;

		static Hermes* getInstance();
        
    private:
        Hermes();
        Hermes(const Hermes& pOther) = delete;
        Hermes(Hermes&& pOther) = delete;
        ~Hermes();
        
        Hermes& operator=(const Hermes& pOther) = delete;
        Hermes& operator=(Hermes&& pOther) = delete;
        
        std::array<unsigned, 3> mVersion;
        
        DataManager* mDataManager = nullptr;
        NetworkManager* mNetworkManager = nullptr;
        TaskManager* mTaskManager = nullptr;
        Logger* mLogger = nullptr;
    };

}

#endif
