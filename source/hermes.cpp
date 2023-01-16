// Copyright (C) 2017-2023 Grupa Pracuj S.A.
// This file is part of the "Hermes" library.
// For conditions of distribution and use, see copyright notice in license.txt.

#include "hermes.hpp"

namespace hms
{

Hermes::Hermes()
{
    mVersion[0] = 1;
    mVersion[1] = 3;
    mVersion[2] = 2;
    
    mDataManager = new DataManager();
    mNetworkManager = std::shared_ptr<NetworkManager>(new NetworkManager(), std::bind(&Hermes::invokeDelete<NetworkManager>, std::placeholders::_1));
    mNetworkManager->mWeakThis = mNetworkManager;
    mTaskManager = new TaskManager();
    mLogger = new Logger();
}

Hermes::~Hermes()
{
    if (mNetworkManager != nullptr)
        mNetworkManager->terminate();
    
    if (mTaskManager != nullptr)
    {
        mTaskManager->terminate();
        delete mTaskManager;
    }
    
    if (mDataManager != nullptr)
    {
        mDataManager->terminate();
        delete mDataManager;
    }
    
    if (mLogger != nullptr)
    {
        mLogger->terminate();
        delete mLogger;
    }
}

DataManager* Hermes::getDataManager() const
{
    return mDataManager;
}

NetworkManager* Hermes::getNetworkManager() const
{
    return mNetworkManager.get();
}

TaskManager* Hermes::getTaskManager() const
{
    return mTaskManager;
}

Logger* Hermes::getLogger() const
{
    return mLogger;
}
        
void Hermes::getVersion(unsigned& pMajor, unsigned& pMinor, unsigned& pPatch) const
{
    pMajor = mVersion[0];
    pMinor = mVersion[1];
    pPatch = mVersion[2];
}

Hermes* Hermes::getInstance()
{
    static Hermes instance;
    
    return &instance;
}

}
