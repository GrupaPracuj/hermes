// Copyright (C) 2017-2019 Grupa Pracuj Sp. z o.o.
// This file is part of the "Hermes" library.
// For conditions of distribution and use, see copyright notice in license.txt.

#ifndef _HMS_JNI_HPP_
#define _HMS_JNI_HPP_

#include <jni.h>
#include <string>

extern "C" JNIEXPORT void JNICALL Java_pl_grupapracuj_hermes_ext_jni_ObjectNative_nativeDestroy(JNIEnv* pEnvironment, jobject pObject, jlong pPointer);

namespace hms
{
namespace ext
{
namespace jni
{
    
    class Reference
    {
    public:
        Reference(jobject pObject, JNIEnv* pEnvironment);
        Reference(const Reference& pOther) = delete;
        Reference(Reference&& pOther);
        ~Reference();

        Reference& operator=(const Reference& pOther) = delete;
        Reference& operator=(Reference&& pOther);
        
        jobject object() const;
        JNIEnv* environment() const;
        
    private:
        jobject mObject;
        JavaVM* mVM;
    };
    
    class ReferenceWeak
    {
    public:
        ReferenceWeak(jobject pObject, JNIEnv* pEnvironment);
        ReferenceWeak(const ReferenceWeak& pOther) = delete;
        ReferenceWeak(ReferenceWeak&& pOther);
        ~ReferenceWeak();

        ReferenceWeak& operator=(const ReferenceWeak& pOther) = delete;
        ReferenceWeak& operator=(ReferenceWeak&& pOther);
        
        jobject objectWeak() const;
        JNIEnv* environment() const;
        
    private:
        jobject mObjectWeak;
        JavaVM* mVM;
    };

    class ObjectNative
    {
    public:
        ObjectNative() = delete;
        ObjectNative(const ObjectNative& pOther) = delete;
        ObjectNative(ObjectNative&& pOther) = delete;

        ObjectNative& operator=(const ObjectNative& pOther) = delete;
        ObjectNative& operator=(ObjectNative&& pOther) = delete;

        template<typename T>
        static jobject create(T pObject, JNIEnv* pEnvironment, jclass pClass = nullptr)
        {
            auto container = new Container<T>();
            container->mObject = std::move(pObject);

            auto result = object(reinterpret_cast<jlong>(container), pEnvironment, pClass);
            if (result == nullptr)
                delete container;

            return result;
        }

        template<typename T>
        static T& get(jobject pObject, JNIEnv* pEnvironment)
        {
            return get<T>(pointer(pObject, pEnvironment));
        }

        template<typename T>
        static T& get(jlong pPointer)
        {
            if (pPointer <= 0)
                throw std::invalid_argument("hmsJNI: parameter is null");

            auto container = reinterpret_cast<Container<T>*>(pPointer);
            return container->mObject;
        }

    private:
        friend void ::Java_pl_grupapracuj_hermes_ext_jni_ObjectNative_nativeDestroy(JNIEnv* pEnvironment, jobject pObject, jlong pPointer);

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

        static jobject object(jlong pPointer, JNIEnv* pEnvironment, jclass pClass);
        static jlong pointer(jobject pObject, JNIEnv* pEnvironment);
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
