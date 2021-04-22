// Copyright (C) 2017-2021 Grupa Pracuj Sp. z o.o.
// This file is part of the "Hermes" library.
// For conditions of distribution and use, see copyright notice in license.txt.

#ifndef _NETWORK_HPP_
#define _NETWORK_HPP_

#include "hmsData.hpp"

#include <array>
#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <random>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

struct curl_slist;

namespace hms
{
    namespace tools
    {
        class URLTool;
    }
    
    class NetworkManager;

    enum class ENetworkCertificate : int32_t
    {
        None = 0,
        Path,
        Content
    };

    enum class ENetworkCode : int32_t
    {
        OK = 0,
        InvalidHttpCodeRange,
        LostConnection,
        Timeout,
        Cancel,
        Unknown = 100
    };

    enum class ENetworkRequest : int32_t
    {
        Delete = 0,
        Get,
        Post,
        Put,
        Head
    };
        
    enum class ENetworkResponse : int32_t
    {
        Message = 0,
        RawData
    };
        
    enum class ENetworkWebSocketDisconnect : int32_t
    {
        User = 0,
        InitialConnection,
        Header,
        Handshake,
        Timeout,
        Close,
        Other
    };
        
    enum class ENetworkWebSocketOpCode : uint8_t
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

    class NetworkResponseDataTaskBackground
    {
    public:
        virtual ~NetworkResponseDataTaskBackground() = default;
    };

    class NetworkResponse
    {
    public:
        ENetworkCode mCode = ENetworkCode::Unknown;
        int32_t mHttpCode = -1;
        std::vector<std::pair<std::string, std::string>> mHeader;
        std::string mMessage;
        DataBuffer mRawData;
        std::unique_ptr<NetworkResponseDataTaskBackground> mDataTaskBackground;
        std::string mMethod;
    };

    class NetworkRequest
    {
    public:
        ENetworkRequest mRequestType = ENetworkRequest::Get;
        ENetworkResponse mResponseType = ENetworkResponse::Message;
        std::string mMethod;
        std::vector<std::pair<std::string, std::string>> mParameter = {};
        std::vector<std::pair<std::string, std::string>> mHeader = {};
        std::string mRequestBody;
        std::function<void(NetworkResponse)> mCallback;
        std::function<std::unique_ptr<NetworkResponseDataTaskBackground>(const NetworkResponse&)> mTaskBackground;
        std::function<void(int64_t, int64_t, int64_t, int64_t)> mProgress;
        uint32_t mRepeatCount = 0;
        bool mAllowRecovery = true;        
        bool mAllowCache = false;
        uint32_t mCacheLifetime = 8640000;
    };
        
    class NetworkWebSocket
    {
    public:
        std::string mUrl;
        std::vector<std::pair<std::string, std::string>> mParameter = {};
        std::vector<std::pair<std::string, std::string>> mHeader = {};
        std::chrono::milliseconds mTimeout = std::chrono::milliseconds(60000);
        std::function<void()> mConnectCallback;
        std::function<void(ENetworkWebSocketDisconnect lpDisconnect, std::chrono::milliseconds lpMiliseconds)> mDisconnectCallback;
        std::function<void(std::string lpMessage, bool lpTextData)> mMessageCallback;
    };
        
    class NetworkRequestHandle
    {
    public:
        NetworkRequestHandle() = default;
        NetworkRequestHandle(const NetworkRequestHandle& pOther) = delete;
        NetworkRequestHandle(NetworkRequestHandle&& pOther) = delete;
        
        NetworkRequestHandle& operator=(const NetworkRequestHandle& pOther) = delete;
        NetworkRequestHandle& operator=(NetworkRequestHandle&& pOther) = delete;
        
        void cancel();
        bool isCancel() const;
        
    private:
        std::atomic<uint32_t> mCancel = {0};
    };
    
    class NetworkWebSocketHandle
    {
    public:
        void sendMessage(const std::string& pMessage, std::function<void(ENetworkCode)> pCallback);

    private:
        friend NetworkManager;
        
        class ControlBlock
        {
        public:
            ~ControlBlock();
        
            ENetworkWebSocketDisconnect mDisconnect = ENetworkWebSocketDisconnect::User;
            std::pair<std::chrono::time_point<std::chrono::steady_clock>, std::chrono::time_point<std::chrono::steady_clock>> mTime = {};
            void* mHandle = nullptr;
            bool mInitialized = false;
            std::atomic<uint32_t> mTerminate = {0};
        };
    
        class Frame
        {
        public:
            uint8_t mCode = 0;
            uint8_t mPayloadLength = 0;
            uint64_t mPayloadLengthExtended = 0;
            uint8_t mMaskingKey[4] = {0};
            std::string mPayload;
        };

