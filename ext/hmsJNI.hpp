// Copyright (C) 2017-2020 Grupa Pracuj Sp. z o.o.
// This file is part of the "Hermes" library.
// For conditions of distribution and use, see copyright notice in license.txt.

#ifndef _HMS_JNI_HPP_
#define _HMS_JNI_HPP_

#include <jni.h>

#include <array>
#include <cassert>
#include <memory>
#include <string>
#include <utility>

extern "C" JNIEXPORT void JNICALL Java_pl_grupapracuj_hermes_ext_jni_ObjectNative_nativeDestroy(JNIEnv* pEnvironment, jobject pObject, jlong pPointer);

namespace hms
{
    class DataBuffer;

namespace ext
{
    class ModuleShared;

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

    template <typename T>
    class ObjectNativeWrapper
    {
    public:
        ObjectNativeWrapper(T pData) : mData(std::move(pData))
        {
        }

        T mData;
    };

    class Utility
    {
    public:
        Utility() = delete;
        Utility(const Utility& pOther) = delete;
        Utility(Utility&& pOther) = delete;
        
        Utility& operator=(const Utility& pOther) = delete;
        Utility& operator=(Utility&& pOther) = delete;

        template <typename T>
        struct is_pair : std::false_type {};

        template <typename T, typename U>
        struct is_pair<std::pair<T, U>> : std::true_type {};

        template <typename T>
        struct is_tuple : std::false_type {};

        template <typename... T>
        struct is_tuple<std::tuple<T...>> : std::true_type {};

        static void initialize(JNIEnv* pEnvironment, jobject pClassLoader);

        static JavaVM* javaVM();
        static void javaVM(JavaVM* pJavaVM);

        template<typename T = Utility, typename... Arguments>
        static jobject methodObject(std::shared_ptr<hms::ext::jni::ReferenceWeak> pObjectWeakRef, std::string pMethod, std::string pSignature, Arguments&&... pArguments)
        {
            assert(pObjectWeakRef != nullptr && pMethod.size() > 0 && pSignature.size() > 2);
            jobject result = nullptr;

            JNIEnv* environment = pObjectWeakRef->environment();
            jobject strongObject = environment->NewGlobalRef(pObjectWeakRef->objectWeak());
            if (strongObject != nullptr)
            {
                jclass jClass = environment->GetObjectClass(strongObject);
                jmethodID jMethod = environment->GetMethodID(jClass, pMethod.c_str(), pSignature.c_str());
                result = methodObjectCall(environment, strongObject, jMethod, T::template convert<T>(environment, std::forward<Arguments>(pArguments))...);
                environment->DeleteLocalRef(jClass);
                environment->DeleteGlobalRef(strongObject);
            }

            return result;
        }

        template<typename T = Utility, typename... Arguments>
        static void methodVoid(std::shared_ptr<hms::ext::jni::ReferenceWeak> pObjectWeakRef, std::string pMethod, std::string pSignature, Arguments&&... pArguments)
        {
            assert(pObjectWeakRef != nullptr && pMethod.size() > 0 && pSignature.size() > 2);
            JNIEnv* environment = pObjectWeakRef->environment();
            jobject strongObject = environment->NewGlobalRef(pObjectWeakRef->objectWeak());
            if (strongObject != nullptr)
            {
                jclass jClass = environment->GetObjectClass(strongObject);
                jmethodID jMethod = environment->GetMethodID(jClass, pMethod.c_str(), pSignature.c_str());
                methodVoidCall(environment, strongObject, jMethod, T::template convert<T>(environment, std::forward<Arguments>(pArguments))...);
                environment->DeleteLocalRef(jClass);
                environment->DeleteGlobalRef(strongObject);
            }
        }

        template <typename T>
        static std::shared_ptr<T> module(jlong pPointer, typename std::enable_if_t<std::is_base_of<hms::ext::ModuleShared, T>::value>* = nullptr)
        {
            std::shared_ptr<T> result = nullptr;

            try
            {
                if (pPointer > 0)
                    result = std::static_pointer_cast<T>(hms::ext::jni::ObjectNative::get<std::weak_ptr<hms::ext::ModuleShared>>(pPointer).lock());
            }
            catch (const std::exception& lpException) {}

            return result;
        }

