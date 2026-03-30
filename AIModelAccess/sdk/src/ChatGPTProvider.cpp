#include"../include/ChatGPTProvider.h"
#include"../include/util/myLog.h"
#include <jsoncpp/json/reader.h>
#include<jsoncpp/json/json.h>
#include<httplib.h>

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
    }
        // 发送消息 -- 流式返回
    std::string ChatGPTProvider::sendMessageStream(const std::vector<Message>& messages,
            const std::map<std::string,std::string>& requestParam,
            std::function<void(const std::string&,bool)> callback);  //callback: 对模型返回的增量数据如何处理，第一个参数为增量数据，第二个参数为是否为最后一个增量数据。
} // end ai_chat_sdk