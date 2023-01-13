// Copyright (C) 2017-2023 Grupa Pracuj S.A.
// This file is part of the "Hermes" library.
// For conditions of distribution and use, see copyright notice in license.txt.

#ifndef _HMS_AASSET_READER_HPP_
#define _HMS_AASSET_READER_HPP_

#include "hmsData.hpp"

#include <android/asset_manager_jni.h>

namespace hms
{
namespace ext
{
namespace jni
{
   
    class AAssetStorageReader : public DataStorageReader
    {
    public:
        AAssetStorageReader(JNIEnv* pEnvironment, jobject pAssetManager, const std::string& pPrefix);
        AAssetStorageReader(AAssetManager* pAssetManager, const std::string& pPrefix);
        AAssetStorageReader() = delete;
        AAssetStorageReader(const AAssetStorageReader& pOther) = delete;
        AAssetStorageReader(AAssetStorageReader&& pOther) = delete;
        
        AAssetStorageReader& operator=(const AAssetStorageReader& pOther) = delete;
        AAssetStorageReader& operator=(AAssetStorageReader&& pOther) = delete;
        
        virtual ~AAssetStorageReader();
        virtual std::unique_ptr<DataReader> openFile(const std::string& pName) const override;
        
        void addSubfolder(const std::string& pName);
        
    private:
        void buildFileStructure();
    
        AAssetManager* mAssetManager = nullptr;
        jobject mObject = nullptr;
        JavaVM* mJavaVM = nullptr;
        std::vector<std::string> mFileName;
        
    };
    
    class AAssetReader : public DataReader
    {
    public:
        AAssetReader(AAssetManager* pAssetManager, const std::string& pFilePath);
        AAssetReader() = delete;
        AAssetReader(const AAssetReader& pOther) = delete;
        AAssetReader(AAssetReader&& pOther) = delete;
        virtual ~AAssetReader();
        
        AAssetReader& operator=(const AAssetReader& pOther) = delete;
        AAssetReader& operator=(AAssetReader&& pOther) = delete;
        
        virtual size_t read(void* pBuffer) override;
        virtual size_t read(void* pBuffer, size_t pSize) override;
        virtual void read(std::string& pText) override;
        virtual void read(std::string& pText, size_t pSize) override;
        virtual void read(std::vector<uint8_t>& pBuffer) override;
        virtual void read(std::vector<uint8_t>& pBuffer, size_t pSize) override;
        
        virtual bool seek(int64_t pPosition, bool pRelative = false) override;
        
        virtual size_t getPosition() const override;
        virtual size_t getSize() const override;
        virtual bool isOpen() const override;
        
    private:
        AAsset* mAsset = nullptr;
        size_t mSize = 0;
        size_t mPosition = 0;
        
    };
    
}
}
}

#endif