        template <typename C = Utility, typename T>
        static auto convert(JNIEnv* pEnvironment, T pValue, typename std::enable_if_t<std::is_pointer_v<std::decay_t<T>>>* = nullptr)
        {
            return pValue != nullptr ? C::convert(pEnvironment, *pValue) : nullptr;
        }

        template <typename C = Utility, typename T>
        static jobject convert(JNIEnv* pEnvironment, hms::ext::jni::ObjectNativeWrapper<T>&& pValue)
        {
            jobject result = nullptr;

            try
            {
                result = hms::ext::jni::ObjectNative::create<T>(std::move(pValue.mData), pEnvironment, mClasses[static_cast<size_t>(EClass::pl_grupapracuj_hermes_ext_jni_ObjectNative)].first);
            }
            catch (const std::exception& lpException) {}

            return result;
        }

        template <typename C = Utility, typename T>
        static jobject convert(JNIEnv* pEnvironment, T&& pValue, typename std::enable_if_t<is_pair<std::decay_t<T>>::value>* = nullptr)
        {
            jobject jFirst = C::template objectCreate<C>(pEnvironment, pValue.first);
            jobject jSecond = C::template objectCreate<C>(pEnvironment, pValue.second);
            jobject jObject = pEnvironment->NewObject(mClasses[static_cast<size_t>(EClass::pl_grupapracuj_pracuj_ext_tuple_Pair)].first, mMethodIds[static_cast<size_t>(EMethodId::pl_grupapracuj_pracuj_ext_tuple_Pair_init)], jFirst, jSecond);
            localRefDelete(pEnvironment, jFirst);
            localRefDelete(pEnvironment, jSecond);

            return jObject;
        }

        template <typename C = Utility, typename T>
        static jobject convert(JNIEnv* pEnvironment, T&& pValue, typename std::enable_if_t<is_tuple<std::decay_t<T>>::value && std::tuple_size<std::decay_t<T>>::value == 2>* = nullptr)
        {
            jobject jFirst = C::template objectCreate<C>(pEnvironment, std::get<0>(pValue));
            jobject jSecond = C::template objectCreate<C>(pEnvironment, std::get<1>(pValue));
            jobject jObject = pEnvironment->NewObject(mClasses[static_cast<size_t>(EClass::pl_grupapracuj_pracuj_ext_tuple_Pair)].first, mMethodIds[static_cast<size_t>(EMethodId::pl_grupapracuj_pracuj_ext_tuple_Pair_init)], jFirst, jSecond);
            localRefDelete(pEnvironment, jFirst);
            localRefDelete(pEnvironment, jSecond);

            return jObject;
        }

        template <typename C = Utility, typename T>
        static jobject convert(JNIEnv* pEnvironment, T&& pValue, typename std::enable_if_t<is_tuple<std::decay_t<T>>::value && std::tuple_size<std::decay_t<T>>::value == 3>* = nullptr)
        {
            jobject jFirst = C::template objectCreate<C>(pEnvironment, std::get<0>(pValue));
            jobject jSecond = C::template objectCreate<C>(pEnvironment, std::get<1>(pValue));
            jobject jThird = C::template objectCreate<C>(pEnvironment, std::get<2>(pValue));
            jobject jObject = pEnvironment->NewObject(mClasses[static_cast<size_t>(EClass::pl_grupapracuj_pracuj_ext_tuple_Triple)].first, mMethodIds[static_cast<size_t>(EMethodId::pl_grupapracuj_pracuj_ext_tuple_Triple_init)], jFirst, jSecond, jThird);
            localRefDelete(pEnvironment, jFirst);
            localRefDelete(pEnvironment, jSecond);
            localRefDelete(pEnvironment, jThird);

            return jObject;
        }

