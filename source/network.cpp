// Copyright (C) 2017 Grupa Pracuj Sp. z o.o.
// This file is part of the "Hermes" library.
// For conditions of distribution and use, see copyright notice in license.txt.

#include "network.hpp"

#include "hermes.hpp"

#include <cassert>
#include <sstream>
#include <utility>
#include <algorithm>
#include <array>
#include <chrono>
#include <tuple>
#include <iomanip>
#include <fstream>
#include <chrono>
#include <ctime>
#include <limits>
#include <dirent.h>
#include <sys/stat.h>

namespace hms
{

    /* libcurl callback */
    
    static int CURL_HEADER_CALLBACK(char* pData, size_t pCount, size_t pSize, std::vector<std::pair<std::string, std::string>>* pUserData)
    {
        if (pData != nullptr)
        {
            auto separate = [](const std::string& lpSource, std::pair<std::string, std::string>& lpDestination) -> void
            {
                auto checkSign = [](char c) -> bool
                {
                    bool status = false;
                    
                    switch (c)
                    {
                        case '\n':
                        case '\r':
                        case '\t':
                        case ' ':
                            status = true;
                            break;
                        default:
                            break;
                    }
                    
                    return status;
                };
                
                std::pair<std::string, std::string> destination = {"", ""};
                
                std::istringstream ss(lpSource);
                
                std::string component = "";
                
                while (std::getline(ss, component, ':'))
                {
                    if (!component.empty())
                    {
                        std::string* element = (destination.first.size() > 0) ? &destination.second : &destination.first;
                        *element = component;
                    }
                }
                
                if (destination.first.size() > 0 && destination.second.size())
                {
                    destination.first.erase(std::remove_if(destination.first.begin(), destination.first.end(), checkSign), destination.first.end());
                    destination.second.erase(std::remove_if(destination.second.begin(), destination.second.end(), checkSign), destination.second.end());
                    
                    lpDestination = destination;
                }
            };
            
            std::string data(pData, pCount * pSize);
            
            std::pair<std::string, std::string> element("", "");
            
            separate(data, element);

            if (element.first.size() > 0 && element.second.size() > 0)
            {
                pUserData->push_back(element);
            }
            
            return static_cast<int>(pCount * pSize);
        }
        
        return 0;
    }
    
    static int CURL_WRITER_MESSAGE_CALLBACK(char* pData, size_t pCount, size_t pSize, std::string* pUserData)
    {
        if (pData != nullptr)
        {
            pUserData->append(pData, pCount * pSize);
            
            return static_cast<int>(pCount * pSize);
        }
        
        return 0;
    }
    
    static int CURL_WRITER_RAWDATA_CALLBACK(char* pData, size_t pCount, size_t pSize, DataBuffer* pUserData)
    {
        if (pData != nullptr)
        {
            const size_t dataSize = pCount * pSize;

            pUserData->push_back<char>(pData, dataSize);

            return static_cast<int>(dataSize);
        }
        
        return 0;
    }
    
    static int CURL_PROGRESS_CALLBACK(void* pClient, curl_off_t pDownloadTotal, curl_off_t pDownloadNow, curl_off_t pUploadTotal, curl_off_t pUploadNow)
    {
        if (pClient != nullptr)
        {
            NetworkManager::ProgressData* progressData = reinterpret_cast<NetworkManager::ProgressData*>(pClient);

            if (progressData->progressTask != nullptr)
                progressData->progressTask(pDownloadNow, pDownloadTotal, pUploadNow, pUploadTotal);
        }
        
        return 0;
    }
    
    static int CURL_WAIT_ON_SOCKET(curl_socket_t pSockfd, int pForRecv, long pTimeoutMS)
    {
        struct timeval tv;
        fd_set infd, outfd, errfd;
        int res;
        
        tv.tv_sec = pTimeoutMS / 1000;
        tv.tv_usec = (pTimeoutMS % 1000) * 1000;
        
        FD_ZERO(&infd);
        FD_ZERO(&outfd);
        FD_ZERO(&errfd);
        
        FD_SET(pSockfd, &errfd); /* always check for error */
        
        if(pForRecv)
            FD_SET(pSockfd, &infd);
        else
            FD_SET(pSockfd, &outfd);
        
        /* select() returns the number of signalled sockets or -1 */
        res = select(pSockfd + 1, &infd, &outfd, &errfd, &tv);
        return res;
    }
    
    /* NetworkRecovery */
    
    NetworkRecovery::NetworkRecovery(size_t pId, const std::string& pName) : mId(pId), mName(pName)
    {
    }
    
    NetworkRecovery::~NetworkRecovery()
    {
    }
    
    bool NetworkRecovery::init(std::function<bool(const NetworkResponse& pResponse, std::string& pRequestBody)> pPreCallback, std::function<void(NetworkResponse pResponse)> pPostCallback,
        ENetworkRequestType pType, std::string pUrl, std::vector<std::pair<std::string, std::string>> pParameter, std::vector<std::pair<std::string, std::string>> pHeader, unsigned pRepeatCount)
    {
        assert(pPreCallback != nullptr && pPostCallback != nullptr);
    
        uint32_t initialized = 0;

        if (mInitialized.compare_exchange_strong(initialized, 1))
        {
            auto callback = std::bind([this](NetworkResponse lpResponse, std::function<void(NetworkResponse pResponse)> lpPostCallback) -> void
            {
                unlock();

                Hermes::getInstance()->getLogger()->print(ELogLevel::Info, "Recovery: %, code: %, http: %, message: %", (lpResponse.mCode == ENetworkCode::OK) ? "passed" : "failed", static_cast<unsigned>(lpResponse.mCode), lpResponse.mHttpCode, lpResponse.mMessage);

                if (lpPostCallback != nullptr)
                {
                    NetworkResponse response;
                    response.mCode = lpResponse.mCode;
                    response.mHttpCode = lpResponse.mHttpCode;
                    response.mHeader = std::move(lpResponse.mHeader);
                    response.mMessage = std::move(lpResponse.mMessage);
                    response.mRawData = std::move(lpResponse.mRawData);

                    lpPostCallback(std::move(response));
                }
                
                while (mReceiver.size() != 0)
                {
                    std::tuple<size_t, NetworkRequestParam>& receiver = mReceiver.front();
                    
                    auto networkManager = Hermes::getInstance()->getNetworkManager();
                    auto networkAPI = (std::get<0>(receiver) < networkManager->count()) ? networkManager->get(std::get<0>(receiver)) : nullptr;

                    if (networkAPI != nullptr && lpResponse.mCode == ENetworkCode::OK)
                    {
                        NetworkRequestParam* param = &std::get<1>(receiver);

                        auto defaultHeader = networkAPI->getDefaultHeader();
                        
                        param->mMethod = networkAPI->getURL() + param->mMethod;
                        param->mHeader.insert(param->mHeader.end(), defaultHeader.begin(), defaultHeader.end());

                        Hermes::getInstance()->getLogger()->print(ELogLevel::Info, "Executed method from recovery: %.", param->mMethod);

                        networkManager->request(*param);
                    }
                    
                    mReceiver.pop();
                }
            }, std::placeholders::_1, std::move(pPostCallback));

            mUnsafeLock = false;
            
            mPreCallback = std::move(pPreCallback);

            mRequestParam.mRequestType = pType;
            mRequestParam.mResponseType = ENetworkResponseType::Message;
            mRequestParam.mMethod = std::move(pUrl);
            mRequestParam.mParameter = std::move(pParameter);
            mRequestParam.mHeader = std::move(pHeader);
            mRequestParam.mRequestBody = "";
            mRequestParam.mCallback = std::move(callback);
            mRequestParam.mProgress = nullptr;
            mRequestParam.mRepeatCount = pRepeatCount;
            mRequestParam.mAllowRecovery = false;
            
            mInitialized.store(2);
        }
        else
        {
            // Already initialized.
            assert(false);
        }
        
        return initialized == 0;
    }
    
    bool NetworkRecovery::isInitialized() const
    {
        return mInitialized.load() == 2;
    }
    
    bool NetworkRecovery::isQueueEmpty() const
    {
        return mReceiver.empty();
    }
    
    bool NetworkRecovery::checkCondition(const NetworkResponse& pResponse, std::string& pRequestBody) const
    {
        assert(mInitialized.load() == 2);
    
        return mPreCallback != nullptr && mPreCallback(pResponse, pRequestBody);
    }
    
    void NetworkRecovery::pushReceiver(std::tuple<size_t, NetworkRequestParam> pReceiver)
    {
        assert(mInitialized.load() == 2);

        mReceiver.push(std::move(pReceiver));
    }
    
    void NetworkRecovery::runRequest(std::string pRequestBody)
    {
        assert(mInitialized.load() == 2);

        mRequestParam.mRequestBody = std::move(pRequestBody);
        Hermes::getInstance()->getNetworkManager()->request(mRequestParam);
    }
    
    bool NetworkRecovery::isLocked() const
    {
        return mUnsafeLock;
    }
    
    bool NetworkRecovery::lock()
    {
        bool status = false;
        
        if (!mUnsafeLock)
        {
            mUnsafeLock = true;
            
            status = true;
        }
        
        return status;
    }
    
    bool NetworkRecovery::unlock()
    {
        bool status = false;
        
        if (mUnsafeLock)
        {
            mUnsafeLock = false;
            
            status = true;
        }
        
        return status;
    }
    
    size_t NetworkRecovery::getId() const
    {
        return mId;
    }
    
    const std::string& NetworkRecovery::getName() const
    {
        return mName;
    }
    
    const std::vector<std::pair<std::string, std::string>>& NetworkRecovery::getHeader() const
    {
        return mRequestParam.mHeader;
    }
    
    void NetworkRecovery::setHeader(const std::vector<std::pair<std::string, std::string>>& pHeader)
    {
        mRequestParam.mHeader = pHeader;
    }
        
    const std::vector<std::pair<std::string, std::string>>& NetworkRecovery::getParameter() const
    {
        return mRequestParam.mParameter;
    }
    
