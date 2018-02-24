// Copyright (C) 2017 Grupa Pracuj Sp. z o.o.
// This file is part of the "Hermes" library.
// For conditions of distribution and use, see copyright notice in license.txt.

#include "network.hpp"

#include "hermes.hpp"
#include "tools.hpp"
#include "curl/curl.h"

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
    
    static int CURL_PROGRESS_CALLBACK(NetworkManager::ProgressData* pUserData, curl_off_t pDownloadTotal, curl_off_t pDownloadNow, curl_off_t pUploadTotal, curl_off_t pUploadNow)
    {
        int result = 0;
    
        if (pUserData != nullptr)
        {
            result = static_cast<int>(pUserData->mTerminateAbort->load()) + static_cast<int>(pUserData->mRequestHandle->isCancel());

            if (pUserData->mProgressTask != nullptr)
                pUserData->mProgressTask(pDownloadNow, pDownloadTotal, pUploadNow, pUploadTotal);
        }
        
        return result;
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
    
    /* NetworkRequestHandle */
    
    void NetworkRequestHandle::cancel()
    {
        mCancel.store(1);
    }
    
    bool NetworkRequestHandle::isCancel() const
    {
        return static_cast<bool>(mCancel.load());
    }
    
    /* NetworkRecovery */
    
    NetworkRecovery::NetworkRecovery(size_t pId, std::string pName, Config pConfig) : mId(pId), mName(std::move(pName))
    {
        assert(pConfig.mCondition != nullptr && pConfig.mCallback != nullptr);

        auto internalData = std::shared_ptr<InternalData>(new InternalData());
        mInternalData = internalData;
        mCondition = std::move(pConfig.mCondition);

        auto callback = [lpCallback = std::move(pConfig.mCallback), internalData](NetworkResponse lpResponse) -> void
        {
            internalData->mActive = false;

            Hermes::getInstance()->getLogger()->print(ELogLevel::Info, "Recovery: %, code: %, http: %, message: %", (lpResponse.mCode == ENetworkCode::OK) ? "passed" : "failed", static_cast<unsigned>(lpResponse.mCode), lpResponse.mHttpCode, lpResponse.mMessage);

            if (lpCallback != nullptr)
            {
                NetworkResponse response;
                response.mCode = lpResponse.mCode;
                response.mHttpCode = lpResponse.mHttpCode;
                response.mHeader = std::move(lpResponse.mHeader);
                response.mMessage = std::move(lpResponse.mMessage);
                response.mRawData = std::move(lpResponse.mRawData);

                lpCallback(std::move(response));
            }
            
            while (internalData->mReceiver.size() != 0)
            {
                auto receiver = &internalData->mReceiver.front();                
                auto networkManager = Hermes::getInstance()->getNetworkManager();
                auto networkAPI = (std::get<0>(*receiver) < networkManager->count()) ? networkManager->get(std::get<0>(*receiver)) : nullptr;

                if (networkAPI != nullptr && lpResponse.mCode == ENetworkCode::OK)
                {
                    NetworkRequestParam* param = &std::get<1>(*receiver);

                    auto defaultHeader = networkAPI->getDefaultHeader();
                    
                    param->mMethod = networkAPI->getUrl() + param->mMethod;
                    param->mHeader.insert(param->mHeader.end(), defaultHeader.begin(), defaultHeader.end());

                    Hermes::getInstance()->getLogger()->print(ELogLevel::Info, "Executed method from recovery: %.", param->mMethod);

                    networkManager->request(std::move(*param), std::get<2>(*receiver));
                }
                
                internalData->mReceiver.pop();
            }
        };

        mRequestParam.mRequestType = pConfig.mRequestType;
        mRequestParam.mResponseType = ENetworkResponseType::Message;
        mRequestParam.mMethod = std::move(pConfig.mUrl);
        mRequestParam.mParameter = std::move(pConfig.mParameter);
        mRequestParam.mHeader = std::move(pConfig.mHeader);
        mRequestParam.mRequestBody = "";
        mRequestParam.mCallback = std::move(callback);
        mRequestParam.mProgress = nullptr;
        mRequestParam.mRepeatCount = pConfig.mRepeatCount;
        mRequestParam.mAllowRecovery = false;
    }
    
    NetworkRecovery::~NetworkRecovery()
    {
    }
    
    bool NetworkRecovery::isQueueEmpty() const
    {
        return mInternalData->mReceiver.empty();
    }
    
    bool NetworkRecovery::checkCondition(const NetworkResponse& pResponse, std::string& pRequestBody, std::vector<std::pair<std::string, std::string>>& pParameter, std::vector<std::pair<std::string, std::string>>& pHeader) const
    {
        return mCondition != nullptr && mCondition(pResponse, pRequestBody, pParameter, pHeader);
    }
    
    void NetworkRecovery::pushReceiver(std::tuple<size_t, NetworkRequestParam, std::shared_ptr<NetworkRequestHandle>> pReceiver)
    {
        mInternalData->mReceiver.push(std::move(pReceiver));
    }
    
    void NetworkRecovery::runRequest(std::string pRequestBody, std::vector<std::pair<std::string, std::string>> pParameter, std::vector<std::pair<std::string, std::string>> pHeader)
    {
        int defaultParameterEnd = -1;
        int defaultHeaderEnd = -1;

        mRequestParam.mRequestBody = std::move(pRequestBody);
        
        if (pParameter.size() != 0)
        {
            defaultParameterEnd = static_cast<int>(mRequestParam.mParameter.size());
            mRequestParam.mParameter.insert(mRequestParam.mParameter.cend(), pParameter.cbegin(), pParameter.cend());
        }
        
        if (pHeader.size() != 0)
        {
            defaultHeaderEnd = static_cast<int>(mRequestParam.mHeader.size());
            mRequestParam.mHeader.insert(mRequestParam.mHeader.cend(), pHeader.cbegin(), pHeader.cend());
        }
        
        Hermes::getInstance()->getNetworkManager()->request(mRequestParam);
        
        if (defaultParameterEnd >= 0)
            mRequestParam.mParameter.erase(mRequestParam.mParameter.cbegin() + defaultParameterEnd, mRequestParam.mParameter.cend());
        
        if (defaultHeaderEnd >= 0)
            mRequestParam.mHeader.erase(mRequestParam.mHeader.cbegin() + defaultHeaderEnd, mRequestParam.mHeader.cend());
    }
    
    bool NetworkRecovery::isActive() const
    {
        return mInternalData->mActive;
    }
    
    bool NetworkRecovery::activate()
    {
        bool status = !mInternalData->mActive;
        mInternalData->mActive = true;            
        
        return status;
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

    size_t NetworkRecovery::getId() const
    {
        return mId;
    }
    
    const std::string& NetworkRecovery::getName() const
    {
        return mName;
    }
    
    /* NetworkAPI */
    
    NetworkAPI::NetworkAPI(size_t pId, std::string pName, std::string pUrl) :  mId(pId), mName(std::move(pName)), mUrl(std::move(pUrl))
    {
        mCallbackContinious = decltype(mCallbackContinious)(new std::vector<std::pair<std::function<void(const NetworkResponse&)>, std::string>>());
    }
    
    NetworkAPI::~NetworkAPI()
    {
    }

    std::shared_ptr<NetworkRequestHandle> NetworkAPI::request(NetworkRequestParam pParam)
    {
        std::shared_ptr<NetworkRequestHandle> requestHandle = std::make_shared<NetworkRequestHandle>();
        auto callbackContinious = mCallbackContinious;
    
        pParam.mCallback = [responseCallback = pParam.mCallback, callbackContinious](NetworkResponse lpResponse) -> void
        {
            for (const auto& v : *callbackContinious)
            {
                if (v.first != nullptr)
                    v.first(lpResponse);
            }
            
            if (responseCallback != nullptr)
            {
                NetworkResponse response;
                response.mCode = lpResponse.mCode;
                response.mHttpCode = lpResponse.mHttpCode;
                response.mHeader = std::move(lpResponse.mHeader);
                response.mMessage = std::move(lpResponse.mMessage);
                response.mRawData = std::move(lpResponse.mRawData);
                
                responseCallback(std::move(response));
            }
        };

        std::string url = mUrl + pParam.mMethod;
        auto recovery = mRecovery.lock();

        if (pParam.mAllowRecovery && recovery != nullptr)
        {
            size_t copyId = mId;
            pParam.mCallback = [recovery, copyId, url, pParam, requestHandle](NetworkResponse lpResponse) -> void
            {
                std::string requestBody = "";
                std::vector<std::pair<std::string, std::string>> parameter;
                std::vector<std::pair<std::string, std::string>> header;
                
                if (recovery->isActive() || recovery->checkCondition(lpResponse, requestBody, parameter, header))
                {
                    recovery->pushReceiver(std::make_tuple(copyId, std::move(pParam), requestHandle));

                    if (recovery->activate())
                        recovery->runRequest(std::move(requestBody), std::move(parameter), std::move(header));

                    Hermes::getInstance()->getLogger()->print(ELogLevel::Info, "Queued method for recovery: %.", url);
                }
                else
                {
                    pParam.mCallback(std::move(lpResponse));
                }
            };
        }

        if (recovery != nullptr && recovery->isActive())
        {
            recovery->pushReceiver(std::make_tuple(mId, pParam, requestHandle));

            Hermes::getInstance()->getLogger()->print(ELogLevel::Info, "Queued method for recovery: %.", url);
        }
        else
        {
            pParam.mMethod = std::move(url);
            pParam.mHeader.insert(pParam.mHeader.end(), mDefaultHeader.begin(), mDefaultHeader.end());
        
            Hermes::getInstance()->getNetworkManager()->request(std::move(pParam), requestHandle);
        }
        
        return requestHandle;
    }
    
    const std::vector<std::pair<std::string, std::string>>& NetworkAPI::getDefaultHeader() const
    {
        return mDefaultHeader;
    }
    
    void NetworkAPI::setDefaultHeader(std::vector<std::pair<std::string, std::string>> pDefaultHeader)
    {
        mDefaultHeader = std::move(pDefaultHeader);
    }
    
    bool NetworkAPI::registerCallback(std::function<void(const NetworkResponse&)> pCallback, std::string pName, bool pOverwrite)
    {
        auto checkItem = [pName](const std::pair<std::function<void(const NetworkResponse&)>, std::string>& lpElement) -> bool
        {
            if (lpElement.second == pName)
                return true;
            
            return false;
        };
        
        auto it = std::find_if(mCallbackContinious->begin(), mCallbackContinious->end(), checkItem);
        
        bool status = false;
    
        if (it == mCallbackContinious->end())
        {
            mCallbackContinious->push_back(std::make_pair(std::move(pCallback), std::move(pName)));
            status = true;
        }
        else if (pOverwrite)
        {
            *it = std::make_pair(std::move(pCallback), std::move(pName));
            status = true;
        }
        
        return status;
    }

    bool NetworkAPI::unregisterCallback(std::string pName)
    {
        bool status = false;
        
        auto checkItem = [&status, pName](const std::pair<std::function<void(const NetworkResponse&)>, std::string>& lpElement) -> bool
        {
            if (lpElement.second == pName)
            {
                status = true;
                return status;
            }
            
            return false;
        };
        
        mCallbackContinious->erase(remove_if(mCallbackContinious->begin(), mCallbackContinious->end(), checkItem), mCallbackContinious->end());
        
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

    const std::string& NetworkAPI::getUrl() const
    {
        return mUrl;
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
    
    bool NetworkManager::initialize(long pTimeout, int pThreadPoolID, std::pair<int, int> pHttpCodeSuccess)
    {
        uint32_t initialized = 0;

        if (mInitialized.compare_exchange_strong(initialized, 1))
        {
            if (curl_global_init(CURL_GLOBAL_ALL) == CURLE_OK)
            {
                {
                    std::lock_guard<std::mutex> lock(mRequestSettingsMutex);
                    mRequestSettings.mHttpCodeSuccess = pHttpCodeSuccess;
                    mRequestSettings.mTimeout = pTimeout;
                }
                
                mThreadPoolID = pThreadPoolID;

                mInitialized.store(2);
            }
            else
            {
                initialized = 1;
                mInitialized.store(0);
            }

        }
    
        return initialized == 0;
    }
    
    bool NetworkManager::terminate()
    {
        uint32_t terminated = 2;

        if (mInitialized.compare_exchange_strong(terminated, 1))
        {
            // finish all network operations
            
            mTerminateAbort.store(1);
            
            Hermes::getInstance()->getTaskManager()->flush(mThreadPoolID);
            Hermes::getInstance()->getTaskManager()->flush(mThreadPoolSimpleSocketID);
            
            while (mActivityCount.load() > 0)
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            
            {
                std::lock_guard<std::mutex> lock(mApiMutex);
                mApi.clear();
            }
            
            {
                std::lock_guard<std::mutex> lock(mRecoveryMutex);
                mRecovery.clear();
            }

            {
                std::lock_guard<std::mutex> lock(mCacheMutex);
                mCacheFileInfo.clear();
                mCacheFileIndex.clear();
            }

            {
                std::lock_guard<std::mutex> lock(mHandleMutex);
            
                for (auto& v : mHandle)
                {
                    if (v.second != nullptr)
                        curl_easy_cleanup(v.second);
                }
                
                mHandle.clear();
            }
                
            {
                std::lock_guard<std::mutex> lock(mMultiHandleMutex);
            
                for (auto& v : mMultiHandle)
                {
                    if (v.second != nullptr)
                        curl_multi_cleanup(v.second);
                }
                
                mMultiHandle.clear();
            }

            curl_global_cleanup();
            
            {
                std::lock_guard<std::mutex> lock(mRequestSettingsMutex);
                mRequestSettings.mHttpCodeSuccess = {200, 299};
                mRequestSettings.mFlag = {false, false, false};
                mRequestSettings.mCACertificatePath = "";
                mRequestSettings.mTimeout = 0;
                mRequestSettings.mProgressTimePeriod = 0;
            }

            mThreadPoolID = mThreadPoolSimpleSocketID = -1;
            
            mCacheInitialized.store(0);
            mSimpleSocketInitialized.store(0);
            mStopSimpleSocketLoop.store(0);
            mSimpleSocketActive.store(0);
            mTerminateAbort.store(0);

            mInitialized.store(0);
        }
        
        return terminated == 2;
    }
        
    std::shared_ptr<NetworkAPI> NetworkManager::add(size_t pId, std::string pName, std::string pUrl)
    {
        std::shared_ptr<NetworkAPI> result = nullptr;
        mActivityCount++;    
        if (mInitialized.load() == 2)
        {
            result = std::shared_ptr<NetworkAPI>(new NetworkAPI(pId, std::move(pName), std::move(pUrl)), std::bind(&NetworkManager::deleterNetworkAPI, this, std::placeholders::_1));
        
            std::lock_guard<std::mutex> lock(mApiMutex);

            if (pId >= mApi.size())
                mApi.resize(pId + 1);

            assert(mApi[pId] == nullptr);

            mApi[pId] = result;
        }        
        mActivityCount--;
        
        return result;
    }
    
    size_t NetworkManager::count() const
    {
        std::lock_guard<std::mutex> lock(mApiMutex);
        return mApi.size();
    }
    
    std::shared_ptr<NetworkAPI> NetworkManager::get(size_t pId) const
    {
        std::lock_guard<std::mutex> lock(mApiMutex);
        assert(pId < mApi.size());

        return mApi[pId];
    }
    
    std::shared_ptr<NetworkRecovery> NetworkManager::addRecovery(size_t pId, std::string pName, NetworkRecovery::Config pConfig)
    {
        std::shared_ptr<NetworkRecovery> result = nullptr;
        mActivityCount++;    
        if (mInitialized.load() == 2)
        {
            result = std::shared_ptr<NetworkRecovery>(new NetworkRecovery(pId, std::move(pName), std::move(pConfig)), std::bind(&NetworkManager::deleterNetworkRecovery, this, std::placeholders::_1));
        
            std::lock_guard<std::mutex> lock(mRecoveryMutex);

            if (pId >= mRecovery.size())
                mRecovery.resize(pId + 1);

            assert(mRecovery[pId] == nullptr);

            mRecovery[pId] = result;
        }
        mActivityCount--;
        
        return mRecovery[pId];
    }
    
    size_t NetworkManager::countRecovery() const
    {
        std::lock_guard<std::mutex> lock(mRecoveryMutex);
        return mRecovery.size();
    }
    
    std::shared_ptr<NetworkRecovery> NetworkManager::getRecovery(size_t pId) const
    {
        std::lock_guard<std::mutex> lock(mRecoveryMutex);
        assert(pId < mRecovery.size());

        return mRecovery[pId];
    }
    
    void NetworkManager::request(NetworkRequestParam pParam, std::shared_ptr<NetworkRequestHandle> pRequestHandle)
    {
        if (pRequestHandle == nullptr)
            pRequestHandle = std::make_shared<NetworkRequestHandle>();
        
        auto weakThis = mWeakThis;
        auto requestTask = [weakThis, pRequestHandle](NetworkRequestParam lpParam, RequestSettings lpRequestSettings) -> void
        {
            std::shared_ptr<NetworkManager> strongThis = weakThis.lock();
            if (strongThis != nullptr)
            {
                strongThis->mActivityCount++;
                if (strongThis->mInitialized.load() == 2)
                {
                    if (lpParam.mAllowCache)
                    {
                        NetworkResponse response = strongThis->getResponseFromCache(lpParam.mMethod, lpRequestSettings.mHttpCodeSuccess.first);
                        
                        if (response.mCode == ENetworkCode::OK)
                        {
                            std::pair<decltype(lpParam.mCallback), decltype(response)> taskData = std::make_pair(std::move(lpParam.mCallback), std::move(response));
                            auto taskHandler = std::make_shared<decltype(taskData)>(std::move(taskData));
                            Hermes::getInstance()->getTaskManager()->execute(-1, [taskHandler]() -> void
                            {
                                auto taskCallback = std::move(taskHandler->first);
                                taskCallback(std::move(taskHandler->second));
                            });
                            
                            strongThis->mActivityCount--;
                            
                            return;
                        }
                    }

                    ENetworkCode code = ENetworkCode::Unknown;
                    long httpCode = -1;
                    std::vector<std::pair<std::string, std::string>> responseHeader;
                    std::string responseMessage = "";
                    DataBuffer responseRawData;
                    
                    auto uniqueHeader = strongThis->createUniqueHeader(lpParam.mHeader);
                    curl_slist* header = nullptr;

                    for (auto& v : uniqueHeader)
                    {
                        std::string singleHeader = v.first;
                        singleHeader += ": ";
                        singleHeader += v.second;
                        
                        header = curl_slist_append(header, singleHeader.c_str());
                    }

                    void* handle = nullptr;
                    
                    {
                        std::lock_guard<std::mutex> lock(strongThis->mHandleMutex);
                        handle = strongThis->mHandle[std::this_thread::get_id()];
                    }
                    
                    if (handle == nullptr)
                    {
                        handle = curl_easy_init();
                        curl_easy_setopt(handle, CURLOPT_HEADERFUNCTION, CURL_HEADER_CALLBACK);

                        std::lock_guard<std::mutex> lock(strongThis->mHandleMutex);
                        strongThis->mHandle[std::this_thread::get_id()] = handle;
                    }
                    
                    char errorBuffer[CURL_ERROR_SIZE];
                    errorBuffer[0] = 0;

                    ProgressData progressData;
                    progressData.mRequestHandle = pRequestHandle;
                    progressData.mTerminateAbort = &strongThis->mTerminateAbort;
                    
                    if (lpParam.mProgress != nullptr)
                    {
                        decltype(lpParam.mProgress) progress = std::move(lpParam.mProgress);
                        decltype(lpRequestSettings.mProgressTimePeriod) progressTimePeriod = lpRequestSettings.mProgressTimePeriod;
                        auto lastTick = std::chrono::system_clock::now();
                        progressData.mProgressTask = [progressTimePeriod, progress, lastTick]
                            (const long long& lpDN, const long long& lpDT, const long long& lpUN, const long long& lpUT) mutable -> void
                        {
                            auto thisTick = std::chrono::system_clock::now();
                            const auto difference = std::chrono::duration_cast<std::chrono::milliseconds>(thisTick - lastTick).count();
                            
                            if (difference > progressTimePeriod || (lpDN != 0 && lpDN == lpDT) || (lpUN != 0 && lpUN == lpUT))
                            {
                                lastTick = std::chrono::system_clock::now();
                                Hermes::getInstance()->getTaskManager()->execute(-1, progress, lpDN, lpDT, lpUN, lpUT);
                            }
                        };
                    }
                    
                    std::string requestUrl = std::move(lpParam.mMethod);
                    strongThis->appendParameter(requestUrl, lpParam.mParameter);
                    
                    strongThis->configureHandle(handle, lpParam.mRequestType, lpParam.mResponseType, requestUrl, lpParam.mRequestBody, &responseMessage, &responseRawData, &responseHeader, header,
                        lpRequestSettings.mTimeout, lpRequestSettings.mFlag, &progressData, errorBuffer, lpRequestSettings.mCACertificatePath);
                    
                    CURLcode curlCode = CURLE_OK;
                    unsigned step = 0;
                    unsigned terminateAbort = 0;

                    do
                    {
                        curlCode = curl_easy_perform(handle);
                        step++;
                    }
                    while ((terminateAbort = strongThis->mTerminateAbort.load()) == 0 && curlCode != CURLE_OK && step <= lpParam.mRepeatCount);

                    curl_slist_free_all(header);

                    strongThis->resetHandle(handle);
                    
                    if (terminateAbort == 0)
                    {
                        switch (curlCode)
                        {
                        case CURLE_OK:
                            curl_easy_getinfo (handle, CURLINFO_RESPONSE_CODE, &httpCode);
                            code = httpCode >= lpRequestSettings.mHttpCodeSuccess.first && httpCode <= lpRequestSettings.mHttpCodeSuccess.second ? ENetworkCode::OK : ENetworkCode::InvalidHttpCodeRange;
                            break;
                        case CURLE_COULDNT_RESOLVE_HOST:
                            code = ENetworkCode::LostConnection;
                            Hermes::getInstance()->getLogger()->print(ELogLevel::Warning, "Lost connection: %", requestUrl);
                            break;
                        case CURLE_OPERATION_TIMEDOUT:
                            code = ENetworkCode::Timeout;
                            Hermes::getInstance()->getLogger()->print(ELogLevel::Warning, "Timeout: %", requestUrl);
                            break;
                        case CURLE_ABORTED_BY_CALLBACK:
                            code = ENetworkCode::Cancel;
                            Hermes::getInstance()->getLogger()->print(ELogLevel::Warning, "Cancel: %", requestUrl);
                            break;
                        default:
                            code = static_cast<ENetworkCode>(static_cast<int>(ENetworkCode::Unknown) + static_cast<int>(curlCode));
                            Hermes::getInstance()->getLogger()->print(ELogLevel::Warning, "CURL code: %. URL: %", curl_easy_strerror(curlCode), requestUrl);
                            break;
                        }

                        if (lpParam.mCallback != nullptr)
                        {
                            NetworkResponse response;
                            response.mCode = code;
                            response.mHttpCode = static_cast<int>(httpCode);
                            response.mHeader = std::move(responseHeader);
                            response.mMessage = strlen(errorBuffer) == 0 ? std::move(responseMessage) : errorBuffer;
                            response.mRawData = std::move(responseRawData);

                            if (response.mCode == ENetworkCode::OK && lpParam.mAllowCache)
                                strongThis->cacheResponse(response, requestUrl, lpParam.mCacheLifetime);

                            std::pair<decltype(lpParam.mCallback), decltype(response)> taskData = std::make_pair(std::move(lpParam.mCallback), std::move(response));
                            auto taskHandler = std::make_shared<decltype(taskData)>(std::move(taskData));
                            Hermes::getInstance()->getTaskManager()->execute(-1, [taskHandler]() -> void
                            {
                                auto taskCallback = std::move(taskHandler->first);
                                taskCallback(std::move(taskHandler->second));
                            });
                        }
                    }
                }
                strongThis->mActivityCount--;
            }
        };
        
        RequestSettings requestSettings;        
        {
            std::lock_guard<std::mutex> lock(mRequestSettingsMutex);
            requestSettings = mRequestSettings;
        }
        
        std::tuple<decltype(requestTask), decltype(pParam), decltype(requestSettings)> taskData = std::make_tuple(std::move(requestTask), std::move(pParam), std::move(requestSettings));
        auto taskHandler = std::make_shared<decltype(taskData)>(std::move(taskData));
        Hermes::getInstance()->getTaskManager()->execute(mThreadPoolID, [taskHandler]() -> void
        {                                
            auto taskCallback = std::move(std::get<0>(*taskHandler));
            taskCallback(std::move(std::get<1>(*taskHandler)), std::move(std::get<2>(*taskHandler)));
        });
    }
    
    void NetworkManager::request(std::vector<NetworkRequestParam> pParam, std::vector<std::shared_ptr<NetworkRequestHandle>> pRequestHandle)
    {
        if (pParam.size() == 0)
            return;

        if (pParam.size() > pRequestHandle.size())
            pRequestHandle.resize(pParam.size());
        
        for (size_t i = 0; i < pRequestHandle.size(); ++i)
        {
            if (pRequestHandle[i] == nullptr)
                pRequestHandle[i] = std::make_shared<NetworkRequestHandle>();
        }

        auto weakThis = mWeakThis;
        auto requestTask = [weakThis, pRequestHandle](std::vector<NetworkRequestParam> lpParam, RequestSettings lpRequestSettings) -> void
        {
            void* handle = nullptr;
            size_t paramSize = lpParam.size();
            std::vector<MultiRequestData> multiRequestData(paramSize);
                        
            for (size_t i = 0; i < paramSize; ++i)
                multiRequestData[i].mParam = &lpParam[i];
            
            std::shared_ptr<NetworkManager> strongThis = weakThis.lock();
            if (strongThis != nullptr)
            {
                strongThis->mActivityCount++;
                if (strongThis->mInitialized.load() == 2)
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
                            NetworkResponse response = strongThis->getResponseFromCache(it->mMethod, lpRequestSettings.mHttpCodeSuccess.first);
                            
                            if (response.mCode != ENetworkCode::OK)
                            {
                                param.push_back(std::move(*it));
                            }
                            else
                            {
                                std::pair<decltype(it->mCallback), decltype(response)> taskData = std::make_pair(std::move(it->mCallback), std::move(response));
                                auto taskHandler = std::make_shared<decltype(taskData)>(std::move(taskData));
                                Hermes::getInstance()->getTaskManager()->execute(-1, [taskHandler]() -> void
                                {
                                    auto taskCallback = std::move(taskHandler->first);
                                    taskCallback(std::move(taskHandler->second));
                                });
                            }
                        }
                    }
                    
                    if (param.size() == 0)
                    {
                        strongThis->mActivityCount--;
                        
                        return;
                    }

                    {
                        std::lock_guard<std::mutex> lock(strongThis->mMultiHandleMutex);
                        handle = strongThis->mMultiHandle[std::this_thread::get_id()];
                    }
                    
                    if (handle == nullptr)
                    {
                        handle = curl_multi_init();

                        std::lock_guard<std::mutex> lock(strongThis->mMultiHandleMutex);
                        strongThis->mMultiHandle[std::this_thread::get_id()] = handle;
                    }
                
                    paramSize = param.size();                    
                    multiRequestData = std::vector<MultiRequestData>(paramSize);
                
                    for (size_t i = 0; i < paramSize; ++i)
                    {
                        multiRequestData[i].mHandle = curl_easy_init();
                        multiRequestData[i].mParam = &param[i];
                        multiRequestData[i].mErrorBuffer.resize(CURL_ERROR_SIZE);
                        multiRequestData[i].mErrorBuffer[0] = 0;

                        curl_easy_setopt(multiRequestData[i].mHandle, CURLOPT_HEADERFUNCTION, CURL_HEADER_CALLBACK);
                        curl_easy_setopt(multiRequestData[i].mHandle, CURLOPT_PRIVATE, &multiRequestData[i]);
                        
                        auto uniqueHeader = strongThis->createUniqueHeader(param[i].mHeader);

                        curl_slist* header = nullptr;

                        for (auto& x : uniqueHeader)
                        {
                            std::string singleHeader = x.first;
                            singleHeader += ": ";
                            singleHeader += x.second;
                            
                            header = curl_slist_append(header, singleHeader.c_str());
                        }

                        multiRequestData[i].mProgressData.mRequestHandle = pRequestHandle[i];
                        multiRequestData[i].mProgressData.mTerminateAbort = &strongThis->mTerminateAbort;
                        
                        if (param[i].mProgress != nullptr)
                        {
                            decltype(param[i].mProgress) progress = std::move(param[i].mProgress);
                            decltype(lpRequestSettings.mProgressTimePeriod) progressTimePeriod = lpRequestSettings.mProgressTimePeriod;
                            auto lastTick = std::chrono::system_clock::now();
                            multiRequestData[i].mProgressData.mProgressTask = [progressTimePeriod, progress, lastTick]
                                (const long long& lpDN, const long long& lpDT, const long long& lpUN, const long long& lpUT) mutable -> void
                            {
                                auto thisTick = std::chrono::system_clock::now();
                                const auto difference = std::chrono::duration_cast<std::chrono::milliseconds>(thisTick - lastTick).count();
                                
                                if (difference > progressTimePeriod || (lpDN != 0 && lpDN == lpDT) || (lpUN != 0 && lpUN == lpUT))
                                {
                                    lastTick = std::chrono::system_clock::now();
                                    Hermes::getInstance()->getTaskManager()->execute(-1, progress, lpDN, lpDT, lpUN, lpUT);
                                }
                            };
                        }
                        
                        std::string requestUrl = param[i].mMethod;
                        strongThis->appendParameter(requestUrl, param[i].mParameter);
                        
                        strongThis->configureHandle(multiRequestData[i].mHandle, param[i].mRequestType, param[i].mResponseType, requestUrl, param[i].mRequestBody, &multiRequestData[i].mResponseMessage,
                            &multiRequestData[i].mResponseRawData, &multiRequestData[i].mResponseHeader, header, lpRequestSettings.mTimeout, lpRequestSettings.mFlag, &multiRequestData[i].mProgressData,
                            &multiRequestData[i].mErrorBuffer[0], lpRequestSettings.mCACertificatePath);
                        
                        curl_multi_add_handle(handle, multiRequestData[i].mHandle);
                    }

                    int activeHandle = 0;
                    unsigned terminateAbort = 0;
                    
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
                        
                        while ((terminateAbort = strongThis->mTerminateAbort.load()) == 0 && (message = curl_multi_info_read(handle, &messageLeft)))
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
                                    code = httpCode >= lpRequestSettings.mHttpCodeSuccess.first && httpCode <= lpRequestSettings.mHttpCodeSuccess.second ? ENetworkCode::OK :
                                        ENetworkCode::InvalidHttpCodeRange;
                                    break;
                                case CURLE_COULDNT_RESOLVE_HOST:
                                    code = ENetworkCode::LostConnection;
                                    Hermes::getInstance()->getLogger()->print(ELogLevel::Warning, "Lost connection: %", (requestData != nullptr) ? requestData->mParam->mMethod : "unknown");
                                    break;
                                case CURLE_OPERATION_TIMEDOUT:
                                    code = ENetworkCode::Timeout;
                                    Hermes::getInstance()->getLogger()->print(ELogLevel::Warning, "Timeout: %", (requestData != nullptr) ? requestData->mParam->mMethod : "unknown");
                                    break;
                                case CURLE_ABORTED_BY_CALLBACK:
                                    code = ENetworkCode::Cancel;
                                    Hermes::getInstance()->getLogger()->print(ELogLevel::Warning, "Cancel: %", (requestData != nullptr) ? requestData->mParam->mMethod : "unknown");
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
                                    response.mMessage = strlen(&requestData->mErrorBuffer[0]) == 0 ? std::move(requestData->mResponseMessage) : &requestData->mErrorBuffer[0];
                                    response.mRawData = std::move(requestData->mResponseRawData);
                                    
                                    if (response.mCode == ENetworkCode::OK && requestData->mParam->mAllowCache)
                                        strongThis->cacheResponse(response, requestData->mParam->mMethod, requestData->mParam->mCacheLifetime);

                                    auto taskData = std::make_pair(std::move(requestData->mParam->mCallback), std::move(response));
                                    auto taskHandler = std::make_shared<decltype(taskData)>(std::move(taskData));
                                    Hermes::getInstance()->getTaskManager()->execute(-1, [taskHandler]() -> void
                                    {
                                        auto taskCallback = std::move(taskHandler->first);
                                        taskCallback(std::move(taskHandler->second));
                                    });
                                }
                            }
                        }
                    }
                    while (activeHandle && terminateAbort == 0);
                }
                strongThis->mActivityCount--;
            }
            
            for (size_t i = 0; i < paramSize; ++i)
            {
                if (handle != nullptr)
                {
                    curl_multi_remove_handle(handle, multiRequestData[i].mHandle);
                    curl_easy_cleanup(multiRequestData[i].mHandle);
                }
            }
        };
        
        RequestSettings requestSettings;        
        {
            std::lock_guard<std::mutex> lock(mRequestSettingsMutex);
            requestSettings = mRequestSettings;
        }
        
        std::tuple<decltype(requestTask), decltype(pParam), decltype(requestSettings)> taskData = std::make_tuple(std::move(requestTask), std::move(pParam), std::move(requestSettings));
        auto taskHandler = std::make_shared<decltype(taskData)>(std::move(taskData));
        Hermes::getInstance()->getTaskManager()->execute(mThreadPoolID, [taskHandler]() -> void
        {
            auto taskCallback = std::move(std::get<0>(*taskHandler));
            taskCallback(std::move(std::get<1>(*taskHandler)), std::move(std::get<2>(*taskHandler)));
        });
    }
    
    long NetworkManager::getTimeout() const
    {
        std::lock_guard<std::mutex> lock(mRequestSettingsMutex);
        return mRequestSettings.mTimeout;
    }
    
    void NetworkManager::setTimeout(long pTimeout)
    {
        mActivityCount++;    
        if (mInitialized.load() == 2)
        {
            std::lock_guard<std::mutex> lock(mRequestSettingsMutex);
            mRequestSettings.mTimeout = pTimeout;
        }
        mActivityCount--;
    }
    
    bool NetworkManager::getFlag(ENetworkFlag pFlag) const
    {
        assert(pFlag < ENetworkFlag::Count);
            
        std::lock_guard<std::mutex> lock(mRequestSettingsMutex);
        return mRequestSettings.mFlag[static_cast<size_t>(pFlag)];
    }
    
    void NetworkManager::setFlag(ENetworkFlag pFlag, bool pState)
    {
        assert(pFlag < ENetworkFlag::Count);
        
        mActivityCount++;    
        if (mInitialized.load() == 2)
        {
            std::lock_guard<std::mutex> lock(mRequestSettingsMutex);
            mRequestSettings.mFlag[static_cast<size_t>(pFlag)] = pState;
        }
        mActivityCount--;
    }
    
    long NetworkManager::getProgressTimePeriod() const
    {
        std::lock_guard<std::mutex> lock(mRequestSettingsMutex);
        return mRequestSettings.mProgressTimePeriod;
    }
    
    void NetworkManager::setProgressTimePeriod(long pTimePeriod)
    {
        mActivityCount++;    
        if (mInitialized.load() == 2)
        {
            std::lock_guard<std::mutex> lock(mRequestSettingsMutex);
            mRequestSettings.mProgressTimePeriod = pTimePeriod;
        }
        mActivityCount--;
    }
    
    std::string NetworkManager::getCACertificatePath() const
    {
        std::lock_guard<std::mutex> lock(mRequestSettingsMutex);
        return mRequestSettings.mCACertificatePath;
    }
    void NetworkManager::setCACertificatePath(std::string pPath)
    {
        mActivityCount++;    
        if (mInitialized.load() == 2)
        {
            std::lock_guard<std::mutex> lock(mRequestSettingsMutex);
            mRequestSettings.mCACertificatePath = std::move(pPath);
        }
        mActivityCount--;
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

    void NetworkManager::configureHandle(void* pHandle, ENetworkRequestType pRequestType, ENetworkResponseType pResponseType, const std::string& pRequestUrl, const std::string& pRequestBody,
        std::string* pResponseMessage, DataBuffer* pResponseRawData, std::vector<std::pair<std::string, std::string>>* pResponseHeader, curl_slist* pHeader,
        long pTimeout, std::array<bool, static_cast<size_t>(ENetworkFlag::Count)> pFlag, ProgressData* pProgressData, char* pErrorBuffer, const std::string pCACertificatePath) const
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
        curl_easy_setopt(pHandle, CURLOPT_SSL_VERIFYPEER, pFlag[static_cast<size_t>(ENetworkFlag::DisableSSLVerifyPeer)] ? 1L : 0L);

        if (pCACertificatePath.size() > 0)
            curl_easy_setopt(pHandle, CURLOPT_CAINFO, pCACertificatePath.c_str());

        if (pProgressData != nullptr)
        {
            curl_easy_setopt(pHandle, CURLOPT_XFERINFOFUNCTION, CURL_PROGRESS_CALLBACK);
            curl_easy_setopt(pHandle, CURLOPT_XFERINFODATA, pProgressData);
            curl_easy_setopt(pHandle, CURLOPT_NOPROGRESS, 0);
        }
    }
    
    void NetworkManager::resetHandle(void* pHandle) const
    {
        curl_easy_setopt(pHandle, CURLOPT_CUSTOMREQUEST, 0);
        curl_easy_setopt(pHandle, CURLOPT_XFERINFOFUNCTION, nullptr);
        curl_easy_setopt(pHandle, CURLOPT_XFERINFODATA, nullptr);
        curl_easy_setopt(pHandle, CURLOPT_NOPROGRESS, 1);
        curl_easy_setopt(pHandle, CURLOPT_ERRORBUFFER, 0);
    }
    
    bool NetworkManager::initCache(const std::string& pDirectoryPath, unsigned pFileCountLimit, unsigned pFileSizeLimit)
    {
        if (mInitialized.load() == 2 && mCacheInitialized.load() == 0)
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
            
            mCacheFileCountLimit = 0;
            mCacheFileSizeLimit = 0;
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
    
    NetworkManager::CacheFileData NetworkManager::decodeCacheHeader(const std::string& pFilePath) const
    {
        CacheFileData info;
        std::ifstream file(pFilePath, std::ifstream::in | std::ifstream::binary);
        
        if (file.is_open())
        {
            const size_t magicWordSize = sizeof(mCacheMagicWord);
            char magicWord[magicWordSize];
            file.read(magicWord, magicWordSize);
            if (!file.good() || file.gcount() != magicWordSize || memcmp(magicWord, mCacheMagicWord, magicWordSize) != 0) return info;
            
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
    
    NetworkResponse NetworkManager::getResponseFromCache(const std::string& pUrl, int pHttpCodeSuccess)
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
                file.seekg(static_cast<std::streamsize>(sizeof(mCacheMagicWord) + sizeof(info.mHeader) + info.mUrl.size()));
                
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
                    response.mHttpCode = pHttpCodeSuccess;
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
                info.mFullFilePath = mCacheDirectoryPath + std::to_string(timestamp) + "_";
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
                file.write(mCacheMagicWord, sizeof(mCacheMagicWord));
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
        
        if (mInitialized.load() == 2 && mSimpleSocketInitialized.compare_exchange_strong(initialized, 1))
        {
            // only one thread for sockets allowed
            assert(Hermes::getInstance()->getTaskManager()->threadCountForPool(pThreadPoolID) == 1);
            mThreadPoolSimpleSocketID = pThreadPoolID;
            mSimpleSocketInitialized.store(2);
        }
        
        return initialized == 0;
    }
    
    void NetworkManager::closeSimpleSocket()
    {
        mStopSimpleSocketLoop.store(1);
        Hermes::getInstance()->getTaskManager()->clearTask(mThreadPoolSimpleSocketID);
    }

    void NetworkManager::sendSimpleSocketMessage(const std::string& pMessage, std::function<void(ENetworkCode)> pCallback)
    {
        std::string message = packSimpleSocketMessage(pMessage);
        
        auto weakThis = mWeakThis;
        auto socketTask = [weakThis](std::string lpMessage, std::function<void(ENetworkCode)> lpCallback) -> void
        {
            std::shared_ptr<NetworkManager> strongThis = weakThis.lock();
            if (strongThis != nullptr)
            {
                strongThis->mActivityCount++;
                if (strongThis->mInitialized.load() == 2 && strongThis->mSimpleSocketInitialized.load() == 2)
                {
                    if (strongThis->mSimpleSocketCURL != nullptr)
                    {
                        unsigned tryCount = 0;
                        
                        curl_socket_t socketfd;
                        CURLcode cCode = curl_easy_getinfo(strongThis->mSimpleSocketCURL, CURLINFO_ACTIVESOCKET, &socketfd);
                        
                        if (cCode != CURLE_OK)
                        {
                            ENetworkCode code = static_cast<ENetworkCode>(static_cast<int>(ENetworkCode::Unknown) + static_cast<int>(cCode));
                            
                            if (lpCallback != nullptr)
                            {
                                std::pair<decltype(lpCallback), decltype(code)> taskData = std::make_pair(std::move(lpCallback), code);
                                auto taskHandler = std::make_shared<decltype(taskData)>(std::move(taskData));
                                Hermes::getInstance()->getTaskManager()->execute(-1, [taskHandler]() -> void
                                {
                                    auto taskCallback = std::move(taskHandler->first);
                                    taskCallback(std::move(taskHandler->second));
                                });
                            }

                            strongThis->mActivityCount--;
                            
                            return;
                        }
                        
                        unsigned terminateAbort = 0;
                        
                        do
                        {
                            if (tryCount != 0)
                                CURL_WAIT_ON_SOCKET(socketfd, 1, strongThis->mSocketWaitTimeout);
                            
                            size_t sendCount = 0;
                            cCode = curl_easy_send(strongThis->mSimpleSocketCURL, lpMessage.c_str(), lpMessage.length(), &sendCount);
                            
                            tryCount++;
                        }
                        while ((terminateAbort = strongThis->mTerminateAbort.load()) == 0 && cCode == CURLE_AGAIN && tryCount < 3);
                        
                        if (terminateAbort == 0)
                        {
                            ENetworkCode code = cCode == CURLE_OK ? ENetworkCode::OK : static_cast<ENetworkCode>(static_cast<int>(ENetworkCode::Unknown) + static_cast<int>(cCode));
                        
                            if (lpCallback != nullptr)
                            {
                                std::pair<decltype(lpCallback), decltype(code)> taskData = std::make_pair(std::move(lpCallback), code);
                                auto taskHandler = std::make_shared<decltype(taskData)>(std::move(taskData));
                                Hermes::getInstance()->getTaskManager()->execute(-1, [taskHandler]() -> void
                                {
                                    auto taskCallback = std::move(taskHandler->first);
                                    taskCallback(std::move(taskHandler->second));
                                });
                            }
                        }
                    }
                }
                strongThis->mActivityCount--;
            }
        };

        std::tuple<decltype(socketTask), decltype(message), decltype(pCallback)> taskData = std::make_tuple(std::move(socketTask), std::move(message), std::move(pCallback));
        auto taskHandler = std::make_shared<decltype(taskData)>(std::move(taskData));
        Hermes::getInstance()->getTaskManager()->execute(mThreadPoolSimpleSocketID, [taskHandler]() -> void
        {
            auto taskCallback = std::move(std::get<0>(*taskHandler));
            taskCallback(std::move(std::get<1>(*taskHandler)), std::move(std::get<2>(*taskHandler)));
        });
    }
    
    void NetworkManager::requestSimpleSocket(SimpleSocketRequestParam pParam)
    {
        if (mSimpleSocketInitialized.load() != 2)
            return;
        
        closeSimpleSocket();
        
        auto weakThis = mWeakThis;
        auto socketTask = [weakThis](SimpleSocketRequestParam lpParam, RequestSettings lpRequestSettings) -> void
        {
            std::shared_ptr<NetworkManager> strongThis = weakThis.lock();
            if (strongThis != nullptr)
            {
                strongThis->mActivityCount++;
                if (strongThis->mInitialized.load() == 2 && strongThis->mSimpleSocketInitialized.load() == 2)
                {
                    std::chrono::time_point<std::chrono::system_clock> time = std::chrono::system_clock::now();
                    std::chrono::time_point<std::chrono::system_clock> lastMessageTime = std::chrono::system_clock::now();
                    
                    ESocketDisconnectCause disconnectCause = ESocketDisconnectCause::User;
                    
                    if (strongThis->mSimpleSocketCURL != nullptr)
                    {
                        Hermes::getInstance()->getLogger()->print(ELogLevel::Warning, "Tried to open new socket before closing old one.");
                        disconnectCause = ESocketDisconnectCause::InitialConnection;
                    }
                    else
                    {
                        strongThis->mSimpleSocketCURL = curl_easy_init();
                        
                        if (strongThis->mSimpleSocketCURL == NULL)
                        {
                            Hermes::getInstance()->getLogger()->print(ELogLevel::Error, "Curl easy init fail for socket.");
                            disconnectCause = ESocketDisconnectCause::InitialConnection;
                        }
                        else
                        {
                            strongThis->mStopSimpleSocketLoop.store(0);
                            
                            strongThis->appendParameter(lpParam.mBaseURL, lpParam.mParameter);
                            tools::URLTool url(lpParam.mBaseURL);
                            
                            curl_easy_setopt(strongThis->mSimpleSocketCURL, CURLOPT_URL, url.getHttpURL().c_str());
                            curl_easy_setopt(strongThis->mSimpleSocketCURL, CURLOPT_CONNECT_ONLY, 1L);
                            curl_easy_setopt(strongThis->mSimpleSocketCURL, CURLOPT_SSL_VERIFYPEER, lpRequestSettings.mFlag[static_cast<size_t>(ENetworkFlag::DisableSSLVerifyPeer)] ? 1L : 0L);

                            if (lpRequestSettings.mCACertificatePath.size() > 0)
                                curl_easy_setopt(strongThis->mSimpleSocketCURL, CURLOPT_CAINFO, lpRequestSettings.mCACertificatePath.c_str());
                            
                            CURLcode res = curl_easy_perform(strongThis->mSimpleSocketCURL);
                            
                            if (res != CURLE_OK)
                            {
                                Hermes::getInstance()->getLogger()->print(ELogLevel::Error, "Socket connection failure for url '%'. CURL CODE %, message: '%'", lpParam.mBaseURL,
                                    res, curl_easy_strerror(res));
                                disconnectCause = ESocketDisconnectCause::InitialConnection;
                            }
                            else
                            {
                                curl_socket_t socketfd;
                                
                                res = curl_easy_getinfo(strongThis->mSimpleSocketCURL, CURLINFO_LASTSOCKET, &socketfd);
                                
                                if (res != CURLE_OK)
                                {
                                    Hermes::getInstance()->getLogger()->print(ELogLevel::Error, "Socket get info failure for url '%'. CURL CODE %, message: '%'", lpParam.mBaseURL,
                                        res, curl_easy_strerror(res));
                                    disconnectCause = ESocketDisconnectCause::InitialConnection;
                                }
                                else
                                {
                                    strongThis->mSimpleSocketActive.store(1);
                                    
                                    std::string secSocketAccept;
                                    res = static_cast<CURLcode>(strongThis->sendUpgradeHeader(strongThis->mSimpleSocketCURL, url, lpParam.mHeader, secSocketAccept));
                                    
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
                                        
                                        while (strongThis->mTerminateAbort.load() == 0 && !error && Hermes::getInstance()->getTaskManager()->canContinueTask(strongThis->mThreadPoolSimpleSocketID))
                                        {
                                            size_t readCount = 0;
                                            
                                            if (strongThis->mStopSimpleSocketLoop.load() == 1)
                                            {
                                                std::string message = strongThis->packSimpleSocketMessage("", ESocketOpCode::Close);
                                                
                                                res = curl_easy_send(strongThis->mSimpleSocketCURL, message.c_str(), message.length(), &readCount);
                                                
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
                                                res = curl_easy_recv(strongThis->mSimpleSocketCURL, buffer, bufferLength * sizeof(char), &readCount);
                                                
                                                if (res != CURLE_OK && res != CURLE_AGAIN)
                                                {
                                                    Hermes::getInstance()->getLogger()->print(ELogLevel::Error, "Socket read fail for url '%'. CURL CODE %, message: '%'", url.getURL(), res,
                                                        curl_easy_strerror(res));
                                                    
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
                                            while (strongThis->mTerminateAbort.load() == 0);
                                            
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
                                                        
                                                        if (codePos != std::string::npos && keyPos != std::string::npos && data.length() >= keyPos + acceptKey.length() + secSocketAccept.length() &&
                                                            data.substr(keyPos + acceptKey.length(), secSocketAccept.length()).compare(secSocketAccept) == 0)
                                                        {
                                                            if (lpParam.mConnectCallback != nullptr)
                                                            {
                                                                auto taskHandler = std::make_shared<decltype(lpParam.mConnectCallback)>(std::move(lpParam.mConnectCallback));
                                                                Hermes::getInstance()->getTaskManager()->execute(-1, [taskHandler]() -> void
                                                                {
                                                                    auto taskCallback = std::move(*taskHandler);
                                                                    taskCallback();
                                                                });
                                                            }
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
                                                        auto newFrame = strongThis->unpackSimpleSocketMessage(data);
                                                        
                                                        if (!strongThis->handleSimpleSocketFrame(strongThis->mSimpleSocketCURL, newFrame, frame, lpParam.mMessageCallback))
                                                        {
                                                            error = true;
                                                            disconnectCause = ESocketDisconnectCause::Close;
                                                        }
                                                    }
                                                }
                                                
                                                if (lpParam.mTimeout == std::chrono::milliseconds::zero() || (lpParam.mTimeout > std::chrono::milliseconds::zero() && std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - lastMessageTime) < lpParam.mTimeout))
                                                {
                                                    CURL_WAIT_ON_SOCKET(socketfd, 1, strongThis->mSocketWaitTimeout);
                                                }
                                                else
                                                {
                                                    error = true;
                                                    disconnectCause = ESocketDisconnectCause::Timeout;
                                                }
                                                
                                                if (!error)
                                                    Hermes::getInstance()->getTaskManager()->performTaskIfExists(strongThis->mThreadPoolSimpleSocketID);
                                            }
                                        }
                                    }
                                }
                            }

                            curl_easy_cleanup(strongThis->mSimpleSocketCURL);
                            strongThis->mSimpleSocketCURL = nullptr;
                        }
                    }
                    
                    strongThis->mSimpleSocketActive.store(0);

                    if (lpParam.mDisconnectCallback != nullptr)
                    {
                        auto timeDiff = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - time);
                        std::tuple<decltype(lpParam.mDisconnectCallback), decltype(disconnectCause), decltype(timeDiff)> taskData = std::make_tuple(std::move(lpParam.mDisconnectCallback),
                            std::move(disconnectCause), timeDiff);
                        auto taskHandler = std::make_shared<decltype(taskData)>(std::move(taskData));
                        Hermes::getInstance()->getTaskManager()->execute(-1, [taskHandler]() -> void
                        {
                            auto taskCallback = std::move(std::get<0>(*taskHandler));
                            taskCallback(std::move(std::get<1>(*taskHandler)), std::move(std::get<2>(*taskHandler)));
                        });
                    }
                }
                strongThis->mActivityCount--;
            }
        };
        
        RequestSettings requestSettings;        
        {
            std::lock_guard<std::mutex> lock(mRequestSettingsMutex);
            requestSettings = mRequestSettings;
        }
        
        std::tuple<decltype(socketTask), decltype(pParam), decltype(requestSettings)> taskData = std::make_tuple(std::move(socketTask), std::move(pParam), std::move(requestSettings));
        auto taskHandler = std::make_shared<decltype(taskData)>(std::move(taskData));
        Hermes::getInstance()->getTaskManager()->execute(mThreadPoolSimpleSocketID, [taskHandler]() -> void
        {
            auto taskCallback = std::move(std::get<0>(*taskHandler));
            taskCallback(std::move(std::get<1>(*taskHandler)), std::move(std::get<2>(*taskHandler)));
        });
    }
    
    int NetworkManager::sendUpgradeHeader(void* const pCurl, const tools::URLTool& pURL, const std::vector<std::pair<std::string, std::string>>& pHeader, std::string& pSecAccept) const
    {
        CURLcode code = CURLE_OK;
        
        std::string key = crypto::getRandomCryptoBytes(16);
        key = crypto::encodeBase64(reinterpret_cast<const unsigned char*>(key.data()), key.size());
        
        std::string shaAccept = crypto::getSHA1Digest(key + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11");
        pSecAccept = crypto::encodeBase64(reinterpret_cast<const unsigned char*>(shaAccept.data()), shaAccept.size());
        
        std::string header = "GET ";
        header.reserve(1024);
        
        size_t pathLength = 0;
        const char* path = pURL.getPath(pathLength, true);
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
        
        memcpy(mask, crypto::getRandomCryptoBytes(4).data(), sizeof(mask));
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
    
    bool NetworkManager::handleSimpleSocketFrame(void* const pCurl, std::vector<SocketFrame>& pNewFrame, std::vector<SocketFrame>& pOldFrame, std::function<void(std::string lpMessage, bool lpTextData)> pMessageCallback) const
    {
        bool endSent = false;
        
        for (size_t i = pNewFrame.size(); i > 0; i--)
        {
            uint8_t opCode = pNewFrame[i-1].mCode & 0x0F;
            
            if (static_cast<ESocketOpCode>(opCode) == ESocketOpCode::Ping)
            {
                pNewFrame.erase(pNewFrame.begin() + static_cast<long>(i - 1));
                
                if (!endSent)
                {
                    std::string message = packSimpleSocketMessage("", ESocketOpCode::Pong);
                    size_t sendCount = 0;
                    
                    if (pCurl != nullptr)
                    {
                        CURLcode code = curl_easy_send(pCurl, message.c_str(), message.length(), &sendCount);
                        
                        if(code != CURLE_OK)
                            Hermes::getInstance()->getLogger()->print(ELogLevel::Error, "Pong send failed. CURLcode %", static_cast<int>(code));
                    }
                }
            }
            else if (static_cast<ESocketOpCode>(opCode) == ESocketOpCode::Close)
            {
                pNewFrame.erase(pNewFrame.begin() + static_cast<long>(i - 1));
                
                std::string message = packSimpleSocketMessage("", ESocketOpCode::Close);
                
                if (pCurl != nullptr)
                {
                    size_t sendCount = 0;
                    CURLcode code = curl_easy_send(pCurl, message.c_str(), message.length(), &sendCount);
                    
                    if(code != CURLE_OK)
                        Hermes::getInstance()->getLogger()->print(ELogLevel::Error, "Close response send failed. CURLcode %", static_cast<int>(code));
                }
                endSent = true;
            }
            else if (opCode > static_cast<uint8_t>(ESocketOpCode::Binary))
            {
                pNewFrame.erase(pNewFrame.begin() + static_cast<long>(i - 1));
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
                    Hermes::getInstance()->getTaskManager()->execute(-1, std::move(pMessageCallback), std::move(it->mPayload), static_cast<ESocketOpCode>(opCode) == ESocketOpCode::Text);
                
                it = pOldFrame.erase(it);
            }
            else
            {
                multiMessage += std::move(it->mPayload);
                
                if (pMessageCallback != nullptr)
                    Hermes::getInstance()->getTaskManager()->execute(-1, std::move(pMessageCallback), std::move(multiMessage), multiFrameOpCode == ESocketOpCode::Text);
                
                multiMessage = "";
                
                it = pOldFrame.erase(firstFrameIt, it + 1);
            }
        }
        
        return !endSent;
    }

}