        NetworkWebSocketHandle() = default;
        NetworkWebSocketHandle(const NetworkWebSocketHandle& pOther) = delete;
        NetworkWebSocketHandle(NetworkWebSocketHandle&& pOther) = delete;
        ~NetworkWebSocketHandle();
        
        NetworkWebSocketHandle& operator=(const NetworkWebSocketHandle& pOther) = delete;
        NetworkWebSocketHandle& operator=(NetworkWebSocketHandle&& pOther) = delete;
        
        void initialize(int32_t pThreadPoolId, NetworkWebSocket pParam, NetworkManager* pNetworkManager);
    
        int32_t sendUpgradeHeader(const tools::URLTool& pUrl, const std::vector<std::pair<std::string, std::string>>& pHeader, std::string& pSecAccept) const;
        std::string packMessage(const std::string& pMessage, ENetworkWebSocketOpCode pOpCode = ENetworkWebSocketOpCode::Text) const;
        std::vector<Frame> unpackMessage(const std::string& pMessage) const;
        bool handleFrame(std::vector<Frame>& pNewFrame, std::vector<Frame>& pOldFrame, std::function<void(std::string lpMessage, bool lpTextData)> pMessageCallback) const;
    
        std::string mSecWebSocketAccept;
        std::vector<Frame> mFrames;
        bool mHeaderCheck = true;
        std::pair<std::queue<std::function<void()>>, std::mutex> mMessage = {};
        
        std::shared_ptr<ControlBlock> mControlBlock;

        std::weak_ptr<NetworkWebSocketHandle> mWeakThis;
    };
    
    class NetworkRecovery
    {
    public:
        class Config
        {
        public:
            std::function<bool(const NetworkResponse& lpResponse, std::string&, std::vector<std::pair<std::string, std::string>>&, std::vector<std::pair<std::string, std::string>>&)> mCondition;
            ENetworkRequest mRequestType = ENetworkRequest::Post;
            std::string mUrl;
            std::vector<std::pair<std::string, std::string>> mParameter;
            std::vector<std::pair<std::string, std::string>> mHeader;
            std::function<void(NetworkResponse)> mCallback;
            std::function<std::unique_ptr<NetworkResponseDataTaskBackground>(const NetworkResponse&)> mTaskBackground;
            uint32_t mRepeatCount = 0;
        };

        bool isQueueEmpty() const;
            
        bool checkCondition(const NetworkResponse& pResponse, std::string& pRequestBody, std::vector<std::pair<std::string, std::string>>& pParameter, std::vector<std::pair<std::string, std::string>>& pHeader) const;
        void pushReceiver(std::tuple<size_t, NetworkRequest, std::shared_ptr<NetworkRequestHandle>> pReceiver);
        void runRequest(std::string pRequestBody, std::vector<std::pair<std::string, std::string>> pParameter, std::vector<std::pair<std::string, std::string>> pHeader);
        
        bool isActive() const;
        bool activate();

        const std::vector<std::pair<std::string, std::string>>& getHeader() const;
        void setHeader(const std::vector<std::pair<std::string, std::string>>& pHeader);
        
        const std::vector<std::pair<std::string, std::string>>& getParameter() const;
        void setParameter(const std::vector<std::pair<std::string, std::string>>& pParameter);

        size_t getId() const;
        const std::string& getName() const;
        
    private:
        friend class NetworkManager;
        
        class InternalData
        {
        public:
            std::queue<std::tuple<size_t, NetworkRequest, std::shared_ptr<NetworkRequestHandle>>> mReceiver;
            bool mActive = false;
        };
        
        NetworkRecovery(size_t pId, std::string pName, Config pConfig);
        NetworkRecovery(const NetworkRecovery& pOther) = delete;
        NetworkRecovery(NetworkRecovery&& pOther) = delete;
        ~NetworkRecovery();
        
        NetworkRecovery& operator=(const NetworkRecovery& pOther) = delete;
        NetworkRecovery& operator=(NetworkRecovery&& pOther) = delete;

        size_t mId = 0;
        std::string mName;

        NetworkRequest mRequestParam;
        std::function<bool(const NetworkResponse& lpResponse, std::string&, std::vector<std::pair<std::string, std::string>>&, std::vector<std::pair<std::string, std::string>>&)> mCondition;
        std::shared_ptr<InternalData> mInternalData;
    };
        
    //! Network API.
    class NetworkAPI
    {
    public:
        //! Call a request.
        /** This method is used to send a HTTP request.
         \param pParam Parameters of the request. */
        virtual std::shared_ptr<NetworkRequestHandle> request(NetworkRequest pParam);
        
