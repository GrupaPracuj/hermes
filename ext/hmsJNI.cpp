#include "hmsJNI.hpp"

#include <cassert>

namespace hms
{
namespace ext
{
namespace jni
{
    
    /* SmartRef */
    
    SmartRef::SmartRef(jobject pObject, JNIEnv* pEnvironment)
    {
        assert(pObject != nullptr && pEnvironment != nullptr);

        jint status = pEnvironment->GetJavaVM(&mJavaVM);
        if (status == JNI_OK)
            mObject = pEnvironment->NewGlobalRef(pObject);
    }

    SmartRef::~SmartRef()
    {
        if (mObject != nullptr)
        {
            JNIEnv* environment = getEnvironment();
            if (environment != nullptr)
                environment->DeleteGlobalRef(mObject);
        }
    }

    jobject SmartRef::getObject() const
    {
        return mObject;
    }

    JNIEnv* SmartRef::getEnvironment() const
    {
        JNIEnv* environment = nullptr;
        if (mJavaVM != nullptr)
            mJavaVM->GetEnv((void**)&environment, JNI_VERSION_1_6);

        return environment;
    }
    
    /* SmartWeakRef */

    SmartWeakRef::SmartWeakRef(jobject pObject, JNIEnv* pEnvironment)
    {
        assert(pObject != nullptr && pEnvironment != nullptr);

        jint status = pEnvironment->GetJavaVM(&mJavaVM);
        if (status == JNI_OK)
            mObject = pEnvironment->NewWeakGlobalRef(pObject);
    }

    SmartWeakRef::~SmartWeakRef()
    {
        if (mObject != nullptr)
        {
            JNIEnv* environment = getEnvironment();
            if (environment != nullptr)
                environment->DeleteWeakGlobalRef(mObject);
        }
    }

    jobject SmartWeakRef::getWeakObject() const
    {
        return mObject;
    }

    JNIEnv* SmartWeakRef::getEnvironment() const
    {
        JNIEnv* environment = nullptr;
        if (mJavaVM != nullptr)
            mJavaVM->GetEnv((void**)&environment, JNI_VERSION_1_6);

        return environment;
    }
    
    /* SmartRefHandler */

    SmartRefHandler::~SmartRefHandler()
    {
        flush();
    }

    void SmartRefHandler::flush()
    {
        {
            auto it = mSmartRef.begin();

            while (it != mSmartRef.end())
            {
                if (it->use_count() == 1)
                    it = mSmartRef.erase(it);
                else
                    ++it;
            }
        }

        {
            auto it = mSmartWeakRef.begin();

            while (it != mSmartWeakRef.end())
            {
                if (it->use_count() == 1)
                    it = mSmartWeakRef.erase(it);
                else
                    ++it;
            }
        }
    }

    void SmartRefHandler::push(std::shared_ptr<SmartRef> pSmartRef)
    {
        mSmartRef.push_back(std::move(pSmartRef));
    }

    void SmartRefHandler::push(std::shared_ptr<SmartWeakRef> pSmartWeakRef)
    {
        mSmartWeakRef.push_back(std::move(pSmartWeakRef));
    }

    SmartRefHandler* SmartRefHandler::getInstance()
    {
        static SmartRefHandler instance;

        return &instance;
    }
    
    /* Utility */
    
    std::string Utility::string(jstring pData, JNIEnv* pEnvironment)
    {
        assert(pEnvironment != nullptr);
        
        std::string data;
        
        if (pData != nullptr)
        {
            const char* tmp = pEnvironment->GetStringUTFChars(pData, nullptr);
            
            if (tmp)
            {
                const jsize length = pEnvironment->GetStringUTFLength(pData);
                data = std::string(tmp, length);
                pEnvironment->ReleaseStringUTFChars(pData, tmp);
            }
        }
        
        return data;
    }
    
}
}
}
