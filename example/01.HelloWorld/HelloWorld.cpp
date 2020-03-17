// Copyright (C) 2017-2020 Grupa Pracuj Sp. z o.o.
// This file is part of the "Hermes" library.
// For conditions of distribution and use, see copyright notice in license.txt.

#include "HelloWorld.hpp"

UserData::UserData(size_t pId) : hms::DataShared(pId)
{
};

HelloWorld::HelloWorld(std::function<void(std::function<void()>)> pMainThreadHandler, std::string pCertificatePath)
{
    auto logger = hms::Hermes::getInstance()->getLogger();
    logger->initialize(hms::ELogLevel::Info, "HelloWorld", nullptr);

    auto dataManager = hms::Hermes::getInstance()->getDataManager();
    dataManager->initialize();
    dataManager->add(std::shared_ptr<hms::DataShared>(new UserData(static_cast<size_t>(EDataType::User))));

    auto taskManager = hms::Hermes::getInstance()->getTaskManager();
    // First element of pair - thread pool id, second element of pair - number of threads in thread pool.
    const std::pair<int, int> threadInfo = {0, 2};
    taskManager->initialize({threadInfo}, pMainThreadHandler);

    auto networkManager = hms::Hermes::getInstance()->getNetworkManager();
    networkManager->initialize(10, threadInfo.first, threadInfo.first, {200, 299});

    auto jphNetwork = networkManager->add(static_cast<size_t>(ENetworkAPI::JsonPlaceholder), "JsonPlaceholder", "https://jsonplaceholder.typicode.com/");
    jphNetwork->setDefaultHeader({{"Content-Type", "application/json; charset=utf-8"}});

    hms::Hermes::getInstance()->getNetworkManager()->setCACertificatePath(std::move(pCertificatePath));
}

HelloWorld::~HelloWorld()
{
    hms::Hermes::getInstance()->getNetworkManager()->terminate();
    hms::Hermes::getInstance()->getTaskManager()->terminate();
    hms::Hermes::getInstance()->getDataManager()->terminate();
    hms::Hermes::getInstance()->getLogger()->terminate();
}

void HelloWorld::execute(std::function<void(std::string)> pCallback, int32_t pRequestIndex)
{
    using namespace std::string_literals;

    hms::NetworkRequestParam requestParams;
    requestParams.mRequestType = hms::ENetworkRequestType::Get;
    requestParams.mMethod = "users/"s + std::to_string(pRequestIndex);
    requestParams.mCallback = [pCallback = std::move(pCallback)](hms::NetworkResponse lpResponse) -> void
    {
        std::string outputText;

        if (lpResponse.mCode == hms::ENetworkCode::OK)
        {
            Json::Value json;
            if (hms::ext::json::convert(lpResponse.mMessage, json))
            {
                auto userData = hms::Hermes::getInstance()->getDataManager()->get<UserData>(static_cast<size_t>(EDataType::User));
                hms::ext::json::deserialize<UserData>(*userData, json);

                std::string userId = "ID: "s + std::to_string(userData->mId);
                std::string userName = "Name: "s + userData->mName;
                std::string userCity = "City: "s + userData->mAddress.mCity;
                outputText = userId + "\n"s + userName + "\n"s + userCity;
                hms::Hermes::getInstance()->getLogger()->print(hms::ELogLevel::Info, userId + " / "s + userName + " / "s + userCity);
            }
        }
        else if (lpResponse.mCode != hms::ENetworkCode::Cancel)
        {
            outputText = "Error: " + std::to_string(lpResponse.mHttpCode);
            hms::Hermes::getInstance()->getLogger()->print(hms::ELogLevel::Info, "Error: % | %", lpResponse.mHttpCode, lpResponse.mMessage);
        }

        if (pCallback != nullptr)
            pCallback(std::move(outputText));
    };

    hms::Hermes::getInstance()->getNetworkManager()->get(static_cast<size_t>(ENetworkAPI::JsonPlaceholder))->request(std::move(requestParams));
}
