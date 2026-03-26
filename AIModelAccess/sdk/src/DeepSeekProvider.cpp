#include"../include/DeepSeekProvider.h"
#include"../include/util/myLog.h"
#include "../include/common.h"
#include<iostream>
#include <jsoncpp/json/reader.h>
#include<map>
#include <string>
#include<vector>
#include<jsoncpp/json/json.h>
#include<httplib.h>
namespace ai_chat_sdk{
    bool DeepSeekProvider::initModel(const std::map<std::string, std::string>& modelconfig)
    {
        // 初始化api key
        auto it = modelconfig.find("_apiKey");
        if(it == modelconfig.end())
        {
            ERR("DeepSeekProvider initModel: _apiKey not found");
            return false;
        }
        else{
            _apiKey = it->second;

        }

        //初始化 Base URL
        it = modelconfig.find("_baseUrl");
        if(it == modelconfig.end())
        {
            ERR("DeepSeekProvider initModel: _baseUrl not found");
            return false;
        }
        else{
            _endpoint = it->second;
        }
        INFO("DeepSeekProvider initModel: _baseUrl = {}", _endpoint.c_str());
        _isAvailable = true;
        return true;
    }
    // 检测模型是否可用
    bool DeepSeekProvider::isAvailable() const
    {
        return _isAvailable;
    }
    // 获取模型名称
    std::string DeepSeekProvider::getModelName() const
    {
        return "deepseek-chat";
    }
    // 获取模型描述信息
    std::string DeepSeekProvider::getModelDesc() const
    {
        return "一款实用性强，中文优化的通用对话助手，适合日常问答与创作";
    }
    std::string DeepSeekProvider::sendMessage(const std::vector<Message>& messages,const std::map<std::string,std::string>& requestParam)
    {
        if(!isAvailable())
        {
            ERR("DeepSeekProvider sendMessage: model not available");
            return "";
        }
        // 构造请求参数
        double temperature = 0.7;
        int maxTokens = 2048;
        if(requestParam.find("temperature") != requestParam.end())
        {
            temperature = std::stod(requestParam.at("temperature"));
        }
        if(requestParam.find("maxTokens") != requestParam.end())
        {
            maxTokens = std::stoi(requestParam.at("maxTokens"));
        }
        // 构造历史消息
        Json::Value historyMessages(Json::arrayValue);
        for(const auto& message : messages)
        {
            Json::Value messageJson;
            messageJson["role"] = message._role;
            messageJson["content"] = message._content;
            historyMessages.append(messageJson);
        }
        //构造请求体
        Json::Value requestBody;
        requestBody["model"] = getModelName();
        requestBody["messages"] = historyMessages;
        requestBody["temperature"] = temperature;
        requestBody["max_tokens"] = maxTokens;

        //序列化
        Json::StreamWriterBuilder writerBuilder;
        writerBuilder["indentation"] = "";
        std::string requestBodyStr = Json::writeString(writerBuilder, requestBody);
        INFO("DeepSeekProvider sendMessage: requestBody = {}", requestBodyStr.c_str());

        // 使用httplib构建HTTP客户端
        httplib::Client client(_endpoint.c_str());
        client.set_connection_timeout(30,0);
        client.set_read_timeout(60,0);
        // 设置请求头
        httplib::Headers headers = {
            {"Authorization", "Bearer " + _apiKey},
            {"Content-Type", "application/json"}
        };
 
        // 发送POST请求
        auto res = client.Post("/v1/chat/completions", headers, requestBodyStr,"application/json");
        if(res == nullptr)
        {
            ERR("DeepSeekProvider sendMessage: request failed");
            return "";
        }
        INFO("DeepSeekProvider sendMessage: responseStatus = {}", res->status);

        INFO("DeepSeekProvider sendMessage: responseBody = {}", res->body.c_str());

        if(res->status != 200)
        {
            ERR("DeepSeekProvider sendMessage: request failed, status = {}", res->status);
            return "";
        }
        // 解析响应体
        Json::Value responseJson;
        Json::CharReaderBuilder readerBuilder;
        std::string parseError;
        std::istringstream responseStream(res->body);
        if(Json::parseFromStream(readerBuilder, responseStream, &responseJson, &parseError))
        {
            if(responseJson.isMember("choices")&& responseJson["choices"].size() > 0)
            {
                // 提取模型返回的消息内容
                std::string modelResponse = responseJson["choices"][0]["message"]["content"].asString();
                INFO("DeepSeekProvider sendMessage: modelResponse = {}", modelResponse.c_str());
                return modelResponse;
            }
            else{
                ERR("DeepSeekProvider sendMessage: responseJson not contain choices");
                return "";
            }
        }
 
            ERR("DeepSeekProvider sendMessage: parse response failed, error = {}", parseError.c_str());
            return "";

    }

    std::string DeepSeekProvider::sendMessageStream(const std::vector<Message>& messages,
            const std::map<std::string,std::string>& requestParam,
            std::function<void(const std::string&,bool)> callback)  //callback: 对模型返回的增量数据如何处理，第一个参数为增量数据，第二个参数为是否为最后一个增量数据。
    {

    }

    
    
};
