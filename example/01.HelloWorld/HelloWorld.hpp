// Copyright (C) 2017-2021 Grupa Pracuj S.A.
// This file is part of the "Hermes" library.
// For conditions of distribution and use, see copyright notice in license.txt.

#ifndef _HELLO_WORLD_HPP_
#define _HELLO_WORLD_HPP_

#include "hermes.hpp"
#include "ext/hmsSerializer.hpp"

enum class EDataType : size_t
{
    User = 0
};

enum class ENetworkAPI : size_t
{
    JsonPlaceholder = 0
};

class UserData : public hms::DataShared
{
public:
    class Address
    {
    public:
        std::string mStreet;
        std::string mSuite;
        std::string mCity;
        std::string mZipCode;
    };

    UserData(size_t pId);

    int32_t mId = -1;
    std::string mName;
    Address mAddress;
};

namespace hms
{
    namespace ext
    {
        template<>
        inline auto registerProperties<UserData::Address>()
        {
            return std::make_tuple(Property<UserData::Address, std::string>("street", &UserData::Address::mStreet, ESerializerMode::Deserialize),
                Property<UserData::Address, std::string>("suite", &UserData::Address::mSuite, ESerializerMode::Deserialize),
                Property<UserData::Address, std::string>("city", &UserData::Address::mCity, ESerializerMode::Deserialize),
                Property<UserData::Address, std::string>("zipcode", &UserData::Address::mZipCode, ESerializerMode::Deserialize));
        }

        template<>
        inline auto registerProperties<UserData>()
        {
            return std::make_tuple(Property<UserData, int32_t>("id", &UserData::mId, ESerializerMode::Deserialize),
                Property<UserData, std::string>("name", &UserData::mName, ESerializerMode::Deserialize),
                Property<UserData, UserData::Address>("address", &UserData::mAddress, ESerializerMode::Deserialize));
        }
    }
}

class HelloWorld
{
public:
    HelloWorld(std::function<void(std::function<void()>)> pMainThreadHandler, std::function<std::pair<hms::ENetworkCertificate, std::string>()> pCertificateProvider);
    ~HelloWorld();

    void execute(std::function<void(std::string)> pCallback, int32_t pRequestIndex);
};

#endif
