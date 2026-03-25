#pragma once
#include<iostream>
#include "LLMProvider.h"
#include<map>
#include<vector>
#include "common.h"
#include<functional>


namespace ai_chat_sdk{
    class DeepSeekProvider: public LLMProvider{
    public:
        virtual bool initModel(const std::map<std::string, std::string>& config);
        virtual std::string getModelName() const;
        virtual std::string getModelDesc() const;
        virtual bool isAvailable() const;
        // 发送消息 -- 全量返回
        void sendMessage(const std::vector<Message>& messages,const std::map<std::string,std::string>& requestParam);
        // 发送消息 -- 流式返回
        void sendMessageStream(const std::vector<Message>& messages,
            const std::map<std::string,std::string>& requestParam,
            std::function<void(const std::string&,bool)> callback);  //callback: 对模型返回的增量数据如何处理，第一个参数为增量数据，第二个参数为是否为最后一个增量数据。

    };
}