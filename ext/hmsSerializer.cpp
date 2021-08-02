// Copyright (C) 2017-2021 Grupa Pracuj S.A.
// This file is part of the "Hermes" library.
// For conditions of distribution and use, see copyright notice in license.txt.

#include "hmsSerializer.hpp"

#include "hermes.hpp"

#include <memory>
#include <sstream>

namespace hms
{
namespace ext
{
namespace json
{
    bool convert(const Json::Value& pSource, std::string& pDestination)
    {
        Json::StreamWriterBuilder builder;
        builder["commentStyle"] = "None";
        builder["indentation"] = "    ";
        std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());
        
        std::ostringstream stream;
        writer->write(pSource, &stream);
        pDestination = stream.str();
        
        return true;
    }
    
    bool convert(const std::string& pSource, Json::Value& pDestination)
    {
        bool status = pSource.length() == 0;
        
        if (pSource.size() > 0)
        {
            Json::CharReaderBuilder builder;
            builder["collectComments"] = false;
            std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
            
            const char* source = pSource.c_str();
            std::string errors;
            status = reader->parse(source, source + pSource.size(), &pDestination, &errors);
            
            if (!status)
                hms::Hermes::getInstance()->getLogger()->print(hms::ELogLevel::Error, "String to json convert fail: %; Source: '%'", errors, pSource);
        }
        
        return status;
    }
    
    template <>
    bool assignOp<bool>(const Json::Value* pSource, bool& pDestination)
    {
        bool status = false;
        
        if (pSource->isBool())
        {
            pDestination = pSource->asBool();
            
            status = true;
        }
        
        return status;
    }
    
    template <>
    bool assignOp<int32_t>(const Json::Value* pSource, int& pDestination)
    {
        bool status = false;
        
        if (pSource->isInt())
        {
            pDestination = pSource->asInt();
            
            status = true;
        }
        
        return status;
    }
    
    template <>
    bool assignOp<int64_t>(const Json::Value* pSource, int64_t& pDestination)
    {
        bool status = false;
        
        if (pSource->isInt64())
        {
            pDestination = pSource->asInt64();
            
            status = true;
        }
        
        return status;
    }
    
    template <>
    bool assignOp<uint32_t>(const Json::Value* pSource, uint32_t& pDestination)
    {
        bool status = false;
        
        if (pSource->isUInt())
        {
            pDestination = pSource->asUInt();
            
            status = true;
        }
        
        return status;
    }
    
    template <>
    bool assignOp<uint64_t>(const Json::Value* pSource, uint64_t& pDestination)
    {
        bool status = false;
        
        if (pSource->isUInt64())
        {
            pDestination = pSource->asUInt64();
            
            status = true;
        }
        
        return status;
    }
    
    template <>
    bool assignOp<float>(const Json::Value* pSource, float& pDestination)
    {
        bool status = false;
        
        if (pSource->isDouble())
        {
            pDestination = pSource->asFloat();
            
            status = true;
        }
        
        return status;
    }
    
    template <>
    bool assignOp<double>(const Json::Value* pSource, double& pDestination)
    {
        bool status = false;
        
        if (pSource->isDouble())
        {
            pDestination = pSource->asDouble();
            
            status = true;
        }
        
        return status;
    }
    
    template <>
    bool assignOp<std::string>(const Json::Value* pSource, std::string& pDestination)
    {
        bool status = false;
        
        if (pSource->isString())
        {
            pDestination = pSource->asString();
            
            status = true;
        }
        
        return status;
    }
}
}
}