        template <typename C = Utility, typename T>
        static jobject convert(JNIEnv* pEnvironment, T&& pValue, typename std::enable_if_t<is_tuple<std::decay_t<T>>::value && std::tuple_size<std::decay_t<T>>::value == 4>* = nullptr)
        {
            jobject jFirst = C::template objectCreate<C>(pEnvironment, std::get<0>(pValue));
            jobject jSecond = C::template objectCreate<C>(pEnvironment, std::get<1>(pValue));
            jobject jThird = C::template objectCreate<C>(pEnvironment, std::get<2>(pValue));
            jobject jFourth = C::template objectCreate<C>(pEnvironment, std::get<3>(pValue));
            jobject jObject = pEnvironment->NewObject(mClasses[static_cast<size_t>(EClass::pl_grupapracuj_pracuj_ext_tuple_Quadruple)].first, mMethodIds[static_cast<size_t>(EMethodId::pl_grupapracuj_pracuj_ext_tuple_Quadruple_init)], jFirst, jSecond, jThird, jFourth);
            localRefDelete(pEnvironment, jFirst);
            localRefDelete(pEnvironment, jSecond);
            localRefDelete(pEnvironment, jThird);
            localRefDelete(pEnvironment, jFourth);

            return jObject;
        }

        template <typename C = Utility, typename T>
        static jobject convert(JNIEnv* pEnvironment, T&& pValue, typename std::enable_if_t<is_tuple<std::decay_t<T>>::value && std::tuple_size<std::decay_t<T>>::value == 5>* = nullptr)
        {
            jobject jFirst = C::template objectCreate<C>(pEnvironment, std::get<0>(pValue));
            jobject jSecond = C::template objectCreate<C>(pEnvironment, std::get<1>(pValue));
            jobject jThird = C::template objectCreate<C>(pEnvironment, std::get<2>(pValue));
            jobject jFourth = C::template objectCreate<C>(pEnvironment, std::get<3>(pValue));
            jobject jFifth = C::template objectCreate<C>(pEnvironment, std::get<4>(pValue));
            jobject jObject = pEnvironment->NewObject(mClasses[static_cast<size_t>(EClass::pl_grupapracuj_pracuj_ext_tuple_Quintuple)].first, mMethodIds[static_cast<size_t>(EMethodId::pl_grupapracuj_pracuj_ext_tuple_Quintuple_init)], jFirst, jSecond, jThird, jFourth, jFifth);
            localRefDelete(pEnvironment, jFirst);
            localRefDelete(pEnvironment, jSecond);
            localRefDelete(pEnvironment, jThird);
            localRefDelete(pEnvironment, jFourth);
            localRefDelete(pEnvironment, jFifth);

            return jObject;
        }

        template <typename C = Utility, typename T>
        static jobject convert(JNIEnv* pEnvironment, T&& pValue, typename std::enable_if_t<is_tuple<std::decay_t<T>>::value && std::tuple_size<std::decay_t<T>>::value == 6>* = nullptr)
        {
            jobject jFirst = C::template objectCreate<C>(pEnvironment, std::get<0>(pValue));
            jobject jSecond = C::template objectCreate<C>(pEnvironment, std::get<1>(pValue));
            jobject jThird = C::template objectCreate<C>(pEnvironment, std::get<2>(pValue));
            jobject jFourth = C::template objectCreate<C>(pEnvironment, std::get<3>(pValue));
            jobject jFifth = C::template objectCreate<C>(pEnvironment, std::get<4>(pValue));
            jobject jSixth = C::template objectCreate<C>(pEnvironment, std::get<5>(pValue));
            jobject jObject = pEnvironment->NewObject(mClasses[static_cast<size_t>(EClass::pl_grupapracuj_pracuj_ext_tuple_Sextuple)].first, mMethodIds[static_cast<size_t>(EMethodId::pl_grupapracuj_pracuj_ext_tuple_Sextuple_init)], jFirst, jSecond, jThird, jFourth, jFifth, jSixth);
            localRefDelete(pEnvironment, jFirst);
            localRefDelete(pEnvironment, jSecond);
            localRefDelete(pEnvironment, jThird);
            localRefDelete(pEnvironment, jFourth);
            localRefDelete(pEnvironment, jFifth);
            localRefDelete(pEnvironment, jSixth);

            return jObject;
        }

