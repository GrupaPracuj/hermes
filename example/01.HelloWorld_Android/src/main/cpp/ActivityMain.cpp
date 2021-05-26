#include "HelloWorld.hpp"
#include "ext/hmsAAssetReader.hpp"
#include "ext/hmsJNI.hpp"

extern "C" JNIEXPORT jobject JNICALL Java_pl_grupapracuj_hermes_helloworld_ActivityMain_nativeCreate(JNIEnv* pEnvironment, jobject pObject, jobject pClassLoader, jobject pAssetManager)
{
    hms::ext::jni::Utility::initialize(pEnvironment, pClassLoader);

    auto dataManager = hms::Hermes::getInstance()->getDataManager();
    dataManager->initialize();
    dataManager->addStorageReader(std::make_unique<hms::ext::jni::AAssetStorageReader>(pEnvironment, pAssetManager, "assets/"));

    std::string certificate;
    dataManager->readFile("assets/certificate.pem", certificate);
    auto helloWorld = std::make_shared<HelloWorld>(nullptr, std::make_pair(hms::ENetworkCertificate::Content, std::move(certificate)));

    return hms::ext::jni::Utility::convert(pEnvironment, hms::ext::jni::ObjectNativeWrapper(helloWorld));
}

extern "C" JNIEXPORT void JNICALL Java_pl_grupapracuj_hermes_helloworld_ActivityMain_nativeExecute(JNIEnv* pEnvironment, jobject pObject, jlong pHelloWorldPointer, jint pRequestIndex)
{
    auto helloWorld = hms::ext::jni::ObjectNative::get<std::shared_ptr<HelloWorld>>(pHelloWorldPointer);

    auto objectWeakRef = std::make_shared<hms::ext::jni::ReferenceWeak>(pObject, pEnvironment);
    helloWorld->execute([objectWeakRef](std::string lpOutputText) -> void
    {
        hms::ext::jni::Utility::methodVoid(objectWeakRef, "callbackExecute", "(Ljava/lang/String;)V", std::move(lpOutputText));
    }, static_cast<int32_t>(pRequestIndex));
}

jint JNI_OnLoad(JavaVM* pVM, void* pReserved)
{
    JNIEnv* environment = nullptr;
    if (pVM->GetEnv(reinterpret_cast<void**>(&environment), JNI_VERSION_1_6) != JNI_OK)
    {
        return -1;
    }

    return JNI_VERSION_1_6;
}
