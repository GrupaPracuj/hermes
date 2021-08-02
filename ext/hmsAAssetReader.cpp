// Copyright (C) 2017-2021 Grupa Pracuj S.A.
// This file is part of the "Hermes" library.
// For conditions of distribution and use, see copyright notice in license.txt.

#include "hmsAAssetReader.hpp"

namespace hms
{
namespace ext
{
namespace jni
{

    /* AAssetStorageReader */

    AAssetStorageReader::AAssetStorageReader(JNIEnv* pEnvironment, jobject pAssetManager, const std::string& pPrefix) : DataStorageReader(pPrefix)
    {
        assert(pAssetManager != nullptr && pEnvironment != nullptr);

        jint status = pEnvironment->GetJavaVM(&mJavaVM);
        if (status == JNI_OK)
        {
            mObject = pEnvironment->NewGlobalRef(pAssetManager);
            mAssetManager = AAssetManager_fromJava(pEnvironment, pAssetManager);
        }
    
        addSubfolder("");
    }

    AAssetStorageReader::AAssetStorageReader(AAssetManager* pAssetManager, const std::string& pPrefix) : DataStorageReader(pPrefix), mAssetManager(pAssetManager)
    {
        addSubfolder("");
    }

    AAssetStorageReader::~AAssetStorageReader()
    {
        if (mObject != nullptr)
        {
            JNIEnv* environment = nullptr;
            if (mJavaVM != nullptr)
                mJavaVM->GetEnv((void**)&environment, JNI_VERSION_1_6);
            
            if (environment != nullptr)
                environment->DeleteGlobalRef(mObject);
        }
    }

    std::unique_ptr<DataReader> AAssetStorageReader::openFile(const std::string& pName) const
    {
        std::unique_ptr<DataReader> reader = nullptr;
            
        for (auto& name : mFileName)
        {
            if (mPrefix + name == pName)
            {
                reader = std::unique_ptr<DataReader>(new AAssetReader(mAssetManager, name));
                break;
            }
        }
        
        return reader;
    }

    void AAssetStorageReader::addSubfolder(const std::string& pName)
    {
        if (mAssetManager != nullptr)
        {
            AAssetDir* dir = AAssetManager_openDir(mAssetManager, pName.c_str());
            if (dir != nullptr)
            {
                while (const char* filename = AAssetDir_getNextFileName(dir))
                    mFileName.push_back(filename);
            
                AAssetDir_close(dir);
            }
        }
    }

    /* AAssetReader */

    AAssetReader::AAssetReader(AAssetManager* pAssetManager, const std::string& pFilePath)
    {
        mAsset = AAssetManager_open(pAssetManager, pFilePath.c_str(), AASSET_MODE_RANDOM);
    }

    AAssetReader::~AAssetReader()
    {
        if (mAsset != nullptr)
            AAsset_close(mAsset);
    }

    size_t AAssetReader::read(void* pBuffer)
    {
        return read(pBuffer, static_cast<size_t>(AAsset_getRemainingLength(mAsset)));
    }

    size_t AAssetReader::read(void* pBuffer, size_t pSize)
    {
        const size_t size = getSize();
        const size_t position = getPosition();
        const size_t readCount = position + pSize > size ? size - position : pSize;
        const int readBytes = AAsset_read(mAsset, pBuffer, readCount);
        return readBytes < 0 ? 0 : static_cast<size_t>(readBytes);
    }

    void AAssetReader::read(std::string& pText)
    {
        read(pText, static_cast<size_t>(AAsset_getRemainingLength(mAsset)));
    }

    void AAssetReader::read(std::string& pText, size_t pSize)
    {
        const size_t size = getSize();
        const size_t position = getPosition();
        const size_t readCount = position + pSize > size ? size - position : pSize;
        const size_t offset = pText.size();
            
        pText.resize(offset + readCount, 0);
        const int readBytes = AAsset_read(mAsset, &pText[0], readCount);
                
        if (readBytes < 0)
            pText.resize(offset);   
        else if (readBytes < readCount)
            pText.resize(offset + readBytes);
    }

    void AAssetReader::read(DataBuffer& pBuffer)
    {
        read(pBuffer, static_cast<size_t>(AAsset_getRemainingLength(mAsset)));
    }

    void AAssetReader::read(DataBuffer& pBuffer, size_t pSize)
    {
        const size_t size = getSize();
        const size_t position = getPosition();
        const size_t readCount = position + pSize > size ? size - position : pSize;
        const size_t offset = pBuffer.size();  
        const size_t readMaxSize = 1024;
    
        char buff[readMaxSize];
        int read = 0;
        do
        {
            int r = AAsset_read(mAsset, buff, std::min(readCount - read, readMaxSize));
            if (r < 0)
                break;
        
            pBuffer.push_back(buff, static_cast<size_t>(r));
            read += r;
        }
        while (read < readCount);
    }

    bool AAssetReader::seek(int64_t pPosition, bool pRelative)
    {
        return static_cast<bool>(AAsset_seek(mAsset, static_cast<off_t>(pPosition), pRelative ? SEEK_CUR : SEEK_SET) + 1);
    }

    size_t AAssetReader::getPosition() const 
    {
        return static_cast<size_t>(AAsset_getLength(mAsset) - AAsset_getRemainingLength(mAsset));
    }

    size_t AAssetReader::getSize() const 
    {
        return static_cast<size_t>(AAsset_getLength(mAsset));
    }

    bool AAssetReader::isOpen() const
    {
        return mAsset != nullptr && AAsset_getLength(mAsset) != 0;
    }

}
}
}
        