#include <jni.h>
#include <string>
#include "hermes.hpp"

struct JNIHandler {
    JNIEnv* mEnv = nullptr;
    jobject mActivity;
};

JNIHandler gJNIHandler;

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

        if (hms::Hermes::getInstance()->getDataManager()->convertJSON(pData, root))
        {
            status = true;

            status &= safeAs<int>(root, mId, "id");
            status &= safeAs<std::string>(root, mName, "name");

            const Json::Value& address = root["address"];
            status &= safeAs<std::string>(address, mCity, "city");
        }

        if (!status)
            hms::Hermes::getInstance()->getLogger()->print(hms::ELogLevel::Error, "VersionData::unpack failed. Data:\n%", pData);

        return status;
    }

    int mId = -1;
    std::string mName;
    std::string mCity;
};

class HelloWorld
{
public:
    HelloWorld()
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

        auto jphNetwork = networkManager->add("JsonPlaceholder", "https://jsonplaceholder.typicode.com/", static_cast<size_t>(ENetworkAPI::JPH));
        jphNetwork->setDefaultHeader({{"Content-Type", "application/json; charset=utf-8"}});
    }

    ~HelloWorld()
    {
        hms::Hermes::getInstance()->getNetworkManager()->terminate();
        hms::Hermes::getInstance()->getTaskManager()->terminate();
        hms::Hermes::getInstance()->getDataManager()->terminate();
        hms::Hermes::getInstance()->getLogger()->terminate();
    }

    void execute()
    {
        auto callback = [](hms::NetworkResponse lpResponse) -> void
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

                    if (gJNIHandler.mEnv != NULL && gJNIHandler.mActivity != NULL)
                    {
                        jclass jClassMainActivity = gJNIHandler.mEnv->GetObjectClass(gJNIHandler.mActivity);
                        jmethodID jMethodAddText = gJNIHandler.mEnv->GetMethodID(jClassMainActivity, "addText", "(Ljava/lang/String;)V");
                        jstring jParamText = gJNIHandler.mEnv->NewStringUTF(outputText.c_str());
                        gJNIHandler.mEnv->CallVoidMethod(gJNIHandler.mActivity, jMethodAddText, jParamText);
                    }
                }
            }
            else
            {
                hms::Hermes::getInstance()->getLogger()->print(hms::ELogLevel::Info, "Connection problem: %", lpResponse.mMessage);
            }
        };

        hms::NetworkRequestParam requestParam;
        requestParam.mRequestType = hms::ENetworkRequestType::Get;
        requestParam.mMethod = "users/1";
        requestParam.mCallback = callback;

        hms::Hermes::getInstance()->getNetworkManager()->get(static_cast<size_t>(ENetworkAPI::JPH))->request(std::move(requestParam));
    }

    void setCertificatePath(std::string pPath)
    {
        hms::Hermes::getInstance()->getNetworkManager()->setCACertificatePath(pPath);
    }
};

HelloWorld* gHelloWorld = nullptr;

jint JNI_OnLoad(JavaVM* pVM, void* pReserved)
{
    if (pVM->GetEnv(reinterpret_cast<void**>(&gJNIHandler.mEnv), JNI_VERSION_1_6) != JNI_OK)
    {
        hms::Hermes::getInstance()->getLogger()->print(hms::ELogLevel::Error, "Failed to get JNI environment.");
        return -1;
    }

    return JNI_VERSION_1_6;
}

extern "C" JNIEXPORT void JNICALL Java_pl_grupapracuj_hermes_helloworld_MainActivity_createHelloWorld(JNIEnv* pEnvironment, jobject pObject, jstring pCertificatePath)
{
    std::string certificatePath;

    if (gJNIHandler.mEnv != NULL)
    {
        const char *tmp = gJNIHandler.mEnv->GetStringUTFChars(pCertificatePath, NULL);

        if (tmp)
        {
            const jsize length = gJNIHandler.mEnv->GetStringUTFLength(pCertificatePath);
            certificatePath = std::string(tmp, length);
            gJNIHandler.mEnv->ReleaseStringUTFChars(pCertificatePath, tmp);
        }
    }

    gJNIHandler.mActivity = pEnvironment->NewGlobalRef(pObject);
    gHelloWorld = new HelloWorld();
    gHelloWorld->setCertificatePath(certificatePath);
}

extern "C" JNIEXPORT void JNICALL Java_pl_grupapracuj_hermes_helloworld_MainActivity_destroyHelloWorld(JNIEnv* pEnvironment, jobject pObject)
{
    gJNIHandler.mEnv->DeleteGlobalRef(gJNIHandler.mActivity);
    delete gHelloWorld;
}

extern "C" JNIEXPORT void JNICALL Java_pl_grupapracuj_hermes_helloworld_MainActivity_executeHelloWorld(JNIEnv* pEnvironment, jobject pObject)
{
    if (gHelloWorld != nullptr)
        gHelloWorld->execute();
}
