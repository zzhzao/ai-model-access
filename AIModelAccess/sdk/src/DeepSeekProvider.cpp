#include"../include/DeepSeekProvider.h"
#include"../include/util/myLog.h"
#include "../include/common.h"
#include <cstddef>
#include <cstdint>
#include<iostream>
#include <jsoncpp/json/reader.h>
#include<map>
#include <sstream>
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
        it = modelconfig.find("endpoint");
        if(it == modelconfig.end())
        {
            ERR("DeepSeekProvider initModel: endpoint not found");
            return false;
        }
        else{
            _endpoint = it->second;
        }
        INFO("DeepSeekProvider initModel: endpoint = {}", _endpoint.c_str());   
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
        // 1. 检测模型是否可用
        if(!isAvailable()){
            ERR("DeepSeekProvider sendMessage model not available");
            return "";
        }

        // 2. 构造请求参数
        double temperature = 0.7;
        int maxTokens = 2048;
        if(requestParam.find("temperature") != requestParam.end()){
            temperature = std::stod(requestParam.at("temperature"));
        }
        if(requestParam.find("max_tokens") != requestParam.end()){
            maxTokens = std::stoi(requestParam.at("max_tokens"));
        }

        // 构造历史消息
        Json::Value messageArray(Json::arrayValue);
        for(const auto& message : messages){
            Json::Value messageObject;
            messageObject["role"] = message._role;
            messageObject["content"] = message._content;
            messageArray.append(messageObject);
        }

        // 3. 构造请求体
        Json::Value requestBody;
        requestBody["model"] = getModelName();
        requestBody["messages"] = messageArray;
        requestBody["temperature"] = temperature;
        requestBody["max_tokens"] = maxTokens;

        // 4. 序列化
        Json::StreamWriterBuilder writerBuilder;
        writerBuilder["indentation"] = "";
        std::string requestBodyStr = Json::writeString(writerBuilder, requestBody);
        INFO("DeepSeekProvider sendMessage requestBody: {}", requestBodyStr);

        // 5. 使用cpp-httplib库构造HTTP客户端
        httplib::Client client(_endpoint.c_str());
        client.set_connection_timeout(30, 0);     // 连接超时时间为30秒
        client.set_read_timeout(60, 0);           // 设置超时时间为60秒

        // 设置请求头
        httplib::Headers headers = {
            {"Authorization", "Bearer " + _apiKey},
            {"Content-Type", "application/json"}
        };

        // 6. 发送POST请求
        auto response = client.Post("/v1/chat/completions", headers, requestBodyStr, "application/json");
        if(!response){
            ERR("DeepSeekProvider sendMessage POST request failed");
            return "";
        }
        INFO("DeepSeekProvider sendMessage POST request success, status : {}", response->status);
        INFO("DeepSeekProvider sendMessage POST request success, body : {}", response->body);

        // 检测响应是否成功
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
                    INFO("DeepSeekProvider response text: {}", replyContent);
                    return replyContent;
                }
            }
        }

        // 8. json解析失败
        ERR("DeepSeekProvider sendMessage POST response body parse failed, error");
        return "deepseek response json parse failed";

    }

    std::string DeepSeekProvider::sendMessageStream(const std::vector<Message>& messages,
            const std::map<std::string,std::string>& requestParam,
            std::function<void(const std::string&,bool)> callback)  //callback: 对模型返回的增量数据如何处理，第一个参数为增量数据，第二个参数为是否为最后一个增量数据。
    {
        // 检测模型是否可用
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

        //构造历史消息
        Json::Value historyMessages(Json::arrayValue);
        for(const auto& message : messages)
        {
            Json::Value messageJson;
            messageJson["role"] = message._role;
            messageJson["content"] = message._content;
            historyMessages.append(messageJson);
        }

        // 构造请求体
        Json::Value requestBody;
        requestBody["model"] = getModelName();
        requestBody["messages"] = historyMessages;
        requestBody["temperature"] = temperature;
        requestBody["max_tokens"] = maxTokens;
        requestBody["stream"] = true;

        //序列化
        Json::StreamWriterBuilder writerBuilder;
        writerBuilder["indentation"] = "";
        std::string requestBodyStr = Json::writeString(writerBuilder, requestBody);
        INFO("DeepSeekProvider sendMessageStream: requestBody = {}", requestBodyStr);

        //使用httplib创建http客户端
        httplib::Client client(_endpoint.c_str());
        client.set_connection_timeout(30,0);
        client.set_read_timeout(300,0);    //流式响应需要更长的时间
        // 设置请求头
        httplib::Headers headers = {
            {"Authorization", "Bearer " + _apiKey},
            {"Content-Type", "application/json"},
            {"Accept", "text/event-stream"}
        };

        std::string buffer;      //接受流式响应的数据
        bool gotError = false;  //响应是否成功
        std::string errorMsg;   //错误描述符
        int statusCode = 0;     //http状态码
        bool streamFinish = false;   //是否完成流式响应
        std::string fullResponse;   //完整的响应体

        //创建请求对象
        httplib::Request req;
        req.method = "POST";
        req.path = "/v1/chat/completions";
        req.headers = headers;
        req.body = requestBodyStr;
        // 响应处理器
        req.response_handler = [&](const httplib::Response& res) {
            statusCode = res.status;
            if(statusCode != 200)
            {
                gotError = true;
                errorMsg = "request failed, status = " + std::to_string(statusCode);
                return false;
            }
            return true;
        };

        // 设置数据接收处理器  --- 解析流式响应的每个块的数据
        req.content_receiver = [&](const char* data,size_t len,size_t offset,size_t totallength){
            // 验证响应头是否出错
            if(gotError)
            {
                return false;
            }

            // 追加数据到buffer
            buffer.append(data,len);
            INFO("DeepSeekProvider sendMessageStream: buffer = {}", buffer);

            // 处理所有的流式响应的数据块  数据块之间以\n\n分隔
            size_t pos = 0;
            while(pos = buffer.find("\n\n") != std::string::npos)
            {
                std::string chunk = buffer.substr(0,pos);
                buffer.erase(0,pos+2);

                //解析chunk
                //处理空行和注释
                if(chunk.empty() || chunk[0] == ':')
                {
                    continue;
                }
                // 获取模型返回的有效数据
                if(chunk.compare(0,6,"data: ") == 0)
                {
                    std::string modelData = chunk.substr(6);
                    //检测是否为结束标记
                    if(modelData == "[DONE]")
                    {
                        streamFinish = true;
                        return true;
                    }

                    //反序列化
                    Json::Value modelDataJson;
                    Json::CharReaderBuilder readerBuilder;
                    std::string errors;
                    std::istringstream modelDataStream(modelData);
                    if(Json::parseFromStream(readerBuilder,modelDataStream,&modelDataJson,&errors))
                    {
                        if(modelDataJson.isMember("choices") && modelDataJson["choices"].isArray() && modelDataJson["choices"].size() > 0)
                        {
                            Json::Value choice = modelDataJson["choices"][0];
                            if(choice.isMember("delta") && choice["delta"].isMember("content"))
                            {
                                std::string deltaContent = choice["delta"]["content"].asString();
                                fullResponse += deltaContent;
                                // 将解析出来的增量数据传递给回调函数
                                callback(deltaContent,false);
                            }
                        }
                    }
                    else {
                        ERR("DeepSeekProvider sendMessageStream: parse modelData failed, errors = {}", errors);
                    }
                }
            }
            return true;
        };
        
        auto result = client.send(req);
        if(!result)
        {
            // 请求发送失败，出现网络问题，比如DNS解析失败，请求超时
            ERR("Network error: {}", to_string(result.error()));
            return "";
        }
        //确保流式操作正常结束
        if(!streamFinish)
        {
            WARN("DeepSeekProvider sendMessageStream: stream not finish");
            callback("",true);
        }
        return fullResponse;
    }

};