        template <typename C = Utility, typename T>
        static jobject convert(JNIEnv* pEnvironment, T&& pValue, typename std::enable_if_t<is_tuple<std::decay_t<T>>::value && std::tuple_size<std::decay_t<T>>::value == 7>* = nullptr)
        {
            jobject jFirst = C::template objectCreate<C>(pEnvironment, std::get<0>(pValue));
            jobject jSecond = C::template objectCreate<C>(pEnvironment, std::get<1>(pValue));
            jobject jThird = C::template objectCreate<C>(pEnvironment, std::get<2>(pValue));
            jobject jFourth = C::template objectCreate<C>(pEnvironment, std::get<3>(pValue));
            jobject jFifth = C::template objectCreate<C>(pEnvironment, std::get<4>(pValue));
            jobject jSixth = C::template objectCreate<C>(pEnvironment, std::get<5>(pValue));
            jobject jSeventh = C::template objectCreate<C>(pEnvironment, std::get<6>(pValue));
            jobject jObject = pEnvironment->NewObject(mClasses[static_cast<size_t>(EClass::pl_grupapracuj_pracuj_ext_tuple_Septuple)].first, mMethodIds[static_cast<size_t>(EMethodId::pl_grupapracuj_pracuj_ext_tuple_Septuple_init)], jFirst, jSecond, jThird, jFourth, jFifth, jSixth, jSeventh);
            localRefDelete(pEnvironment, jFirst);
            localRefDelete(pEnvironment, jSecond);
            localRefDelete(pEnvironment, jThird);
            localRefDelete(pEnvironment, jFourth);
            localRefDelete(pEnvironment, jFifth);
            localRefDelete(pEnvironment, jSixth);
            localRefDelete(pEnvironment, jSeventh);

            return jObject;
        }

        template <typename C = Utility, typename T>
        static jobject convert(JNIEnv* pEnvironment, T&& pValue, typename std::enable_if_t<is_tuple<std::decay_t<T>>::value && std::tuple_size<std::decay_t<T>>::value == 8>* = nullptr)
        {
            jobject jFirst = C::template objectCreate<C>(pEnvironment, std::get<0>(pValue));
            jobject jSecond = C::template objectCreate<C>(pEnvironment, std::get<1>(pValue));
            jobject jThird = C::template objectCreate<C>(pEnvironment, std::get<2>(pValue));
            jobject jFourth = C::template objectCreate<C>(pEnvironment, std::get<3>(pValue));
            jobject jFifth = C::template objectCreate<C>(pEnvironment, std::get<4>(pValue));
            jobject jSixth = C::template objectCreate<C>(pEnvironment, std::get<5>(pValue));
            jobject jSeventh = C::template objectCreate<C>(pEnvironment, std::get<6>(pValue));
            jobject jEighth = C::template objectCreate<C>(pEnvironment, std::get<7>(pValue));
            jobject jObject = pEnvironment->NewObject(mClasses[static_cast<size_t>(EClass::pl_grupapracuj_pracuj_ext_tuple_Octuple)].first, mMethodIds[static_cast<size_t>(EMethodId::pl_grupapracuj_pracuj_ext_tuple_Octuple_init)], jFirst, jSecond, jThird, jFourth, jFifth, jSixth, jSeventh, jEighth);
            localRefDelete(pEnvironment, jFirst);
            localRefDelete(pEnvironment, jSecond);
            localRefDelete(pEnvironment, jThird);
            localRefDelete(pEnvironment, jFourth);
            localRefDelete(pEnvironment, jFifth);
            localRefDelete(pEnvironment, jSixth);
            localRefDelete(pEnvironment, jSeventh);
            localRefDelete(pEnvironment, jEighth);

            return jObject;
        }

