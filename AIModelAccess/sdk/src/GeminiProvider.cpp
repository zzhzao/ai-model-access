#include "../include/GeminiProvider.h"
#include"../include/util/myLog.h"
#include <jsoncpp/json/json.h>
namespace ai_chat_sdk{
    virtual bool initModel(const std::map<std::string, std::string>& modelConfig)
    {
        auto it = modelConfig.find("api_key");
        if(it == modelConfig.end()){
            ERR("GeminiProvider initModel api_key not found");  
            return false;
        }else{
            _apiKey = it->second;
        }

        // 初始化Base URL
        it = modelConfig.find("endpoint");
        if(it == modelConfig.end()){
            _endpoint = "https://api.gemini.ai";
        }else{
            _endpoint = it->second;
        }
        
        _isAvailable = true;
        INFO("GeminiProvider initModel success, endpoint: {}",_endpoint);
        return true;
    }
        virtual std::string getModelName() const 
        {
            return "Gemini-2.0-flash";
        }
        virtual std::string getModelDesc() const 
        {
            return "Google 的急速响应模型，专为大模型部署和快速交互的场景实现";
        }
        virtual bool isAvailable() const 
        {
            return _isAvailable;
        }
        // 发送消息 -- 全量返回
        virtual std::string sendMessage(const std::vector<Message>& messages,const std::map<std::string,std::string>& requestParam) 
        {
            // 检测模型是否可用
            if(!_isAvailable){
                ERR("GeminiProvider sendMessage model not available");
                return "";
            }

            //构造请求参数
        double temperature = 0.7;
        int maxTokens = 2048;
        if(requestParam.find("temperature") != requestParam.end()){
            temperature = std::stod(requestParam.at("temperature"));
        }
        if(requestParam.find("max_tokens") != requestParam.end()){
            maxTokens = std::stoi(requestParam.at("max_tokens"));
        }

        Json::Value messageArray(Json::arrayValue);
        for(const auto& message : messages){
            Json::Value messageObject;
            messageObject["role"] = message._role;
            messageObject["content"] = message._content;
            messageArray.append(messageObject);
        }
        // 构造请求体
        Json::Value requestBody(Json::objectValue);
        requestBody["model"] = getModelName();
        requestBody["messages"] = messageArray;
        requestBody["temperature"] = temperature;
        requestBody["max_tokens"] = maxTokens;

        // 序列化请求体
        Json::StreamWriterBuilder writerBuilder;
        writerBuilder["indentation"] = "";
        std::string requestBodyStr = Json::writeString(writerBuilder, requestBody);
        INFO("GeminiProvider sendMessage requestBody: {}", requestBodyStr);

        // 创建http客户端
        httplib::Client client(_endpoint);
        client.set_connection_timeout(30,0);
        client.set_read_timeout(60,0);
        client.set_proxy("127.0.0.1",7890);

        httplib::Headers headers = {
            {"Authorization", "Bearer " + _apiKey},
        };
        auto response = client.Post("/v1/chat/completions", headers, requestBodyStr, "application/json");
        if(!response){
            ERR("GeminiProvider sendMessage POST request failed");
            return "";
        }
        INFO("GeminiProvider sendMessage POST request success, status : {}", response->status);
        INFO("GeminiProvider sendMessage POST request success, body : {}", response->body);
        if(response->status != 200){
            return "";
        }

        // 7. 解析响应体
        Json::Value responseBody;
        Json::CharReaderBuilder readerBuilder;
        std::string parseError;
        std::istringstream responseStream(response->body);
        if(Json::parseFromStream(readerBuilder, responseStream, &responseBody, &parseError)){
            // 获取message数组
            if(responseBody.isMember("choices") && responseBody["choices"].isArray() && !responseBody["choices"].empty()){
                auto choice = responseBody["choices"][0];
                if(choice.isMember("message") && choice["message"].isMember("content")){
                    std::string replyContent = choice["message"]["content"].asString();
                    INFO("GeminiProvider response text: {}", replyContent);
                    return replyContent;
                }
            }
        }

        // 8. json解析失败
        ERR("GeminiProvider sendMessage POST response body parse failed, error: {}", parseError);
        return "gemini response json parse failed";   
        }
        // 发送消息 -- 流式返回
        virtual std::string sendMessageStream(const std::vector<Message>& messages,
            const std::map<std::string,std::string>& requestParam,
            std::function<void(const std::string&,bool)> callback)
        {
            return "";
        }

}//end ai_chat_sdk