    void NetworkRecovery::setParameter(const std::vector<std::pair<std::string, std::string>>& pParameter)
    {
        mRequestParam.mParameter = pParameter;
    }
    
    /* NetworkAPI */
    
    NetworkAPI::NetworkAPI(size_t pId, const std::string& pName, const std::string& pURL) : mId(pId), mName(pName), mURL(pURL)
    {
        mCallback = decltype(mCallback)(new std::vector<std::tuple<std::function<bool(const NetworkResponse&)>, std::function<void(const NetworkRequestParam&, const NetworkResponse&)>, std::string>>());
    }
    
    NetworkAPI::~NetworkAPI()
    {
    }

    void NetworkAPI::request(NetworkRequestParam pParam)
    {
        auto postRequestCallback = mCallback;
    
        auto callback = [pParam, postRequestCallback](NetworkResponse lpResponse) -> void
        {
            for (auto& v : *postRequestCallback)
            {
                auto& preCallback = std::get<0>(v);
                
                if (preCallback == nullptr || preCallback(lpResponse))
                {
                    auto& postCallback = std::get<1>(v);
                    
                    if (postCallback != nullptr)
                    {
                        postCallback(pParam, lpResponse);
                    }
                }
            }
            
            if (pParam.mCallback != nullptr)
            {
                NetworkResponse response;
                response.mCode = lpResponse.mCode;
                response.mHttpCode = lpResponse.mHttpCode;
                response.mHeader = std::move(lpResponse.mHeader);
                response.mMessage = std::move(lpResponse.mMessage);
                response.mRawData = std::move(lpResponse.mRawData);
                
                pParam.mCallback(std::move(response));
            }
        };
        
        auto recovery = mRecovery.lock();
        
        if (recovery != nullptr && !recovery->isInitialized())
            recovery = nullptr;
        
        size_t identifier = mId;
        std::string url = mURL + pParam.mMethod;

        if (pParam.mAllowRecovery && recovery != nullptr)
        {
            auto callbackForRecovery = [recovery, identifier, url, pParam, callback](NetworkResponse lpResponse) -> void
            {
                std::string requestBody = "";
                
                if (recovery->isLocked() || recovery->checkCondition(lpResponse, requestBody))
                {
                    recovery->pushReceiver(std::make_tuple(identifier, pParam));

                    if (recovery->lock())
                        recovery->runRequest(std::move(requestBody));

                    Hermes::getInstance()->getLogger()->print(ELogLevel::Info, "Queued method for recovery: %.", url);
                }
                else
                {
                    callback(std::move(lpResponse));
                }
            };
            
            pParam.mCallback = callbackForRecovery;
        }
        else
        {
            pParam.mCallback = callback;
        }

        if (recovery != nullptr && recovery->isLocked())
        {
            recovery->pushReceiver(std::make_tuple(mId, pParam));

            Hermes::getInstance()->getLogger()->print(ELogLevel::Info, "Queued method for recovery: %.", url);
        }
        else
        {
            pParam.mMethod = std::move(url);
            pParam.mHeader.insert(pParam.mHeader.end(), mDefaultHeader.begin(), mDefaultHeader.end());
        
            Hermes::getInstance()->getNetworkManager()->request(pParam);
        }
    }
    
    const std::vector<std::pair<std::string, std::string>>& NetworkAPI::getDefaultHeader() const
    {
        return mDefaultHeader;
    }
    
    void NetworkAPI::setDefaultHeader(std::vector<std::pair<std::string, std::string>> pDefaultHeader)
    {
        mDefaultHeader = std::move(pDefaultHeader);
    }
    
    const std::string& NetworkAPI::getURL() const
    {
        return mURL;
    }
    
    bool NetworkAPI::registerCallback(std::function<void(const NetworkRequestParam& pRequest, const NetworkResponse& pResponse)> pCallback, std::string pName, bool pOverwrite)
    {
        return registerAdvancedCallback(nullptr, pCallback, pName, pOverwrite);
    }
    
    bool NetworkAPI::registerAdvancedCallback(std::function<bool(const NetworkResponse& pResponse)> pCondition,
        std::function<void(const NetworkRequestParam& pRequest, const NetworkResponse& pResponse)> pCallback, std::string pName, bool pOverwrite)
    {
        auto checkItem = [pName](const std::tuple<std::function<bool(const NetworkResponse& pResponse)>, std::function<void(const NetworkRequestParam& pRequest, const NetworkResponse& pResponse)>,
                std::string>& pItem) -> bool
        {
            if (std::get<2>(pItem) == pName)
                return true;
            
            return false;
        };
        
        auto it = std::find_if(mCallback->begin(), mCallback->end(), checkItem);
        
        bool status = false;
    
        if (it == mCallback->end())
        {
            mCallback->push_back(std::make_tuple(pCondition, pCallback, pName));
            status = true;
        }
        else if (pOverwrite)
        {
            *it = std::make_tuple(pCondition, pCallback, pName);
            status = true;
        }
        
        return status;
    }

    bool NetworkAPI::unregisterCallback(std::string pName)
    {
        bool status = false;
        
        auto checkItem = [pName, &status](const std::tuple<std::function<bool(const NetworkResponse& pResponse)>, std::function<void(const NetworkRequestParam& pRequest, const NetworkResponse& pResponse)>,
                std::string>& pItem) -> bool
        {
            if (std::get<2>(pItem) == pName)
            {
                status = true;
                return true;
            }
            
            return false;
        };
        
        mCallback->erase(remove_if(mCallback->begin(), mCallback->end(), checkItem), mCallback->end());
        
        return status;
    }
    
    std::weak_ptr<NetworkRecovery> NetworkAPI::getRecovery() const
    {
        return mRecovery;
    }
    
    void NetworkAPI::setRecovery(std::weak_ptr<NetworkRecovery> pRecovery)
    {
        mRecovery = pRecovery;
    }
    
    size_t NetworkAPI::getId() const
    {
        return mId;
    }
    
    const std::string& NetworkAPI::getName() const
    {
        return mName;
    }
    
    /* NetworkManager */
    
    NetworkManager::NetworkManager()
    {
    }
    
    NetworkManager::~NetworkManager()
    {
        terminate();
    }
    
    void NetworkManager::deleterNetworkAPI(NetworkAPI* pObject)
    {
        delete pObject;
    }
    
    void NetworkManager::deleterNetworkRecovery(NetworkRecovery* pObject)
    {
        delete pObject;
    }
    
    bool NetworkManager::initialize(long pTimeout, int pThreadPoolID, std::pair<int, int> pHttpCodeSuccess, std::string pCACertificatePath)
    {
        uint32_t initialized = 0;

        if (mInitialized.compare_exchange_strong(initialized, 1))
        {
            if (curl_global_init(CURL_GLOBAL_ALL) == CURLE_OK)
            {
                mTimeout = pTimeout;
                mThreadPoolID = pThreadPoolID;
                mHttpCodeSuccess = pHttpCodeSuccess;
                mCACertificatePath = std::move(pCACertificatePath);
                
                mInitialized.store(2);
            }
            else
            {
                initialized = 1;
                mInitialized.store(0);
            }

        }
        else
        {
            // Already initialized.
            assert(false);
        }
    
        return initialized == 0;
    }
    
    bool NetworkManager::terminate()
    {
        uint32_t terminated = 2;

        if (mInitialized.compare_exchange_strong(terminated, 1))
        {
            // finish all network operations
            
            Hermes::getInstance()->getTaskManager()->flush(mThreadPoolID);
            Hermes::getInstance()->getTaskManager()->flush(mThreadPoolSimpleSocketID);
            
            mSimpleSocketInitialized.store(0);
            
            mAPI.clear();
            mRecovery.clear();
            
            mAPI.clear();
            
            // remove cache info
            {
                std::lock_guard<std::mutex> lock(mCacheMutex);
                mCacheFileInfo.clear();
                mCacheFileIndex.clear();
            }
            mCacheInitialized.store(0);

            for (auto& v : mHandle)
            {
                if (v.second != nullptr)
                    curl_easy_cleanup(v.second);
            }
            
            mHandle.clear();
            
            for (auto& v : mMultiHandle)
            {
                if (v.second != nullptr)
                    curl_multi_cleanup(v.second);
            }
            
            mMultiHandle.clear();

            curl_global_cleanup();

            mTimeout = 0;
            mThreadPoolID = mThreadPoolSimpleSocketID = -1;
            mHttpCodeSuccess = {200, 299};
            mCACertificatePath = "";
            mFlag = {false, false};
            mProgressTimePeriod = 0;

            mInitialized.store(0);
        }
        else
        {
            // Already terminated.
            assert(false);
        }
        
        return terminated == 2;
    }
        
    std::shared_ptr<NetworkAPI> NetworkManager::add(const std::string& pName, const std::string& pURL, size_t pUniqueID)
    {
        // resize array if required
        if (pUniqueID >= mAPI.size())
            mAPI.resize(pUniqueID + 1);
        
        // check if ID is in use
        assert(mAPI[pUniqueID] == nullptr);

        mAPI[pUniqueID] = std::shared_ptr<NetworkAPI>(new NetworkAPI(pUniqueID, pName, pURL), std::bind(&NetworkManager::deleterNetworkAPI, this, std::placeholders::_1));
        
        return mAPI[pUniqueID];
    }
    
    size_t NetworkManager::count() const
    {
        return mAPI.size();
    }
    
    std::shared_ptr<NetworkAPI> NetworkManager::get(size_t pID) const
    {
        // check if ID has a valid value
        assert(pID < mAPI.size());
        
        return mAPI[pID];
    }
    
    std::shared_ptr<NetworkRecovery> NetworkManager::addRecovery(const std::string& pName, size_t pUniqueID)
    {
        // resize array if required
        if (pUniqueID >= mRecovery.size())
            mRecovery.resize(pUniqueID + 1);
        
        // check if ID is in use
        assert(mRecovery[pUniqueID] == nullptr);

        mRecovery[pUniqueID] = std::shared_ptr<NetworkRecovery>(new NetworkRecovery(pUniqueID, pName), std::bind(&NetworkManager::deleterNetworkRecovery, this, std::placeholders::_1));
        
        return mRecovery[pUniqueID];
    }
    
