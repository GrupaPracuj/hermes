// Copyright (C) 2017-2019 Grupa Pracuj Sp. z o.o.
// This file is part of the "Hermes" library.
// For conditions of distribution and use, see copyright notice in license.txt.

#ifndef _HMS_JNI_HPP_
#define _HMS_JNI_HPP_

#include <jni.h>
#include <string>

extern "C" JNIEXPORT void JNICALL Java_pl_grupapracuj_hermes_ext_jni_NativeObject_nativeDestroy(JNIEnv* pEnvironment, jobject pObject, jlong pPointer);

namespace hms
{
namespace ext
{
namespace jni
{

    class SmartEnvironment
    {
        friend class SmartRef;
        friend class SmartWeakRef;
    public:
        SmartEnvironment() = default;
        SmartEnvironment(JavaVM* pJavaVM);
        SmartEnvironment(SmartEnvironment&& pOther) = delete;
        SmartEnvironment(const SmartEnvironment& pOther) = delete;
        ~SmartEnvironment();

        SmartEnvironment& operator=(const SmartEnvironment& pOther) = delete;

        JNIEnv* getJNIEnv() const;

    private:        
        SmartEnvironment& operator=(SmartEnvironment&& pOther);

        JavaVM* mJavaVM = nullptr;
        JNIEnv* mEnvironment = nullptr;
        bool mDetachCurrentThread = false;
    };
    
    class SmartRef
    {
    public:
        SmartRef(jobject pObject, JNIEnv* pEnvironment);
        ~SmartRef();
        
        jobject getObject() const;
        void getEnvironment(SmartEnvironment& pSmartEnvironment) const;
        
    private:
        jobject mObject = nullptr;
        JavaVM* mJavaVM = nullptr;
    };
    
    class SmartWeakRef
    {
    public:
        SmartWeakRef(jobject pObject, JNIEnv* pEnvironment);
        ~SmartWeakRef();
        
        jobject getWeakObject() const;
        void getEnvironment(SmartEnvironment& pSmartEnvironment) const;
        
    private:
        jobject mObject = nullptr;
        JavaVM* mJavaVM = nullptr;
    };

    class NativeObject
    {
    public:
        NativeObject() = delete;
        NativeObject(const NativeObject& pOther) = delete;
        NativeObject(NativeObject&& pOther) = delete;

        NativeObject& operator=(const NativeObject& pOther) = delete;
        NativeObject& operator=(NativeObject&& pOther) = delete;

        template<typename T>
        static jobject create(const T& pObject, JNIEnv* pEnvironment, jclass pClass = nullptr)
        {
            auto container = new Container<T>();
            container->mObject = pObject;

            return createObject(reinterpret_cast<jlong>(container), pEnvironment, pClass);
        }

        template<typename T>
        static jobject create(T&& pObject, JNIEnv* pEnvironment, jclass pClass = nullptr)
        {
            auto container = new Container<T>();
            container->mObject = pObject;

            return createObject(reinterpret_cast<jlong>(container), pEnvironment, pClass);
        }

        template<typename T>
        static T& get(jobject pObject, JNIEnv* pEnvironment)
        {
            return get<T>(getPointer(pObject, pEnvironment));
        }

        template<typename T>
        static T& get(jlong pPointer)
        {
            assert(pPointer > 0);

            auto container = reinterpret_cast<Container<T>*>(pPointer);
            return container->mObject;
        }

    private:
        friend void ::Java_pl_grupapracuj_hermes_ext_jni_NativeObject_nativeDestroy(JNIEnv* pEnvironment, jobject pObject, jlong pPointer);

        class ContainerGeneric
        {
        public:
            ContainerGeneric() = default;
            virtual ~ContainerGeneric() = default;
        };

        template<typename T>
        class Container : public ContainerGeneric
        {
        public:
            T mObject;
        };

        static jobject createObject(jlong pPointer, JNIEnv* pEnvironment, jclass pClass);
        static jlong getPointer(jobject pObject, JNIEnv* pEnvironment);
    };
    
    class Utility
    {
    public:
        Utility() = delete;
        Utility(const Utility& pOther) = delete;
        Utility(Utility&& pOther) = delete;
        
        Utility& operator=(const Utility& pOther) = delete;
        Utility& operator=(Utility&& pOther) = delete;
        
        static std::string string(jstring pData, JNIEnv* pEnvironment);
    };

}
}
}

#endif
