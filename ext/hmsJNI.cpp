// Copyright (C) 2017-2022 Grupa Pracuj S.A.
// This file is part of the "Hermes" library.
// For conditions of distribution and use, see copyright notice in license.txt.

#include "hmsJNI.hpp"

extern "C" JNIEXPORT void JNICALL Java_pl_grupapracuj_hermes_ext_jni_ObjectNative_nativeDestroy(JNIEnv* pEnvironment, jobject pObject, jlong pPointer)
{
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
            if (mVM != nullptr)
                mVM->DetachCurrentThread();
        }

        Environment& operator=(const Environment& pOther) = delete;
        Environment& operator=(Environment&& pOther) = delete;

        JNIEnv* initialize(JavaVM* pVM)
        {
            if (pVM->GetEnv((void**)&mEnvironment, JNI_VERSION_1_6) == JNI_EDETACHED && pVM->AttachCurrentThread(&mEnvironment, nullptr) == JNI_OK)
                mVM = pVM;

            return mEnvironment;
        }

        JNIEnv* environmentJNI() const
        {
            return mEnvironment;
        }

    private:        
        JavaVM* mVM = nullptr;
        JNIEnv* mEnvironment = nullptr;
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

    jobject ObjectNative::object(void* pObject, JNIEnv* pEnvironment, jclass pClass)
    {
        if (pEnvironment == nullptr)
            throw std::invalid_argument("hmsJNI: parameter is null");

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

            pEnvironment->SetLongField(result, fieldPointerJNI, reinterpret_cast<jlong>(pObject));
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

    JavaVM* Utility::mJavaVM = nullptr;
    std::array<std::pair<jclass, bool>, static_cast<size_t>(Utility::EClass::COUNT)> Utility::mClasses = {};
    std::array<jmethodID, static_cast<size_t>(Utility::EMethodId::COUNT)> Utility::mMethodIds = {};
    
    void Utility::initialize(JNIEnv* pEnvironment, jobject pClassLoader)
    {
        std::array<std::string, 18> classNames = {{
            "java/lang/Boolean",
            "java/lang/Byte",
            "java/lang/Double",
            "java/lang/Float",
            "java/lang/Integer",
            "java/lang/Long",
            "java/lang/Short",
            "java/lang/String",
            "[B",
            "pl/grupapracuj/hermes/ext/jni/ObjectNative",
            "pl/grupapracuj/hermes/ext/tuple/Nontuple",
            "pl/grupapracuj/hermes/ext/tuple/Octuple",
            "pl/grupapracuj/hermes/ext/tuple/Pair",
            "pl/grupapracuj/hermes/ext/tuple/Quadruple",
            "pl/grupapracuj/hermes/ext/tuple/Quintuple",
            "pl/grupapracuj/hermes/ext/tuple/Septuple",
            "pl/grupapracuj/hermes/ext/tuple/Sextuple",
            "pl/grupapracuj/hermes/ext/tuple/Triple"
        }};
        static_assert(classNames.size() == static_cast<size_t>(EClass::COUNT), "Class names table needs to be updated.");

        classLoad(pEnvironment, pClassLoader, mClasses.data(), classNames.data(), mClasses.size());

        mMethodIds[static_cast<size_t>(EMethodId::java_lang_Boolean_init)] = pEnvironment->GetMethodID(mClasses[static_cast<size_t>(EClass::java_lang_Boolean)].first, "<init>", "(Z)V");
        mMethodIds[static_cast<size_t>(EMethodId::java_lang_Byte_init)] = pEnvironment->GetMethodID(mClasses[static_cast<size_t>(EClass::java_lang_Byte)].first, "<init>", "(B)V");
        mMethodIds[static_cast<size_t>(EMethodId::java_lang_Double_init)] = pEnvironment->GetMethodID(mClasses[static_cast<size_t>(EClass::java_lang_Double)].first, "<init>", "(D)V");
        mMethodIds[static_cast<size_t>(EMethodId::java_lang_Float_init)] = pEnvironment->GetMethodID(mClasses[static_cast<size_t>(EClass::java_lang_Float)].first, "<init>", "(F)V");
        mMethodIds[static_cast<size_t>(EMethodId::java_lang_Integer_init)] = pEnvironment->GetMethodID(mClasses[static_cast<size_t>(EClass::java_lang_Integer)].first, "<init>", "(I)V");
        mMethodIds[static_cast<size_t>(EMethodId::java_lang_Long_init)] = pEnvironment->GetMethodID(mClasses[static_cast<size_t>(EClass::java_lang_Long)].first, "<init>", "(J)V");
        mMethodIds[static_cast<size_t>(EMethodId::java_lang_Short_init)] = pEnvironment->GetMethodID(mClasses[static_cast<size_t>(EClass::java_lang_Short)].first, "<init>", "(S)V");
        mMethodIds[static_cast<size_t>(EMethodId::pl_grupapracuj_pracuj_ext_tuple_Nontuple_init)] = pEnvironment->GetMethodID(mClasses[static_cast<size_t>(EClass::pl_grupapracuj_pracuj_ext_tuple_Nontuple)].first, "<init>", "(Ljava/lang/Object;Ljava/lang/Object;Ljava/lang/Object;Ljava/lang/Object;Ljava/lang/Object;Ljava/lang/Object;Ljava/lang/Object;Ljava/lang/Object;Ljava/lang/Object;)V");
        mMethodIds[static_cast<size_t>(EMethodId::pl_grupapracuj_pracuj_ext_tuple_Octuple_init)] = pEnvironment->GetMethodID(mClasses[static_cast<size_t>(EClass::pl_grupapracuj_pracuj_ext_tuple_Octuple)].first, "<init>", "(Ljava/lang/Object;Ljava/lang/Object;Ljava/lang/Object;Ljava/lang/Object;Ljava/lang/Object;Ljava/lang/Object;Ljava/lang/Object;Ljava/lang/Object;)V");
        mMethodIds[static_cast<size_t>(EMethodId::pl_grupapracuj_pracuj_ext_tuple_Pair_init)] = pEnvironment->GetMethodID(mClasses[static_cast<size_t>(EClass::pl_grupapracuj_pracuj_ext_tuple_Pair)].first, "<init>", "(Ljava/lang/Object;Ljava/lang/Object;)V");
        mMethodIds[static_cast<size_t>(EMethodId::pl_grupapracuj_pracuj_ext_tuple_Quadruple_init)] = pEnvironment->GetMethodID(mClasses[static_cast<size_t>(EClass::pl_grupapracuj_pracuj_ext_tuple_Quadruple)].first, "<init>", "(Ljava/lang/Object;Ljava/lang/Object;Ljava/lang/Object;Ljava/lang/Object;)V");
        mMethodIds[static_cast<size_t>(EMethodId::pl_grupapracuj_pracuj_ext_tuple_Quintuple_init)] = pEnvironment->GetMethodID(mClasses[static_cast<size_t>(EClass::pl_grupapracuj_pracuj_ext_tuple_Quintuple)].first, "<init>", "(Ljava/lang/Object;Ljava/lang/Object;Ljava/lang/Object;Ljava/lang/Object;Ljava/lang/Object;)V");
        mMethodIds[static_cast<size_t>(EMethodId::pl_grupapracuj_pracuj_ext_tuple_Septuple_init)] = pEnvironment->GetMethodID(mClasses[static_cast<size_t>(EClass::pl_grupapracuj_pracuj_ext_tuple_Septuple)].first, "<init>", "(Ljava/lang/Object;Ljava/lang/Object;Ljava/lang/Object;Ljava/lang/Object;Ljava/lang/Object;Ljava/lang/Object;Ljava/lang/Object;)V");
        mMethodIds[static_cast<size_t>(EMethodId::pl_grupapracuj_pracuj_ext_tuple_Sextuple_init)] = pEnvironment->GetMethodID(mClasses[static_cast<size_t>(EClass::pl_grupapracuj_pracuj_ext_tuple_Sextuple)].first, "<init>", "(Ljava/lang/Object;Ljava/lang/Object;Ljava/lang/Object;Ljava/lang/Object;Ljava/lang/Object;Ljava/lang/Object;)V");
        mMethodIds[static_cast<size_t>(EMethodId::pl_grupapracuj_pracuj_ext_tuple_Triple_init)] = pEnvironment->GetMethodID(mClasses[static_cast<size_t>(EClass::pl_grupapracuj_pracuj_ext_tuple_Triple)].first, "<init>", "(Ljava/lang/Object;Ljava/lang/Object;Ljava/lang/Object;)V");
    }

    JavaVM* Utility::javaVM()
    {
        assert(mJavaVM != nullptr);
        return mJavaVM;
    }

    void Utility::javaVM(JavaVM* pJavaVM)
    {
        mJavaVM = pJavaVM;
    }

    bool Utility::classLoad(JNIEnv* pEnvironment, jobject pClassLoader, std::pair<jclass, bool>* pClasses, std::string* pSignatures, size_t pCount)
    {
        jclass jClassLoaderClass = pEnvironment->GetObjectClass(pClassLoader);
        jmethodID jClassLoaderLoadClass = pEnvironment->GetMethodID(jClassLoaderClass, "loadClass", "(Ljava/lang/String;)Ljava/lang/Class;");

        for (size_t i = 0; i < pCount; ++i)
        {
            if (pClasses[i].second)
            {
                pEnvironment->DeleteGlobalRef(pClasses[i].first);
                pClasses[i].second = false;
            }

            jstring jClassName = pEnvironment->NewStringUTF(pSignatures[i].c_str());
            jobject jClass = pEnvironment->CallObjectMethod(pClassLoader, jClassLoaderLoadClass, jClassName);

            if (!pEnvironment->ExceptionCheck())
            {
                pClasses[i].first = static_cast<jclass>(pEnvironment->NewGlobalRef(jClass));
                pClasses[i].second = true;
            }
            else
            {
                return false;
            }

            pEnvironment->DeleteLocalRef(jClass);
            pEnvironment->DeleteLocalRef(jClassName);
        }

        pEnvironment->DeleteLocalRef(jClassLoaderClass);

        return true;
    }

    std::string Utility::jconvert(JNIEnv* pEnvironment, jstring pValue)
    {
        std::string result;
        if (pValue != nullptr && pEnvironment != nullptr)
        {
            const char* tmp = pEnvironment->GetStringUTFChars(pValue, nullptr);
            if (tmp != nullptr)
            {
                const jsize length = pEnvironment->GetStringUTFLength(pValue);
                result = std::string(tmp, length);
                pEnvironment->ReleaseStringUTFChars(pValue, tmp);
            }
        }

        return result;
    }
}
}
}
