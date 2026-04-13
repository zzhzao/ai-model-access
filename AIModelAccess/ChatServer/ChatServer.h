#pragma once
#include<httplib.h>
#include<memory>
#include<ai_chat_sdk/ChatSDK.h>

namespace ai_chat_server {

    //服务器配置信息
struct ServerConfig {
    std::string host = "0.0.0.0";  //服务器绑定IP
    int port = 8000;               //服务器绑定端口
    std::string logLevel = "info";  //日志级别

    // 模型需要的配置信息
    double temperature = 0.7;      // 温度参数，控制输出文本的随机性
    int maxTokens = 1024;          // 最大输出文本长度

    //API key
    std::string deepseekAPIKey = "";
    std::string geminiAPIKey = "";
    std::string chatGPTAPIKey = "";

    std::string olloamaModelName = "";
    std::string olloamaModelDesc = "";
    std::string olloamaEndpoint = "";
};

class ChatServer {
    public:
        ChatServer(const ServerConfig& config);
        ~ChatServer();
        bool start();
        void stop();
        bool isRunning() const;
    private:
        // 构造错误响应
        std::string buildResponse(const std::string& message,bool success = false);
        
        //处理创建会话的请求
        void handleCreateSessionRequest(const httplib::Request& req, httplib::Response& resp);
        //处理获取会话的请求
        void handleGetSessionRequest(const httplib::Request& req, httplib::Response& resp);
        //处理获取模型列表请求
        void handleGetModelListRequest(const httplib::Request& req, httplib::Response& resp);
        //处理删除会话请求
        void handleDeleteSessionRequest(const httplib::Request& req, httplib::Response& resp);
        //处理发送消息请求  --全量返回
        void handleSendMessageRequest(const httplib::Request& req, httplib::Response& resp);
        //处理发送消息请求  --流式返回
        void handleSendMessageStreamRequest(const httplib::Request& req, httplib::Response& resp);
        //处理获取历史消息请求
        void handleGetSessionMessagesRequest(const httplib::Request& req, httplib::Response& resp);

    private:
        ServerConfig _config;
        std::unique_ptr<httplib::Server> _chatServer = nullptr;
        std::shared_ptr<ai_chat_sdk::ChatSDK> _chatSDK = nullptr;
        std::atomic<bool> _isRunning = {false};
};

}//namespace ai_chat_sdk