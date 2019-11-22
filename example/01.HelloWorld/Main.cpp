#include "HelloWorld.hpp"

int main(int pArgumentsCount, char* pArguments[])
{
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
    }, "./certificate.pem");

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