        template <typename C = Utility, typename T>
        static jobject convert(JNIEnv* pEnvironment, T&& pValue, typename std::enable_if_t<is_tuple<std::decay_t<T>>::value && std::tuple_size<std::decay_t<T>>::value == 9>* = nullptr)
        {
            jobject jFirst = C::template objectCreate<C>(pEnvironment, std::get<0>(pValue));
            jobject jSecond = C::template objectCreate<C>(pEnvironment, std::get<1>(pValue));
            jobject jThird = C::template objectCreate<C>(pEnvironment, std::get<2>(pValue));
            jobject jFourth = C::template objectCreate<C>(pEnvironment, std::get<3>(pValue));
            jobject jFifth = C::template objectCreate<C>(pEnvironment, std::get<4>(pValue));
            jobject jSixth = C::template objectCreate<C>(pEnvironment, std::get<5>(pValue));
            jobject jSeventh = C::template objectCreate<C>(pEnvironment, std::get<6>(pValue));
            jobject jEighth = C::template objectCreate<C>(pEnvironment, std::get<7>(pValue));
            jobject jNinth = C::template objectCreate<C>(pEnvironment, std::get<8>(pValue));
            jobject jObject = pEnvironment->NewObject(mClasses[static_cast<size_t>(EClass::pl_grupapracuj_pracuj_ext_tuple_Nontuple)].first, mMethodIds[static_cast<size_t>(EMethodId::pl_grupapracuj_pracuj_ext_tuple_Nontuple_init)], jFirst, jSecond, jThird, jFourth, jFifth, jSixth, jSeventh, jEighth, jNinth);
            localRefDelete(pEnvironment, jFirst);
            localRefDelete(pEnvironment, jSecond);
            localRefDelete(pEnvironment, jThird);
            localRefDelete(pEnvironment, jFourth);
            localRefDelete(pEnvironment, jFifth);
            localRefDelete(pEnvironment, jSixth);
            localRefDelete(pEnvironment, jSeventh);
            localRefDelete(pEnvironment, jEighth);
            localRefDelete(pEnvironment, jNinth);

            return jObject;
        }

        template <typename C = Utility, typename T>
        static jobject convert(JNIEnv* pEnvironment, T&& pValue, typename std::enable_if_t<std::is_same_v<std::decay_t<T>, std::string>>* = nullptr)
        {
            return pEnvironment->NewStringUTF(pValue.c_str());
        }

        template <typename C = Utility, typename T>
        static jbyteArray convert(JNIEnv* pEnvironment, T&& pValue, typename std::enable_if_t<std::is_same_v<std::decay_t<T>, hms::DataBuffer>>* = nullptr)
        {
            return convertHmsDataBuffer(pEnvironment, pValue);
        }

        template <typename C = Utility, typename T>
        static jintArray convert(JNIEnv* pEnvironment, const std::vector<T>& pValue, typename std::enable_if_t<std::is_same_v<std::decay_t<T>, jint> || std::is_same_v<std::decay_t<T>, int32_t> || std::is_same_v<std::decay_t<T>, uint32_t> || std::is_same_v<std::decay_t<T>, size_t> || std::is_enum_v<std::decay_t<T>>>* = nullptr)
        {
            return arrayIntCreate(pEnvironment, pValue);
        }

        template <typename C = Utility, typename T>
        static jobjectArray convert(JNIEnv* pEnvironment, const std::vector<T>& pValue, typename std::enable_if_t<std::is_same_v<std::decay_t<T>, std::string> || std::is_same_v<std::decay_t<T>, hms::DataBuffer> || is_pair<std::decay_t<T>>::value || is_tuple<std::decay_t<T>>::value>* = nullptr)
        {
            jclass jClass = nullptr;
            if constexpr(std::is_same<std::decay_t<T>, std::string>::value)
            {
                jClass = mClasses[static_cast<size_t>(EClass::java_lang_String)].first;
            }
            else if constexpr(std::is_same<std::decay_t<T>, hms::DataBuffer>::value)
            {
                jClass = mClasses[static_cast<size_t>(EClass::java_lang_arrayByte)].first;
            }
            else if constexpr(is_pair<std::decay_t<T>>::value)
            {
                jClass = mClasses[static_cast<size_t>(EClass::pl_grupapracuj_pracuj_ext_tuple_Pair)].first;
            }
            else if constexpr(is_tuple<std::decay_t<T>>::value)
            {
                assert(std::tuple_size<std::decay_t<T>>::value < 10);

                if constexpr(std::tuple_size<std::decay_t<T>>::value == 2)
                    jClass = mClasses[static_cast<size_t>(EClass::pl_grupapracuj_pracuj_ext_tuple_Pair)].first;
                else if constexpr(std::tuple_size<std::decay_t<T>>::value == 3)
                    jClass = mClasses[static_cast<size_t>(EClass::pl_grupapracuj_pracuj_ext_tuple_Triple)].first;
                else if constexpr(std::tuple_size<std::decay_t<T>>::value == 4)
                    jClass = mClasses[static_cast<size_t>(EClass::pl_grupapracuj_pracuj_ext_tuple_Quadruple)].first;
                else if constexpr(std::tuple_size<std::decay_t<T>>::value == 5)
                    jClass = mClasses[static_cast<size_t>(EClass::pl_grupapracuj_pracuj_ext_tuple_Quintuple)].first;
                else if constexpr(std::tuple_size<std::decay_t<T>>::value == 6)
                    jClass = mClasses[static_cast<size_t>(EClass::pl_grupapracuj_pracuj_ext_tuple_Sextuple)].first;
                else if constexpr(std::tuple_size<std::decay_t<T>>::value == 7)
                    jClass = mClasses[static_cast<size_t>(EClass::pl_grupapracuj_pracuj_ext_tuple_Septuple)].first;
                else if constexpr(std::tuple_size<std::decay_t<T>>::value == 8)
                    jClass = mClasses[static_cast<size_t>(EClass::pl_grupapracuj_pracuj_ext_tuple_Octuple)].first;
                else if constexpr(std::tuple_size<std::decay_t<T>>::value == 9)
                    jClass = mClasses[static_cast<size_t>(EClass::pl_grupapracuj_pracuj_ext_tuple_Nontuple)].first;
            }

            return arrayObjectCreate(pEnvironment, pValue, jClass);
        }