    size_t NetworkManager::countRecovery() const
    {
        return mRecovery.size();
    }
    
    std::shared_ptr<NetworkRecovery> NetworkManager::getRecovery(size_t pID) const
    {
        // check if ID has a valid value
        assert(pID < mRecovery.size());
        
        return mRecovery[pID];
    }
    
    void NetworkManager::request(NetworkRequestParam pParam)
    {
        std::string requestUrl = std::move(pParam.mMethod);
        appendParameter(requestUrl, pParam.mParameter);
    
        ENetworkRequestType requestType = pParam.mRequestType;
        ENetworkResponseType responseType = pParam.mResponseType;
        long timeoutCopy = mTimeout;
        long progressTimePeriodCopy = mProgressTimePeriod;
        auto flagCopy = mFlag;
        
        bool allowCache = pParam.mAllowCache;
        u_int32_t cacheLifetime = pParam.mCacheLifetime;

        auto request = [this, requestType, responseType, timeoutCopy, progressTimePeriodCopy, allowCache, cacheLifetime, flagCopy](std::string lpRequestUrl, std::string lpRequestBody,
            std::vector<std::pair<std::string, std::string>> lpHeader, std::function<void(NetworkResponse pResponse)> lpCallback,
            std::function<void(const long long& lpDN, const long long& lpDT, const long long& lpUN, const long long& lpUT)> lpProgress, unsigned lpRepeatCount) -> void
        {
            if (allowCache)
            {
                NetworkResponse response = getResponseFromCache(lpRequestUrl);
                
                if (response.mCode == ENetworkCode::OK)
                {
                    auto taskData = std::make_pair(std::move(lpCallback), std::move(response));
                    auto taskHandler = std::make_shared<decltype(taskData)>(std::move(taskData));

                    Hermes::getInstance()->getTaskManager()->execute(-1, [taskHandler]() -> void
                    {
                        taskHandler->first(std::move(taskHandler->second));
                    });
                    
                    return;
                }
            }
            
            ENetworkCode code = ENetworkCode::Unknown;
			long httpCode = -1;
			std::vector<std::pair<std::string, std::string>> responseHeader;
			std::string responseMessage = "";
			DataBuffer responseRawData;
            
            auto uniqueHeader = createUniqueHeader(lpHeader);
            curl_slist* header = nullptr;

            for (auto& v : uniqueHeader)
            {
                std::string singleHeader = v.first;
                singleHeader += ": ";
                singleHeader += v.second;
                
                header = curl_slist_append(header, singleHeader.c_str());
            }

			CURL* handle = nullptr;
			
			{
				std::lock_guard<std::mutex> lock(mHandleMutex);
				handle = mHandle[std::this_thread::get_id()];
			}
            
            if (handle == nullptr)
            {
                handle = curl_easy_init();

                curl_easy_setopt(handle, CURLOPT_HEADERFUNCTION, CURL_HEADER_CALLBACK);
                curl_easy_setopt(handle, CURLOPT_SSL_VERIFYPEER, 1L);

                if (mCACertificatePath.size() > 0)
                    curl_easy_setopt(handle, CURLOPT_CAINFO, mCACertificatePath.c_str());

				std::lock_guard<std::mutex> lock(mHandleMutex);
                mHandle[std::this_thread::get_id()] = handle;
            }
            
            char errorBuffer[CURL_ERROR_SIZE];
            errorBuffer[0] = 0;
            
            const bool enableProgressInfo = (lpProgress != nullptr);
            
            ProgressData progressData;
            
            if (enableProgressInfo)
            {
                std::chrono::time_point<std::chrono::system_clock> lastTick = std::chrono::system_clock::now();

                progressData.progressTask = [=](const long long& lpDN, const long long& lpDT, const long long& lpUN, const long long& lpUT) mutable -> void
                {
                    auto thisTick = std::chrono::system_clock::now();
                    
                    const auto difference = std::chrono::duration_cast<std::chrono::milliseconds>(thisTick - lastTick).count();
                    
                    if (difference > progressTimePeriodCopy || (lpDN != 0 && lpDN == lpDT) || (lpUN != 0 && lpUN == lpUT))
                    {
                        lastTick = std::chrono::system_clock::now();
                        
                        Hermes::getInstance()->getTaskManager()->execute(-1, lpProgress, lpDN, lpDT, lpUN, lpUT);
                    }
                };
            }
            
            configureHandle(handle, requestType, responseType, lpRequestUrl, lpRequestBody, &responseMessage, &responseRawData, &responseHeader, header,
                timeoutCopy, flagCopy, (enableProgressInfo) ? &progressData : nullptr, errorBuffer);
            
            CURLcode curlCode = CURLE_OK;
            unsigned step = 0;

            do
            {
                curlCode = curl_easy_perform(handle);
                
                step++;
            }
            while (curlCode != CURLE_OK && step <= lpRepeatCount);

            resetHandle(handle);
            
            curl_slist_free_all(header);
            
            switch (curlCode)
            {
            case CURLE_OK:
                curl_easy_getinfo (handle, CURLINFO_RESPONSE_CODE, &httpCode);
                code = (httpCode >= mHttpCodeSuccess.first && httpCode <= mHttpCodeSuccess.second) ? ENetworkCode::OK : ENetworkCode::InvalidHttpCodeRange;
                break;
            case CURLE_COULDNT_RESOLVE_HOST:
                code = ENetworkCode::LostConnection;
                Hermes::getInstance()->getLogger()->print(ELogLevel::Warning, "Lost connection. URL: %", lpRequestUrl);
                break;
            case CURLE_OPERATION_TIMEDOUT:
                code = ENetworkCode::Timeout;
                Hermes::getInstance()->getLogger()->print(ELogLevel::Warning, "Timeout. URL: %", lpRequestUrl);
                break;
            default:
                code = static_cast<ENetworkCode>(static_cast<int>(ENetworkCode::Unknown) + static_cast<int>(curlCode));
                Hermes::getInstance()->getLogger()->print(ELogLevel::Warning, "CURL code: %. URL: %", curl_easy_strerror(curlCode), lpRequestUrl);
                break;
            }

			if (lpCallback != nullptr)
			{
				NetworkResponse response;
				response.mCode = code;
				response.mHttpCode = static_cast<int>(httpCode);
				response.mHeader = std::move(responseHeader);
				response.mMessage = strlen(errorBuffer) == 0 ? std::move(responseMessage) : errorBuffer;
				response.mRawData = std::move(responseRawData);
                
                if (response.mCode == ENetworkCode::OK && allowCache)
                    cacheResponse(response, lpRequestUrl, cacheLifetime);
                
                auto taskData = std::make_pair(std::move(lpCallback), std::move(response));
                auto taskHandler = std::make_shared<decltype(taskData)>(std::move(taskData));

				Hermes::getInstance()->getTaskManager()->execute(-1, [taskHandler]() -> void
                {
                    taskHandler->first(std::move(taskHandler->second));
                });
			}
        };
        
        auto taskData = std::make_tuple(std::move(request), std::move(requestUrl), std::move(pParam.mRequestBody), std::move(pParam.mHeader), std::move(pParam.mCallback),
            std::move(pParam.mProgress), pParam.mRepeatCount);
        auto taskHandler = std::make_shared<decltype(taskData)>(std::move(taskData));

        Hermes::getInstance()->getTaskManager()->execute(mThreadPoolID, [taskHandler]() -> void
        {
            std::get<0>(*taskHandler)(std::move(std::get<1>(*taskHandler)), std::move(std::get<2>(*taskHandler)), std::move(std::get<3>(*taskHandler)), std::move(std::get<4>(*taskHandler)),
                std::move(std::get<5>(*taskHandler)), std::get<6>(*taskHandler));
        });
    }
    
