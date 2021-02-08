// Copyright (C) 2017-2021 Grupa Pracuj Sp. z o.o.
// This file is part of the "Hermes" library.
// For conditions of distribution and use, see copyright notice in license.txt.

#ifndef _LOGGER_HPP_
#define _LOGGER_HPP_

#include <string>
#include <vector>
#include <sstream>
#include <functional>

namespace hms
{

    enum class ELogLevel : size_t
    {
        Info = 0,
        Warning,
        Error,
        Fatal
    };

    class Logger
    {
    public:
        bool initialize(ELogLevel pLevel, std::string pTag, std::function<void(ELogLevel pLevel, std::string pMessage)> pCallback);
        bool terminate();
        
        ELogLevel getLevel() const;
        void setLevel(ELogLevel pLevel);
        
        const std::string& getTag() const;
        void setTag(std::string pTag);

        const std::function<void(ELogLevel pLevel, std::string pMessage)>& getCallback() const;
        void setCallback(std::function<void(ELogLevel pLevel, std::string pMessage)> pCallback);
        
        template<typename... Argument>
        void print(ELogLevel pLevel, const std::string& pMessage, Argument... pArgument)
        {
            if (mInitialized && static_cast<size_t>(pLevel) >= static_cast<size_t>(mLevel))
            {
                std::string buffer;
                createBuffer(buffer, pMessage.c_str(), pArgument...);

                printNative(pLevel, buffer);

                if (mCallback != nullptr)
                    mCallback(pLevel, std::move(buffer));
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
        
        void printNative(ELogLevel pLevel, const std::string& pMessage) const;
        std::string createPrefix(ELogLevel pLevel) const;
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
        std::string mTag;
        std::function<void(ELogLevel, std::string)> mCallback = nullptr;
        bool mInitialized = false;
    };

}

#endif
