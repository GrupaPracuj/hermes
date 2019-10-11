// Copyright (C) 2017-2019 Grupa Pracuj Sp. z o.o.
// This file is part of the "Hermes" library.
// For conditions of distribution and use, see copyright notice in license.txt.

#include "hmsJNI.hpp"

extern "C" JNIEXPORT void JNICALL Java_pl_grupapracuj_hermes_ext_jni_ObjectNative_nativeDestroy(JNIEnv* pEnvironment, jobject pObject, jlong pPointer)
{
    if (pPointer > 0)
        delete reinterpret_cast<hms::ext::jni::ObjectNative::ContainerGeneric*>(pPointer);
}

namespace hms
{
namespace ext
{
namespace jni
{

    /* Environment */

    class Environment
    {
    public:
        Environment() = default;
        Environment(Environment& pOther) = delete;
        Environment(Environment&& pOther) = delete;

        ~Environment()
        {
            if (mDetachCurrentThread)
                mVM->DetachCurrentThread();
        }

        Environment& operator=(const Environment& pOther) = delete;
        Environment& operator=(Environment&& pOther) = delete;

        JNIEnv* initialize(JavaVM* pVM)
        {
            if (pVM->GetEnv((void**)&mEnvironment, JNI_VERSION_1_6) == JNI_EDETACHED && pVM->AttachCurrentThread(&mEnvironment, nullptr) == JNI_OK)
            {
                mVM = pVM;
                mDetachCurrentThread = true;
            }

            return mEnvironment;
        }

        JNIEnv* environmentJNI() const
        {
            return mEnvironment;
        }

    private:        
        JavaVM* mVM = nullptr;
        JNIEnv* mEnvironment = nullptr;
        bool mDetachCurrentThread = false;
    };

    thread_local Environment gEnvironment;
    
    /* Reference */
    
    Reference::Reference(jobject pObject, JNIEnv* pEnvironment) : mObject(nullptr), mVM(nullptr)
    {
        if (pObject == nullptr || pEnvironment == nullptr)
            throw std::invalid_argument("hmsJNI: one or more parameter is null");

        if (pEnvironment->GetJavaVM(&mVM) == JNI_OK)
            mObject = pEnvironment->NewGlobalRef(pObject);
        else
            throw std::invalid_argument("hmsJNI: couldn't get JavaVM");
    }

    Reference::Reference(Reference&& pOther)
    {
        mObject = pOther.mObject;
        mVM = pOther.mVM;

        pOther.mObject = nullptr;
        pOther.mVM = nullptr;
    }

    Reference::~Reference()
    {
        if (mObject != nullptr)
        {
            try
            {
                environment()->DeleteGlobalRef(mObject);
            }
            catch (const std::exception& lpException) {}
        }
    }

    Reference& Reference::operator=(Reference&& pOther)
    {
        if (&pOther == this)
            return *this;

        mObject = pOther.mObject;
        mVM = pOther.mVM;

        pOther.mObject = nullptr;
        pOther.mVM = nullptr;
        
        return *this;
    }

    jobject Reference::object() const
    {
        return mObject;
    }

    JNIEnv* Reference::environment() const
    {
        auto environmentJNI = gEnvironment.environmentJNI();
        if (environmentJNI == nullptr)
        {
            environmentJNI = gEnvironment.initialize(mVM);
            if (environmentJNI == nullptr)
                throw std::runtime_error("hmsJNI: couldn't get JNIEnv");
        }

        return environmentJNI;
    }
    
    /* ReferenceWeak */

    ReferenceWeak::ReferenceWeak(jobject pObject, JNIEnv* pEnvironment) : mObjectWeak(nullptr), mVM(nullptr)
    {
        if (pObject == nullptr || pEnvironment == nullptr)
            throw std::invalid_argument("hmsJNI: one or more parameter is null");

        if (pEnvironment->GetJavaVM(&mVM) == JNI_OK)
            mObjectWeak = pEnvironment->NewWeakGlobalRef(pObject);
        else
            throw std::invalid_argument("hmsJNI: couldn't get JavaVM");
    }

    ReferenceWeak::ReferenceWeak(ReferenceWeak&& pOther)
    {
        mObjectWeak = pOther.mObjectWeak;
        mVM = pOther.mVM;

        pOther.mObjectWeak = nullptr;
        pOther.mVM = nullptr;
    }

    ReferenceWeak::~ReferenceWeak()
    {
        if (mObjectWeak != nullptr)
        {
            try
            {
                environment()->DeleteWeakGlobalRef(mObjectWeak);
            }
            catch (const std::exception& lpException) {}
        }
    }

