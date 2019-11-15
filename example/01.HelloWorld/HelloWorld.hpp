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
            return std::make_tuple(Property<UserData, int32_t>("id", &UserData::mId),
                Property<UserData, std::string>("name", &UserData::mName),
                Property<UserData, UserData::Address>("address", &UserData::mAddress, ESerializerMode::Deserialize));
        }
    }
}

class HelloWorld
{
public:
    HelloWorld(std::string pCertificatePath);
    ~HelloWorld();

    void execute(std::function<void(std::string)> pCallback, int32_t pRequestIndex);
};

#endif