        //! Call a request.
        /** This is an alternative method for a HTTP request.
         \param pRequestType Type of the request.
         \param pMethod Endpoint of the request.
         \param pRequestBody The request body.
         \param pCallback Callback with a response from the server. */
        std::shared_ptr<NetworkRequestHandle> request(ENetworkRequest pRequestType, std::string pMethod, std::string pRequestBody, std::function<void(NetworkResponse)> pCallback)
        {
            NetworkRequest param;
            
            param.mRequestType = pRequestType;
            param.mMethod = std::move(pMethod);
            param.mRequestBody = std::move(pRequestBody);
            param.mCallback = std::move(pCallback);
            
            return request(std::move(param));
        }

        //! Get a list of default headers.
        /** This method is used most of the time by internal methods, however if you want to retrieve default headers for this API you may use it.
         \return List of strings with headers. */
        const std::vector<std::pair<std::string, std::string>>& getDefaultHeader() const;
        
        //! Set a new list of default headers.
        /** Provided headers will be added to each request called through this API.
         \param pDefaultHeader List of headers, simple element may look like this: "{"Content-Type", "application/json; charset=UTF-8"}". */
        void setDefaultHeader(std::vector<std::pair<std::string, std::string>> pDefaultHeader);
        
        //! Register a post request callback.
        /** This feature is useful when you want to do some cyclic operation after a request call.
         \param pCallback Method which will be executed after a request.
         \param pName Identificator of this callback.
         \param pOverwrite If set to True and a callback with requested identificator already existing an old callback will be overwritten.
         \return True if callback with requested identificator was registered properly otherwise False. */
        bool registerCallback(std::function<void(const NetworkResponse&)> pCallback, std::string pName, bool pOverwrite);
        
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

        //! Get a global URL of this API.
        /** This method returns URL provided for this API at initialization process.
         \return The URL. */
        const std::string& getUrl() const;
        
    protected:
        NetworkAPI(size_t pId, std::string pName, std::string pUrl);
        NetworkAPI(const NetworkAPI& pOther) = delete;
        NetworkAPI(NetworkAPI&& pOther) = delete;
        virtual ~NetworkAPI();
        
        NetworkAPI& operator=(const NetworkAPI& pOther) = delete;
        NetworkAPI& operator=(NetworkAPI&& pOther) = delete;
        
    private:
        friend class NetworkManager;

        size_t mId = 0;
        std::string mName;
        std::string mUrl;

        std::vector<std::pair<std::string, std::string>> mDefaultHeader;
        std::shared_ptr<std::vector<std::pair<std::function<void(const NetworkResponse&)>, std::string>>> mCallbackContinious;

        std::weak_ptr<NetworkRecovery> mRecovery;
    };

    class NetworkManager : public std::enable_shared_from_this<NetworkManager>
    {
    public:
        struct ProgressData
        {
            std::function<void(int64_t lpDN, int64_t lpDT, int64_t lpUN, int64_t lpUT)> mProgressTask = nullptr;
            std::shared_ptr<NetworkRequestHandle> mRequestHandle;
            std::atomic<uint32_t>* mTerminateAbort = nullptr;
        };
    
        bool initialize(int64_t pTimeout, int32_t pThreadPoolId, int32_t pWebSocketThreadPoolId, std::pair<ENetworkCertificate /* type */, std::string /* file path or content */> pCertificate);
        bool terminate();

        std::shared_ptr<NetworkAPI> add(size_t pId, std::string pName, std::string pUrl);
        template <typename T, typename = typename std::enable_if<std::is_base_of<NetworkAPI, T>::value>::type, typename... U>
        std::shared_ptr<T> add(size_t pId, std::string pName, std::string pUrl, U&&... pArgument)
        {
            std::shared_ptr<T> result = nullptr;
            if (mInitialized.load() == 2)
            {
                result = std::shared_ptr<T>(new T(pId, std::move(pName), std::move(pUrl), std::forward<U>(pArgument)...), std::bind(&NetworkManager::deleterNetworkAPI, std::placeholders::_1));
            
                std::lock_guard<std::mutex> lock(mApiMutex);
                if (pId >= mApi.size())
                    mApi.resize(pId + 1);

                mApi[pId] = result;
            }
            
            return result;
        }
        size_t count() const;
        std::shared_ptr<NetworkAPI> get(size_t pId) const;
        
        std::shared_ptr<NetworkRecovery> addRecovery(size_t pId, std::string pName, NetworkRecovery::Config pConfig);
        size_t countRecovery() const;
        std::shared_ptr<NetworkRecovery> getRecovery(size_t pId) const;

        void request(NetworkRequest pParam, std::shared_ptr<NetworkRequestHandle> pRequestHandle = nullptr);
        void request(std::vector<NetworkRequest> pParam, std::vector<std::shared_ptr<NetworkRequestHandle>> pRequestHandle = {});
        
        int64_t getTimeout() const;
        void setTimeout(int64_t pTimeout);
        
