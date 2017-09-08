// Copyright (C) 2017 Grupa Pracuj Sp. z o.o.
// This file is part of the "Hermes" library.
// For conditions of distribution and use, see copyright notice in license.txt.

#ifndef _NETWORK_HPP_
#define _NETWORK_HPP_

#include "data.hpp"

#include <string>
#include <vector>
#include <array>
#include <functional>
#include <queue>
#include <utility>
#include <atomic>
#include <mutex>
#include <unordered_map>
#include <thread>
#include <random>
#include <cstdint>

struct curl_slist;

namespace hms
{
    
    namespace tools
    {
        class URLTool;
    }

    enum class ENetworkCode : int
    {
        OK = 0,
        InvalidHttpCodeRange,
        LostConnection,
        Timeout,
        Unknown = 100
    };

    enum class ENetworkRequestType : int
    {
        Delete = 0,
        Get,
        Post,
        Put,
        Head
    };
        
    enum class ENetworkResponseType : int
    {
        Message = 0,
        RawData
    };
        
    enum class ESocketDisconnectCause : int
    {
        User = 0,
        InitialConnection,
        Header,
        Handshake,
        Timeout,
        Close,
        Other
    };
        
    enum class ESocketOpCode : uint8_t
    {
        Continue = 0,
        Text = 0x1,
        Binary = 0x2,
        // 3-7 reserved.
        Close = 0x8,
        Ping = 0x9,
        Pong = 0xA,
        // B-F reserved.
    };
        
    enum class ENetworkFlag : size_t
    {
        TimeoutInMilliseconds = 0,
        Redirect,
        DisableSSLVerifyPeer,
        Count
    };
    
    struct NetworkResponse
    {
        ENetworkCode mCode = ENetworkCode::Unknown;
        int mHttpCode = -1;
        std::vector<std::pair<std::string, std::string>> mHeader;
        std::string mMessage;
        DataBuffer mRawData;
    };
        
    struct NetworkRequestParam
    {
        ENetworkRequestType mRequestType = ENetworkRequestType::Get;
        ENetworkResponseType mResponseType = ENetworkResponseType::Message;
        std::string mMethod;
        std::vector<std::pair<std::string, std::string>> mParameter = {};
        std::vector<std::pair<std::string, std::string>> mHeader = {};
        std::string mRequestBody;
        std::function<void(NetworkResponse pResponse)> mCallback = nullptr;
        std::function<void(const long long& pDN, const long long& pDT, const long long& pUN, const long long& pUT)> mProgress = nullptr;
        unsigned mRepeatCount = 0;
        bool mAllowRecovery = true;        
        bool mAllowCache = false;
        u_int32_t mCacheLifetime = 8640000;
    };
        
    struct SimpleSocketRequestParam
    {
        std::string mURL;
        std::vector<std::pair<std::string, std::string>> mHeader = {};
        std::chrono::milliseconds mTimeout = std::chrono::milliseconds(60000);
        std::function<void()> mConnectCallback;
        std::function<void(std::string lpMessage, bool lpTextData)> mMessageCallback;
        std::function<void(ESocketDisconnectCause lpCause, std::chrono::milliseconds lpMiliseconds)> mDisconnectCallback;
    };
        
    class NetworkRecovery
    {
    public:
        bool init(std::function<bool(const NetworkResponse& pResponse, std::string& pRequestBody)> pPreCallback, std::function<void(NetworkResponse pResponse)> pPostCallback,
            ENetworkRequestType pType, std::string pUrl, std::vector<std::pair<std::string, std::string>> pParameter, std::vector<std::pair<std::string, std::string>> pHeader,
            unsigned pRepeatCount = 0);
            
        bool isInitialized() const;
        bool isQueueEmpty() const;
            
        bool checkCondition(const NetworkResponse& pResponse, std::string& pRequestBody) const;
        void pushReceiver(std::tuple<size_t, NetworkRequestParam> pReceiver);
        void runRequest(std::string pRequestBody);
        
        bool isLocked() const;
        bool lock();
        bool unlock();
        
        size_t getId() const;
        const std::string& getName() const;
        
        const std::vector<std::pair<std::string, std::string>>& getHeader() const;
		void setHeader(const std::vector<std::pair<std::string, std::string>>& pHeader);
        
