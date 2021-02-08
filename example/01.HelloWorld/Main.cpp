// Copyright (C) 2017-2021 Grupa Pracuj Sp. z o.o.
// This file is part of the "Hermes" library.
// For conditions of distribution and use, see copyright notice in license.txt.

#include "HelloWorld.hpp"

#include <string>
#include <fstream>
#include <streambuf>

int main(int pArgumentsCount, char* pArguments[])
{
    std::ifstream t("./certificate.pem");
    std::string str;
    t.seekg(0, std::ios::end);   
    str.reserve(t.tellg());
    t.seekg(0, std::ios::beg);
    str.assign((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
    
    printf("%s \n\n", str.c_str());

    std::mutex mainThreadMutex;
    std::queue<std::function<void()>> mainThreadTasks;
    auto dequeueMainThreadTasks = [&mainThreadMutex, &mainThreadTasks]()
    {
        bool hasMainThreadTask = false;

        {
            std::lock_guard<std::mutex> lock(mainThreadMutex);
            hasMainThreadTask = !mainThreadTasks.empty();
        }

        while (hasMainThreadTask)
        {
            std::function<void()> task = nullptr;

            {
                std::lock_guard<std::mutex> lock(mainThreadMutex);
                task = std::move(mainThreadTasks.front());
                mainThreadTasks.pop();
                hasMainThreadTask = !mainThreadTasks.empty();
            }

            task();
        }
    };

    HelloWorld helloWorld([&mainThreadMutex, &mainThreadTasks](std::function<void()> lpTask) -> void
    {
        std::lock_guard<std::mutex> lock(mainThreadMutex);
        mainThreadTasks.push(std::move(lpTask));
    }, {hms::ENetworkCertificate::Content, str});

    for (int32_t i = 1; i < 8; ++i)
    {
        bool wait = true;
        helloWorld.execute([&wait](std::string lpOutputText) -> void
        {
            wait = false;
        }, i);

        while (wait)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            dequeueMainThreadTasks();
        }
    }

    return 0;
}