        static std::string jconvert(JNIEnv* pEnvironment, jstring pValue);

    protected:
        static bool classLoad(JNIEnv* pEnvironment, jobject pClassLoader, std::pair<jclass, bool>* pClasses, std::string* pSignatures, size_t pCount);

        template <typename C = Utility, typename T>
        static jboolean convert(JNIEnv* pEnvironment, T pValue, typename std::enable_if_t<std::is_same_v<std::decay_t<T>, jboolean> || std::is_same_v<std::decay_t<T>, bool>>* = nullptr)
        {
            return pValue;
        }

        template <typename C = Utility, typename T>
        static jbyte convert(JNIEnv* pEnvironment, T pValue, typename std::enable_if_t<std::is_same_v<std::decay_t<T>, jbyte>>* = nullptr)
        {
            return pValue;
        }

        template <typename C = Utility, typename T>
        static jchar convert(JNIEnv* pEnvironment, T pValue, typename std::enable_if_t<std::is_same_v<std::decay_t<T>, jchar>>* = nullptr)
        {
            return pValue;
        }

        template <typename C = Utility, typename T>
        static jdouble convert(JNIEnv* pEnvironment, T pValue, typename std::enable_if_t<std::is_same_v<std::decay_t<T>, jdouble> || std::is_same_v<std::decay_t<T>, double>>* = nullptr)
        {
            return pValue;
        }

        template <typename C = Utility, typename T>
        static jfloat convert(JNIEnv* pEnvironment, T pValue, typename std::enable_if_t<std::is_same_v<std::decay_t<T>, jfloat> || std::is_same_v<std::decay_t<T>, float>>* = nullptr)
        {
            return pValue;
        }

        template <typename C = Utility, typename T>
        static jint convert(JNIEnv* pEnvironment, T pValue, typename std::enable_if_t<std::is_same_v<std::decay_t<T>, jint> || std::is_same_v<std::decay_t<T>, int32_t> || std::is_same_v<std::decay_t<T>, uint32_t> || std::is_same_v<std::decay_t<T>, size_t> || std::is_enum_v<std::decay_t<T>>>* = nullptr)
        {
            return pValue;
        }

        template <typename C = Utility, typename T>
        static jlong convert(JNIEnv* pEnvironment, T pValue, typename std::enable_if_t<std::is_same_v<std::decay_t<T>, jlong>>* = nullptr)
        {
            return pValue;
        }

        template <typename C = Utility, typename T>
        static jshort convert(JNIEnv* pEnvironment, T pValue, typename std::enable_if_t<std::is_same_v<std::decay_t<T>, jshort>>* = nullptr)
        {
            return pValue;
        }

        template <typename C = Utility, typename T>
        static jintArray arrayIntCreate(JNIEnv* pEnvironment, const std::vector<T>& pValue)
        {
            jintArray jArray = pEnvironment->NewIntArray(static_cast<jsize>(pValue.size()));
            pEnvironment->SetIntArrayRegion(jArray, 0, static_cast<jsize>(pValue.size()), reinterpret_cast<const jint*>(pValue.data()));

            return jArray;
        }

