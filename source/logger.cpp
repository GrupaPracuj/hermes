// Copyright (C) 2017 Grupa Pracuj Sp. z o.o.
// This file is part of the "Hermes" library.
// For conditions of distribution and use, see copyright notice in license.txt.

#include "logger.hpp"

#include <iostream>

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
    
    bool Logger::initialize(ELogLevel pLevel, std::function<void(ELogLevel pType, const std::string& pText)> pPostCallback)
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
    
    void Logger::printNative(const std::string& pText) const
    {
        std::cout << pText << "\n";
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
