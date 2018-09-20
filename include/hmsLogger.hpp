// Copyright (C) 2017-2018 Grupa Pracuj Sp. z o.o.
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
        bool initialize(ELogLevel pLevel, std::string pTag, std::function<void(ELogLevel pType, std::string pText)> pCallback);
        bool terminate();
        
        ELogLevel getLevel() const;
        void setLevel(ELogLevel pLevel);
        
        const std::string& getTag() const;
        void setTag(std::string pTag);

        const std::function<void(ELogLevel pType, std::string pText)>& getCallback() const;
        void setCallback(std::function<void(ELogLevel pType, std::string pText)> pCallback);
        
        template<typename... Argument>
        void print(ELogLevel pType, const std::string& pText, Argument... pArgument)
        {
            if (mInitialized && static_cast<size_t>(pType) >= static_cast<size_t>(mLevel))
            {
                std::string buffer;
                createBuffer(buffer, pText.c_str(), pArgument...);

                printNative(pType, buffer);

                if (mCallback != nullptr)
                    mCallback(pType, std::move(buffer));
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
        
        void printNative(ELogLevel pType, const std::string& pText) const;
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
        std::string mTag;
        std::function<void(ELogLevel pType, std::string pText)> mCallback = nullptr;
        bool mInitialized = false;
    };

}

#endif
