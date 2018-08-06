#include <string>
#include <unordered_map>
#include "hermes.hpp"
#include "ext/hmsJNI.hpp"
#include "ext/hmsSerializer.hpp"

enum class EDataType : size_t
{
    User = 0
};

enum class ENetworkAPI : size_t
{
    JsonPlaceholder = 0
};

class UserData : public hms::DataShared
{
public:
    class Address
    {
    public:
        std::string mStreet;
        std::string mSuite;
        std::string mCity;
        std::string mZipCode;
    };

    UserData(size_t pId) : DataShared(pId)
    {
    };

    int32_t mId = -1;
    std::string mName;
    Address mAddress;
};

namespace hms
{
    namespace ext
    {
        template<>
        inline auto registerProperties<UserData::Address>()
        {
            return std::make_tuple(Property<UserData::Address, std::string>("street", &UserData::Address::mStreet, ESerializerMode::Deserialize),
                Property<UserData::Address, std::string>("suite", &UserData::Address::mSuite, ESerializerMode::Deserialize),
                Property<UserData::Address, std::string>("city", &UserData::Address::mCity, ESerializerMode::Deserialize),
                Property<UserData::Address, std::string>("zipcode", &UserData::Address::mZipCode, ESerializerMode::Deserialize));
        }

        template<>
        inline auto registerProperties<UserData>()
        {
            return std::make_tuple(Property<UserData, int32_t>("id", &UserData::mId),
                Property<UserData, std::string>("name", &UserData::mName),
                Property<UserData, UserData::Address>("address", &UserData::mAddress, ESerializerMode::Deserialize));
        }
    }
}

class HelloWorld
{
public:
    HelloWorld(std::string pCertificatePath)
    {
        auto logger = hms::Hermes::getInstance()->getLogger();
        logger->initialize(hms::ELogLevel::Info, nullptr);

        auto dataManager = hms::Hermes::getInstance()->getDataManager();
        dataManager->initialize();
        dataManager->add(std::shared_ptr<hms::DataShared>(new UserData(static_cast<size_t>(EDataType::User))));

        auto taskManager = hms::Hermes::getInstance()->getTaskManager();
        // First element of pair - thread pool id, second element of pair - number of threads in thread pool.
        const std::pair<int, int> threadInfo = {0, 2};
        taskManager->initialize({threadInfo});

        auto networkManager = hms::Hermes::getInstance()->getNetworkManager();
        // Connection timeout.
        const long timeout = 15;
        networkManager->initialize(timeout, threadInfo.first, {200, 299});

        auto jphNetwork = networkManager->add(static_cast<size_t>(ENetworkAPI::JsonPlaceholder), "JsonPlaceholder", "https://jsonplaceholder.typicode.com/");
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
                Json::Value json;
                if (hms::ext::json::convert(lpResponse.mMessage, json))
                {
                    auto userData = hms::Hermes::getInstance()->getDataManager()->get<UserData>(static_cast<size_t>(EDataType::User));
                    hms::ext::json::deserialize<UserData>(*userData, json);

                    std::string outputText = "ID: ";
                    outputText += std::to_string(userData->mId);
                    outputText += " / Name: ";
                    outputText += userData->mName;
                    outputText += " / City: ";
                    outputText += userData->mAddress.mCity;

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

        hms::Hermes::getInstance()->getNetworkManager()->get(static_cast<size_t>(ENetworkAPI::JsonPlaceholder))->request(std::move(requestParam));
    }
};

std::unordered_map<std::string, jclass> gClassLoader;

extern "C" JNIEXPORT jobject JNICALL Java_pl_grupapracuj_hermes_helloworld_MainActivity_createNative(JNIEnv* pEnvironment, jobject pObject, jstring pCertificatePath)
{
    auto helloWorld = std::make_shared<HelloWorld>(hms::ext::jni::Utility::string(pCertificatePath, pEnvironment));

    return hms::ext::jni::NativeObject::create<std::shared_ptr<HelloWorld>>(helloWorld, pEnvironment, gClassLoader["pl/grupapracuj/hermes/ext/jni/NativeObject"]);
}

extern "C" JNIEXPORT void JNICALL Java_pl_grupapracuj_hermes_helloworld_MainActivity_executeNative(JNIEnv* pEnvironment, jobject pObject, jlong pHelloWorldPointer)
{
    auto helloWorld = hms::ext::jni::NativeObject::get<std::shared_ptr<HelloWorld>>(pHelloWorldPointer);

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

jint JNI_OnLoad(JavaVM* pVM, void* pReserved)
{
    JNIEnv* environment = nullptr;
    if (pVM->GetEnv(reinterpret_cast<void**>(&environment), JNI_VERSION_1_6) != JNI_OK)
    {
        return -1;
    }

    std::array<std::string, 1> classNames = {
        "pl/grupapracuj/hermes/ext/jni/NativeObject"
    };

    for (const auto& v : classNames)
    {
        jclass classWeakRef = environment->FindClass(v.c_str());
        gClassLoader[v] = static_cast<jclass>(environment->NewGlobalRef(classWeakRef));
        environment->DeleteLocalRef(classWeakRef);
    }

    return JNI_VERSION_1_6;
}
