#include "hmsJNI.hpp"

#include <cassert>

namespace hms
{
namespace ext
{
namespace jni
{

    /* SmartEnvironment */

    SmartEnvironment::SmartEnvironment(JavaVM* pJavaVM) : mJavaVM(pJavaVM)
    {
        assert(mJavaVM != nullptr);

        if (mJavaVM->GetEnv((void**)&mEnvironment, JNI_VERSION_1_6) == JNI_EDETACHED && mJavaVM->AttachCurrentThread(&mEnvironment, nullptr) == JNI_OK)
            mDetachCurrentThread = true;
    }

    SmartEnvironment::~SmartEnvironment()
    {
        if (mDetachCurrentThread)
            mJavaVM->DetachCurrentThread();

    }

    SmartEnvironment& SmartEnvironment::operator=(SmartEnvironment&& pOther)
    {
        if (&pOther == this)
            return *this;

        mJavaVM = pOther.mJavaVM;
        mEnvironment = pOther.mEnvironment;
        mDetachCurrentThread = pOther.mDetachCurrentThread;

        pOther.mJavaVM = nullptr;
        pOther.mEnvironment = nullptr;
        pOther.mDetachCurrentThread = false;
        
        return *this;
    }

    JNIEnv* SmartEnvironment::getJNIEnv() const
    {
        return mEnvironment;
    }
    
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
            SmartEnvironment smartEnvironment;
            smartEnvironment = SmartEnvironment(mJavaVM);
            JNIEnv* environment = smartEnvironment.getJNIEnv();
            if (environment != nullptr)
                environment->DeleteGlobalRef(mObject);
        }
    }

    jobject SmartRef::getObject() const
    {
        return mObject;
    }

    void SmartRef::getEnvironment(SmartEnvironment& pSmartEnvironment) const
    {
        pSmartEnvironment = SmartEnvironment(mJavaVM);
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
            SmartEnvironment smartEnvironment;
            smartEnvironment = SmartEnvironment(mJavaVM);
            JNIEnv* environment = smartEnvironment.getJNIEnv();
            if (environment != nullptr)
                environment->DeleteWeakGlobalRef(mObject);
        }
    }

    jobject SmartWeakRef::getWeakObject() const
    {
        return mObject;
    }

    void SmartWeakRef::getEnvironment(SmartEnvironment& pSmartEnvironment) const
    {
        pSmartEnvironment = SmartEnvironment(mJavaVM);
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
