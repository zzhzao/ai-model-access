#pragma once
#include<string>
#include<vector>
#include<functional>
#include "common.h"
#include <map>
namespace ai_chat_sdk{
    class LLMProvider{
    public:
        virtual bool initModel(const std::map<std::string, std::string>& config) = 0;
        virtual std::string getModelName() const = 0;
        virtual std::string getModelDesc() const = 0;
        virtual bool isAvailable() const = 0;
        // 发送消息 -- 全量返回
        virtual std::string sendMessage(const std::vector<Message>& messages,const std::map<std::string,std::string>& requestParam) = 0;
        // 发送消息 -- 流式返回
        virtual std::string sendMessageStream(const std::vector<Message>& messages,
            const std::map<std::string,std::string>& requestParam,
            std::function<void(const std::string&,bool)> callback) = 0;  //callback: 对模型返回的增量数据如何处理，第一个参数为增量数据，第二个参数为是否为最后一个增量数据。
    protected:
        std::string _apiKey;    //API密钥
        std::string _endpoint;    //模型API endpoint base url
        bool _isAvailable = false;
    };
}