    void NetworkManager::request(std::vector<NetworkRequestParam> pParam)
    {
        long timeoutCopy = mTimeout;
        long progressTimePeriodCopy = mProgressTimePeriod;
        auto flagCopy = mFlag;
        
        auto requestTask = [this, timeoutCopy, progressTimePeriodCopy, flagCopy](std::vector<NetworkRequestParam> lpParam) -> void
        {
            std::vector<NetworkRequestParam> param;
            param.reserve(lpParam.size());
            for (auto it = lpParam.begin(); it != lpParam.end(); it++)
            {
                if (!(*it).mAllowCache)
                {
                    param.push_back(std::move(*it));
                }
                else
                {
                    NetworkResponse response = getResponseFromCache((*it).mMethod);
                    
                    if (response.mCode != ENetworkCode::OK)
                    {
                        param.push_back(std::move(*it));
                    }
                    else
                    {
                        auto taskData = std::make_pair(std::move((*it).mCallback), std::move(response));
                        auto taskHandler = std::make_shared<decltype(taskData)>(std::move(taskData));

                        Hermes::getInstance()->getTaskManager()->execute(-1, [taskHandler]() -> void
                        {
                            taskHandler->first(std::move(taskHandler->second));
                        });
                    }
                }
            }
            
            if (param.size() == 0)
                return;
            
            CURLM* handle = nullptr;
			
			{
				std::lock_guard<std::mutex> lock(mMultiHandleMutex);
				handle = mMultiHandle[std::this_thread::get_id()];
			}
            
            if (handle == nullptr)
            {
                handle = curl_multi_init();

				std::lock_guard<std::mutex> lock(mMultiHandleMutex);
                mMultiHandle[std::this_thread::get_id()] = handle;
            }
        
            const size_t paramSize = param.size();
            
            auto multiRequestData = new MultiRequestData[paramSize];
        
            for (size_t i = 0; i < paramSize; ++i)
            {
                std::string requestUrl = param[i].mMethod;
                appendParameter(requestUrl, param[i].mParameter);
            
                multiRequestData[i].mHandle = curl_easy_init();
                multiRequestData[i].mParam = &param[i];
                multiRequestData[i].mErrorBuffer = new char[CURL_ERROR_SIZE];
                multiRequestData[i].mErrorBuffer[0] = 0;

                curl_easy_setopt(multiRequestData[i].mHandle, CURLOPT_HEADERFUNCTION, CURL_HEADER_CALLBACK);
                curl_easy_setopt(multiRequestData[i].mHandle, CURLOPT_SSL_VERIFYPEER, 1L);
                curl_easy_setopt(multiRequestData[i].mHandle, CURLOPT_PRIVATE, &multiRequestData[i]);

                if (mCACertificatePath.size() > 0)
                    curl_easy_setopt(multiRequestData[i].mHandle, CURLOPT_CAINFO, mCACertificatePath.c_str());
                
                auto uniqueHeader = createUniqueHeader(param[i].mHeader);

                curl_slist* header = nullptr;

                for (auto& x : uniqueHeader)
                {
                    std::string singleHeader = x.first;
                    singleHeader += ": ";
                    singleHeader += x.second;
                    
                    header = curl_slist_append(header, singleHeader.c_str());
                }
                    
                const bool enableProgressInfo = (param[i].mProgress != nullptr);
            
                ProgressData progressData;
                
                if (enableProgressInfo)
                {
                    std::chrono::time_point<std::chrono::system_clock> lastTick = std::chrono::system_clock::now();

                    progressData.progressTask = [=](const long long& lpDN, const long long& lpDT, const long long& lpUN, const long long& lpUT) mutable -> void
                    {
                        auto thisTick = std::chrono::system_clock::now();
                        
                        const auto difference = std::chrono::duration_cast<std::chrono::milliseconds>(thisTick - lastTick).count();
                        
                        if (difference > progressTimePeriodCopy || (lpDN != 0 && lpDN == lpDT) || (lpUN != 0 && lpUN == lpUT))
                        {
                            lastTick = std::chrono::system_clock::now();
                            
                            Hermes::getInstance()->getTaskManager()->execute(-1, param[i].mProgress, lpDN, lpDT, lpUN, lpUT);
                        }
                    };
                }
                
                configureHandle(multiRequestData[i].mHandle, param[i].mRequestType, param[i].mResponseType, requestUrl, param[i].mRequestBody, &multiRequestData[i].mResponseMessage,
                    &multiRequestData[i].mResponseRawData, &multiRequestData[i].mResponseHeader, header, timeoutCopy, flagCopy, (enableProgressInfo) ? &progressData : nullptr, multiRequestData[i].mErrorBuffer);
                
                curl_multi_add_handle(handle, multiRequestData[i].mHandle);
            }

            int activeHandle = 0;
            
            do
            {
                long multiTimeout = -1;
                curl_multi_timeout(handle, &multiTimeout);
            
                timeval timeout = {1, 0};
                
                if (multiTimeout >= 0)
                {
                    timeout.tv_sec = multiTimeout / 1000;
                    
                    if (timeout.tv_sec > 1)
                        timeout.tv_sec = 1;
                    else
                        timeout.tv_usec = (multiTimeout % 1000) * 1000;
                }

                fd_set readFd;
                fd_set writeFd;
                fd_set exceptionFd;

                FD_ZERO(&readFd);
                FD_ZERO(&writeFd);
                FD_ZERO(&exceptionFd);
                
                int maxFd = -1;
     
                CURLMcode codeFdset = curl_multi_fdset(handle, &readFd, &writeFd, &exceptionFd, &maxFd);
                
                if (codeFdset != CURLM_OK)
                {
                    Hermes::getInstance()->getLogger()->print(ELogLevel::Warning, "CURL fdset code: \"%\"", curl_multi_strerror(codeFdset));
                    
                    break;
                }

                int codeSelect = 0;
             
                if (maxFd == -1)
                {
                    timeval wait = {0, 100000};
                    codeSelect = select(0, nullptr, nullptr, nullptr, &wait);
                }
                else
                {
                    codeSelect = select(maxFd + 1, &readFd, &writeFd, &exceptionFd, &timeout);
                }
                
                switch (codeSelect)
                {
                case -1: // error
                    break;
                case 0: // timeout
                default:
                    curl_multi_perform(handle, &activeHandle);
                    break;
                }
                
                CURLMsg* message = nullptr;
                int messageLeft = 0;
                
                while ((message = curl_multi_info_read(handle, &messageLeft)))
                {
                    if (message->msg == CURLMSG_DONE)
                    {
                        CURLcode curlCode = message->data.result;
                        MultiRequestData* requestData = nullptr;
                        
                        ENetworkCode code = ENetworkCode::Unknown;
                        long httpCode = -1;
                        
                        curl_easy_getinfo(message->easy_handle, CURLINFO_PRIVATE, &requestData);
                        
                        switch (curlCode)
                        {
                        case CURLE_OK:
                            curl_easy_getinfo(message->easy_handle, CURLINFO_RESPONSE_CODE, &httpCode);
                            code = (httpCode >= mHttpCodeSuccess.first && httpCode <= mHttpCodeSuccess.second) ? ENetworkCode::OK : ENetworkCode::InvalidHttpCodeRange;
                            break;
                        case CURLE_COULDNT_RESOLVE_HOST:
                            code = ENetworkCode::LostConnection;
                            Hermes::getInstance()->getLogger()->print(ELogLevel::Warning, "Lost connection. URL: %", (requestData != nullptr) ? requestData->mParam->mMethod : "unknown");
                            break;
                        case CURLE_OPERATION_TIMEDOUT:
                            code = ENetworkCode::Timeout;
                            Hermes::getInstance()->getLogger()->print(ELogLevel::Warning, "Timeout. URL: %", (requestData != nullptr) ? requestData->mParam->mMethod : "unknown");
                            break;
                        default:
                            code = static_cast<ENetworkCode>(static_cast<int>(ENetworkCode::Unknown) + static_cast<int>(curlCode));
                            Hermes::getInstance()->getLogger()->print(ELogLevel::Warning, "CURL code: %. URL: %", curl_easy_strerror(curlCode), (requestData != nullptr) ? requestData->mParam->mMethod : "unknown");
                            break;
                        }

                        if (requestData != nullptr && requestData->mParam->mCallback != nullptr)
                        {
                            NetworkResponse response;

                            response.mCode = code;
                            response.mHttpCode = static_cast<int>(httpCode);
                            response.mHeader = std::move(requestData->mResponseHeader);
                            response.mMessage = strlen(requestData->mErrorBuffer) == 0 ? std::move(requestData->mResponseMessage) : requestData->mErrorBuffer;
                            response.mRawData = std::move(requestData->mResponseRawData);
                            
                            if (response.mCode == ENetworkCode::OK && requestData->mParam->mAllowCache)
                                cacheResponse(response, requestData->mParam->mMethod, requestData->mParam->mCacheLifetime);
                            
                            auto taskData = std::make_pair(std::move(requestData->mParam->mCallback), std::move(response));
                            auto taskHandler = std::make_shared<decltype(taskData)>(std::move(taskData));

                            Hermes::getInstance()->getTaskManager()->execute(-1, [taskHandler]() -> void
                            {
                                taskHandler->first(std::move(taskHandler->second));
                            });
                        }
                    }
                }
            }
            while(activeHandle);

            for (size_t i = 0; i < paramSize; ++i)
            {
                curl_multi_remove_handle(handle, multiRequestData[i].mHandle);
                curl_easy_cleanup(multiRequestData[i].mHandle);
                
                delete[] multiRequestData[i].mErrorBuffer;
            }
            
            delete[] multiRequestData;
        };

        auto taskData = std::make_pair(std::move(requestTask), std::move(pParam));
        auto taskHandler = std::make_shared<decltype(taskData)>(std::move(taskData));

        Hermes::getInstance()->getTaskManager()->execute(mThreadPoolID, [taskHandler]() -> void
        {
            taskHandler->first(std::move(taskHandler->second));
        });
    }
    
    long NetworkManager::getTimeout() const
    {
        return mTimeout;
    }
    
    void NetworkManager::setTimeout(long pTimeout)
    {
        mTimeout = pTimeout;
    }
    
    bool NetworkManager::getFlag(ENetworkFlag pFlag) const
    {
        assert(pFlag < ENetworkFlag::Count);
            
        return mFlag[static_cast<size_t>(pFlag)];
    }
    
    void NetworkManager::setFlag(ENetworkFlag pFlag, bool pState)
    {
        assert(pFlag < ENetworkFlag::Count);
        
        mFlag[static_cast<size_t>(pFlag)] = pState;
    }
    
    long NetworkManager::getProgressTimePeriod() const
    {
        return mProgressTimePeriod;
    }
    
    void NetworkManager::setProgressTimePeriod(long pTimePeriod)
    {
        mProgressTimePeriod = pTimePeriod;
    }
    
    void NetworkManager::appendParameter(std::string& pURL, const std::vector<std::pair<std::string, std::string>>& pParameter) const
    {
        bool first = true;
        
        for (size_t i = 0; i < pParameter.size(); ++i)
        {
            if (pParameter[i].first.size() > 0 && pParameter[i].second.size() > 0)
            {
                pURL += (first) ? "?" : "&";
                pURL += pParameter[i].first;
                pURL += "=";
                
                std::string value = pParameter[i].second;
                encodeURL(value);
                
                pURL += value;
                
                first = false;
            }
        }
    }

    void NetworkManager::decodeURL(std::string& pData) const
    {
        char* data = curl_unescape(pData.c_str(), static_cast<int>(pData.size()));
        pData = data;
        curl_free(data);
    }
    
