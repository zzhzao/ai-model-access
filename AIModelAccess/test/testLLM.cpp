#include<gtest/gtest.h>
#include <memory>
#include <spdlog/common.h>
#include "../sdk/include/DeepSeekProvider.h"
#include "../sdk/include/common.h"
#include"../sdk/include/util/myLog.h"
TEST(DeepSeekProviderTest, sendMessage)
{
    auto provider = std::make_shared<ai_chat_sdk::DeepSeekProvider>();
    ASSERT_TRUE(provider != nullptr);
    std::map<std::string, std::string> modelParam;
    modelParam["api_key"] = std::getenv("deepseek_apikey");
    modelParam["endpoint"] = "https://api.deepseek.com";
    provider->initModel(modelParam);

        std::map<std::string, std::string> requestParam = {
        {"temperature", "0.7"},
        {"max_tokens", "2048"}
        };
    std::vector<ai_chat_sdk::Message> messages;
    messages.push_back({"user", "你是谁"});

        std::string response = provider->sendMessage(messages,requestParam);
        ASSERT_TRUE(response != "");
}


int main(int argc, char **argv)
{
    util_log::Logger::initLogger("testLLM","stdout",spdlog::level::debug);
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
