// Copyright (C) 2017 Grupa Pracuj Sp. z o.o.
// This file is part of the "Hermes" library.
// For conditions of distribution and use, see copyright notice in license.txt.

#ifndef _LOGGER_HPP_
#define _LOGGER_HPP_

#include <string>
#include <vector>
#include <sstream>

namespace hms
{

    enum class ELogLevel : int
    {
        Info = 0,
        Warning,
        Error,
        Fatal
    };

    class Logger
    {
    public:
        bool initialize(ELogLevel pLevel, std::function<void(ELogLevel pType, const std::string& pText)> pPostCallback);
        bool terminate();
        
        template<typename... Argument>
        void print(ELogLevel pType, const std::string& pText, Argument... pArgument)
        {
            if (mInitialized && static_cast<unsigned>(pType) >= static_cast<unsigned>(mLevel))
            {
                std::string buffer;
                createBuffer(buffer, pText.c_str(), pArgument...);
                
                std::string output = createPrefix(pType) + buffer;
                printNative(output);

                if (mPostCallback != nullptr)
                    mPostCallback(pType, buffer);
            }
        }

    private:
        friend class Hermes;
    
        Logger();
        Logger(const Logger& pOther) = delete;
        Logger(Logger&& pOther) = delete;
        ~Logger();
        
        Logger& operator=(const Logger& pOther) = delete;
        Logger& operator=(Logger&& pOther) = delete;
        
        void printNative(const std::string& pText) const;
        std::string createPrefix(ELogLevel pType) const;
        void createBuffer(std::string& pDestination, const char* pSource) const;
                
        template<typename T, typename... Argument>
        void createBuffer(std::string& pDestination, const char* pSource, T& pValue, Argument... pArgument) const
        {
            while (*pSource)
            {
                if (*pSource == '%')
                {
                    if (*(pSource + 1) == '%')
                    {
                        ++pSource;
                    }
                    else
                    {
                        std::stringstream ss;
                        ss << pValue;
                        
                        pDestination += ss.str();
                        createBuffer(pDestination, pSource + 1, pArgument...);
                        
                        return;
                    }
                }
                
                pDestination += *(pSource++);
            }
        }
        
        ELogLevel mLevel = ELogLevel::Info;
        std::function<void(ELogLevel pType, const std::string& pText)> mPostCallback = nullptr;
        bool mInitialized = false;
    };

}

#endif