    void NetworkManager::encodeURL(std::string& pData) const
    {
        auto toHex = [](const char lpData) -> const char
        {
            static const char hexData[] = "0123456789abcdef";
            return hexData[lpData & 15];
        };
        
        auto convert = [&toHex](const char lpData) -> std::string
        {
            std::string output;
            
            output += '%';
            output += toHex(lpData >> 4);
            output += toHex(lpData & 15);

            return output;
        };
        
        std::string output;

        for (size_t i = 0; i < pData.size(); ++i)
        {
            const char sign = pData[i];
            
            if (sign == '.' || sign == '~' || sign == '-' || sign == '_' || (sign >= 'A' && sign <= 'Z') || (sign >= 'a' && sign <= 'z') || (sign >= '0' && sign <= '9'))
                output += sign;
            else
                output += convert(sign);
        }
        
        pData = output;
    }
    
    std::vector<std::pair<std::string, std::string>> NetworkManager::createUniqueHeader(const std::vector<std::pair<std::string, std::string>>& pHeader) const
    {
        std::vector<std::pair<std::string, std::string>> header;
            
        for (auto v = pHeader.rbegin(); v != pHeader.rend(); ++v)
        {
            std::string headerType = v->first;
            std::transform(headerType.begin(), headerType.end(), headerType.begin(), ::tolower);
            
            bool isUnique = true;
            
            for (auto x = header.begin(); x != header.end(); ++x)
            {
                if (headerType.size() == x->first.size() && headerType == x->first)
                {
                    isUnique = false;
                    
                    break;
                }
            }
            
            if (isUnique)
                header.push_back({headerType, v->second});
        }
        
        return header;
    }

    void NetworkManager::configureHandle(CURL* pHandle, ENetworkRequestType pRequestType, ENetworkResponseType pResponseType, const std::string& pRequestUrl, const std::string& pRequestBody,
        std::string* pResponseMessage, DataBuffer* pResponseRawData, std::vector<std::pair<std::string, std::string>>* pResponseHeader, curl_slist* pHeader,
        long pTimeout, std::array<bool, static_cast<size_t>(ENetworkFlag::Count)> pFlag, ProgressData* pProgressData, char* pErrorBuffer) const
    {
        switch (pRequestType)
        {
        case ENetworkRequestType::Delete:
            curl_easy_setopt(pHandle, CURLOPT_NOBODY, 1);
            curl_easy_setopt(pHandle, CURLOPT_CUSTOMREQUEST, "DELETE");
            break;
        case ENetworkRequestType::Get:
            curl_easy_setopt(pHandle, CURLOPT_HTTPGET, 1);
            break;
        case ENetworkRequestType::Post:
            curl_easy_setopt(pHandle, CURLOPT_HTTPPOST, 1);
            curl_easy_setopt(pHandle, CURLOPT_POSTFIELDS, pRequestBody.c_str());
            curl_easy_setopt(pHandle, CURLOPT_POSTFIELDSIZE, pRequestBody.size());
            break;
        case ENetworkRequestType::Put:
            curl_easy_setopt(pHandle, CURLOPT_NOBODY, 0);
            curl_easy_setopt(pHandle, CURLOPT_CUSTOMREQUEST, "PUT");
            curl_easy_setopt(pHandle, CURLOPT_POSTFIELDS, pRequestBody.c_str());
            curl_easy_setopt(pHandle, CURLOPT_POSTFIELDSIZE, pRequestBody.size());
            break;
        case ENetworkRequestType::Head:
            curl_easy_setopt(pHandle, CURLOPT_NOBODY, 1);
            break;
        default:
            // wrong request type
            assert(false);
            break;
        }
        
        switch (pResponseType)
        {
        case ENetworkResponseType::Message:
            curl_easy_setopt(pHandle, CURLOPT_WRITEFUNCTION, CURL_WRITER_MESSAGE_CALLBACK);
            curl_easy_setopt(pHandle, CURLOPT_WRITEDATA, pResponseMessage);
            break;
        case ENetworkResponseType::RawData:
            curl_easy_setopt(pHandle, CURLOPT_WRITEFUNCTION, CURL_WRITER_RAWDATA_CALLBACK);
            curl_easy_setopt(pHandle, CURLOPT_WRITEDATA, pResponseRawData);
            break;
        default:
            // wrong response type
            assert(false);
            break;
        }
            
        if (pFlag[static_cast<size_t>(ENetworkFlag::Redirect)])
            curl_easy_setopt(pHandle, CURLOPT_FOLLOWLOCATION, 1);
        
        curl_easy_setopt(pHandle, pFlag[static_cast<size_t>(ENetworkFlag::TimeoutInMilliseconds)] ? CURLOPT_TIMEOUT_MS : CURLOPT_TIMEOUT, pTimeout);
        curl_easy_setopt(pHandle, CURLOPT_HEADERDATA, pResponseHeader);
        curl_easy_setopt(pHandle, CURLOPT_HTTPHEADER, pHeader);
        curl_easy_setopt(pHandle, CURLOPT_URL, pRequestUrl.c_str());
        curl_easy_setopt(pHandle, CURLOPT_ERRORBUFFER, pErrorBuffer);

        if (pProgressData != nullptr)
        {
            curl_easy_setopt(pHandle, CURLOPT_XFERINFOFUNCTION, CURL_PROGRESS_CALLBACK);
            curl_easy_setopt(pHandle, CURLOPT_XFERINFODATA, pProgressData);
            curl_easy_setopt(pHandle, CURLOPT_NOPROGRESS, 0);
        }
    }
    
    void NetworkManager::resetHandle(CURL* pHandle) const
    {
        curl_easy_setopt(pHandle, CURLOPT_CUSTOMREQUEST, 0);
        curl_easy_setopt(pHandle, CURLOPT_XFERINFOFUNCTION, nullptr);
        curl_easy_setopt(pHandle, CURLOPT_XFERINFODATA, nullptr);
        curl_easy_setopt(pHandle, CURLOPT_NOPROGRESS, 1);
        curl_easy_setopt(pHandle, CURLOPT_ERRORBUFFER, 0);
    }
    
    bool NetworkManager::initCache(const std::string& pDirectoryPath, unsigned pFileCountLimit, unsigned pFileSizeLimit)
    {
        if (mInitialized.load() != 0 && mCacheInitialized.load() == 0)
        {
            DIR* dir = NULL;
            if ((dir = opendir(pDirectoryPath.c_str())) != NULL)
            {
                std::string dirAppendPath = pDirectoryPath[pDirectoryPath.length()-1] != '/' ? pDirectoryPath + '/' : pDirectoryPath;
                
                std::unordered_map<std::string, size_t> indexes;
                std::vector<CacheFileData> info;
                info.reserve(pFileCountLimit);
                
                auto isDir = [](const std::string& lpFilePath) -> bool
                {
                    struct stat stat_buf;
                    stat(lpFilePath.c_str(), &stat_buf);
                    return S_ISDIR(stat_buf.st_mode);
                };
                
                auto duration = std::chrono::system_clock::now().time_since_epoch();
                u_int64_t timestamp = static_cast<u_int64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(duration).count());
                
                struct dirent *file;
                while ((file = readdir(dir)) != NULL)
                {
                    std::string fullPath = dirAppendPath + file->d_name;
                    if (file->d_name[0] != '.' && !isDir(fullPath))
                    {
                        CacheFileData cache = decodeCacheHeader(fullPath);
                        if (cache.mIsValid && timestamp >= cache.mHeader.mTimestamp && (timestamp - cache.mHeader.mTimestamp) < cache.mHeader.mLifetime)
                            info.push_back(std::move(cache));
                        else if (std::remove(fullPath.c_str()) != 0)
                            Hermes::getInstance()->getLogger()->print(ELogLevel::Warning, "Cache failed to delete file at '%'", fullPath);
                    }
                }
                closedir(dir);
                
                std::sort(info.begin(), info.end(), [](const CacheFileData& lpFile1, const CacheFileData& lpFile2) -> bool
                          {
                              return lpFile1.mHeader.mTimestamp < lpFile2.mHeader.mTimestamp;
                          });
                
                if (info.size() > pFileCountLimit)
                {
                    std::rotate(info.begin(), info.begin() + static_cast<long>(info.size() - pFileCountLimit), info.end());
                    for (auto it = info.begin() + static_cast<long>(pFileCountLimit); it != info.end(); it++)
                    {
                        if (std::remove((*it).mFullFilePath.c_str()) != 0)
                            Hermes::getInstance()->getLogger()->print(ELogLevel::Warning, "Cache failed to delete file at '%'", (*it).mFullFilePath);
                    }
                    info.resize(pFileCountLimit);
                }
                
                for (size_t i = 0; i < info.size(); i++)
                    indexes[info[i].mUrl] = i;
                
                {
                    std::lock_guard<std::mutex> lock(mCacheMutex);
                    mCacheDirectoryPath = std::move(dirAppendPath);
                    mCacheFileInfo = std::move(info);
                    mCacheFileIndex = std::move(indexes);
                    mCacheFileSizeLimit = pFileSizeLimit;
                    mCacheFileCountLimit = pFileCountLimit;
                    
                    std::random_device rd;
                    mRandomGenerator.seed(rd());
                }
                
                mCacheInitialized.store(1);
            }
            else
            {
                Hermes::getInstance()->getLogger()->print(ELogLevel::Warning, "Failed to open cache directory at '%' during build", pDirectoryPath);
            }
        }
        
