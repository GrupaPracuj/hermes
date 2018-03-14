#ifndef _HMS_JNI_HPP_
#define _HMS_JNI_HPP_

#include <jni.h>
#include <string>

namespace hms
{
namespace ext
{
namespace jni
{

    template<typename T>
    class ObjectHandle
    {
    public:
        ObjectHandle(JNIEnv* pEnvironment, jobject pObject, const char* pFieldName, T pNativeObject)
        {
            mNativeObject = pNativeObject;

            if (pEnvironment != nullptr && pObject != nullptr && pFieldName != nullptr)
                set(pEnvironment, pObject, pFieldName, this);
        }

        virtual ~ObjectHandle() noexcept
        {
        }

        T get() const
        {
            return mNativeObject;
        }

        static ObjectHandle* get(JNIEnv* pEnvironment, jobject pObject, const char* pFieldName)
        {
            jclass jClass = pEnvironment->GetObjectClass(pObject);
            jfieldID jField = pEnvironment->GetFieldID(jClass, pFieldName, "J");
            jlong handle = pEnvironment->GetLongField(pObject, jField);
            return reinterpret_cast<ObjectHandle*>(handle);
        }

        static void set(JNIEnv* pEnvironment, jobject pObject, const char* pFieldName, ObjectHandle* pNativeObject)
        {
            jclass jClass = pEnvironment->GetObjectClass(pObject);
            jfieldID jField = pEnvironment->GetFieldID(jClass, pFieldName, "J");
            jlong handle = reinterpret_cast<jlong>(pNativeObject);
            pEnvironment->SetLongField(pObject, jField, handle);
        }

        static void destroy(JNIEnv* pEnvironment, jobject pObject, const char* pFieldName)
        {
            auto handle = get(pEnvironment, pObject, pFieldName);
            set(pEnvironment, pObject, pFieldName, nullptr);
            delete handle;
        }

    private:
        T mNativeObject;
    };

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
