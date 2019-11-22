#include "HelloWorld.hpp"
#include "ext/hmsJNI.hpp"

std::unordered_map<std::string, jclass> gClassLoader;

extern "C" JNIEXPORT jobject JNICALL Java_pl_grupapracuj_hermes_helloworld_ActivityMain_nativeCreate(JNIEnv* pEnvironment, jobject pObject, jstring pCertificatePath)
{
    auto helloWorld = std::make_shared<HelloWorld>(nullptr, hms::ext::jni::Utility::string(pCertificatePath, pEnvironment));

    return hms::ext::jni::ObjectNative::create<std::shared_ptr<HelloWorld>>(helloWorld, pEnvironment, gClassLoader["pl/grupapracuj/hermes/ext/jni/NativeObject"]);
}

extern "C" JNIEXPORT void JNICALL Java_pl_grupapracuj_hermes_helloworld_ActivityMain_nativeExecute(JNIEnv* pEnvironment, jobject pObject, jlong pHelloWorldPointer, jint pRequestIndex)
{
    auto helloWorld = hms::ext::jni::ObjectNative::get<std::shared_ptr<HelloWorld>>(pHelloWorldPointer);

    auto objectWeakRef = std::make_shared<hms::ext::jni::ReferenceWeak>(pObject, pEnvironment);
    auto callback = [objectWeakRef](std::string lpOutputText) -> void
    {
        JNIEnv* environment = objectWeakRef->environment();
        jobject strongObject = environment->NewGlobalRef(objectWeakRef->objectWeak());
        if (strongObject != nullptr)
        {
            jclass jClass = environment->GetObjectClass(strongObject);
            jmethodID jMethod = environment->GetMethodID(jClass, "callbackExecute", "(Ljava/lang/String;)V");
            jstring jOutputText = environment->NewStringUTF(lpOutputText.c_str());
            environment->CallVoidMethod(strongObject, jMethod, jOutputText);
            environment->DeleteLocalRef(jOutputText);
            environment->DeleteGlobalRef(strongObject);
        }
    };

    helloWorld->execute(std::move(callback), static_cast<int32_t>(pRequestIndex));
}

jint JNI_OnLoad(JavaVM* pVM, void* pReserved)
{
    JNIEnv* environment = nullptr;
    if (pVM->GetEnv(reinterpret_cast<void**>(&environment), JNI_VERSION_1_6) != JNI_OK)
    {
        return -1;
    }

    std::array<std::string, 1> classNames = {
        "pl/grupapracuj/hermes/ext/jni/ObjectNative"
    };

    for (const auto& v : classNames)
    {
        jclass classWeakRef = environment->FindClass(v.c_str());
        gClassLoader[v] = static_cast<jclass>(environment->NewGlobalRef(classWeakRef));
        environment->DeleteLocalRef(classWeakRef);
    }

    return JNI_VERSION_1_6;
}
