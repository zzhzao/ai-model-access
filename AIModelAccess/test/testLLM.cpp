#include<gtest/gtest.h>
#include <memory>
#include <spdlog/common.h>
#include "../sdk/include/DeepSeekProvider.h"
#include "../sdk/include/common.h"
#include"../sdk/include/util/myLog.h"
TEST(DeepSeekProviderTest, sendMessage){
    auto provider = std::make_shared<ai_chat_sdk::DeepSeekProvider>();
    ASSERT_TRUE(provider != nullptr);

    std::map<std::string, std::string> modelParam;
    modelParam["api_key"] = std::getenv("deepseek_apikey");
    modelParam["endpoint"] = "https://api.deepseek.com";

    provider->initModel(modelParam);
    ASSERT_TRUE(provider->isAvailable());

    std::map<std::string, std::string> requestParam = {
        {"temperature", "0.7"},
        {"max_tokens", "2048"}
    };
    std::vector<ai_chat_sdk::Message> messages;
    messages.push_back({"user", "你是谁？"});

    // 实例化DeepSeekProvider的对象
    // 调用sendMessage方法
    //std::string response = provider->sendMessage(messages, requestParam);

    auto writeChunk = [&](const std::string& chunk, bool last){
        INFO("chunk : {}", chunk);
        if(last){
            INFO("[DONE]");
        } 
    };
    std::string fullData = provider->sendMessageStream(messages, requestParam, writeChunk);
    ASSERT_FALSE(fullData.empty());
    INFO("response : {}", fullData);
}

// TEST(ChatGPTProviderTest, sendMessage){
//     auto provider = std::make_shared<ai_chat_sdk::ChatGPTProvider>();
//     ASSERT_TRUE(provider != nullptr);

//     std::map<std::string, std::string> modelParam;
//     modelParam["api_key"] = std::getenv("chatgpt_apikey");
//     modelParam["endpoint"] = "https://api.openai.com";

//     provider->initModel(modelParam);
//     ASSERT_TRUE(provider->isAvailable());

//     std::map<std::string, std::string> requestParam = {
//         {"temperature", "0.7"},
//         {"max_output_tokens", "2048"}
//     };
//     std::vector<ai_chat_sdk::Message> messages;
//     messages.push_back({"user", "你是谁？"});

//     // 实例化DeepSeekProvider的对象
//     // 调用sendMessage方法
//     //std::string fullData = provider->sendMessage(messages, requestParam);
//     //ASSERT_FALSE(fullData.empty());

//     auto writeChunk = [&](const std::string& chunk, bool last){ 
//         INFO("chunk : {}", chunk);
//         if(last){
//             INFO("[DONE]"); 
//         } 
//     };
//     std::string fullData = provider->sendMessageStream(messages, requestParam, writeChunk);
//     ASSERT_FALSE(fullData.empty());
//     INFO("response : {}", fullData);
// }

int main(int argc, char **argv)
{
    util_log::Logger::initLogger("testLLM","stdout",spdlog::level::debug);
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
