// ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// You can use this code to check a basic functionality of Hermes library. Please paste this code into your project and wait until a callback will be called (in this example after a max 15 seconds).
// ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

#include "hermes.hpp"

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

void hermes_sample()
{
	// --- Logger ---

	hms::Logger* logger = hms::Hermes::getInstance()->getLogger();
    logger->initialize(hms::ELogLevel::Info, nullptr);

	// --- Data ---
    hms::DataManager* dataManager = hms::Hermes::getInstance()->getDataManager();
    dataManager->initialize();
    
    const size_t jphDataId = 0;
    dataManager->add(std::shared_ptr<hms::DataShared>(new JphData(jphDataId)));

    // --- Thread ---
    
    // First element of pair - thread pool id, second element of pair - number of threads in thread pool.
	const std::pair<int, int> threadInfo = {0, 2};
    hms::TaskManager* taskManager = hms::Hermes::getInstance()->getTaskManager();
    taskManager->initialize({threadInfo});
    
    // --- Network ---
    
    // Connection timeout.
	const long timeout = 15;    
    hms::NetworkManager* networkManager = hms::Hermes::getInstance()->getNetworkManager();
    networkManager->initialize(timeout, threadInfo.first);
    
    // Prepare network API.
    const size_t jphNetworkId = 0;
	std::shared_ptr<hms::NetworkAPI> jphNetwork = networkManager->add("JsonPlaceholder", "https://jsonplaceholder.typicode.com/", jphNetworkId);
	jphNetwork->setDefaultHeader({{"Content-Type", "application/json; charset=utf-8"}});
	
	// --- Post setup code ---

    // Request callback.
    auto callback = [jphDataId](hms::NetworkResponse lpResponse) -> void
    {
        if (lpResponse.mCode == hms::ENetworkCode::OK)
        {
            auto jphData = hms::Hermes::getInstance()->getDataManager()->get<JphData>(jphDataId);
            
            if (jphData->unpack(lpResponse.mMessage, {}))
            {
            	hms::Hermes::getInstance()->getLogger()->print(hms::ELogLevel::Info, "User Id: %, User Name: %, User City: %", jphData->mId, jphData->mName, jphData->mCity);
            }
        }
        else
        {
        	hms::Hermes::getInstance()->getLogger()->print(hms::ELogLevel::Error, "Connection problem: %", lpResponse.mMessage);
        }
    };
    
    hms::NetworkRequestParam requestParam;
    requestParam.mRequestType = hms::ENetworkRequestType::Get;
    requestParam.mMethod = "users/1";
    requestParam.mCallback = callback;

    jphNetwork->request(std::move(requestParam));
}
