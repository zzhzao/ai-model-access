#include "../include/GeminiProvider.h"
#include"../include/util/myLog.h"
#include <httplib.h>
#include <jsoncpp/json/forwards.h>
#include <jsoncpp/json/json.h>
#include <jsoncpp/json/reader.h>
namespace ai_chat_sdk{
    bool GeminiProvider::initModel(const std::map<std::string, std::string>& modelConfig)
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
            _endpoint = "https://generativelanguage.googleapis.com";
        }else{
            _endpoint = it->second;
        }
        
        _isAvailable = true;
        INFO("GeminiProvider initModel success, endpoint: {}",_endpoint);
        return true;
    }
        std::string GeminiProvider::getModelName() const 
        {
            return "Gemini-2.0-flash";
        }
        std::string GeminiProvider::getModelDesc() const 
        {
            return "Google 的急速响应模型，专为大模型部署和快速交互的场景实现";
        }
        bool GeminiProvider::isAvailable() const 
        {
            return _isAvailable;
        }
        // 发送消息 -- 全量返回
        std::string GeminiProvider::sendMessage(const std::vector<Message>& messages,const std::map<std::string,std::string>& requestParam) 
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
        auto response = client.Post("/v1beta/openai/chat/completions", headers, requestBodyStr, "application/json");
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
        std::string GeminiProvider::sendMessageStream(const std::vector<Message>& messages,
            const std::map<std::string,std::string>& requestParam,
            std::function<void(const std::string&,bool)> callback)
        {
            //检测模型是否可用
            if(!_isAvailable){
                ERR("GeminiProvider sendMessageStream model not available");
                return "";
            }

            //构建请求参数
                    double temperature = 0.7;
        int maxTokens = 2048;
        if(requestParam.find("temperature") != requestParam.end()){
            temperature = std::stod(requestParam.at("temperature"));
        }
        if(requestParam.find("max_tokens") != requestParam.end()){
            maxTokens = std::stoi(requestParam.at("max_tokens"));
        }

        //构建历史消息
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
        requestBody["stream"] = true;
        // 序列化请求体
        Json::StreamWriterBuilder writerBuilder;
        writerBuilder["indentation"] = "";
        std::string requestBodyStr = Json::writeString(writerBuilder, requestBody);
        INFO("GeminiProvider sendMessageStream requestBody: {}", requestBodyStr);
        // 创建http客户端
        httplib::Client client(_endpoint);
        client.set_connection_timeout(30,0);
        client.set_read_timeout(300,0);
        client.set_proxy("127.0.0.1",7890);
        httplib::Headers headers = {
            {"Authorization", "Bearer " + _apiKey},
        };

        // 流式处理变量
        std::string buffer;          // 接受流式响应的数据块
        bool gotError = false;       // 标记响应是否成功
        std::string errorMsg;        // 错误描述符
        int statusCode = 0;          // 响应状态码
        bool streamFinish = false;   // 标记流式响应是否完成
        std::string fullResponse;    // 累积完整的响应

        httplib::Request req;
        req.method = "POST";
        req.path = "/v1beta/openai/chat/completions";
        req.headers = headers;
        req.body = requestBodyStr;
        // 设置响应处理器
        req.response_handler = [&](const httplib::Response& res) {
            if(res.status != 200){
                gotError = true;
                errorMsg = "HTTP status code: " + std::to_string(res.status);
                return false;    // 终止请求
            }
            return true;   // 继续接收后续数据
        };
        req.content_receiver = [&](const char* data, size_t dataLength, size_t offset,size_t totalLength){
            // 如果HTTP请求失败
            if(gotError){
                return false;
            }
            // 累加数据块
            buffer.append(data, dataLength);
            //处理所有的增量数据，数据之间以"\n\n"分隔
            size_t pos = 0;
            while((pos = buffer.find("\n\n", pos)) != std::string::npos){
                std::string line = buffer.substr(0,pos);
                buffer.erase(0,pos+2);
                // : 开头的行，直接跳过 代表注释
                if(line.empty()|| line[0] ==':'){
                    continue;
                }
                if(line.compare(0,6,"data: ") == 0){
                    std::string dataStr = line.substr(6);
                    // 处理结尾标记
                    if(dataStr == "[DONE]"){
                        callback("", true);
                        streamFinish = true;
                        return true;
                    }
                    // 解析json字符串
                    Json::Value chunk;
                    Json::CharReaderBuilder reader;
                    std::stringstream ss(dataStr);
                    std::string errors;
                    if(Json::parseFromStream(reader, ss, &chunk, &errors)){
                        if(chunk.isMember("choices") &&
                          chunk["choices"].isArray() && 
                          !chunk["choices"].empty() &&
                          chunk["choices"][0].isMember("delta") &&
                          chunk["choices"][0]["delta"].isMember("content")){
                            std::string content = chunk["choices"][0]["delta"]["content"].asString();
                            // 处理deltaContent，例如追加到fullResponse
                            fullResponse += content;

                            // 将本次解析出的模型返回的有效数据转给调用sendMessageStraem函数的用户使用---callback
                            callback(content, false);
                        }

                    }
                    else
                    {
                        ERR("GeminiProvider sendMessageStream dataStr parse failed, error: {}", errors);
                        return false;
                    }

                }
            }
        };
        auto res = client.send(req);
        if(!res){
            ERR("GeminiProvider sendMessageStream send request failed, error: {}", to_string(res.error()));
            return "";
        }
        // 确保流式操作完成
        if(!streamFinish){
            WARN("stream ended without [DONE] marker");
            return "";
        }
        return fullResponse;

        }
}//end ai_chat_sdk