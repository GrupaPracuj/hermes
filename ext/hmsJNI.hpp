#ifndef _HMS_JNI_HPP_
#define _HMS_JNI_HPP_

#include <jni.h>
#include <memory>
#include <list>

namespace hms
{
namespace ext
{

    template<typename T>
    class JNIObjectHandle
    {
    public:
        JNIObjectHandle(JNIEnv* pEnvironment, jobject pObject, const char* pFieldName, T pNativeObject)
        {
            mNativeObject = pNativeObject;

            if (pEnvironment != nullptr && pObject != nullptr && pFieldName != nullptr)
                set(pEnvironment, pObject, pFieldName, this);
        }

        virtual ~JNIObjectHandle() noexcept
        {
        }

        T get() const
        {
            return mNativeObject;
        }

        static JNIObjectHandle* get(JNIEnv* pEnvironment, jobject pObject, const char* pFieldName)
        {
            jclass jClass = pEnvironment->GetObjectClass(pObject);
            jfieldID jField = pEnvironment->GetFieldID(jClass, pFieldName, "J");
            jlong handle = pEnvironment->GetLongField(pObject, jField);
            return reinterpret_cast<JNIObjectHandle*>(handle);
        }

        static void set(JNIEnv* pEnvironment, jobject pObject, const char* pFieldName, JNIObjectHandle* pNativeObject)
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
    
    class JNISmartRef
    {
    public:
        JNISmartRef(jobject pObject, JNIEnv* pEnvironment);
        ~JNISmartRef();
        
        jobject getObject() const;
        JNIEnv* getEnvironment() const;
        
    private:
        jobject mObject = nullptr;
        JavaVM* mJavaVM = nullptr;
    };
    
    class JNISmartWeakRef
    {
    public:
        JNISmartWeakRef(jobject pObject, JNIEnv* pEnvironment);
        ~JNISmartWeakRef();
        
        jobject getWeakObject() const;
        JNIEnv* getEnvironment() const;
        
    private:
        jobject mObject = nullptr;
        JavaVM* mJavaVM = nullptr;
    };
    
    class JNISmartRefHandler
    {
    public:
        void flush();
        
        void push(std::shared_ptr<JNISmartRef> pSmartRef);
        void push(std::shared_ptr<JNISmartWeakRef> pSmartWeakRef);
        
        static JNISmartRefHandler* getInstance();
        
    private:
        JNISmartRefHandler() = default;
        ~JNISmartRefHandler();
        JNISmartRefHandler(const JNISmartRefHandler& pOther) = delete;
        JNISmartRefHandler(JNISmartRefHandler&& pOther) = delete;
        
        JNISmartRefHandler& operator=(const JNISmartRefHandler& pOther) = delete;
        JNISmartRefHandler& operator=(JNISmartRefHandler&& pOther) = delete;
        
        std::list<std::shared_ptr<JNISmartRef>> mSmartRef;
        std::list<std::shared_ptr<JNISmartWeakRef>> mSmartWeakRef;
    };

}
}

#endif