        bool getFlag(ENetworkFlag pFlag) const;
        void setFlag(ENetworkFlag pFlag, bool pState);
        
        int64_t getProgressTimePeriod() const;
        void setProgressTimePeriod(int64_t pTimePeriod);
        
        void appendParameter(std::string& pURL, const std::vector<std::pair<std::string, std::string>>& pParameter) const;
        void decodeURL(std::string& pData) const;
        void encodeURL(std::string& pData) const;
        
        bool initCache(const std::string& pDirectoryPath, unsigned pFileCountLimit, unsigned pFileSizeLimit);
        void clearCache();
        
        std::shared_ptr<NetworkWebSocketHandle> connectWebSocket(NetworkWebSocket pParam);
        
    private:
        friend class Hermes;
        friend NetworkWebSocketHandle;

        class Certificate;
        
        class CacheFileData
        {
        public:
            class Header
            {
            public:
                uint64_t mTimestamp = 0;
                uint32_t mLifetime = 0;
                uint16_t mFlagsAndSize = 0;
            };

            Header mHeader;
            
            bool mStringType = false;
            bool mIsValid = false;
            
            std::string mFullFilePath;
            std::string mUrl;
        };

        class MultiRequestData
        {
        public:
            std::vector<std::pair<std::string, std::string>> mResponseHeader;
            std::string mResponseMessage;
            DataBuffer mResponseRawData;
            ProgressData mProgressData;
            void* mHandle = nullptr;
            std::vector<char> mErrorBuffer;
            NetworkRequest* mParam = nullptr;
        };
        
        class RequestSettings
        {
        public:
            std::array<bool, static_cast<size_t>(ENetworkFlag::Count)> mFlag = {false, false, false};
            int64_t mTimeout = 0;
            int64_t mProgressTimePeriod = 0;
        };
    
        NetworkManager();
        NetworkManager(const NetworkManager& pOther) = delete;
        NetworkManager(NetworkManager&& pOther) = delete;
        ~NetworkManager();
        
        NetworkManager& operator=(const NetworkManager& pOther) = delete;
        NetworkManager& operator=(NetworkManager&& pOther) = delete;
        
        static void deleterNetworkAPI(NetworkAPI* pObject);
        static void deleterNetworkRecovery(NetworkRecovery* pObject);
        
        std::vector<std::pair<std::string, std::string>> createUniqueHeader(const std::vector<std::pair<std::string, std::string>>& pHeader) const;

        void configureHandle(void* pHandle, ENetworkRequest pRequestType, ENetworkResponse pResponseType, const std::string& pRequestUrl, const std::string& pRequestBody, std::string* pResponseMessage, DataBuffer* pResponseRawData, std::vector<std::pair<std::string, std::string>>* pResponseHeader, curl_slist* pHeader, int64_t pTimeout, std::array<bool, static_cast<size_t>(ENetworkFlag::Count)> pFlag, ProgressData* pProgressData, char* pErrorBuffer) const;
        
        void resetHandle(void* pHandle) const;
        
        CacheFileData decodeCacheHeader(const std::string& pFilePath) const;
        NetworkResponse getResponseFromCache(const std::string& pUrl);
        bool cacheResponse(const NetworkResponse& pResponse, const std::string& pUrl, u_int32_t pLifetime);

        std::weak_ptr<NetworkManager> mWeakThis;
        
        std::vector<std::shared_ptr<NetworkAPI>> mApi;
        mutable std::mutex mApiMutex;
        std::vector<std::shared_ptr<NetworkRecovery>> mRecovery;
        mutable std::mutex mRecoveryMutex;
        std::vector<CacheFileData> mCacheFileInfo;
        std::unordered_map<std::string, size_t> mCacheFileIndex;

        std::unordered_map<std::thread::id, void*> mHandle;
        std::mutex mHandleMutex;
        std::unordered_map<std::thread::id, void*> mMultiHandle;
        std::mutex mMultiHandleMutex;

        int32_t mThreadPoolId = -1;
        int32_t mWebSocketThreadPoolId = -1;
        std::shared_ptr<Certificate> mCertificate;

        std::atomic<uint32_t> mInitialized {0};
        std::atomic<uint32_t> mCacheInitialized {0};
        std::atomic<uint32_t> mTerminateAbort {0};
        
        unsigned mCacheFileCountLimit = 0;
        unsigned mCacheFileSizeLimit = 0;
        std::mutex mCacheMutex;
        std::string mCacheDirectoryPath;
        const char mCacheMagicWord[5] = { 'Y', 'G', 'G', '_', 'H'};
        
        std::mt19937 mRandomGenerator;

        RequestSettings mRequestSettings;
        mutable std::mutex mRequestSettingsMutex;
    };
}

#endif
