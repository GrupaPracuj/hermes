// Copyright (C) 2017 Grupa Pracuj Sp. z o.o.
// This file is part of the "Hermes" library.
// For conditions of distribution and use, see copyright notice in license.txt.

#include "logger.hpp"

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
    
    bool Logger::initialize(ELogLevel pLevel, std::function<void(ELogLevel pType, std::string pText)> pPostCallback)
    {
        if (mInitialized)
            return false;

        mInitialized = true;
        
        mLevel = pLevel;
        mPostCallback = pPostCallback;
        
        return true;
    }
    
    bool Logger::terminate()
    {
        if (!mInitialized)
            return false;
        
        mLevel = ELogLevel::Info;
        mPostCallback = nullptr;

        mInitialized = false;
        
        return true;
    }
    
    void Logger::printNative(ELogLevel pType, const std::string& pText) const
    {
#if defined(ANDROID) || defined(__ANDROID__)
        static android_LogPriority const translationTable[] = {
                ANDROID_LOG_INFO,
                ANDROID_LOG_WARN,
                ANDROID_LOG_ERROR,
                ANDROID_LOG_FATAL
        };

        __android_log_print(translationTable[static_cast<size_t>(pType)], "Hermes", "%s", pText.c_str());
#else
        std::string output = createPrefix(pType);
        output += pText;
        std::cout << output << "\n";
#endif
    }
    
    std::string Logger::createPrefix(ELogLevel pType) const
    {
        std::string prefix;
        
        switch (pType)
        {
            case ELogLevel::Info:
                prefix = "INFO: ";
                break;
            case ELogLevel::Warning:
                prefix = "WARNING: ";
                break;
            case ELogLevel::Error:
                prefix = "ERROR: ";
                break;
            case ELogLevel::Fatal:
                prefix = "FATAL: ";
                break;
            default:
                prefix = "UNKNOWN: ";
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