        return mCacheInitialized.load() != 0;
    }
    
    void NetworkManager::clearCache()
    {
        if (mCacheInitialized.load() != 0)
        {
            std::vector<CacheFileData> info;
            {
                std::lock_guard<std::mutex> lock(mCacheMutex);
                info = std::move(mCacheFileInfo);
                mCacheFileInfo.clear();
                mCacheFileIndex.clear();
            }
            
            for (auto& file : info)
            {
                if (std::remove(file.mFullFilePath.c_str()) != 0)
                    Hermes::getInstance()->getLogger()->print(ELogLevel::Warning, "Cache clear failed to delete file at '%'", file.mFullFilePath);
            }
        }
    }
    
    /* 
     HEADER FORMAT
     
     5 bytes - beginning of a proper cache file: 01011001 01000111 01000111 01011111 01001000
     8 bytes - timestamp in miliseconds when file was created
     4 bytes - lifetime of cache object in miliseconds
     2 bytes - first four bits are flags; first three bits for future, fourth bit says if content should be treated as string (1) or binary data (0);
               next twelve bits are number of bytes request url will occupy
     
     CONTENT FORMAT
     x bytes - request url
     rest    - content
    */
    
    const char cMagicWord[5] = { 'Y', 'G', 'G', '_', 'H'};
    
    NetworkManager::CacheFileData NetworkManager::decodeCacheHeader(const std::string& pFilePath) const
    {
        CacheFileData info;
        std::ifstream file(pFilePath, std::ifstream::in | std::ifstream::binary);
        
        if (file.is_open())
        {
            const size_t magicWordSize = sizeof(cMagicWord);
            char magicWord[magicWordSize];
            file.read(magicWord, magicWordSize);
            if (!file.good() || file.gcount() != magicWordSize || memcmp(magicWord, cMagicWord, magicWordSize) != 0) return info;
            
            const std::streamsize readSize = sizeof(info.mHeader);
            file.read((char*)&info.mHeader, readSize);
            if (!file.good() || file.gcount() != readSize) return info;
            
            size_t urlSizeSubtract = 0;
            u_int16_t stringMask = 1 << 12;
            if ((stringMask & info.mHeader.mFlagsAndSize) == stringMask)
            {
                urlSizeSubtract += stringMask;
                info.mStringType = true;
            }
            urlSizeSubtract += (1 << 13) & info.mHeader.mFlagsAndSize;
            urlSizeSubtract += (1 << 14) & info.mHeader.mFlagsAndSize;
            urlSizeSubtract += (1 << 15) & info.mHeader.mFlagsAndSize;
            
            u_int16_t urlSize = info.mHeader.mFlagsAndSize - static_cast<u_int16_t>(urlSizeSubtract);
            
            info.mUrl.resize(urlSize);
            file.read(&info.mUrl[0], urlSize);
            if (!file.good() || file.gcount() != urlSize) return info;
            
            info.mFullFilePath = pFilePath;
            info.mIsValid = true;
        }
        
        return info;
    }
    
    NetworkResponse NetworkManager::getResponseFromCache(const std::string& pUrl)
    {
        NetworkResponse response;
        CacheFileData info;
        
        if (mCacheInitialized.load() != 0)
        {
            std::lock_guard<std::mutex> lock(mCacheMutex);
            std::unordered_map<std::string, size_t>::const_iterator fileIndex = mCacheFileIndex.find(pUrl);
            if (fileIndex != mCacheFileIndex.end() && fileIndex->second < mCacheFileInfo.size())
                info = mCacheFileInfo[fileIndex->second];
        }
        
        auto duration = std::chrono::system_clock::now().time_since_epoch();
        u_int64_t timestamp = static_cast<u_int64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(duration).count());
        
        if (info.mIsValid && timestamp >= info.mHeader.mTimestamp && (timestamp - info.mHeader.mTimestamp) < info.mHeader.mLifetime)
        {
            std::ifstream file(info.mFullFilePath, std::ifstream::in | std::ifstream::binary);
            
            if (file.is_open())
            {
                file.seekg(static_cast<std::streamsize>(sizeof(cMagicWord) + sizeof(info.mHeader) + info.mUrl.size()));
                
                if (file.good())
                {
                    std::stringstream ss;
                    ss << file.rdbuf();
                    
                    if (info.mStringType)
                    {
                        response.mMessage = ss.str();
                    }
                    else
                    {
                        std::string content = ss.str();
                        response.mRawData.push_back(content.data(), content.size());
                    }
                    
                    response.mCode = ENetworkCode::OK;
                    response.mHttpCode = mHttpCodeSuccess.first;
                }
            }
        }
        
        return response;
    }
    
    bool NetworkManager::cacheResponse(const NetworkResponse& lpResponse, const std::string& pUrl, u_int32_t pLifetime)
    {
        static const char lookup[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
        
        bool status = false;
        
        size_t maxFileSize = 0;
        size_t dataSize = lpResponse.mRawData.size() > 0 ? lpResponse.mRawData.size() : lpResponse.mMessage.size();
        
        if (dataSize > 0 && mCacheInitialized.load() != 0)
        {
            std::lock_guard<std::mutex> lock(mCacheMutex);
            maxFileSize = mCacheFileSizeLimit;
            dataSize += sizeof(CacheFileData::Header) + pUrl.size();
        }
        
        if (lpResponse.mCode == ENetworkCode::OK && dataSize > 0 && dataSize <= maxFileSize)
        {
            auto duration = std::chrono::system_clock::now().time_since_epoch();
            u_int64_t timestamp = static_cast<u_int64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(duration).count());
            CacheFileData info;
            
            {
                std::lock_guard<std::mutex> lock(mCacheMutex);
#if defined(ANDROID) || defined(__ANDROID__)
                std::stringstream ss;
                ss << timestamp;
                info.mFullFilePath = mCacheDirectoryPath + ss.str() + "_";
#else
                info.mFullFilePath = mCacheDirectoryPath + std::to_string(timestamp) + "_";
#endif
            }
            
            const size_t hashCount = 5;
            const size_t lookupLength = sizeof(lookup);
            size_t pathSize = info.mFullFilePath.size();
            info.mFullFilePath.resize(pathSize + hashCount);
            
            std::uniform_int_distribution<int> dist(0, lookupLength - 1);
            
            for (size_t i = pathSize; i < info.mFullFilePath.size(); i++)
                info.mFullFilePath[i] = lookup[static_cast<size_t>(dist(mRandomGenerator))];
            
            info.mUrl = pUrl;
            info.mHeader.mTimestamp = timestamp;
            info.mHeader.mLifetime = pLifetime;
            info.mIsValid = true;
            
            if (lpResponse.mMessage.size() == 0)
            {
                info.mHeader.mFlagsAndSize = pUrl.size();
            }
            else
            {
                info.mStringType = true;
                info.mHeader.mFlagsAndSize = pUrl.size() + (1 << 12);
            }
            
            std::ofstream file(info.mFullFilePath, std::fstream::out | std::fstream::trunc | std::fstream::binary);
            
            if (file.is_open())
            {
                file.write(cMagicWord, sizeof(cMagicWord));
                if (!file.good()) return status;
                
                file.write(reinterpret_cast<const char*>(&info.mHeader), sizeof(info.mHeader));
                if (!file.good()) return status;
                
                file << pUrl;
                if (!file.good()) return status;
                
                if (info.mStringType)
                    file << lpResponse.mMessage;
                else
                    file.write(reinterpret_cast<const char*>(lpResponse.mRawData.data()), static_cast<std::streamsize>(lpResponse.mRawData.size()));
                
                if (!file.good()) return status;
                
                file.close();
                
                {
                    std::lock_guard<std::mutex> lock(mCacheMutex);
                    if (mCacheFileInfo.size() < mCacheFileCountLimit)
                    {
                        mCacheFileInfo.push_back(std::move(info));
                    }
                    else
                    {
                        mCacheFileIndex.erase(mCacheFileInfo[0].mUrl);
                        
                        std::rotate(mCacheFileInfo.begin(), mCacheFileInfo.begin() + 1, mCacheFileInfo.end());
                        mCacheFileInfo.back() = std::move(info);
                        
                        for (auto it = mCacheFileIndex.begin(); it != mCacheFileIndex.end(); it++)
                            it->second -= 1;
                    }
                    
                    mCacheFileIndex[pUrl] = mCacheFileInfo.size() - 1;
                }
                
                status = true;
            }
        }
        
        return status;
    }
    
    bool NetworkManager::initSimpleSocket(int pThreadPoolID)
    {
        uint32_t initialized = 0;
        
        if (mInitialized.load() != 0 && mSimpleSocketInitialized.compare_exchange_strong(initialized, 1))
        {
            // only one thread for sockets allowed
            assert(Hermes::getInstance()->getTaskManager()->threadCountForPool(pThreadPoolID) == 1);
            mThreadPoolSimpleSocketID = pThreadPoolID;
        }
        else
        {
            assert(false);
        }
        
        return initialized == 0;
    }
    
    void NetworkManager::closeSimpleSocket()
    {
        mStopSimpleSocketLoop.store(1);
        Hermes::getInstance()->getTaskManager()->clearTask(mThreadPoolSimpleSocketID);
    }
    
    const long long cWaitTimeout = 1000;
    void NetworkManager::sendSimpleSocketMessage(const std::string& pMessage, std::function<void(ENetworkCode)> pCallback) const
    {
        std::string message = packSimpleSocketMessage(pMessage);
        
        auto task = [this](std::string lpMessage, std::function<void(ENetworkCode)> lpCallback) -> void
        {
            if (mSimpleSocketCURL != nullptr)
            {
                unsigned tryCount = 0;
                
                curl_socket_t socketfd;
                CURLcode cCode = curl_easy_getinfo(mSimpleSocketCURL, CURLINFO_ACTIVESOCKET, &socketfd);
                
                if (cCode != CURLE_OK)
                {
                    ENetworkCode code = static_cast<ENetworkCode>(static_cast<int>(ENetworkCode::Unknown) + static_cast<int>(cCode));
                    
                    if (lpCallback != nullptr)
                        Hermes::getInstance()->getTaskManager()->execute(-1, lpCallback, code);
                    
                    return;
                }
                
                do
                {
                    if (tryCount != 0)
                        CURL_WAIT_ON_SOCKET(socketfd, 1, cWaitTimeout);
                    
                    size_t sendCount = 0;
                    cCode = curl_easy_send(mSimpleSocketCURL, lpMessage.c_str(), lpMessage.length(), &sendCount);
                    
                    tryCount++;
                }
                while (cCode == CURLE_AGAIN && tryCount < 3);
                
                ENetworkCode code = cCode == CURLE_OK ? ENetworkCode::OK : static_cast<ENetworkCode>(static_cast<int>(ENetworkCode::Unknown) + static_cast<int>(cCode));
                
                if (lpCallback != nullptr)
                    Hermes::getInstance()->getTaskManager()->execute(-1, lpCallback, code);
            }
        };
        
        Hermes::getInstance()->getTaskManager()->execute(mThreadPoolSimpleSocketID, task, std::move(message), std::move(pCallback));
    }
    
    void NetworkManager::requestSimpleSocket(SimpleSocketRequestParam pRequest)
    {
        if (mSimpleSocketInitialized.load() == 0)
            return;
        
        closeSimpleSocket();

        auto request = [this](SimpleSocketRequestParam lpRequest) -> void
        {
            mStopSimpleSocketLoop.store(0);
            
            std::chrono::time_point<std::chrono::system_clock> time = std::chrono::system_clock::now();
            std::chrono::time_point<std::chrono::system_clock> lastMessageTime = std::chrono::system_clock::now();
            
            ESocketDisconnectCause disconnectCause = ESocketDisconnectCause::User;
            
            mSimpleSocketCURL = curl_easy_init();
            
            if (mSimpleSocketCURL == NULL)
            {
                Hermes::getInstance()->getLogger()->print(ELogLevel::Error, "Curl easy init fail for socket.");
                disconnectCause = ESocketDisconnectCause::InitialConnection;
            }
            else
            {
                tools::URLTool url(lpRequest.mURL);
                
                curl_easy_setopt(mSimpleSocketCURL, CURLOPT_URL, url.getHttpURL().c_str());
                curl_easy_setopt(mSimpleSocketCURL, CURLOPT_CONNECT_ONLY, 1L);
                curl_easy_setopt(mSimpleSocketCURL, CURLOPT_SSL_VERIFYPEER, 1L);

                if (mCACertificatePath.size() > 0)
                    curl_easy_setopt(mSimpleSocketCURL, CURLOPT_CAINFO, mCACertificatePath.c_str());
                
                CURLcode res = curl_easy_perform(mSimpleSocketCURL);
                
                if (res != CURLE_OK)
                {
                    Hermes::getInstance()->getLogger()->print(ELogLevel::Error, "Socket connection failure for url '%'. CURL CODE %, message: '%'", lpRequest.mURL, res, curl_easy_strerror(res));
                    disconnectCause = ESocketDisconnectCause::InitialConnection;
                }
                else
                {
                    curl_socket_t socketfd;
                    
                    res = curl_easy_getinfo(mSimpleSocketCURL, CURLINFO_LASTSOCKET, &socketfd);
                    
                    if (res != CURLE_OK)
                    {
                        Hermes::getInstance()->getLogger()->print(ELogLevel::Error, "Socket get info failure for url '%'. CURL CODE %, message: '%'", lpRequest.mURL, res, curl_easy_strerror(res));
                        disconnectCause = ESocketDisconnectCause::InitialConnection;
                    }
                    else
                    {
                        mSimpleSocketActive.store(1);
                        
                        std::string secSocketAccept;
                        res = sendUpgradeHeader(mSimpleSocketCURL, url, lpRequest.mHeader, secSocketAccept);
                        
                        if (res != CURLE_OK)
                        {
                            Hermes::getInstance()->getLogger()->print(ELogLevel::Error, "Socket header message fail for url '%'. CURL CODE %, message: '%'", url.getURL(), res, curl_easy_strerror(res));
                            disconnectCause = ESocketDisconnectCause::Header;
                        }
                        else
                        {
                            const size_t bufferLength = 2048;
                            char buffer[bufferLength];
                            std::string data;
                            bool error = false;
                            bool headerCheck = true;
                            std::vector<SocketFrame> frame;
                            
                            data.reserve(bufferLength);
                            
                            while (!error && Hermes::getInstance()->getTaskManager()->canContinueTask(mThreadPoolSimpleSocketID))
                            {
                                size_t readCount = 0;
                                
                                if (mStopSimpleSocketLoop.load() == 1)
                                {
                                    std::string message = packSimpleSocketMessage("", ESocketOpCode::Close);
                                    
                                    res = curl_easy_send(mSimpleSocketCURL, message.c_str(), message.length(), &readCount);
                                    
                                    if (res != CURLE_OK)
                                        Hermes::getInstance()->getLogger()->print(ELogLevel::Error, "Close send failed. CURLcode %", static_cast<int>(res));
                                    
                                    error = true;
                                    disconnectCause = ESocketDisconnectCause::Close;
                                    
                                    break;
                                }
                                
                                data.clear();
                                
                                do
                                {
                                    readCount = 0;
                                    
                                    res = curl_easy_recv(mSimpleSocketCURL, buffer, bufferLength * sizeof(char), &readCount);
                                    
                                    if (res != CURLE_OK && res != CURLE_AGAIN)
                                    {
                                        Hermes::getInstance()->getLogger()->print(ELogLevel::Error, "Socket read fail for url '%'. CURL CODE %, message: '%'", url.getURL(), res, curl_easy_strerror(res));
                                        
                                        disconnectCause = ESocketDisconnectCause::Other;
                                        error = true;
                                        
                                        break;
                                    }
                                    else if (readCount > 0)
                                    {
                                        data.append(buffer, readCount);
                                    }
                                    else
                                    {
                                        if (!data.empty())
                                            lastMessageTime = std::chrono::system_clock::now();
                                            
                                        break;
                                    }
                                }
                                while (true);
                                
                                if (!error)
                                {
                                    if (!data.empty())
                                    {
                                        if (headerCheck)
                                        {
                                            headerCheck = false;
                                            
                                            const std::string acceptKey = "Sec-WebSocket-Accept: ";
                                            size_t keyPos = data.find(acceptKey);
                                            size_t codePos = data.find("HTTP/1.1 101 Switching Protocols");
                                            
                                            if (codePos != std::string::npos && keyPos != std::string::npos && data.length() >= keyPos + acceptKey.length() + secSocketAccept.length() && data.substr(keyPos + acceptKey.length(), secSocketAccept.length()).compare(secSocketAccept) == 0)
                                            {
                                                if (lpRequest.mConnectCallback != nullptr)
                                                    Hermes::getInstance()->getTaskManager()->execute(-1, lpRequest.mConnectCallback);
                                            }
                                            else
                                            {
                                                Hermes::getInstance()->getLogger()->print(ELogLevel::Error, "Socket invalid handshake.");
                                                error = true;
                                                disconnectCause = ESocketDisconnectCause::Handshake;
                                            }
                                        }
                                        else
                                        {
                                            auto newFrame = unpackSimpleSocketMessage(data);
                                            
                                            if (!handleSimpleSocketFrame(mSimpleSocketCURL, newFrame, frame, lpRequest.mMessageCallback))
                                            {
                                                error = true;
                                                disconnectCause = ESocketDisconnectCause::Close;
                                            }
                                        }
                                    }
                                    
                                    if (lpRequest.mTimeout == std::chrono::milliseconds::zero() || (lpRequest.mTimeout > std::chrono::milliseconds::zero() && std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - lastMessageTime) < lpRequest.mTimeout))
                                    {
                                        CURL_WAIT_ON_SOCKET(socketfd, 1, cWaitTimeout);
                                    }
                                    else
                                    {
                                        error = true;
                                        disconnectCause = ESocketDisconnectCause::Timeout;
                                    }
                                    
                                    if (!error)
                                        Hermes::getInstance()->getTaskManager()->performTaskIfExists(mThreadPoolSimpleSocketID);
                                }
                            }
                        }
                    }
                }
                
                
                curl_easy_cleanup(mSimpleSocketCURL);
                mSimpleSocketCURL = nullptr;
            }
            
            mSimpleSocketActive.store(0);
                        
            if (lpRequest.mDisconnectCallback != nullptr)
                Hermes::getInstance()->getTaskManager()->execute(-1, lpRequest.mDisconnectCallback, disconnectCause, std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - time));
        };
        
        Hermes::getInstance()->getTaskManager()->execute(mThreadPoolSimpleSocketID, request, std::move(pRequest));
    }
    
    CURLcode NetworkManager::sendUpgradeHeader(CURL* const pCurl, const tools::URLTool& pURL, const std::vector<std::pair<std::string, std::string>>& pHeader, std::string& pSecAccept) const
    {
        CURLcode code = CURLE_OK;
        
        std::string key = tools::getRandomCryptoBytes(16);
        key = tools::encodeBase64(reinterpret_cast<const unsigned char*>(key.data()), key.size());
        
        std::string shaAccept = tools::getSHA1Digest(key + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11");
        pSecAccept = tools::encodeBase64(reinterpret_cast<const unsigned char*>(shaAccept.data()), shaAccept.size());
        
        std::string header = "GET ";
        header.reserve(1024);
        
        size_t pathLength = 0;
        const char* path = pURL.getPath(pathLength);
        header.append(path, pathLength);
        
        header += " HTTP/1.1\r\nHost: ";
        
        size_t hostLength = 0;
        const char* host = pURL.getHost(hostLength, true);
        header.append(host, hostLength);
        
        header += "\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Key: " + key + "\r\nSec-WebSocket-Version: 13\r\nOrigin: " + pURL.getHttpURL(false) + "\r\n";
        
        for (auto& v : pHeader)
            header += v.first + ": " + v.second + "\r\n";
        
        header += "\r\n";
        header.shrink_to_fit();
        
        size_t sendCount = 0;
        code = curl_easy_send(pCurl, header.c_str(), header.length(), &sendCount);
        
        return code;
    }
    
    std::string NetworkManager::packSimpleSocketMessage(const std::string& pMessage, ESocketOpCode pOpCode) const
    {
        std::string message;
        
        uint8_t codes = static_cast<uint8_t>(pOpCode);
        uint8_t payloadLength = 0x80;
        uint8_t mask[4];
        uint16_t payloadLength16 = 0;
        uint64_t payloadLength64 = 0;
        
        if (pMessage.length() <= 125)
        {
            payloadLength |= static_cast<uint8_t>(pMessage.length());
            
            message.reserve(sizeof(codes) + sizeof(payloadLength) + sizeof(mask) + pMessage.length());
        }
        else if (pMessage.length() <= std::numeric_limits<uint16_t>::max())
        {
            payloadLength |= 126;
            payloadLength16 = pMessage.length();
            payloadLength16 = tools::isLittleEndian() ? tools::byteSwap16(payloadLength16) : payloadLength16;
            
            message.reserve(sizeof(codes) + sizeof(payloadLength) + sizeof(payloadLength16) + sizeof(mask) + pMessage.size());
        } else {
            payloadLength |= 127;
            payloadLength64 = pMessage.length();
            payloadLength64 = tools::isLittleEndian() ? tools::byteSwap64(payloadLength64) : payloadLength64;
            
            message.reserve(sizeof(codes) + sizeof(payloadLength) + sizeof(payloadLength64) + sizeof(mask) + pMessage.size());
        }
        
        message.append(reinterpret_cast<const char*>(&codes), sizeof(codes));
        message.append(reinterpret_cast<const char*>(&payloadLength), sizeof(payloadLength));
        
        if (payloadLength16 > 0)
            message.append(reinterpret_cast<const char*>(&payloadLength16), sizeof(payloadLength16));
        else if (payloadLength64 > 0)
            message.append(reinterpret_cast<const char*>(&payloadLength64), sizeof(payloadLength64));
        
        memcpy(mask, tools::getRandomCryptoBytes(4).data(), sizeof(mask));
        message.append(reinterpret_cast<const char*>(mask), sizeof(mask));
        
        if (pMessage.length() > 0)
        {
            uint8_t maskedMessage[pMessage.size()];
            memcpy(maskedMessage, pMessage.data(), pMessage.size());
            
            for (size_t i = 0; i < pMessage.size(); i++)
                maskedMessage[i] ^= mask[i % 4];
            
            message.append(reinterpret_cast<const char*>(maskedMessage), pMessage.size());
        }
        
        return message;
    }
    
    /* Any scenario when received anything other than single unmasked full frame (no continuation frames) is not tested */
    std::vector<NetworkManager::SocketFrame> NetworkManager::unpackSimpleSocketMessage(const std::string& pMessage) const
    {
        if (pMessage.length() == 0) return {};
        
        std::vector<SocketFrame> frames;
        
        const bool isLittleEndian = tools::isLittleEndian();
        
        bool loopContinue = false;
        size_t frameOffset = 0;
        
        do
        {
            SocketFrame frame;
            
            if (frameOffset + sizeof(frame.mCode) + sizeof(frame.mPayloadLength) > pMessage.length())
            {
                Hermes::getInstance()->getLogger()->print(ELogLevel::Error, "Socket frame parsing error. Message smaller than minimal header. Message length %, frameOffset %", pMessage.length(), frameOffset);
                
                break;
            }
            
            memcpy((char*)&frame, pMessage.data() + frameOffset, sizeof(frame.mCode) + sizeof(frame.mPayloadLength));
            
            bool useMask = frame.mPayloadLength & 0x80;
            
            size_t offset = sizeof(frame.mCode) + sizeof(frame.mPayloadLength);
            uint64_t payloadSize = 0;
            
            if (frame.mPayloadLength <= 125)
            {
                payloadSize = frame.mPayloadLength & 0x7F;
            }
            else if (frame.mPayloadLength == 126)
            {
                uint16_t length = 0;
                
                if (frameOffset + offset + sizeof(length) > pMessage.length())
                {
                    Hermes::getInstance()->getLogger()->print(ELogLevel::Error, "Socket frame parsing error. Message smaller than extended header. Message length %, frameOffset %", pMessage.length(), frameOffset);
                    
                    break;
                }
                
                memcpy((char*)&length, pMessage.data() + frameOffset + offset, sizeof(length));
                payloadSize = frame.mPayloadLengthExtended = isLittleEndian ? tools::byteSwap16(length) : length;
                
                offset += sizeof(length);
            }
            else
            {
                if (frameOffset + offset + sizeof(frame.mPayloadLengthExtended) > pMessage.length())
                {
                    Hermes::getInstance()->getLogger()->print(ELogLevel::Error, "Socket frame parsing error. Message smaller than super extended header. Message length %, frameOffset %", pMessage.length(), frameOffset);
                    break;
                }
                
                memcpy((char*)&frame.mPayloadLengthExtended, pMessage.data() + frameOffset + offset, sizeof(frame.mPayloadLengthExtended));
                payloadSize = frame.mPayloadLengthExtended = isLittleEndian ? tools::byteSwap64(frame.mPayloadLengthExtended) : frame.mPayloadLengthExtended;
                
                offset += sizeof(frame.mPayloadLengthExtended);
            }
            
            if (useMask)
            {
                if (frameOffset + offset + sizeof(frame.mMaskingKey) > pMessage.length())
                {
                    Hermes::getInstance()->getLogger()->print(ELogLevel::Error, "Socket frame parsing error. Message smaller than header with mask. Message length %, frameOffset %, offset %, payload size %", pMessage.length(), frameOffset, offset, payloadSize);
                    
                    break;
                }
                
                memcpy((char*)&frame.mMaskingKey, pMessage.data() + frameOffset + offset, sizeof(frame.mMaskingKey));
                offset += sizeof(frame.mMaskingKey);
            }
            
            if (pMessage.length() - (frameOffset + offset) < payloadSize)
            {
                Hermes::getInstance()->getLogger()->print(ELogLevel::Error, "Socket frame parsing error. Payload bigger than message. Message length %, frameOffset %, offset %, payload size %", pMessage.length(), frameOffset, offset, payloadSize);
                
                break;
            }
            
            if (!useMask)
            {
                frame.mPayload = pMessage.substr(frameOffset + offset, static_cast<size_t>(payloadSize));
            }
            else
            {
                uint8_t message[payloadSize];
                memcpy(message, pMessage.data() + frameOffset + offset, static_cast<size_t>(payloadSize));
                
                for (size_t i = 0; i < payloadSize; i++)
                    message[i] ^= frame.mMaskingKey[i % 4];
                
                frame.mPayload.append(reinterpret_cast<const char*>(message), static_cast<size_t>(payloadSize));
            }
                        
            if (pMessage.length() - (frameOffset + offset) == payloadSize)
            {
                loopContinue = false;
            }
            else
            {
                loopContinue = true;
                frameOffset += offset + payloadSize;
            }
            
            frames.push_back(std::move(frame));
        }
        while (loopContinue);
        
        return frames;
    }
    
    bool NetworkManager::handleSimpleSocketFrame(CURL* const pCurl, std::vector<SocketFrame>& pNewFrame, std::vector<SocketFrame>& pOldFrame, std::function<void(std::string lpMessage, bool lpTextData)> pMessageCallback) const
    {
        bool endSent = false;
        
        for (auto it = pNewFrame.rbegin(); it != pNewFrame.rend(); it++)
        {
            uint8_t opCode = it->mCode & 0x0F;
            
            if (static_cast<ESocketOpCode>(opCode) == ESocketOpCode::Ping)
            {
                it = std::vector<SocketFrame>::reverse_iterator(pNewFrame.erase(std::next(it).base()));
                
                if (!endSent)
                {
                    std::string message = packSimpleSocketMessage("", ESocketOpCode::Pong);
                    size_t sendCount = 0;
                    
                    if (pCurl != nullptr)
                    {
                        CURLcode code = curl_easy_send(mSimpleSocketCURL, message.c_str(), message.length(), &sendCount);
                        
                        if(code != CURLE_OK)
                            Hermes::getInstance()->getLogger()->print(ELogLevel::Error, "Pong send failed. CURLcode %", static_cast<int>(code));
                    }
                }
            }
            else if (static_cast<ESocketOpCode>(opCode) == ESocketOpCode::Close)
            {
                it = std::vector<SocketFrame>::reverse_iterator(pNewFrame.erase(std::next(it).base()));
                
                std::string message = packSimpleSocketMessage("", ESocketOpCode::Close);
                
                if (pCurl != nullptr)
                {
                    size_t sendCount = 0;
                    CURLcode code = curl_easy_send(mSimpleSocketCURL, message.c_str(), message.length(), &sendCount);
                    
                    if(code != CURLE_OK)
                        Hermes::getInstance()->getLogger()->print(ELogLevel::Error, "Close response send failed. CURLcode %", static_cast<int>(code));
                }
                endSent = true;
            }
            else if (opCode > static_cast<uint8_t>(ESocketOpCode::Binary))
            {
                it = std::vector<SocketFrame>::reverse_iterator(pNewFrame.erase(std::next(it).base()));
            }
        }
        
        pOldFrame.insert(pOldFrame.end(), pNewFrame.begin(), pNewFrame.end());
        
        ESocketOpCode multiFrameOpCode = ESocketOpCode::Continue;
        std::vector<SocketFrame>::iterator firstFrameIt;
        std::string multiMessage;
        
        for (auto it = pOldFrame.begin(); it != pOldFrame.end();)
        {
            const bool fin = it->mCode & 0x80;
            const uint8_t opCode = it->mCode & 0x0F;
            
            if (!fin)
            {
                multiFrameOpCode = static_cast<ESocketOpCode>(opCode);
                multiMessage += std::move(it->mPayload);
                
                firstFrameIt = it++;
            }
            else if (static_cast<ESocketOpCode>(opCode) != ESocketOpCode::Continue)
            {
                if (pMessageCallback != nullptr)
                    Hermes::getInstance()->getTaskManager()->execute(-1, pMessageCallback, std::move(it->mPayload), static_cast<ESocketOpCode>(opCode) == ESocketOpCode::Text);
                
                it = pOldFrame.erase(it);
            }
            else
            {
                multiMessage += std::move(it->mPayload);
                
                if (pMessageCallback != nullptr)
                    Hermes::getInstance()->getTaskManager()->execute(-1, pMessageCallback, std::move(multiMessage), multiFrameOpCode == ESocketOpCode::Text);
                
                multiMessage = "";
                
                it = pOldFrame.erase(firstFrameIt, it + 1);
            }
        }
        
        return !endSent;
    }

}
