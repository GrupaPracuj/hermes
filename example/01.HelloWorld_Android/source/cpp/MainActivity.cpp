#include <jni.h>
#include <string>
#include "hermes.hpp"
#include "ext/hmsJNI.hpp"
#include "ext/hmsSerializer.hpp"

enum class EDataType : size_t
{
    JPH = 0
};

enum class ENetworkAPI : size_t
{
    JPH = 0
};

class JphData : public hms::DataShared
{
public:
    JphData(size_t pId) : DataShared(pId) {};

    virtual bool pack(std::string& pData, const std::vector<unsigned>& pUserData) const override
    {
        // You can use this method to prepare a request body. If you need to handle different request bodies in this method, you should provide additional data in pUserData and use switch-case.
        // Because we don't need this functionality for our example this method will just return true.

        bool status = true;
        // Json::Value root;
        // root["user_name"] = mName;
        //
        // status = hms::Hermes::getInstance()->getDataManager()->convertJSON(root, pData);
        //
        // if (!status)
        //		hms::Hermes::getInstance()->getLogger()->print(hms::ELogLevel::Error, "JphData::pack failed. Data:\n%", pData);
        //
        return status;
    }

    virtual bool unpack(const std::string& pData, const std::vector<unsigned int>& pUserData) override
    {
        // You can use this method to retrieve data from a response message. If you need to handle different response messages in this method, you should provide additional data in pUserData and use switch-case.
        // In our example we'll retrieve data from a request to 'https://jsonplaceholder.typicode.com/users/1'.

        bool status = false;
        Json::Value root;

        if (hms::ext::json::convert(pData, root))
        {
            status = true;

            status &= hms::ext::json::safeAs<int>(root, mId, "id");
            status &= hms::ext::json::safeAs<std::string>(root, mName, "name");

            const Json::Value& address = root["address"];
            status &= hms::ext::json::safeAs<std::string>(address, mCity, "city");
        }

        if (!status)
            hms::Hermes::getInstance()->getLogger()->print(hms::ELogLevel::Error, "JphData::unpack failed. Data:\n%", pData);

        return status;
    }

    int mId = -1;
    std::string mName;
    std::string mCity;
};

class HelloWorld
{
public:
    HelloWorld(std::string pCertificatePath)
    {
        auto logger = hms::Hermes::getInstance()->getLogger();
        logger->initialize(hms::ELogLevel::Info, nullptr);

        auto dataManager = hms::Hermes::getInstance()->getDataManager();
        dataManager->initialize();
        dataManager->add(std::shared_ptr<hms::DataShared>(new JphData(static_cast<size_t>(EDataType::JPH))));

        auto taskManager = hms::Hermes::getInstance()->getTaskManager();
        // First element of pair - thread pool id, second element of pair - number of threads in thread pool.
        const std::pair<int, int> threadInfo = {0, 2};
        taskManager->initialize({threadInfo});

        auto networkManager = hms::Hermes::getInstance()->getNetworkManager();
        // Connection timeout.
        const long timeout = 15;
        networkManager->initialize(timeout, threadInfo.first, {200, 299});

        auto jphNetwork = networkManager->add(static_cast<size_t>(ENetworkAPI::JPH), "JsonPlaceholder", "https://jsonplaceholder.typicode.com/");
        jphNetwork->setDefaultHeader({{"Content-Type", "application/json; charset=utf-8"}});

        hms::Hermes::getInstance()->getNetworkManager()->setCACertificatePath(std::move(pCertificatePath));
    }

    ~HelloWorld()
    {
        hms::Hermes::getInstance()->getNetworkManager()->terminate();
        hms::Hermes::getInstance()->getTaskManager()->terminate();
        hms::Hermes::getInstance()->getDataManager()->terminate();
        hms::Hermes::getInstance()->getLogger()->terminate();
    }

    void execute(std::function<void(std::string)> pCallback)
    {
        auto responseCallback = [pCallback = std::move(pCallback)](hms::NetworkResponse lpResponse) -> void
        {
            if (lpResponse.mCode == hms::ENetworkCode::OK)
            {
                auto jphData = hms::Hermes::getInstance()->getDataManager()->get<JphData>(static_cast<size_t>(EDataType::JPH));
                if (jphData->unpack(lpResponse.mMessage, {}))
                {
                    std::string outputText = "ID: ";
                    outputText += std::to_string(jphData->mId);
                    outputText += " / Name: ";
                    outputText += jphData->mName;
                    outputText += " / City: ";
                    outputText += jphData->mCity;

                    hms::Hermes::getInstance()->getLogger()->print(hms::ELogLevel::Info, outputText);

                    if (pCallback != nullptr)
                        pCallback(std::move(outputText));
                }
            }
            else if (lpResponse.mCode != hms::ENetworkCode::Cancel)
            {
                hms::Hermes::getInstance()->getLogger()->print(hms::ELogLevel::Info, "Connection problem: %", lpResponse.mMessage);
            }
        };

        hms::NetworkRequestParam requestParam;
        requestParam.mRequestType = hms::ENetworkRequestType::Get;
        requestParam.mMethod = "users/1";
        requestParam.mCallback = std::move(responseCallback);

        hms::Hermes::getInstance()->getNetworkManager()->get(static_cast<size_t>(ENetworkAPI::JPH))->request(std::move(requestParam));
    }
};

typedef hms::ext::jni::ObjectHandle<std::shared_ptr<HelloWorld>> NativeHandle;

extern "C" JNIEXPORT jlong JNICALL Java_pl_grupapracuj_hermes_helloworld_MainActivity_createNative(JNIEnv* pEnvironment, jobject pObject, jstring pCertificatePath)
{
    auto helloWorld = std::make_shared<HelloWorld>(hms::ext::jni::Utility::string(pCertificatePath, pEnvironment));

    return reinterpret_cast<jlong>(new NativeHandle(nullptr, nullptr, nullptr, helloWorld));
}

extern "C" JNIEXPORT void JNICALL Java_pl_grupapracuj_hermes_helloworld_MainActivity_destroyNative(JNIEnv* pEnvironment, jobject pObject)
{
    delete NativeHandle::get(pEnvironment, pObject, "nativeHandle");
}

extern "C" JNIEXPORT void JNICALL Java_pl_grupapracuj_hermes_helloworld_MainActivity_executeNative(JNIEnv* pEnvironment, jobject pObject)
{
    auto helloWorld = NativeHandle::get(pEnvironment, pObject, "nativeHandle")->get();

    auto objectWeakRef = std::make_shared<hms::ext::jni::SmartWeakRef>(pObject, pEnvironment);
    auto callback = [objectWeakRef](std::string lpOutputText) -> void
    {
        hms::ext::jni::SmartEnvironment smartEnvironment;
        objectWeakRef->getEnvironment(smartEnvironment);
        JNIEnv* environment = smartEnvironment.getJNIEnv();
        jobject strongObject = environment->NewGlobalRef(objectWeakRef->getWeakObject());
        if (strongObject != nullptr)
        {
            jclass jClass = environment->GetObjectClass(strongObject);
            jmethodID jMethod = environment->GetMethodID(jClass,"addText", "(Ljava/lang/String;)V");
            jstring jOutputText = environment->NewStringUTF(lpOutputText.c_str());
            environment->CallVoidMethod(strongObject, jMethod, jOutputText);
            environment->DeleteLocalRef(jOutputText);
            environment->DeleteGlobalRef(strongObject);
        }
    };

    helloWorld->execute(std::move(callback));
}