        const std::vector<std::pair<std::string, std::string>>& getParameter() const;
		void setParameter(const std::vector<std::pair<std::string, std::string>>& pParameter);
        
    private:
        friend class NetworkManager;
        
        NetworkRecovery(size_t pId, const std::string& pName);
        NetworkRecovery(const NetworkRecovery& pOther) = delete;
        NetworkRecovery(NetworkRecovery&& pOther) = delete;
        ~NetworkRecovery();
        
        NetworkRecovery& operator=(const NetworkRecovery& pOther) = delete;
        NetworkRecovery& operator=(NetworkRecovery&& pOther) = delete;
        
        std::function<bool(const NetworkResponse& pResponse, std::string& pRequestBody)> mPreCallback = nullptr;
        NetworkRequestParam mRequestParam;
        std::queue<std::tuple<size_t, NetworkRequestParam>> mReceiver;
            
        bool mUnsafeLock = false;
        
        size_t mId = 0;
        std::string mName;
        
        std::atomic<uint32_t> mInitialized {0};
    };
        
    //! Network API.
    class NetworkAPI
    {
    public:
        //! Call a request.
        /** This method is used to send a HTTP request.
         \param pParam Parameters of the request. */
        void request(NetworkRequestParam pParam);
        
        //! Call a request.
        /** This is an alternative method for a HTTP request.
         \param pRequestType Type of the request.
         \param pMethod Endpoint of the request.
         \param pRequestBody The request body.
         \param pCallback Callback with a response from the server. */
        void request(ENetworkRequestType pRequestType, std::string pMethod, std::string pRequestBody, std::function<void(NetworkResponse pResponse)> pCallback)
        {
            NetworkRequestParam param;
            
            param.mRequestType = pRequestType;
            param.mMethod = std::move(pMethod);
            param.mRequestBody = std::move(pRequestBody);
            param.mCallback = std::move(pCallback);
            
            request(std::move(param));
        }

        //! Get a list of default headers.
        /** This method is used most of the time by internal methods, however if you want to retrieve default headers for this API you may use it.
         \return List of strings with headers. */
		const std::vector<std::pair<std::string, std::string>>& getDefaultHeader() const;
        
        //! Set a new list of default headers.
        /** Provided headers will be added to each request called through this API.
         \param pDefaultHeader List of headers, simple element may look like this: "{"Content-Type", "application/json; charset=UTF-8"}". */
		void setDefaultHeader(std::vector<std::pair<std::string, std::string>> pDefaultHeader);
        
        //! Get a global URL of this API.
        /** This method returns URL provided for this API at initialization process.
         \return The URL. */
		const std::string& getURL() const;
        
        //! Register a post request callback.
        /** This feature is useful when you want to do some cyclic operation after a request call.
         \param pCallback Method which will be executed after a request.
         \param pName Identificator of this callback.
         \param pOverwrite If set to True and a callback with requested identificator already existing an old callback will be overwritten.
         \return True if callback with requested identificator was registered properly otherwise False. */
		bool registerCallback(std::function<void(const NetworkRequestParam& pRequest, const NetworkResponse& pResponse)> pCallback, std::string pName, bool pOverwrite);
        
        //! Register a post request advanced callback.
        /** This feature is useful when you want to do some cyclic operation after a request call.
         \param pCondition condition which must pass to execute a callback.
         \param pCallback Method which will be executed after a request.
         \param pName Identificator of this callback.
         \param pOverwrite If set to True and a callback with requested identificator already existing an old callback will be overwritten.
         \return True if callback with requested identificator was registered properly otherwise False. */
        bool registerAdvancedCallback(std::function<bool(const NetworkResponse& pResponse)> pCondition,
            std::function<void(const NetworkRequestParam& pRequest, const NetworkResponse& pResponse)> pCallback, std::string pName, bool pOverwrite);
        
        //! Unregister a post request callback.
        /** Use this method if you want to unregister previously registered callback.
         \param pName identificator of the callback.
         \return True if callback with requested pName was unregistered properly otherwise False. */
		bool unregisterCallback(std::string pName);
        
        //! Get the recovery of the Network API.
		/** \return The pointer to a NetworkRecovery object. */
		std::weak_ptr<NetworkRecovery> getRecovery() const;
        
