#include "ChatServer.h"
#include <ai_chat_sdk/util/myLog.h>
#include <cstddef>
#include <cstdint>
#include <httplib.h>
#include <jsoncpp/json/forwards.h>
#include <jsoncpp/json/value.h>
#include <jsoncpp/json/reader.h>
#include <jsoncpp/json/writer.h>



namespace ai_chat_server {
ChatServer::ChatServer(const ServerConfig& config){
    _chatSDK = std::make_shared<ai_chat_sdk::ChatSDK>();

    auto deepseekConfig = std::make_shared<ai_chat_sdk::APIConfig>();
    deepseekConfig->_modelName = "deepseek-chat";
    deepseekConfig->_apiKey = config.deepseekAPIKey;
    deepseekConfig->_temperature = config.temperature;
    deepseekConfig->_maxTokens = config.maxTokens;

    // gpt-4o-mini
    auto chatGPTConfig = std::make_shared<ai_chat_sdk::APIConfig>();
    chatGPTConfig->_modelName = "gpt-4o-mini";
    chatGPTConfig->_apiKey = config.chatGPTAPIKey;
    chatGPTConfig->_temperature = config.temperature;
    chatGPTConfig->_maxTokens = config.maxTokens;


    // gemini-2.0-flash
    auto geminiConfig = std::make_shared<ai_chat_sdk::APIConfig>();
    geminiConfig->_modelName = "gemini-2.0-flash";
    geminiConfig->_apiKey = config.geminiAPIKey;
    geminiConfig->_temperature = config.temperature;
    geminiConfig->_maxTokens = config.maxTokens;

    // Ollama本地接入deepseek-r1:1.5b
    auto ollamaConfig = std::make_shared<ai_chat_sdk::OllamaConfig>();
    ollamaConfig->_modelName = config.olloamaModelName;
    ollamaConfig->_modelDesc = config.olloamaModelDesc;
    ollamaConfig->_endpoint = config.olloamaEndpoint;
    ollamaConfig->_temperature = config.temperature;
    ollamaConfig->_maxTokens = config.maxTokens;

    std::vector<std::shared_ptr<ai_chat_sdk::Config>> modelConfigs = {
        deepseekConfig, chatGPTConfig, geminiConfig, ollamaConfig
    };

    INFO("start init ChatSDK models...");
    if(!_chatSDK->initModels(modelConfigs)){
        ERR("ChatSDK init Failed!!!");
        return;
    }
    INFO("ChatSDK models init success!!!");

    // 创建http服务器
    _chatServer = std::make_unique<httplib::Server>();
    if(!_chatServer){
        ERR("ChatServer init Failed!!!");
        return;
    }
}

bool ChatServer::start(){
    if(_isRunning){
        ERR("ChatServer is already running!!!");
        return false;
    }
    // 创建线程监听http请求
    std::thread serverThread([this](){
        _chatServer->listen(_config.host, _config.port);
    });
    serverThread.detach();
    _isRunning.store(true);
    return true;
}

void ChatServer::stop(){
    if(!_isRunning){
        ERR("ChatServer is not running!!!");
        return;
    }
    if(_chatServer){
        _chatServer->stop();
    }
    _isRunning.store(false);
    INFO("ChatServer stopped success!");
}

bool ChatServer::isRunning() const{
    return _isRunning.load();
}
std::string ChatServer::buildResponse(const std::string& message,bool success)
{
    Json::Value responseJson;
    responseJson["success"] = success;
    responseJson["message"] = message;
    //序列化
    Json::StreamWriterBuilder writerBuilder;
    writerBuilder["indentation"] = "  ";
    std::string responseJsonStr = Json::writeString(writerBuilder, responseJson);
    return responseJsonStr;
}
    //处理创建会话的请求
    void ChatServer::handleCreateSessionRequest(const httplib::Request& req, httplib::Response& resp)
    {
        // 获取请求参数 请求参数在请求体
        // 通过反序列化拿到请求体的json格式
        Json::Value requestJson;
        Json::Reader reader;
        if(!reader.parse(req.body, requestJson)){
            std::string errorJsonStr = buildResponse("parse request body failed",false);
            
            resp.status = 400;  //客户端请求由语法错误
            resp.set_content(errorJsonStr,"application/json");
            return;
        }

        // 获取请求参数
        std::string modelName = requestJson.get("model", "deepseek-chat").asString();
        //创建会话
        std::string sessionID = _chatSDK->createSession(modelName);
        if(sessionID.empty()){
            std::string errorJsonStr = buildResponse("create session failed",false);

            resp.status = 500;  //服务器内部错误
            resp.set_content(errorJsonStr,"application/json");
            return;
        }
        // 构建响应体
        Json::Value dataJson;
        dataJson["session_id"] = sessionID;
        dataJson["model"] = modelName;
        Json::Value responseJson;
        responseJson["success"] = true;
        responseJson["message"] = "create session success";
        responseJson["data"] = dataJson;
        //序列化
        Json::StreamWriterBuilder writerBuilder;
        writerBuilder["indentation"] = "  ";
        std::string responseJsonStr = Json::writeString(writerBuilder, responseJson);
        resp.status = 200;  //成功
        resp.set_content(responseJsonStr,"application/json");

        return;
    
    }
    //处理获取会话的请求
    void ChatServer::handleGetSessionRequest(const httplib::Request& req, httplib::Response& resp)
    {
        // 获取会话列表
        std::vector<std::string> sessionIDs = _chatSDK->getSessionLists();

        // 构建session信息
        Json::Value dataArray;
        for(const auto& sessionID : sessionIDs){

            auto session = _chatSDK->getSession(sessionID);
            if(session){
                Json::Value sessionJson;
                sessionJson["id"] = sessionID;
                sessionJson["model"] = session->_modelName;
                sessionJson["created_at"] = static_cast<int64_t>(session->_createTime);
                sessionJson["updated_at"] = static_cast<int64_t>(session->_updateTime);
                sessionJson["message_count"] = session->_messages.size();
                if(!session->_messages.empty()){
                    sessionJson["first_user_message"] = session->_messages.front()._content;
                }
                dataArray.append(sessionJson);

            }
        }
        // 构建响应体
        Json::Value responseJson;
        responseJson["success"] = true;
        responseJson["message"] = "get session success";
        responseJson["data"] = dataArray;
        //序列化
        Json::StreamWriterBuilder writerBuilder;
        writerBuilder["indentation"] = "  ";
        std::string responseJsonStr = Json::writeString(writerBuilder, responseJson);
        resp.status = 200;  //成功
        resp.set_content(responseJsonStr,"application/json");

        return;
    
    }
    //处理获取模型列表请求
    void ChatServer::handleGetModelListRequest(const httplib::Request& req, httplib::Response& resp)
    {
        auto modelLists = _chatSDK->getAvailableModels();

        // 构建模型信息
        Json::Value modelArray;
        for(const auto& model : modelLists){
            Json::Value modelJson;
            modelJson["name"] = model._modelName;
            modelJson["desc"] = model._modelDesc;
            modelArray.append(modelJson);
        }
        // 构建响应体
        Json::Value responseJson;
        responseJson["success"] = true;
        responseJson["message"] = "get model success";
        responseJson["data"] = modelArray;
        //序列化
        Json::StreamWriterBuilder writerBuilder;
        writerBuilder["indentation"] = "  ";
        std::string responseJsonStr = Json::writeString(writerBuilder, responseJson);
        resp.status = 200;  //成功
        resp.set_content(responseJsonStr,"application/json");

        return;
    
    }
    //处理删除会话请求
    void ChatServer::handleDeleteSessionRequest(const httplib::Request& req, httplib::Response& resp)
    {
        //获取会话ID，从请求参数中获取
        std::string sessionID = req.matches[1];

        bool ret = _chatSDK->deleteSession(sessionID);
        if(!ret){
            std::string errorJsonStr = buildResponse("delete session failed",false);
            resp.status = 404;  //会话不存在
            resp.set_content(errorJsonStr,"application/json");
            return;
        }
        // 构建响应体
        std::string responseJsonStr = buildResponse("delete session success",true);
        resp.status = 200;  //成功
        resp.set_content(responseJsonStr,"application/json");

        return;
    
    }
    //处理发送消息请求  --全量返回
    void ChatServer::handleSendMessageRequest(const httplib::Request& req, httplib::Response& resp)
    {
        Json::Value requestJson;
        Json::Reader reader;
        if(!reader.parse(req.body, requestJson)){
            std::string errorJsonStr = buildResponse("parse request body failed",false);
            
            resp.status = 400;  //客户端请求由语法错误
            resp.set_content(errorJsonStr,"application/json");
            return;
        }
        // 解析请求参数
        std::string sessionID = requestJson.get("session_id", "").asString();
        std::string message = requestJson.get("message", "").asString();
        if(sessionID.empty() || message.empty()){
            std::string errorJsonStr = buildResponse("session_id or message is empty",false);
            resp.status = 400;  //客户端请求参数错误
            resp.set_content(errorJsonStr,"application/json");
            return;
        }
        // 发送消息
        std::string response = _chatSDK->sendMessage(sessionID, message);
        if(response.empty()){
            std::string errorJsonStr = buildResponse("Failed to send AI response message",false);
            resp.status = 500;  //服务器内部错误
            resp.set_content(errorJsonStr,"application/json");
            return;
        }
        // 构建响应体
        Json::Value dataJson;
        dataJson["session_id"] = sessionID;
        dataJson["response"] = response;
        dataJson["data"]["assistant_message"] = response;

        Json::Value responseJson;
        responseJson["success"] = true;
        responseJson["message"] = "send message success";
        responseJson["data"] = dataJson;
        //序列化
        Json::StreamWriterBuilder writerBuilder;
        writerBuilder["indentation"] = "  ";
        std::string responseJsonStr = Json::writeString(writerBuilder, responseJson);
        resp.status = 200;  //成功
        resp.set_content(responseJsonStr,"application/json");

        return;
    }
    //处理发送消息请求  --流式返回
    void ChatServer::handleSendMessageStreamRequest(const httplib::Request& req, httplib::Response& resp)
    {
        Json::Value requestJson;
        Json::Reader reader;
        if(!reader.parse(req.body, requestJson)){
            std::string errorJsonStr = buildResponse("parse request body failed",false);
            
            resp.status = 400;  //客户端请求由语法错误
            resp.set_content(errorJsonStr,"application/json");
            return;
        }
        // 解析请求参数
        std::string sessionID = requestJson.get("session_id", "").asString();
        std::string message = requestJson.get("message", "").asString();
        if(sessionID.empty() || message.empty()){
            std::string errorJsonStr = buildResponse("session_id or message is empty",false);
            resp.status = 400;  //客户端请求参数错误
            resp.set_content(errorJsonStr,"application/json");
            return;
        }

        // 准备流式响应
        resp.status = 200;  //成功
        resp.set_header("Content-Type", "text/event-stream");
        resp.set_header("Cache-Control", "no-cache");
        resp.set_header("Connection", "keep-alive");
        // 告诉服务器，响应内容不是一次性发送的，是分多次逐步发送给客户端的
        resp.set_chunked_content_provider("application/json",[this,sessionID,message](size_t offset,httplib::DataSink& dataSink)->bool{

            auto writeChunk = [&](const std::string& chunk,bool isLast){
                // 将chunk转换为SSE的数据格式
                std::string sseData = "data: " + Json::valueToQuotedString(chunk.c_str()) + "\n\n";
                dataSink.write(sseData.c_str(), sseData.size());
                if(isLast){
                    std::string doneData = "data: [DONE]\n\n";
                    dataSink.write(doneData.c_str(), doneData.size());
                    dataSink.done();
                    return false;//没有更多数据了
                }
                return true;//后续还有数据
            };
            // 先给客户端发送一个空的chunk，通知客户端开始接收数据流
            if(!writeChunk("",false)){
                return false;
            }
            _chatSDK->sendMessageStream(sessionID, message,writeChunk);
            return false;//没有更多数据了
        });

    }
    //处理获取历史消息请求
    void ChatServer::handleGetSessionMessagesRequest(const httplib::Request& req, httplib::Response& resp)
    {
        //获取会话ID，从请求参数中获取
        std::string sessionID = req.matches[1];

        // 获取会话消息记录
        auto session = _chatSDK->getSession(sessionID);
        if(!session){
            std::string errorJsonStr = buildResponse("session not found",false);
            resp.status = 404;  //会话不存在
            resp.set_content(errorJsonStr,"application/json");
            return;
        }
        //构建历史消息列表
        Json::Value dataArray(Json::arrayValue);
        for(const auto& message : session->_messages){
            Json::Value messageJson;
            messageJson["id"] = message._messageId;
            messageJson["role"] = message._role;
            messageJson["content"] = message._content;
            messageJson["timestamp"] = static_cast<int64_t>(message._timestamp);
            dataArray.append(messageJson);
        }
        // 构建响应体
        Json::Value responseJson;
        responseJson["success"] = true;
        responseJson["message"] = "get session messages success";
        responseJson["data"] = dataArray;
        //序列化
        Json::StreamWriterBuilder writerBuilder;
        writerBuilder["indentation"] = "  ";
        std::string responseJsonStr = Json::writeString(writerBuilder, responseJson);
        resp.status = 200;  //成功
        resp.set_content(responseJsonStr,"application/json");
    
    }

}//namespace ai_chat_server