    ReferenceWeak& ReferenceWeak::operator=(ReferenceWeak&& pOther)
    {
        if (&pOther == this)
            return *this;

        mObjectWeak = pOther.mObjectWeak;
        mVM = pOther.mVM;

        pOther.mObjectWeak = nullptr;
        pOther.mVM = nullptr;
        
        return *this;
    }

    jobject ReferenceWeak::objectWeak() const
    {
        return mObjectWeak;
    }

    JNIEnv* ReferenceWeak::environment() const
    {
        auto environmentJNI = gEnvironment.environmentJNI();
        if (environmentJNI == nullptr)
        {
            environmentJNI = gEnvironment.initialize(mVM);
            if (environmentJNI == nullptr)
                throw std::runtime_error("hmsJNI: couldn't get JNIEnv");
        }

        return environmentJNI;
    }

    /* ObjectNative */

    jobject ObjectNative::object(jlong pPointer, JNIEnv* pEnvironment, jclass pClass)
    {
        if (pPointer <= 0 || pEnvironment == nullptr)
            throw std::invalid_argument("hmsJNI: one or more parameter is null");

        bool deleteLocalRef = false;
        jclass classJNI = pClass;
        if (classJNI == nullptr)
        {
            classJNI = pEnvironment->FindClass("pl/grupapracuj/hermes/ext/jni/ObjectNative");
            if (pEnvironment->ExceptionCheck())
                throw std::runtime_error("hmsJNI: JNI method 'FindClass' failed");

            deleteLocalRef = true;
        }

        jobject result = nullptr;

        if (classJNI != nullptr)
        {
            jmethodID methodInitJNI = pEnvironment->GetMethodID(classJNI, "<init>", "()V");
            if (pEnvironment->ExceptionCheck())
            {
                if (deleteLocalRef)
                    pEnvironment->DeleteLocalRef(classJNI);

                throw std::runtime_error("hmsJNI: JNI method 'GetMethodID' failed");
            }

            jfieldID fieldPointerJNI = pEnvironment->GetFieldID(classJNI, "mPointer", "J");
            if (pEnvironment->ExceptionCheck())
            {
                if (deleteLocalRef)
                    pEnvironment->DeleteLocalRef(classJNI);

                throw std::runtime_error("hmsJNI: JNI method 'GetFieldID' failed");
            }

            result = pEnvironment->NewObject(classJNI, methodInitJNI);
            if (pEnvironment->ExceptionCheck())
            {
                if (deleteLocalRef)
                    pEnvironment->DeleteLocalRef(classJNI);

                throw std::runtime_error("hmsJNI: JNI method 'NewObject' failed");
            }

            pEnvironment->SetLongField(result, fieldPointerJNI, pPointer);
            if (pEnvironment->ExceptionCheck())
            {
                if (deleteLocalRef)
                    pEnvironment->DeleteLocalRef(classJNI);

                throw std::runtime_error("hmsJNI: JNI method 'SetLongField' failed");
            }
        }

        if (deleteLocalRef)
            pEnvironment->DeleteLocalRef(classJNI);

        return result;
    }

    jlong ObjectNative::pointer(jobject pObject, JNIEnv* pEnvironment)
    {
        if (pObject == nullptr || pEnvironment == nullptr)
            throw std::invalid_argument("hmsJNI: one or more parameter is null");

        jclass classJNI = pEnvironment->GetObjectClass(pObject);

        jfieldID fieldPointerJNI = pEnvironment->GetFieldID(classJNI, "mPointer", "J");        
        if (pEnvironment->ExceptionCheck())
        {
            pEnvironment->DeleteLocalRef(classJNI);
            throw std::runtime_error("hmsJNI: JNI method 'GetFieldID' failed");
        }

        jlong result = pEnvironment->GetLongField(pObject, fieldPointerJNI);

        pEnvironment->DeleteLocalRef(classJNI);

        return result;
    }
    
    /* Utility */
    
    std::string Utility::string(jstring pData, JNIEnv* pEnvironment)
    {
        std::string result;
        
        if (pData != nullptr && pEnvironment != nullptr)
        {
            const char* tmp = pEnvironment->GetStringUTFChars(pData, nullptr);            
            if (tmp != nullptr)
            {
                const jsize length = pEnvironment->GetStringUTFLength(pData);
                result = std::string(tmp, length);
                pEnvironment->ReleaseStringUTFChars(pData, tmp);
            }
        }
        
        return result;
    }
    
}
}
}