        //! Set the recovery for the Network API.
		/** \param pRecovery The pointer to a NetworkRecovery object. */
        void setRecovery(std::weak_ptr<NetworkRecovery> pRecovery);
        
        //! Get the Id of the Network API.
		/** \return The Id. */
        size_t getId() const;
        
        //! Get the Name of the Network API.
		/** \return The Name. */
        const std::string& getName() const;
        
    private:
        friend class NetworkManager;

        NetworkAPI(size_t pId, const std::string& pName, const std::string& pURL);
        NetworkAPI(const NetworkAPI& pOther) = delete;
        NetworkAPI(NetworkAPI&& pOther) = delete;
        ~NetworkAPI();
        
        NetworkAPI& operator=(const NetworkAPI& pOther) = delete;
        NetworkAPI& operator=(NetworkAPI&& pOther) = delete;
        
        size_t mId = 0;
        std::string mName;
        std::string mURL;
        
        std::vector<std::pair<std::string, std::string>> mDefaultHeader;
        std::shared_ptr<std::vector<std::tuple<std::function<bool(const NetworkResponse&)>, std::function<void(const NetworkRequestParam&, const NetworkResponse&)>, std::string>>> mCallback;

        std::weak_ptr<NetworkRecovery> mRecovery;
    };

    class NetworkManager
    {
    public:
        struct ProgressData
        {
            std::function<void(const long long& lpDN, const long long& lpDT, const long long& lpUN, const long long& lpUT)> mProgressTask = nullptr;
            std::atomic<uint32_t>* mTerminateAbort = nullptr;
        };
    
		bool initialize(long pTimeout, int pThreadPoolID, std::pair<int, int> pHttpCodeSuccess = {200, 299});
		bool terminate();
        
		std::shared_ptr<NetworkAPI> add(const std::string& pName, const std::string& pURL, size_t pUniqueID);
		size_t count() const;
		std::shared_ptr<NetworkAPI> get(size_t pID) const;
        
        std::shared_ptr<NetworkRecovery> addRecovery(const std::string& pName, size_t pUniqueID);
        size_t countRecovery() const;
        std::shared_ptr<NetworkRecovery> getRecovery(size_t pID) const;

		void request(NetworkRequestParam pParam);
        void request(std::vector<NetworkRequestParam> pParam);
        
		long getTimeout() const;
		void setTimeout(long pTimeout);
        
        bool getFlag(ENetworkFlag pFlag) const;
        void setFlag(ENetworkFlag pFlag, bool pState);
        
        long getProgressTimePeriod() const;
        void setProgressTimePeriod(long pTimePeriod);
        
        std::string getCACertificatePath() const;
        void setCACertificatePath(std::string pPath);
        
        void appendParameter(std::string& pURL, const std::vector<std::pair<std::string, std::string>>& pParameter) const;
        void decodeURL(std::string& pData) const;
        void encodeURL(std::string& pData) const;
        
        bool initCache(const std::string& pDirectoryPath, unsigned pFileCountLimit, unsigned pFileSizeLimit);
        void clearCache();
        
        bool initSimpleSocket(int pThreadPoolID);
        void requestSimpleSocket(SimpleSocketRequestParam pRequest);
        void sendSimpleSocketMessage(const std::string& pMessage, std::function<void(ENetworkCode)> pCallback);
        void closeSimpleSocket();
        
    private:
        friend class Hermes;

        struct MultiRequestData
        {
            std::vector<std::pair<std::string, std::string>> mResponseHeader;
            std::string mResponseMessage;
            DataBuffer mResponseRawData;
            ProgressData mProgressData;
            void* mHandle = nullptr;
            char* mErrorBuffer = nullptr;
            NetworkRequestParam* mParam = nullptr;
        };
        
        struct CacheFileData
        {
            struct Header {
                u_int64_t mTimestamp = 0;
                u_int32_t mLifetime = 0;
                u_int16_t mFlagsAndSize = 0;
            } mHeader;
            
            bool mStringType = false;
            bool mIsValid = false;
            
            std::string mFullFilePath;
            std::string mUrl;
        };
        
