#include "hmsJNI.hpp"

#include <cassert>

namespace hms
{
namespace ext
{
    
    /* JNISmartRef */
    
    JNISmartRef::JNISmartRef(jobject pObject, JNIEnv* pEnvironment)
    {
        assert(pObject != nullptr && pEnvironment != nullptr);

        jint status = pEnvironment->GetJavaVM(&mJavaVM);
        if (status == JNI_OK)
            mObject = pEnvironment->NewGlobalRef(pObject);
    }

    JNISmartRef::~JNISmartRef()
    {
        if (mObject != nullptr)
        {
            JNIEnv* environment = getEnvironment();
            if (environment != nullptr)
                environment->DeleteGlobalRef(mObject);
        }
    }

    jobject JNISmartRef::getObject() const
    {
        return mObject;
    }

    JNIEnv* JNISmartRef::getEnvironment() const
    {
        JNIEnv* environment = nullptr;
        if (mJavaVM != nullptr)
            mJavaVM->GetEnv((void**)&environment, JNI_VERSION_1_6);

        return environment;
    }
    
    /* JNISmartWeakRef */

    JNISmartWeakRef::JNISmartWeakRef(jobject pObject, JNIEnv* pEnvironment)
    {
        assert(pObject != nullptr && pEnvironment != nullptr);

        jint status = pEnvironment->GetJavaVM(&mJavaVM);
        if (status == JNI_OK)
            mObject = pEnvironment->NewWeakGlobalRef(pObject);
    }

    JNISmartWeakRef::~JNISmartWeakRef()
    {
        if (mObject != nullptr)
        {
            JNIEnv* environment = getEnvironment();
            if (environment != nullptr)
                environment->DeleteWeakGlobalRef(mObject);
        }
    }

    jobject JNISmartWeakRef::getWeakObject() const
    {
        return mObject;
    }

    JNIEnv* JNISmartWeakRef::getEnvironment() const
    {
        JNIEnv* environment = nullptr;
        if (mJavaVM != nullptr)
            mJavaVM->GetEnv((void**)&environment, JNI_VERSION_1_6);

        return environment;
    }
    
    /* JNISmartRefHandler */

    JNISmartRefHandler::~JNISmartRefHandler()
    {
        flush();
    }

    void JNISmartRefHandler::flush()
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

    void JNISmartRefHandler::push(std::shared_ptr<JNISmartRef> pSmartRef)
    {
        mSmartRef.push_back(std::move(pSmartRef));
    }

    void JNISmartRefHandler::push(std::shared_ptr<JNISmartWeakRef> pSmartWeakRef)
    {
        mSmartWeakRef.push_back(std::move(pSmartWeakRef));
    }

    JNISmartRefHandler* JNISmartRefHandler::getInstance()
    {
        static JNISmartRefHandler instance;

        return &instance;
    }
    
}
}
