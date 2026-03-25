#include"../include/DeepSeekProvider.h"
#include"../include/util/myLog.h"
#include "../include/common.h"
#include<iostream>
#include<map>
#include<vector>

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
    void DeepSeekProvider::sendMessage(const std::vector<Message>& messages,const std::map<std::string,std::string>& requestParam)
    {

    }

    void DeepSeekProvider::sendMessageStream(const std::vector<Message>& messages,
            const std::map<std::string,std::string>& requestParam,
            std::function<void(const std::string&,bool)> callback)  //callback: 对模型返回的增量数据如何处理，第一个参数为增量数据，第二个参数为是否为最后一个增量数据。
    {

    }

    
    
};