        struct SocketFrame
        {
            uint8_t mCode = 0;
            uint8_t mPayloadLength = 0;
            uint64_t mPayloadLengthExtended = 0;
            uint8_t mMaskingKey[4];
            
            std::string mPayload;
        };
    
        NetworkManager();
        NetworkManager(const NetworkManager& pOther) = delete;
        NetworkManager(NetworkManager&& pOther) = delete;
        ~NetworkManager();
        
        NetworkManager& operator=(const NetworkManager& pOther) = delete;
        NetworkManager& operator=(NetworkManager&& pOther) = delete;
        
        void deleterNetworkAPI(NetworkAPI* pObject);
        void deleterNetworkRecovery(NetworkRecovery* pObject);
        
        std::vector<std::pair<std::string, std::string>> createUniqueHeader(const std::vector<std::pair<std::string, std::string>>& pHeader) const;

        void configureHandle(void* pHandle, ENetworkRequestType pRequestType, ENetworkResponseType pResponseType, const std::string& pRequestUrl, const std::string& pRequestBody,
            std::string* pResponseMessage, DataBuffer* pResponseRawData, std::vector<std::pair<std::string, std::string>>* pResponseHeader, curl_slist* pHeader,
            long pTimeout, std::array<bool, static_cast<size_t>(ENetworkFlag::Count)> pFlag, ProgressData* pProgressData, char* pErrorBuffer, const std::string pCACertificatePath) const;
        
        void resetHandle(void* pHandle) const;
        
        CacheFileData decodeCacheHeader(const std::string& pFilePath) const;
        NetworkResponse getResponseFromCache(const std::string& pUrl);
        bool cacheResponse(const NetworkResponse& pResponse, const std::string& pUrl, u_int32_t pLifetime);
        
        int sendUpgradeHeader(void* const pCurl, const tools::URLTool& pURL, const std::vector<std::pair<std::string, std::string>>& pHeader, std::string& pSecAccept) const;
        std::string packSimpleSocketMessage(const std::string& pMessage, ESocketOpCode pOpCode = ESocketOpCode::Text) const;
        std::vector<SocketFrame> unpackSimpleSocketMessage(const std::string& pMessage) const;
        bool handleSimpleSocketFrame(void* const pCurl, std::vector<SocketFrame>& pNewFrame, std::vector<SocketFrame>& pOldFrame, std::function<void(std::string lpMessage, bool lpTextData)> pMessageCallback) const;
        
        std::vector<std::shared_ptr<NetworkAPI>> mAPI;
        std::vector<std::shared_ptr<NetworkRecovery>> mRecovery;
        std::vector<CacheFileData> mCacheFileInfo;
        std::unordered_map<std::string, size_t> mCacheFileIndex;

        std::unordered_map<std::thread::id, void*> mHandle;
        std::unordered_map<std::thread::id, void*> mMultiHandle;
		std::mutex mHandleMutex;
        std::mutex mMultiHandleMutex;
        void* mSimpleSocketCURL = nullptr;

        long mTimeout = 0;
        int mThreadPoolID = -1;
        int mThreadPoolSimpleSocketID = -1;
        std::pair<int, int> mHttpCodeSuccess = {200, 299};
        std::string mCACertificatePath;
        std::array<bool, static_cast<size_t>(ENetworkFlag::Count)> mFlag = {false, false, false};
        
        std::atomic<uint32_t> mInitialized {0};
        std::atomic<uint32_t> mCacheInitialized {0};
        std::atomic<uint32_t> mSimpleSocketInitialized {0};
        std::atomic<uint32_t> mStopSimpleSocketLoop {0};
        std::atomic<uint32_t> mSimpleSocketActive {0};
        std::atomic<uint32_t> mTerminateAbort {0};
        std::atomic<uint32_t> mActivityCount {0};
        
        unsigned mCacheFileCountLimit = 0;
        unsigned mCacheFileSizeLimit = 0;
        std::mutex mCacheMutex;
        std::string mCacheDirectoryPath;
        const char mCacheMagicWord[5] = { 'Y', 'G', 'G', '_', 'H'};
        
        long mProgressTimePeriod = 0;
        
        std::mt19937 mRandomGenerator;
        const long long mSocketWaitTimeout = 1000;
    };

}

#endif