        template <typename C = Utility, typename T>
        static jobjectArray arrayObjectCreate(JNIEnv* pEnvironment, const std::vector<T>& pValue, jclass pClass)
        {
            assert(pClass != nullptr);

            jobjectArray jArray = pEnvironment->NewObjectArray(static_cast<jsize>(pValue.size()), pClass, nullptr);
            for (size_t i = 0; i < pValue.size(); ++i)
            {
                auto jValue = C::template objectCreate<C>(pEnvironment, pValue[i]);
                pEnvironment->SetObjectArrayElement(jArray, static_cast<jsize>(i), static_cast<jobject>(jValue));
                localRefDelete(pEnvironment, jValue);
            }

            return jArray;
        }

        template <typename C = Utility, typename T>
        static auto objectCreate(JNIEnv* pEnvironment, T pValue, typename std::enable_if_t<std::is_pointer_v<std::decay_t<T>>>* = nullptr)
        {
            return pValue != nullptr ? C::template objectCreate<C>(pEnvironment, *pValue) : nullptr;
        }

        template <typename C = Utility, typename T>
        static jobject objectCreate(JNIEnv* pEnvironment, T pValue, typename std::enable_if_t<std::is_same_v<std::decay_t<T>, jboolean> || std::is_same_v<std::decay_t<T>, bool>>* = nullptr)
        {
            return pEnvironment->NewObject(mClasses[static_cast<size_t>(EClass::java_lang_Boolean)].first, mMethodIds[static_cast<size_t>(EMethodId::java_lang_Boolean_init)], pValue);
        }

        template <typename C = Utility, typename T>
        static jobject objectCreate(JNIEnv* pEnvironment, T pValue, typename std::enable_if_t<std::is_same_v<std::decay_t<T>, jbyte>>* = nullptr)
        {
            return pEnvironment->NewObject(mClasses[static_cast<size_t>(EClass::java_lang_Byte)].first, mMethodIds[static_cast<size_t>(EMethodId::java_lang_Byte_init)], pValue);
        }

        template <typename C = Utility, typename T>
        static jobject objectCreate(JNIEnv* pEnvironment, T pValue, typename std::enable_if_t<std::is_same_v<std::decay_t<T>, jdouble> || std::is_same_v<std::decay_t<T>, double>>* = nullptr)
        {
            return pEnvironment->NewObject(mClasses[static_cast<size_t>(EClass::java_lang_Double)].first, mMethodIds[static_cast<size_t>(EMethodId::java_lang_Double_init)], pValue);
        }

        template <typename C = Utility, typename T>
        static jobject objectCreate(JNIEnv* pEnvironment, T pValue, typename std::enable_if_t<std::is_same_v<std::decay_t<T>, jfloat> || std::is_same_v<std::decay_t<T>, float>>* = nullptr)
        {
            return pEnvironment->NewObject(mClasses[static_cast<size_t>(EClass::java_lang_Float)].first, mMethodIds[static_cast<size_t>(EMethodId::java_lang_Float_init)], pValue);
        }

        template <typename C = Utility, typename T>
        static jobject objectCreate(JNIEnv* pEnvironment, T pValue, typename std::enable_if_t<std::is_same_v<std::decay_t<T>, jint> || std::is_same_v<std::decay_t<T>, int32_t> || std::is_same_v<std::decay_t<T>, uint32_t> || std::is_same_v<std::decay_t<T>, size_t> || std::is_enum_v<std::decay_t<T>>>* = nullptr)
        {
            return pEnvironment->NewObject(mClasses[static_cast<size_t>(EClass::java_lang_Integer)].first, mMethodIds[static_cast<size_t>(EMethodId::java_lang_Integer_init)], static_cast<jint>(pValue));
        }

        template <typename C = Utility, typename T>
        static jobject objectCreate(JNIEnv* pEnvironment, T pValue, typename std::enable_if_t<std::is_same_v<std::decay_t<T>, jlong>>* = nullptr)
        {
            return pEnvironment->NewObject(mClasses[static_cast<size_t>(EClass::java_lang_Long)].first, mMethodIds[static_cast<size_t>(EMethodId::java_lang_Long_init)], pValue);
        }

