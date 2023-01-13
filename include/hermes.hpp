// Copyright (C) 2017-2023 Grupa Pracuj S.A.
// This file is part of the "Hermes" library.
// For conditions of distribution and use, see copyright notice in license.txt.

#ifndef _HERMES_HPP_
#define _HERMES_HPP_

#include "hmsData.hpp"
#include "hmsNetwork.hpp"
#include "hmsTask.hpp"
#include "hmsLogger.hpp"
#include "hmsTools.hpp"

#include <array>
#include <memory>

namespace hms
{

    class Hermes
    {
    public:
        DataManager* getDataManager() const;
        NetworkManager* getNetworkManager() const;
        TaskManager* getTaskManager() const;
        Logger* getLogger() const;
        
        void getVersion(unsigned& pMajor, unsigned& pMinor, unsigned& pPatch) const;

		static Hermes* getInstance();
        
    private:
        Hermes();
        Hermes(const Hermes& pOther) = delete;
        Hermes(Hermes&& pOther) = delete;
        ~Hermes();
        
        Hermes& operator=(const Hermes& pOther) = delete;
        Hermes& operator=(Hermes&& pOther) = delete;
        
        template <typename T>
        static void invokeDelete(T* pObject)
        {
            delete pObject;
        }
        
        std::array<unsigned, 3> mVersion;
        
        DataManager* mDataManager = nullptr;
        std::shared_ptr<NetworkManager> mNetworkManager = nullptr;
        TaskManager* mTaskManager = nullptr;
        Logger* mLogger = nullptr;
    };

}

#endif
