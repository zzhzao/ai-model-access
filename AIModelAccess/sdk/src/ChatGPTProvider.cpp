#include"../include/ChatGPTProvider.h"
#include"../include/util/myLog.h"
#include <jsoncpp/json/reader.h>
#include<jsoncpp/json/json.h>
#include<httplib.h>
#include <jsoncpp/json/value.h>
#include <jsoncpp/json/writer.h>

namespace ai_chat_sdk
{
    bool ChatGPTProvider::initModel(const std::map<std::string, std::string>& modelconfig)
    {
        // 初始化api key
        auto it = modelconfig.find("_apiKey");
        if(it == modelconfig.end())
        {
            ERR("ChatGPTProvider initModel: _apiKey not found");
            return false;
        }
        else{
            _apiKey = it->second;

        }
        it = modelconfig.find("endpoint");
        if(it == modelconfig.end())
        {
            ERR("ChatGPTProvider initModel: endpoint not found");
            return false;
        }
        else{
            _endpoint = it->second;
        }
        INFO("ChatGPTProvider initModel: endpoint = {}", _endpoint.c_str());   
        _isAvailable = true;
        return true;
    }

        // 检测模型是否可用
    bool ChatGPTProvider::isAvailable() const
    {
        return _isAvailable;
    }
    // 获取模型名称
    std::string ChatGPTProvider::getModelName() const
    {
        return "gpt-4o-mini";
    }
    // 获取模型描述信息
    std::string ChatGPTProvider::getModelDesc() const
    {
        return "GPT-4o Mini 是一个基于 GPT-4o 模型的对话助手，它提供了一个简单、方便的 API 接口，用于构建自己的应用。";
    }
    std::string ChatGPTProvider::sendMessage(const std::vector<Message>& messages,const std::map<std::string,std::string>& requestParam)
    {
        //检测模型是否可用
        if(!isAvailable())
        {
            ERR("ChatGPTProvider sendMessage: model not available");
            return "";
        }
        // 构造请求参数
        double temperature = 0.7;
        if(requestParam.find("temperature") != requestParam.end())
        {
            temperature = std::stod(requestParam.at("temperature"));
        }
        int max_tokens = 2048;
        if(requestParam.find("max_output_tokens") != requestParam.end())
        {
            max_tokens = std::stoi(requestParam.at("max_output_tokens"));
        }

        // 构建消息列表
        Json::Value messagesJson(Json::arrayValue);
        for(const auto& msg : messages)
        {
            Json::Value messageJson(Json::objectValue);
            messageJson["role"] = msg._role;
            messageJson["content"] = msg._content;
            messagesJson.append(messageJson);
        }
        // 构建请求体
        Json::Value requestBody;
        requestBody["model"] = getModelName();
        requestBody["input"] = messagesJson;
        requestBody["temperature"] = temperature;
        requestBody["max_out_tokens"] = max_tokens;

        Json::StreamWriterBuilder writer;
        writer["indentation"] = "  ";
        std::string requestBodyJsonstr = Json::writeString(writer,requestBody);

        // 创建http客户端
        httplib::Client client(_endpoint.c_str());
        client.set_connection_timeout(30,0);
        client.set_read_timeout(60,0);
        client.set_proxy("127.0.0.1",7890);

        httplib::Headers headers = {
                {"Authorization", "Bearer " + _apiKey}
           // {"Content-Type", "application/json"},
        };

        auto response = client.Post("/v1/response",  headers,requestBodyJsonstr, "application/json");
        if(!response)
        {
            ERR("ChatGPTProvider sendMessage: request failed, error = {}", to_string(response.error()));
            return "";
        }
        // 查看相应是否成功
        if(response->status != 200)
        {
            ERR("ChatGPTProvider sendMessage: request failed, status = {}", response->status);
            return "";
        }
        INFO("ChatGPTProvider sendMessage: request success, status = {}", response->status);
        // 解析响应体
        Json::CharReaderBuilder reader;
        std::string errorJson;
        Json::Value responseJson;
        std::istringstream responseStream(response->body);
        if(!Json::parseFromStream(reader, responseStream, &responseJson,&errorJson))
        {
            ERR("ChatGPTProvider sendMessage: parse response body failed");
            return "";
        }
        
        if(responseJson.isMember("output")&& responseJson["output"].isArray() && responseJson["output"].size() > 0)
        {
            auto output = responseJson["output"][0];
            if(output.isMember("content") && output["content"].isArray() && output["content"].size() > 0 && output["content"][0].isMember("text"))
            {
                return output["content"][0]["text"].asString();
            }
        }
    }
        // 发送消息 -- 流式返回
    std::string ChatGPTProvider::sendMessageStream(const std::vector<Message>& messages,
            const std::map<std::string,std::string>& requestParam,
            std::function<void(const std::string&,bool)> callback)
            {

            }  //callback: 对模型返回的增量数据如何处理，第一个参数为增量数据，第二个参数为是否为最后一个增量数据。
} // end ai_chat_sdk