        template <typename C = Utility, typename T>
        static jobject objectCreate(JNIEnv* pEnvironment, T pValue, typename std::enable_if_t<std::is_same_v<std::decay_t<T>, jshort>>* = nullptr)
        {
            return pEnvironment->NewObject(mClasses[static_cast<size_t>(EClass::java_lang_Short)].first, mMethodIds[static_cast<size_t>(EMethodId::java_lang_Short_init)], pValue);
        }

        template <typename C = Utility, typename T>
        static auto objectCreate(JNIEnv* pEnvironment, T&& pValue, typename std::enable_if_t<!std::is_pod_v<std::decay_t<T>>>* = nullptr)
        {
            return C::template convert<C>(pEnvironment, std::forward<T>(pValue));
        }

        template <typename T>
        static void localRefDelete(JNIEnv* pEnvironment, T&& pObject, typename std::enable_if_t<!std::is_convertible_v<std::decay_t<T>, jobject>>* = nullptr)
        {
        }

        template <typename T>
        static void localRefDelete(JNIEnv* pEnvironment, T&& pObject, typename std::enable_if_t<std::is_convertible_v<std::decay_t<T>, jobject>>* = nullptr)
        {
            if (pObject != nullptr)
                pEnvironment->DeleteLocalRef(pObject);
        }

        template <typename... T>
        static jobject methodObjectCall(JNIEnv* pEnvironment, jobject pObject, jmethodID pMethod, T&&... pParameters)
        {
            jobject result = pEnvironment->CallObjectMethod(pObject, pMethod, std::forward<T>(pParameters)...);
            (void)std::initializer_list<int32_t>{(localRefDelete(pEnvironment, std::forward<T>(pParameters)), 0)...};

            return result;
        }

        template <typename... T>
        static void methodVoidCall(JNIEnv* pEnvironment, jobject pObject, jmethodID pMethod, T&&... pParameters)
        {
            pEnvironment->CallVoidMethod(pObject, pMethod, std::forward<T>(pParameters)...);
            (void)std::initializer_list<int32_t>{(localRefDelete(pEnvironment, std::forward<T>(pParameters)), 0)...};
        }

        static jbyteArray convertHmsDataBuffer(JNIEnv* pEnvironment, const hms::DataBuffer& pValue);

    private:
        enum class EClass : size_t
        {
            java_lang_Boolean = 0,
            java_lang_Byte,
            java_lang_Double,
            java_lang_Float,
            java_lang_Integer,
            java_lang_Long,
            java_lang_Short,
            java_lang_String,
            java_lang_arrayByte,
            pl_grupapracuj_hermes_ext_jni_ObjectNative,
            pl_grupapracuj_pracuj_ext_tuple_Nontuple,
            pl_grupapracuj_pracuj_ext_tuple_Octuple,
            pl_grupapracuj_pracuj_ext_tuple_Pair,
            pl_grupapracuj_pracuj_ext_tuple_Quadruple,
            pl_grupapracuj_pracuj_ext_tuple_Quintuple,
            pl_grupapracuj_pracuj_ext_tuple_Septuple,
            pl_grupapracuj_pracuj_ext_tuple_Sextuple,
            pl_grupapracuj_pracuj_ext_tuple_Triple,
            COUNT
        };

        enum class EMethodId : size_t
        {
            java_lang_Boolean_init = 0,
            java_lang_Byte_init,
            java_lang_Double_init,
            java_lang_Float_init,
            java_lang_Integer_init,
            java_lang_Long_init,
            java_lang_Short_init,
            pl_grupapracuj_pracuj_ext_tuple_Nontuple_init,
            pl_grupapracuj_pracuj_ext_tuple_Octuple_init,
            pl_grupapracuj_pracuj_ext_tuple_Pair_init,
            pl_grupapracuj_pracuj_ext_tuple_Quadruple_init,
            pl_grupapracuj_pracuj_ext_tuple_Quintuple_init,
            pl_grupapracuj_pracuj_ext_tuple_Septuple_init,
            pl_grupapracuj_pracuj_ext_tuple_Sextuple_init,
            pl_grupapracuj_pracuj_ext_tuple_Triple_init,
            COUNT
        };

        static std::array<std::pair<jclass, bool>, static_cast<size_t>(EClass::COUNT)> mClasses;
        static std::array<jmethodID, static_cast<size_t>(EMethodId::COUNT)> mMethodIds;
        static JavaVM* mJavaVM;
    };
}
}
}

#endif
