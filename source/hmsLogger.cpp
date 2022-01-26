// Copyright (C) 2017-2022 Grupa Pracuj S.A.
// This file is part of the "Hermes" library.
// For conditions of distribution and use, see copyright notice in license.txt.

#include "hmsLogger.hpp"

#include <iostream>
#if defined(ANDROID) || defined(__ANDROID__)
#include <android/log.h>
#endif

namespace hms
{
    
    /* Logger */
    
    Logger::Logger()
    {
    }
    
    Logger::~Logger()
    {
        terminate();
    }

    bool Logger::initialize(ELogLevel pLevel, std::string pTag, std::function<void(ELogLevel, std::string)> pCallback)
    {
        if (mInitialized)
            return false;

        mInitialized = true;
        
        mLevel = pLevel;
        mTag = std::move(pTag);
        mCallback = std::move(pCallback);
        
        return true;
    }
    
    bool Logger::terminate()
    {
        if (!mInitialized)
            return false;
        
        mLevel = ELogLevel::Info;
        mTag = "";
        mCallback = nullptr;

        mInitialized = false;
        
        return true;
    }
    
    ELogLevel Logger::getLevel() const
    {
        return mLevel;
    }
    
    void Logger::setLevel(ELogLevel pLevel)
    {
        mLevel = pLevel;
    }
    
    const std::string& Logger::getTag() const
    {
        return mTag;
    }
    
    void Logger::setTag(std::string pTag)
    {
        mTag = std::move(pTag);
    }
    
    const std::function<void(ELogLevel, std::string)>& Logger::getCallback() const
    {
        return mCallback;
    }

    void Logger::setCallback(std::function<void(ELogLevel, std::string)> pCallback)
    {
        mCallback = std::move(pCallback);
    }
    
    void Logger::printNative(ELogLevel pLevel, const std::string& pMessage) const
    {
#if defined(ANDROID) || defined(__ANDROID__)
        static android_LogPriority const translationTable[] =
        {
                ANDROID_LOG_INFO,
                ANDROID_LOG_WARN,
                ANDROID_LOG_ERROR,
                ANDROID_LOG_FATAL
        };

        __android_log_print(translationTable[static_cast<size_t>(pLevel)], mTag.c_str(), "%s", pMessage.c_str());
#else
        std::string output = createPrefix(pLevel);
        output += pMessage;
        std::cout << output << "\n";
#endif
    }
    
    std::string Logger::createPrefix(ELogLevel pLevel) const
    {
        std::string prefix;
        
        if (mTag.size() > 0)
            prefix += mTag + " | ";
        
        switch (pLevel)
        {
        case ELogLevel::Info:
            prefix += "INFO: ";
            break;
        case ELogLevel::Warning:
            prefix += "WARNING: ";
            break;
        case ELogLevel::Error:
            prefix += "ERROR: ";
            break;
        case ELogLevel::Fatal:
            prefix += "FATAL: ";
            break;
        default:
            prefix += "UNKNOWN: ";
            break;
        }
        
        return prefix;
    }
    
    void Logger::createBuffer(std::string& pDestination, const char* pSource) const
    {
        while (*pSource)
        {
            if (*pSource == '%')
            {
                if (*(pSource + 1) == '%')
                {
                    ++pSource;
                }
            }
            
            pDestination += *(pSource++);
        }
    }
    
}
