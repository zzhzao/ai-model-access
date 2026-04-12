#pragma once
#include <string>
#include <ctime>
#include<vector>
#include<iostream>
namespace ai_chat_sdk{
    struct Message{
    std::string _messageId;    //消息ID
    std::string _role;         //角色
    std::string _content;      //消息内容
    std::time_t _timestamp;    //时间戳

    Message(const std::string& role, const std::string& content)
        :  _role(role), _content(content)
        {

        }   
    };
    
    //模型公共配置信息
     struct Config{
        std::string _modelName;       // 模型名称
        double _temperature = 0.7;     //温度
        int _maxTokens = 2048;        //最大token数

        virtual ~Config() = default;    // 实现向下转型的安全性
     };

     // 通过API连接云端模型
     struct APIConfig: public Config{
        std::string _apiKey;    //API Key
     };
// 通过Ollama接入本地模型---不需要apikey
   struct OllamaConfig : public Config{
      std::string _modelName;       // 模型名称
      std::string _modelDesc;       // 模型描述
      std::string _endpoint;        // 模型API endpoint  base url
   };
     // LLM信息
     struct ModelInfo{
        std::string _modelName;    //模型名称
        std::string _modelVersion;    //模型版本
        std::string _modelDesc;    //模型描述
        std::string _endpoint;    //模型API endpoint base url
        bool _isAvailable = false;    //模型是否可用
 
        ModelInfo(const std::string& modelName, const std::string& modelVersion = "", const std::string& modelDesc = "", const std::string& endpoint = "")
            : _modelName(modelName), _modelVersion(modelVersion), _modelDesc(modelDesc), _endpoint(endpoint)
            {

            }
     };
     struct Session{
        std::string _sessionId;    //会话ID
        std::string _modelName;    //模型名称
        std::vector<Message> _messages;    //会话消息记录
        std::time_t _createTime;    //会话创建时间
        std::time_t _updateTime;    //会话更新时间

        Session(const std::string& modelName = "")
            : _modelName(modelName)
            {

            }
     };

}